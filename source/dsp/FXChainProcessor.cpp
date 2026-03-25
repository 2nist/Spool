#include "FXChainProcessor.h"

//==============================================================================
FXChainProcessor::FXType FXChainProcessor::typeFromString (const juce::String& id) noexcept
{
    if (id == "filter")      return FXType::Filter;
    if (id == "reverb")      return FXType::Reverb;
    if (id == "eq")          return FXType::EQ;
    if (id == "compressor")  return FXType::Compressor;
    if (id == "delay")       return FXType::Delay;
    if (id == "distortion")  return FXType::Distortion;
    if (id == "chorus")      return FXType::Chorus;
    return FXType::None;
}

//==============================================================================
FXChainProcessor::FXChainProcessor()
{
    // Initialise all atomic chain slots to empty
    for (int i = 0; i < kMaxChain; ++i)
    {
        m_chainType  [i].store (static_cast<int> (FXType::None));
        m_chainBypass[i].store (false);
    }

    // Initialise all atomic params to 0
    for (int t = 0; t < static_cast<int> (FXType::Count); ++t)
        for (int p = 0; p < kMaxParams; ++p)
            m_params[t][p].store (0.0f);

    // Write effect-type default params
    // Filter: CUTOFF=2000, RESO=0.3, ENV=0, DRIVE=0, MODE=0 (LP)
    m_params[static_cast<int> (FXType::Filter)][0].store (2000.0f);
    m_params[static_cast<int> (FXType::Filter)][1].store (0.3f);

    // Reverb: SIZE=0.5, DAMP=0.5, MIX=0.3, WIDTH=0.8, FREEZE=0
    m_params[static_cast<int> (FXType::Reverb)][0].store (0.5f);
    m_params[static_cast<int> (FXType::Reverb)][1].store (0.5f);
    m_params[static_cast<int> (FXType::Reverb)][2].store (0.3f);
    m_params[static_cast<int> (FXType::Reverb)][3].store (0.8f);

    // Compressor: THRESH=-12, RATIO=4, ATTACK=10, RELEASE=100, GAIN=0
    m_params[static_cast<int> (FXType::Compressor)][0].store (-12.0f);
    m_params[static_cast<int> (FXType::Compressor)][1].store (4.0f);
    m_params[static_cast<int> (FXType::Compressor)][2].store (10.0f);
    m_params[static_cast<int> (FXType::Compressor)][3].store (100.0f);

    // Delay: TIME=0.25, FEED=0.4, SPREAD=0.5, MIX=0.3
    m_params[static_cast<int> (FXType::Delay)][0].store (0.25f);
    m_params[static_cast<int> (FXType::Delay)][1].store (0.4f);
    m_params[static_cast<int> (FXType::Delay)][2].store (0.5f);
    m_params[static_cast<int> (FXType::Delay)][3].store (0.3f);

    // Distortion: DRIVE=0.3, TONE=0.5, MIX=0.5
    m_params[static_cast<int> (FXType::Distortion)][0].store (0.3f);
    m_params[static_cast<int> (FXType::Distortion)][1].store (0.5f);
    m_params[static_cast<int> (FXType::Distortion)][2].store (0.5f);

    // Chorus: RATE=0.5, DEPTH=0.3, FEED=0.2, MIX=0.5
    m_params[static_cast<int> (FXType::Chorus)][0].store (0.5f);
    m_params[static_cast<int> (FXType::Chorus)][1].store (0.3f);
    m_params[static_cast<int> (FXType::Chorus)][2].store (0.2f);
    m_params[static_cast<int> (FXType::Chorus)][3].store (0.5f);
}

//==============================================================================
void FXChainProcessor::prepare (double sampleRate, int blockSize)
{
    m_sampleRate = sampleRate;
    m_blockSize  = blockSize;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<uint32_t> (blockSize);
    spec.numChannels      = 2;

    m_filter.prepare (spec);
    m_filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    m_reverb.prepare (spec);

    m_eqLow.prepare  (spec);
    m_eqMid.prepare  (spec);
    m_eqHigh.prepare (spec);

    m_compressor.prepare (spec);

    m_chorus.prepare (spec);
    m_chorus.setCentreDelay (7.0f);

    juce::dsp::ProcessSpec monoSpec;
    monoSpec.sampleRate       = sampleRate;
    monoSpec.maximumBlockSize = static_cast<uint32_t> (blockSize);
    monoSpec.numChannels      = 1;

    m_delayL.prepare (monoSpec);
    m_delayR.prepare (monoSpec);

    // Set EQ default coefficients (flat)
    *m_eqLow.coefficients  = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
                                  sampleRate, 80.0f,  0.7f, 1.0f);
    *m_eqMid.coefficients  = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                                  sampleRate, 1000.0f, 0.7f, 1.0f);
    *m_eqHigh.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
                                  sampleRate, 8000.0f, 0.7f, 1.0f);

    m_prepared = true;
}

//==============================================================================
void FXChainProcessor::reset()
{
    m_filter.reset();
    m_reverb.reset();
    m_eqLow.reset();
    m_eqMid.reset();
    m_eqHigh.reset();
    m_compressor.reset();
    m_chorus.reset();
    m_delayL.reset();
    m_delayR.reset();
}

//==============================================================================
void FXChainProcessor::rebuildFromChain (const ChainState& chain)
{
    // Called on MESSAGE thread only.
    const int n = juce::jmin (chain.nodes.size(), kMaxChain);

    // Write new chain types
    for (int i = 0; i < n; ++i)
    {
        const auto t = typeFromString (chain.nodes[i].effectType);
        m_chainType  [i].store (static_cast<int> (t));
        m_chainBypass[i].store (chain.nodes[i].bypassed);

        // Sync current param values from ChainState into atomic param storage
        if (t != FXType::None)
        {
            const int ti = static_cast<int> (t);
            const auto& nodeParams = chain.nodes[i].params;
            for (int p = 0; p < juce::jmin (nodeParams.size(), kMaxParams); ++p)
                m_params[ti][p].store (nodeParams[p]);
        }
    }

    // Clear any positions beyond new chain length
    for (int i = n; i < kMaxChain; ++i)
    {
        m_chainType  [i].store (static_cast<int> (FXType::None));
        m_chainBypass[i].store (false);
    }

    // Write length last so audio thread sees the complete update
    m_chainLength.store (n, std::memory_order_release);
}

//==============================================================================
void FXChainProcessor::setParam (int nodeIndex, int paramIndex, float value) noexcept
{
    // Called on MESSAGE thread only.
    if (nodeIndex < 0 || nodeIndex >= kMaxChain)   return;
    if (paramIndex < 0 || paramIndex >= kMaxParams) return;

    const int typeIdx = m_chainType[nodeIndex].load();
    if (typeIdx < 0 || typeIdx >= static_cast<int> (FXType::Count))
        return;

    m_params[typeIdx][paramIndex].store (value);
}

//==============================================================================
void FXChainProcessor::process (juce::AudioBuffer<float>& buffer)
{
    if (!m_prepared) return;

    const int n = m_chainLength.load (std::memory_order_acquire);
    if (n == 0) return;

    for (int i = 0; i < n; ++i)
    {
        const int  typeIdx  = m_chainType  [i].load();
        const bool bypassed = m_chainBypass[i].load();
        if (bypassed || typeIdx < 0) continue;

        const auto type = static_cast<FXType> (typeIdx);

        // Load params for this effect type
        float p[kMaxParams];
        for (int j = 0; j < kMaxParams; ++j)
            p[j] = m_params[typeIdx][j].load();

        switch (type)
        {
            case FXType::Filter:     applyFilter     (buffer, p); break;
            case FXType::Reverb:     applyReverb     (buffer, p); break;
            case FXType::EQ:         applyEQ         (buffer, p); break;
            case FXType::Compressor: applyCompressor (buffer, p); break;
            case FXType::Delay:      applyDelay      (buffer, p); break;
            case FXType::Distortion: applyDistortion (buffer, p); break;
            case FXType::Chorus:     applyChorus     (buffer, p); break;
            default: break;
        }
    }
}

//==============================================================================
// Filter
// p[0]=CUTOFF(20-20000), p[1]=RESO(0-1), p[2]=ENV(unused), p[3]=DRIVE(unused), p[4]=MODE enum
//==============================================================================
void FXChainProcessor::updateFilterParams (const float* p)
{
    const float cutoff = juce::jlimit (20.0f, 20000.0f, p[0]);
    const float reso   = juce::jlimit (0.0f,  1.0f,     p[1]);
    const int   mode   = static_cast<int> (p[4] + 0.5f);  // round to nearest int

    m_filter.setCutoffFrequency (cutoff);
    m_filter.setResonance (0.5f + reso * 9.5f);  // map 0-1 → 0.5-10 Q

    switch (mode)
    {
        case 1:  m_filter.setType (juce::dsp::StateVariableTPTFilterType::highpass);  break;
        case 2:  m_filter.setType (juce::dsp::StateVariableTPTFilterType::bandpass);  break;
        default: m_filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);   break;
        // NOTCH: no direct enum in SVTF — use lowPass as fallback
    }
}

void FXChainProcessor::applyFilter (juce::AudioBuffer<float>& buf, const float* p)
{
    updateFilterParams (p);
    auto block   = juce::dsp::AudioBlock<float> (buf);
    auto context = juce::dsp::ProcessContextReplacing<float> (block);
    m_filter.process (context);
}

//==============================================================================
// Reverb
// p[0]=SIZE(0-1), p[1]=DAMP(0-1), p[2]=MIX(0-1), p[3]=WIDTH(0-1), p[4]=FREEZE(0/1)
//==============================================================================
void FXChainProcessor::updateReverbParams (const float* p)
{
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize   = juce::jlimit (0.0f, 1.0f, p[0]);
    rp.damping    = juce::jlimit (0.0f, 1.0f, p[1]);
    const float mix = juce::jlimit (0.0f, 1.0f, p[2]);
    rp.wetLevel   = mix;
    rp.dryLevel   = 1.0f - mix;
    rp.width      = juce::jlimit (0.0f, 1.0f, p[3]);
    rp.freezeMode = (p[4] > 0.5f) ? 1.0f : 0.0f;
    m_reverb.setParameters (rp);
}

void FXChainProcessor::applyReverb (juce::AudioBuffer<float>& buf, const float* p)
{
    updateReverbParams (p);
    auto block   = juce::dsp::AudioBlock<float> (buf);
    auto context = juce::dsp::ProcessContextReplacing<float> (block);
    m_reverb.process (context);
}

//==============================================================================
// EQ
// p[0]=LOGAIN(-12..12), p[1]=MIDGAIN, p[2]=HIGAIN, p[3]=LOFREQ(20-500), p[4]=HIFREQ(2000-20000)
//==============================================================================
void FXChainProcessor::updateEQParams (const float* p)
{
    const float loGain  = juce::jlimit (-12.0f, 12.0f, p[0]);
    const float midGain = juce::jlimit (-12.0f, 12.0f, p[1]);
    const float hiGain  = juce::jlimit (-12.0f, 12.0f, p[2]);
    const float loFreq  = juce::jlimit (20.0f,  500.0f,   p[3]);
    const float hiFreq  = juce::jlimit (2000.0f, 20000.0f, p[4]);

    *m_eqLow.coefficients  = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        m_sampleRate, static_cast<float> (loFreq), 0.7f,
        juce::Decibels::decibelsToGain (loGain));

    *m_eqMid.coefficients  = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
        m_sampleRate, 1000.0f, 0.7f,
        juce::Decibels::decibelsToGain (midGain));

    *m_eqHigh.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        m_sampleRate, static_cast<float> (hiFreq), 0.7f,
        juce::Decibels::decibelsToGain (hiGain));
}

void FXChainProcessor::applyEQ (juce::AudioBuffer<float>& buf, const float* p)
{
    updateEQParams (p);

    auto block   = juce::dsp::AudioBlock<float> (buf);
    auto context = juce::dsp::ProcessContextReplacing<float> (block);

    // Apply each IIR band — these are mono processors, so duplicate for stereo
    const int numCh      = buf.getNumChannels();
    const int numSamples = buf.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto chBlock = block.getSingleChannelBlock (static_cast<size_t> (ch));
        auto chCtx   = juce::dsp::ProcessContextReplacing<float> (chBlock);
        m_eqLow.process  (chCtx);
        m_eqMid.process  (chCtx);
        m_eqHigh.process (chCtx);
    }

    juce::ignoreUnused (context, numCh, numSamples);
}

//==============================================================================
// Compressor
// p[0]=THRESH(-60..0), p[1]=RATIO(1..20), p[2]=ATTACK(0.1..100), p[3]=RELEASE(10..1000), p[4]=GAIN
//==============================================================================
void FXChainProcessor::updateCompressorParams (const float* p)
{
    m_compressor.setThreshold (juce::jlimit (-60.0f, 0.0f,    p[0]));
    m_compressor.setRatio     (juce::jlimit (1.0f,   20.0f,   p[1]));
    m_compressor.setAttack    (juce::jlimit (0.1f,   100.0f,  p[2]));
    m_compressor.setRelease   (juce::jlimit (10.0f,  1000.0f, p[3]));
}

void FXChainProcessor::applyCompressor (juce::AudioBuffer<float>& buf, const float* p)
{
    updateCompressorParams (p);

    const float makeupGain = juce::Decibels::decibelsToGain (
                                 juce::jlimit (0.0f, 24.0f, p[4]));

    auto block   = juce::dsp::AudioBlock<float> (buf);
    auto context = juce::dsp::ProcessContextReplacing<float> (block);
    m_compressor.process (context);

    if (makeupGain != 1.0f)
        buf.applyGain (makeupGain);
}

//==============================================================================
// Delay  (manual implementation using DelayLine)
// p[0]=TIME(0.01..2s), p[1]=FEED(0..0.99), p[2]=SPREAD(0..1), p[3]=MIX(0..1)
//==============================================================================
void FXChainProcessor::applyDelay (juce::AudioBuffer<float>& buf, const float* p)
{
    const float time     = juce::jlimit (0.01f, 2.0f,  p[0]);
    const float feedback = juce::jlimit (0.0f,  0.99f, p[1]);
    const float spread   = juce::jlimit (0.0f,  1.0f,  p[2]);
    const float mix      = juce::jlimit (0.0f,  1.0f,  p[3]);

    // Spread: left channel slightly shorter, right slightly longer
    const float timeL = time * (1.0f - spread * 0.1f);
    const float timeR = time * (1.0f + spread * 0.1f);

    const float delayL = timeL * static_cast<float> (m_sampleRate);
    const float delayR = timeR * static_cast<float> (m_sampleRate);

    m_delayL.setDelay (juce::jmin (delayL, static_cast<float> (kMaxDelaySamples - 1)));
    m_delayR.setDelay (juce::jmin (delayR, static_cast<float> (kMaxDelaySamples - 1)));

    const int numCh      = juce::jmin (buf.getNumChannels(), 2);
    const int numSamples = buf.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto& dl = (ch == 0) ? m_delayL : m_delayR;
        auto* data = buf.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            const float dry = data[i];
            const float wet = dl.popSample (0);
            dl.pushSample (0, dry + wet * feedback);
            data[i] = dry * (1.0f - mix) + wet * mix;
        }
    }
}

//==============================================================================
// Distortion  (soft-clipping waveshaper)
// p[0]=DRIVE(0..1), p[1]=TONE(0..1), p[2]=MIX(0..1), p[3]=MODE(0..3)
//==============================================================================
void FXChainProcessor::applyDistortion (juce::AudioBuffer<float>& buf, const float* p)
{
    const float drive = 1.0f + juce::jlimit (0.0f, 1.0f, p[0]) * 19.0f;  // 1..20
    const float mix   = juce::jlimit (0.0f, 1.0f, p[2]);
    const int   mode  = static_cast<int> (p[3] + 0.5f);

    const int numCh      = buf.getNumChannels();
    const int numSamples = buf.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* data = buf.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            const float dry = data[i];
            float wet = dry * drive;

            switch (mode)
            {
                case 1: // Hard clip
                    wet = juce::jlimit (-1.0f, 1.0f, wet);
                    break;
                case 2: // Tape (asymmetric)
                    wet = std::tanh (wet * 0.7f) * 1.3f;
                    wet = juce::jlimit (-1.0f, 1.0f, wet);
                    break;
                case 3: // Fold
                    while (wet > 1.0f)  wet = 2.0f - wet;
                    while (wet < -1.0f) wet = -2.0f - wet;
                    break;
                default: // Soft (tanh)
                    wet = std::tanh (wet);
                    break;
            }

            // Compensate for drive gain
            wet /= drive * 0.3f + 0.7f;
            data[i] = dry * (1.0f - mix) + wet * mix;
        }
    }
}

//==============================================================================
// Chorus
// p[0]=RATE(0.01..10Hz), p[1]=DEPTH(0..1), p[2]=FEED(0..0.99), p[3]=MIX(0..1), p[4]=MODE
//==============================================================================
void FXChainProcessor::updateChorusParams (const float* p)
{
    m_chorus.setRate     (juce::jlimit (0.01f, 100.0f, p[0]));
    m_chorus.setDepth    (juce::jlimit (0.0f,  1.0f,   p[1]));
    m_chorus.setFeedback (juce::jlimit (-0.99f, 0.99f,  p[2]));
    m_chorus.setMix      (juce::jlimit (0.0f,  1.0f,   p[3]));
    // MODE p[4]: 0=Chorus (7ms), 1=Flanger (1ms)
    const float centreDelay = (p[4] > 0.5f) ? 1.5f : 7.0f;
    m_chorus.setCentreDelay (centreDelay);
}

void FXChainProcessor::applyChorus (juce::AudioBuffer<float>& buf, const float* p)
{
    updateChorusParams (p);
    auto block   = juce::dsp::AudioBlock<float> (buf);
    auto context = juce::dsp::ProcessContextReplacing<float> (block);
    m_chorus.process (context);
}

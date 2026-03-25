#include "PolySynthProcessor.h"

//==============================================================================
void PolySynthProcessor::prepare (double sampleRate, int maxBlock)
{
    m_sampleRate = sampleRate;
    m_maxBlock   = maxBlock;
    m_lfoPhase   = 0.0;

    for (auto& v : m_voices)
        v.prepare (sampleRate, maxBlock);
}

void PolySynthProcessor::reset()
{
    m_lfoPhase = 0.0;
    for (auto& v : m_voices)
        v.reset();
}

//==============================================================================
void PolySynthProcessor::process (juce::AudioBuffer<float>& output,
                                   const juce::MidiBuffer&   midi)
{
    const int numSamples = output.getNumSamples();
    const int numCh      = juce::jmin (output.getNumChannels(), 2);

    if (numCh == 0 || numSamples == 0)
        return;

    // ---- Snapshot parameters from atomics (once per block) -----------------
    const int   osc1Shape   = m_osc1Shape.load();
    const int   osc2Shape   = m_osc2Shape.load();
    const float osc1Det     = m_osc1Detune.load();
    const float osc2Det     = m_osc2Detune.load();
    const int   osc2Oct     = m_osc2Octave.load();
    const float osc2Lvl     = m_osc2Level.load();

    const float filterCutoff = m_filterCutoff.load();
    const float filterRes    = m_filterRes.load();
    const float filterEnvAmt = m_filterEnvAmt.load();

    const float ampA = m_ampA.load(), ampD = m_ampD.load();
    const float ampS = m_ampS.load(), ampR = m_ampR.load();
    const float filtA = m_filtA.load(), filtD = m_filtD.load();
    const float filtS = m_filtS.load(), filtR = m_filtR.load();

    const float lfoRate   = m_lfoRate.load();
    const float lfoDepth  = m_lfoDepth.load();
    const int   lfoTarget = m_lfoTarget.load();

    // Build ADSR params to pass when triggering new notes
    const juce::ADSR::Parameters ampParams  { ampA, ampD, ampS, ampR };
    const juce::ADSR::Parameters filtParams { filtA, filtD, filtS, filtR };

    // ---- Process MIDI (block-start offset — good enough for sequenced use) --
    handleMidi (midi);
    // Note: handleMidi uses ampParams/filtParams captured above via the noteOn
    // path, so we temporarily cache them where noteOn() can reach them.
    // Because handleMidi is called above, we re-snapshot here for clarity:
    // (noteOn uses the atomics directly, safe because audio thread reads only)

    // ---- Compute block-rate LFO --------------------------------------------
    const double lfoInc = juce::MathConstants<double>::twoPi
                          * static_cast<double> (lfoRate) / m_sampleRate;
    const float lfoSample = static_cast<float> (std::sin (m_lfoPhase));
    m_lfoPhase += lfoInc * numSamples;
    while (m_lfoPhase >= juce::MathConstants<double>::twoPi)
        m_lfoPhase -= juce::MathConstants<double>::twoPi;

    const float lfoMod = lfoSample * lfoDepth;   // scaled modulation value

    // ---- Clear output -------------------------------------------------------
    output.clear();

    // ---- Render each active voice -------------------------------------------
    for (auto& v : m_voices)
    {
        if (! v.active)
            continue;

        // Compute per-voice frequencies
        const double baseFreq  = juce::MidiMessage::getMidiNoteInHertz (v.note);
        const double osc1Freq  = baseFreq * std::pow (2.0, static_cast<double> (osc1Det) / 1200.0);
        const double osc2Freq  = baseFreq * std::pow (2.0,
            static_cast<double> (osc2Det + static_cast<float> (osc2Oct) * 1200.0f) / 1200.0);

        // Apply pitch LFO at block rate
        const double pitchMult = (lfoTarget == LfoToPitch)
                                 ? std::pow (2.0, static_cast<double> (lfoMod) / 12.0)  // ±1 semitone per unit depth
                                 : 1.0;

        renderVoice (v, numSamples,
                     osc1Shape, osc2Shape,
                     osc1Freq * pitchMult, osc2Freq * pitchMult,
                     osc2Lvl,
                     filterCutoff, filterRes, filterEnvAmt,
                     lfoMod, lfoTarget);

        // Mix mono voice scratch into stereo output
        const float* src = v.scratch.getReadPointer (0);
        for (int ch = 0; ch < numCh; ++ch)
        {
            float* dst = output.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                dst[i] += src[i];
        }

        // Retire voices whose amp envelope has expired
        if (v.isFinished())
            v.reset();
    }
}

//==============================================================================
void PolySynthProcessor::renderVoice (PolyVoice& v, int numSamples,
                                       int osc1Shape, int osc2Shape,
                                       double osc1Freq, double osc2Freq,
                                       float osc2Level,
                                       float baseCutoff, float filterRes,
                                       float filterEnvAmt,
                                       float lfoValue, int lfoTarget) noexcept
{
    float* buf = v.scratch.getWritePointer (0);

    const double osc1Inc = juce::MathConstants<double>::twoPi * osc1Freq / m_sampleRate;
    const double osc2Inc = juce::MathConstants<double>::twoPi * osc2Freq / m_sampleRate;

    // ---- Sample loop: oscillators + envelopes + per-sample filter ----------
    for (int i = 0; i < numSamples; ++i)
    {
        // Oscillators
        const float s1 = renderOsc (v.osc1Phase, osc1Inc, osc1Shape);
        const float s2 = renderOsc (v.osc2Phase, osc2Inc, osc2Shape);

        // Envelope values
        const float filtEnvVal = v.filterEnv.getNextSample();
        const float ampEnvVal  = v.ampEnv.getNextSample();

        // Mix oscillators and apply amplitude envelope + velocity
        buf[i] = (s1 + s2 * osc2Level) * ampEnvVal * v.velocity;

        // Compute modulated filter cutoff
        float cutoff = baseCutoff
                       + filtEnvVal * filterEnvAmt * (20000.0f - baseCutoff);

        if (lfoTarget == LfoToFilter)
            cutoff *= std::pow (2.0f, lfoValue * 2.0f);   // ±2 octaves max

        cutoff = juce::jlimit (20.0f, 20000.0f, cutoff);

        // Update filter (LadderFilter smooths internally via SmoothedValue)
        v.filter.setCutoffFrequencyHz (cutoff);
        v.filter.setResonance (filterRes);

        // Process one sample through the ladder filter in-place
        float* chanPtr = buf + i;
        juce::dsp::AudioBlock<float> block (&chanPtr, 1, 1);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        v.filter.process (ctx);
    }
}

//==============================================================================
float PolySynthProcessor::renderOsc (double& phase, double phaseInc, int shape) noexcept
{
    float sample = 0.0f;

    switch (shape)
    {
        case Saw:
            // phase in [0, 2π] → output in [-1, +1]
            sample = static_cast<float> (phase / juce::MathConstants<double>::pi) - 1.0f;
            break;

        case Square:
            sample = (phase < juce::MathConstants<double>::pi) ? 1.0f : -1.0f;
            break;

        case Sine:
            sample = static_cast<float> (std::sin (phase));
            break;

        case Triangle:
            if (phase < juce::MathConstants<double>::pi)
                sample = static_cast<float> (phase / juce::MathConstants<double>::pi * 2.0 - 1.0);
            else
                sample = static_cast<float> (3.0 - phase / juce::MathConstants<double>::pi * 2.0);
            break;

        case Noise:
            sample = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
            break;

        default:
            break;
    }

    // Advance phase
    phase += phaseInc;
    if (phase >= juce::MathConstants<double>::twoPi)
        phase -= juce::MathConstants<double>::twoPi;

    return sample;
}

//==============================================================================
void PolySynthProcessor::handleMidi (const juce::MidiBuffer& midi)
{
    const juce::ADSR::Parameters ampParams  { m_ampA.load(), m_ampD.load(),
                                              m_ampS.load(), m_ampR.load() };
    const juce::ADSR::Parameters filtParams { m_filtA.load(), m_filtD.load(),
                                              m_filtS.load(), m_filtR.load() };

    for (const auto& event : midi)
    {
        const auto msg = event.getMessage();

        if (msg.isNoteOn() && msg.getVelocity() > 0)
        {
            noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), ampParams, filtParams);
        }
        else if (msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0))
        {
            noteOff (msg.getNoteNumber());
        }
    }
}

//==============================================================================
void PolySynthProcessor::noteOn (int note, float velocity,
                                  const juce::ADSR::Parameters& ampP,
                                  const juce::ADSR::Parameters& filtP)
{
    const bool mono = m_monoMode.load();

    if (mono)
    {
        // Mono: steal or reuse the single active voice
        PolyVoice* v = findVoiceForNote (-1);   // find any active voice
        if (v == nullptr)
            v = &m_voices[0];

        v->ampEnv.setParameters    (ampP);
        v->filterEnv.setParameters (filtP);
        v->noteOn (note, velocity);
    }
    else
    {
        PolyVoice* v = findFreeVoice();
        if (v == nullptr)
            v = stealVoice();

        if (v != nullptr)
        {
            v->ampEnv.setParameters    (ampP);
            v->filterEnv.setParameters (filtP);
            v->noteOn (note, velocity);
        }
    }
}

void PolySynthProcessor::noteOff (int note)
{
    for (auto& v : m_voices)
    {
        if (v.active && ! v.releasing && v.note == note)
        {
            v.noteOff();
            return;
        }
    }
}

//==============================================================================
PolyVoice* PolySynthProcessor::findFreeVoice() noexcept
{
    for (auto& v : m_voices)
        if (! v.active)
            return &v;
    return nullptr;
}

PolyVoice* PolySynthProcessor::findVoiceForNote (int note) noexcept
{
    for (auto& v : m_voices)
        if (v.active && v.note == note)
            return &v;
    return nullptr;
}

PolyVoice* PolySynthProcessor::stealVoice() noexcept
{
    // Prefer a releasing voice first
    for (auto& v : m_voices)
        if (v.releasing)
            return &v;

    // Otherwise steal first active voice
    for (auto& v : m_voices)
        if (v.active)
            return &v;

    return nullptr;
}

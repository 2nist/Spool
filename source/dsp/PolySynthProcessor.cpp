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
    const bool  osc1Enabled = m_osc1Enabled.load();
    const bool  osc2Enabled = m_osc2Enabled.load();
    const int   osc1Shape   = m_osc1Shape.load();
    const int   osc2Shape   = m_osc2Shape.load();
    const float osc1Det     = m_osc1Detune.load();
    const float osc2Det     = m_osc2Detune.load();
    const int   osc1Oct     = m_osc1Octave.load();
    const int   osc2Oct     = m_osc2Octave.load();
    const float osc1Pw      = m_osc1PulseWidth.load();
    const float osc2Pw      = m_osc2PulseWidth.load();
    const float oscMix      = m_osc2Level.load();

    const float filterCutoff = m_filterCutoff.load();
    const float filterRes    = m_filterRes.load();
    const float filterEnvAmt = m_filterEnvAmt.load();

    const float ampA = m_ampA.load(), ampD = m_ampD.load();
    const float ampS = m_ampS.load(), ampR = m_ampR.load();
    const float filtA = m_filtA.load(), filtD = m_filtD.load();
    const float filtS = m_filtS.load(), filtR = m_filtR.load();

    const bool  lfoEnabled = m_lfoEnabled.load();
    const float lfoRate   = m_lfoRate.load();
    const float lfoDepth  = m_lfoDepth.load();
    const int   lfoTarget = m_lfoTarget.load();

    const float charAsym  = m_charAsym.load();
    const float charDrive = m_charDrive.load();
    const float charDrift = m_charDrift.load();
    const float outputLevel = m_outputLevel.load();
    const float outputPan   = m_outputPan.load();

    // Pre-compute drive gain / normalization factor once per block
    const float driveGain = 1.f + charDrive * 3.f;
    const float dg2       = driveGain * driveGain;
    const float driveNorm = driveGain * (27.f + dg2) / (27.f + 9.f * dg2);

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

    const float lfoMod = lfoEnabled ? (lfoSample * lfoDepth) : 0.0f;

    // ---- Clear output -------------------------------------------------------
    output.clear();

    // ---- Render each active voice -------------------------------------------
    for (auto& v : m_voices)
    {
        if (! v.active)
            continue;

        // Compute per-voice frequencies
        const double baseFreq  = juce::MidiMessage::getMidiNoteInHertz (v.note);
        const double osc1Freq  = baseFreq * std::pow (2.0,
            static_cast<double> (osc1Det + static_cast<float> (osc1Oct) * 1200.0f) / 1200.0);
        const double osc2Freq  = baseFreq * std::pow (2.0,
            static_cast<double> (osc2Det + static_cast<float> (osc2Oct) * 1200.0f) / 1200.0);

        // Apply pitch LFO at block rate
        const double pitchMult = (lfoTarget == LfoToPitch)
                                 ? std::pow (2.0, static_cast<double> (lfoMod) / 12.0)  // ±1 semitone per unit depth
                                 : 1.0;

        renderVoice (v, numSamples,
                     osc1Enabled, osc2Enabled,
                     osc1Shape, osc2Shape,
                     osc1Freq * pitchMult, osc2Freq * pitchMult,
                     osc1Pw, osc2Pw,
                     oscMix,
                     filterCutoff, filterRes, filterEnvAmt,
                     lfoMod, lfoTarget,
                     charAsym, charDrive, driveGain, driveNorm,
                     charDrift);

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

    const float level = juce::jlimit (0.0f, 1.5f, outputLevel);
    const float pan = juce::jlimit (-1.0f, 1.0f, outputPan);
    const float leftGain = level * (pan <= 0.0f ? 1.0f : 1.0f - pan);
    const float rightGain = level * (pan >= 0.0f ? 1.0f : 1.0f + pan);

    if (numCh >= 1)
        output.applyGain (0, 0, numSamples, leftGain);
    if (numCh >= 2)
        output.applyGain (1, 0, numSamples, rightGain);
}

//==============================================================================
void PolySynthProcessor::renderVoice (PolyVoice& v, int numSamples,
                                       bool osc1Enabled, bool osc2Enabled,
                                       int osc1Shape, int osc2Shape,
                                       double osc1Freq, double osc2Freq,
                                       float osc1PulseWidth, float osc2PulseWidth,
                                       float oscMix,
                                       float baseCutoff, float filterRes,
                                       float filterEnvAmt,
                                       float lfoValue, int lfoTarget,
                                       float charAsym, float charDrive,
                                       float driveGain, float driveNorm,
                                       float charDrift) noexcept
{
    float* buf = v.scratch.getWritePointer (0);

    const double osc1Inc = juce::MathConstants<double>::twoPi * osc1Freq / m_sampleRate;
    const double osc2Inc = juce::MathConstants<double>::twoPi * osc2Freq / m_sampleRate;

    const bool useDrive = (charDrive > 0.002f);
    const bool useDrift = (charDrift > 0.002f && v.driftOscInc > 0.0);

    // ---- Sample loop: oscillators + envelopes + per-sample filter ----------
    for (int i = 0; i < numSamples; ++i)
    {
        // Drift: per-sample freq wobble via slow voice oscillator
        double driftMod = 1.0;
        if (useDrift)
        {
            driftMod = 1.0 + static_cast<double> (charDrift)
                           * std::sin (v.driftOscPhase)
                           * 0.003;
            v.driftOscPhase += v.driftOscInc;
            if (v.driftOscPhase >= juce::MathConstants<double>::twoPi)
                v.driftOscPhase -= juce::MathConstants<double>::twoPi;
        }

        // Oscillators (with drift-modulated increment)
        float s1 = osc1Enabled ? renderOsc (v.osc1Phase, osc1Inc * driftMod, osc1Shape, osc1PulseWidth) : 0.0f;
        float s2 = osc2Enabled ? renderOsc (v.osc2Phase, osc2Inc * driftMod, osc2Shape, osc2PulseWidth) : 0.0f;

        // Envelope values
        const float filtEnvVal = v.filterEnv.getNextSample();
        const float ampEnvVal  = v.ampEnv.getNextSample();

        // Oscillator asymmetry — odd-function harmonic shaper, no DC offset
        if (charAsym > 0.002f)
        {
            s1 += charAsym * s1 * std::abs (s1) * 0.35f;
            s2 += charAsym * s2 * std::abs (s2) * 0.35f;
        }

        const float mix = juce::jlimit (0.0f, 1.0f, oscMix);
        const float osc1Gain = 1.0f - mix;
        const float osc2Gain = mix;
        float sig = (s1 * osc1Gain + s2 * osc2Gain) * ampEnvVal * v.velocity;

        // Pre-filter saturation — Padé tanh approximation, normalized
        if (useDrive)
        {
            const float sg  = sig * driveGain;
            const float sg2 = sg * sg;
            const float sat = sg * (27.f + sg2) / (27.f + 9.f * sg2) / driveNorm;
            sig = sig + charDrive * (sat - sig);
        }

        buf[i] = sig;

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
float PolySynthProcessor::renderOsc (double& phase, double phaseInc, int shape, float pulseWidth) noexcept
{
    float sample = 0.0f;
    const double wrappedPulseWidth = juce::jlimit (0.05, 0.95, static_cast<double> (pulseWidth));

    switch (shape)
    {
        case Saw:
            // phase in [0, 2π] → output in [-1, +1]
            sample = static_cast<float> (phase / juce::MathConstants<double>::pi) - 1.0f;
            break;

        case Square:
            sample = (phase < juce::MathConstants<double>::twoPi * wrappedPulseWidth) ? 1.0f : -1.0f;
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

    PolyVoice* v = nullptr;

    if (mono)
    {
        // Mono: steal or reuse the single active voice
        v = findVoiceForNote (-1);   // find any active voice
        if (v == nullptr)
            v = &m_voices[0];
    }
    else
    {
        v = findFreeVoice();
        if (v == nullptr)
            v = stealVoice();
    }

    if (v == nullptr)
        return;

    v->ampEnv.setParameters    (ampP);
    v->filterEnv.setParameters (filtP);
    v->noteOn (note, velocity);

    // Seed drift state — note-deterministic so same note = same wobble
    const float drift = m_charDrift.load();
    if (drift > 0.002f)
    {
        const uint32_t seed = static_cast<uint32_t> (note + 37) * 2654435761u;
        const float    r    = static_cast<float> (seed & 0xFFFFu) / 65535.0f;

        // Randomize initial osc phases proportionally to drift amount
        v->osc1Phase = static_cast<double> (drift) * 0.3
                       * juce::MathConstants<double>::twoPi * static_cast<double> (r);
        v->osc2Phase = static_cast<double> (drift) * 0.3
                       * juce::MathConstants<double>::twoPi * static_cast<double> (1.0f - r);

        // Slow wobble oscillator — note-deterministic rate 0.4..2.4 Hz
        const float driftHz  = 0.4f + r * 2.0f;
        v->driftOscPhase     = 0.0;
        v->driftOscInc       = juce::MathConstants<double>::twoPi
                               * static_cast<double> (driftHz) / m_sampleRate;
    }
    else
    {
        v->driftOscPhase = 0.0;
        v->driftOscInc   = 0.0;
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

//==============================================================================
// ModuleProcessor interface
//==============================================================================

static const ModuleProcessor::ParamDef kPolySynthParams[] =
{
    // id                 name             group      min    max      default  discrete
    { "osc1.on",       "OSC1 On",       "osc",      0.f,   1.f,     1.f,     true  },
    { "osc2.on",       "OSC2 On",       "osc",      0.f,   1.f,     1.f,     true  },
    { "osc1.shape",    "OSC1 Shape",    "osc",      0.f,   4.f,     0.f,     true  },
    { "osc2.shape",    "OSC2 Shape",    "osc",      0.f,   4.f,     0.f,     true  },
    { "osc1.detune",   "OSC1 Detune",   "osc",    -50.f,  50.f,     0.f,     false },
    { "osc2.detune",   "OSC2 Detune",   "osc",    -50.f,  50.f,     7.f,     false },
    { "osc1.octave",   "OSC1 Octave",   "osc",     -2.f,   2.f,     0.f,     true  },
    { "osc2.octave",   "OSC2 Octave",   "osc",     -2.f,   2.f,     0.f,     true  },
    { "osc1.pulse_width","OSC1 Pulse Width","osc", 0.05f, 0.95f,   0.5f,     false },
    { "osc2.pulse_width","OSC2 Pulse Width","osc", 0.05f, 0.95f,   0.5f,     false },
    { "osc2.level",    "Osc Mix",       "osc",      0.f,   1.f,     0.5f,    false },
    { "filter.cutoff", "Cutoff",        "filter",  20.f, 20000.f, 4000.f,   false },
    { "filter.res",    "Resonance",     "filter",   0.f,   1.f,     0.3f,    false },
    { "filter.env_amt","Env Amount",    "filter",   0.f,   1.f,     0.5f,    false },
    { "amp.attack",    "Amp Attack",    "amp",      0.001f, 4.f,   0.01f,    false },
    { "amp.decay",     "Amp Decay",     "amp",      0.001f, 4.f,   0.1f,     false },
    { "amp.sustain",   "Amp Sustain",   "amp",      0.f,   1.f,     0.8f,    false },
    { "amp.release",   "Amp Release",   "amp",      0.001f, 4.f,   0.3f,     false },
    { "filt.attack",   "Filt Attack",   "filtenv",  0.001f, 4.f,   0.01f,    false },
    { "filt.decay",    "Filt Decay",    "filtenv",  0.001f, 4.f,   0.2f,     false },
    { "filt.sustain",  "Filt Sustain",  "filtenv",  0.f,   1.f,     0.5f,    false },
    { "filt.release",  "Filt Release",  "filtenv",  0.001f, 4.f,   0.3f,     false },
    { "lfo.on",        "LFO On",        "lfo",      0.f,   1.f,     1.f,     true  },
    { "lfo.rate",      "LFO Rate",      "lfo",      0.1f,  20.f,    5.f,     false },
    { "lfo.depth",     "LFO Depth",     "lfo",      0.f,   1.f,     0.f,     false },
    { "lfo.target",    "LFO Target",    "lfo",      0.f,   1.f,     0.f,     true  },
    { "voice.mono",    "Mono Mode",     "voice",    0.f,   1.f,     0.f,     true  },
    { "char.asym",     "Asymmetry",     "char",     0.f,   1.f,     0.15f,   false },
    { "char.drive",    "Drive",         "char",     0.f,   1.f,     0.20f,   false },
    { "char.drift",    "Drift",         "char",     0.f,   1.f,     0.15f,   false },
    { "out.level",     "Output Level",  "output",   0.f,   1.5f,    1.f,     false },
    { "out.pan",       "Output Pan",    "output",  -1.f,   1.f,     0.f,     false },
};

static constexpr int kPolySynthNumParams = static_cast<int> (std::size (kPolySynthParams));

const ModuleProcessor::ParamDef* PolySynthProcessor::getParamDefs() const noexcept
{
    return kPolySynthParams;
}

int PolySynthProcessor::getNumParams() const noexcept
{
    return kPolySynthNumParams;
}

void PolySynthProcessor::setParam (juce::StringRef id, float v) noexcept
{
    // Explicit StringRef(const char*) disambiguates MSVC operator== overload resolution
    auto eq = [&] (const char* s) noexcept -> bool { return id == juce::StringRef (s); };

    if      (eq ("osc1.on"))       setOsc1Enabled  (v >= 0.5f);
    else if (eq ("osc2.on"))       setOsc2Enabled  (v >= 0.5f);
    else if (eq ("osc1.shape"))    setOsc1Shape    (static_cast<int> (v));
    else if (eq ("osc2.shape"))    setOsc2Shape    (static_cast<int> (v));
    else if (eq ("osc1.detune"))   setOsc1Detune   (v);
    else if (eq ("osc2.detune"))   setOsc2Detune   (v);
    else if (eq ("osc1.octave"))   setOsc1Octave   (static_cast<int> (v));
    else if (eq ("osc2.octave"))   setOsc2Octave   (static_cast<int> (v));
    else if (eq ("osc1.pulse_width")) setOsc1PulseWidth (v);
    else if (eq ("osc2.pulse_width")) setOsc2PulseWidth (v);
    else if (eq ("osc2.level"))    setOsc2Level    (v);
    else if (eq ("filter.cutoff")) setFilterCutoff    (v);
    else if (eq ("filter.res"))    setFilterResonance (v);
    else if (eq ("filter.env_amt"))setFilterEnvAmt    (v);
    else if (eq ("amp.attack"))    setAmpAttack  (v);
    else if (eq ("amp.decay"))     setAmpDecay   (v);
    else if (eq ("amp.sustain"))   setAmpSustain (v);
    else if (eq ("amp.release"))   setAmpRelease (v);
    else if (eq ("filt.attack"))   setFiltAttack  (v);
    else if (eq ("filt.decay"))    setFiltDecay   (v);
    else if (eq ("filt.sustain"))  setFiltSustain (v);
    else if (eq ("filt.release"))  setFiltRelease (v);
    else if (eq ("lfo.on"))        setLfoEnabled (v >= 0.5f);
    else if (eq ("lfo.rate"))      setLfoRate   (v);
    else if (eq ("lfo.depth"))     setLfoDepth  (v);
    else if (eq ("lfo.target"))    setLfoTarget (static_cast<int> (v));
    else if (eq ("voice.mono"))    setMonoMode  (v >= 0.5f);
    else if (eq ("char.asym"))     setCharAsym  (v);
    else if (eq ("char.drive"))    setCharDrive (v);
    else if (eq ("char.drift"))    setCharDrift (v);
    else if (eq ("out.level"))     setOutputLevel (v);
    else if (eq ("out.pan"))       setOutputPan   (v);
}

float PolySynthProcessor::getParam (juce::StringRef id) const noexcept
{
    auto eq = [&] (const char* s) noexcept -> bool { return id == juce::StringRef (s); };

    if      (eq ("osc1.on"))        return isOsc1Enabled() ? 1.f : 0.f;
    else if (eq ("osc2.on"))        return isOsc2Enabled() ? 1.f : 0.f;
    else if (eq ("osc1.shape"))     return static_cast<float> (getOsc1Shape());
    else if (eq ("osc2.shape"))     return static_cast<float> (getOsc2Shape());
    else if (eq ("osc1.detune"))    return getOsc1Detune();
    else if (eq ("osc2.detune"))    return getOsc2Detune();
    else if (eq ("osc1.octave"))    return static_cast<float> (getOsc1Octave());
    else if (eq ("osc2.octave"))    return static_cast<float> (getOsc2Octave());
    else if (eq ("osc1.pulse_width")) return getOsc1PulseWidth();
    else if (eq ("osc2.pulse_width")) return getOsc2PulseWidth();
    else if (eq ("osc2.level"))     return getOsc2Level();
    else if (eq ("filter.cutoff"))  return getFilterCutoff();
    else if (eq ("filter.res"))     return getFilterResonance();
    else if (eq ("filter.env_amt")) return getFilterEnvAmt();
    else if (eq ("amp.attack"))     return getAmpAttack();
    else if (eq ("amp.decay"))      return getAmpDecay();
    else if (eq ("amp.sustain"))    return getAmpSustain();
    else if (eq ("amp.release"))    return getAmpRelease();
    else if (eq ("filt.attack"))    return getFiltAttack();
    else if (eq ("filt.decay"))     return getFiltDecay();
    else if (eq ("filt.sustain"))   return getFiltSustain();
    else if (eq ("filt.release"))   return getFiltRelease();
    else if (eq ("lfo.on"))         return isLfoEnabled() ? 1.f : 0.f;
    else if (eq ("lfo.rate"))       return getLfoRate();
    else if (eq ("lfo.depth"))      return getLfoDepth();
    else if (eq ("lfo.target"))     return static_cast<float> (getLfoTarget());
    else if (eq ("voice.mono"))     return isMonoMode() ? 1.f : 0.f;
    else if (eq ("char.asym"))      return getCharAsym();
    else if (eq ("char.drive"))     return getCharDrive();
    else if (eq ("char.drift"))     return getCharDrift();
    else if (eq ("out.level"))      return getOutputLevel();
    else if (eq ("out.pan"))        return getOutputPan();
    return 0.f;
}

void PolySynthProcessor::getState (juce::MemoryBlock& dest) const
{
    struct Header { uint32_t magic; uint32_t version; };
    constexpr uint32_t kMagic   = 0x504F4C59u;  // "POLY"
    constexpr uint32_t kVersion = 3u;

    const int dataSize = static_cast<int> (sizeof (Header))
                       + kPolySynthNumParams * static_cast<int> (sizeof (float));
    dest.setSize (static_cast<size_t> (dataSize), false);

    auto* ptr = static_cast<char*> (dest.getData());

    Header hdr { kMagic, kVersion };
    std::memcpy (ptr, &hdr, sizeof (hdr));
    ptr += sizeof (hdr);

    for (int i = 0; i < kPolySynthNumParams; ++i)
    {
        const float v = getParam (kPolySynthParams[i].id);
        std::memcpy (ptr, &v, sizeof (float));
        ptr += sizeof (float);
    }
}

bool PolySynthProcessor::setState (const void* data, int sizeBytes)
{
    struct Header { uint32_t magic; uint32_t version; };
    constexpr uint32_t kMagic = 0x504F4C59u;

    constexpr int kV1ParamCount = 21;
    constexpr int kV2ParamCount = 24;
    const int v1Size = static_cast<int> (sizeof (Header))
                     + kV1ParamCount * static_cast<int> (sizeof (float));
    const int v2Size = static_cast<int> (sizeof (Header))
                     + kV2ParamCount * static_cast<int> (sizeof (float));
    const int v3Size = static_cast<int> (sizeof (Header))
                     + kPolySynthNumParams * static_cast<int> (sizeof (float));

    if (sizeBytes < v1Size)
        return false;

    const auto* ptr = static_cast<const char*> (data);

    Header hdr{};
    std::memcpy (&hdr, ptr, sizeof (hdr));
    if (hdr.magic != kMagic)
        return false;
    ptr += sizeof (hdr);

    const bool validV1 = (hdr.version == 1u && sizeBytes >= v1Size);
    const bool validV2 = (hdr.version == 2u && sizeBytes >= v2Size);
    const bool validV3 = (hdr.version == 3u && sizeBytes >= v3Size);
    if (!validV1 && !validV2 && !validV3)
        return false;

    const int numToLoad = validV3 ? kPolySynthNumParams
                        : validV2 ? kV2ParamCount
                                  : kV1ParamCount;

    for (int i = 0; i < numToLoad && i < kPolySynthNumParams; ++i)
    {
        float v{};
        std::memcpy (&v, ptr, sizeof (float));
        ptr += sizeof (float);
        setParam (kPolySynthParams[i].id, v);
    }
    return true;
}

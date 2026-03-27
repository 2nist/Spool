#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
    PolyVoice — one voice of the subtractive poly-synth.

    Contains two phase-accumulator oscillators, amplitude and filter ADSR
    envelopes, and a Moog-style ladder filter (LPF24 default).

    Owned and driven by PolySynthProcessor.  Never allocated during audio;
    all buffers are pre-sized in prepare().
*/
struct PolyVoice
{
    //==========================================================================
    // Voice state

    int   note      { -1    };
    float velocity  { 0.0f  };
    bool  active    { false };
    bool  releasing { false };

    //==========================================================================
    // Oscillator phases (0 … 2π)

    double osc1Phase { 0.0 };
    double osc2Phase { 0.0 };

    //==========================================================================
    // Drift oscillator state (slow per-voice LFO, set on noteOn by processor)

    double driftOscPhase { 0.0 };
    double driftOscInc   { 0.0 };

    //==========================================================================
    // Envelopes

    juce::ADSR ampEnv;
    juce::ADSR filterEnv;

    //==========================================================================
    // Filter

    juce::dsp::LadderFilter<float> filter;

    //==========================================================================
    // Per-voice mono scratch buffer (pre-allocated in prepare())

    juce::AudioBuffer<float> scratch;

    //==========================================================================

    void prepare (double sampleRate, int maxBlock)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate       = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (maxBlock);
        spec.numChannels      = 1;

        filter.prepare (spec);
        filter.setMode (juce::dsp::LadderFilterMode::LPF24);
        filter.setResonance (0.3f);

        scratch.setSize (1, maxBlock);

        ampEnv.setSampleRate    (sampleRate);
        filterEnv.setSampleRate (sampleRate);
    }

    void reset()
    {
        osc1Phase      = 0.0;
        osc2Phase      = 0.0;
        driftOscPhase  = 0.0;
        driftOscInc    = 0.0;
        active         = false;
        releasing  = false;
        note       = -1;
        velocity   = 0.0f;
        ampEnv.reset();
        filterEnv.reset();
        filter.reset();
    }

    /** Called with ADSR params already configured via ampEnv/filterEnv.setParameters(). */
    void noteOn (int noteNumber, float vel)
    {
        note      = noteNumber;
        velocity  = vel;
        active    = true;
        releasing = false;
        osc1Phase = 0.0;
        osc2Phase = 0.0;
        ampEnv.noteOn();
        filterEnv.noteOn();
    }

    void noteOff()
    {
        releasing = true;
        ampEnv.noteOff();
        filterEnv.noteOff();
    }

    /** True once the amp envelope fully finishes its release phase. */
    bool isFinished() const noexcept
    {
        return active && ! ampEnv.isActive();
    }
};

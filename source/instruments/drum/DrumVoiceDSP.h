#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "DrumVoiceParams.h"

//==============================================================================
/**
    DrumVoiceDSP — per-voice synthesis engine for the unified drum synthesizer.

    Four synthesis layers:
      TONE   — pitched oscillator with pitch-drop envelope (kick character)
      NOISE  — band-passed noise with independent decay (snare/hat body)
      CLICK  — short transient burst, up to 5 bursts for clap simulation
      METAL  — cluster of inharmonic oscillators + HP filter (hat/cymbal)

    All envelopes are one-pole exponential (no allocations, no complex state).
    Filters are simple one-pole IIR — sufficient for drum noise shaping.
    Pink noise uses a 1-pole leaky integrator approximation.

    Real-time safe: no allocations after prepare().
*/
class DrumVoiceDSP
{
public:
    DrumVoiceDSP() = default;

    /** Must be called before first use.  May be called again on sample-rate change. */
    void prepare (double sampleRate);

    /** Apply new params and pre-compute all coefficients.
        Safe to call from audio thread between renders (params are copied locally). */
    void applyParams (const DrumVoiceParams& p);

    /** Trigger the voice with a given velocity (0..1).
        Resets all envelope states and phases. */
    void trigger (float velocity);

    /** True while any envelope amplitude is above threshold. */
    bool isActive() const noexcept { return m_active; }

    /** Render a block into buf (adds, does not overwrite). */
    void renderBlock (juce::AudioBuffer<float>& buf, int startSample, int numSamples) noexcept;

private:
    //==========================================================================
    // Simple one-pole IIR filter (HP and LP) — no juce::dsp dependency

    struct OnePole
    {
        float coeff = 0.0f;
        float state = 0.0f;

        void setLP (float fc, float fs) noexcept
        {
            coeff = std::exp (-juce::MathConstants<float>::twoPi * fc / fs);
        }

        float processLP (float x) noexcept
        {
            return state = state * coeff + x * (1.0f - coeff);
        }

        float processHP (float x) noexcept
        {
            // HP = input minus its own LP version
            state = state * coeff + x * (1.0f - coeff);
            return x - state;
        }

        void reset() noexcept { state = 0.0f; }
    };

    //==========================================================================
    // State

    double m_sampleRate  = 44100.0;
    float  m_velocity    = 1.0f;
    bool   m_active      = false;

    // TONE
    double m_tonePhase        = 0.0;
    double m_tonePitchCurrent = 55.0;
    double m_tonePitchTarget  = 55.0;
    double m_tonePitchDecayCoeff = 0.0;
    double m_toneDecayCoeff      = 0.0;
    float  m_toneEnv          = 0.0f;

    // NOISE
    float  m_noiseEnv         = 0.0f;
    double m_noiseDecayCoeff  = 0.0;
    float  m_pinkState        = 0.0f;  // pink noise 1-pole state
    OnePole m_noiseHP;
    OnePole m_noiseLP;

    // CLICK
    float  m_clickEnv         = 0.0f;
    double m_clickDecayCoeff  = 0.0;
    int    m_clickBurstCount  = 0;
    int    m_clickBurstSamples = 0;
    int    m_nextBurstAt      = 0;

    // METAL
    struct MetalOsc { double phase = 0.0; double freq = 0.0; };
    MetalOsc m_metalOscs[8];
    float    m_metalEnv        = 0.0f;
    double   m_metalDecayCoeff = 0.0;
    OnePole  m_metalHP;

    // Cached params
    DrumVoiceParams m_params;

    // Per-voice random generator (audio thread only — no locking needed)
    juce::Random m_random;

    //==========================================================================
    // Helpers

    float renderTone()   noexcept;
    float renderNoise()  noexcept;
    float renderClick()  noexcept;
    float renderMetal()  noexcept;

    /** Exponential decay coefficient from a time constant in seconds. */
    double decayCoeff (double seconds) const noexcept
    {
        if (seconds <= 0.0) return 0.0;
        return std::exp (-1.0 / (m_sampleRate * seconds));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumVoiceDSP)
};

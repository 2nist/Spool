#include "FmDrumVoice.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

//==============================================================================
void FmDrumVoice::renderBlock (float* outputL, float* outputR, int numSamples,
                               float level, float modIndex, float ratio,
                               double decayCoeff, float pan) noexcept
{
    const double twoPi = juce::MathConstants<double>::twoPi;
    const double modFreq = 440.0 * ratio;
    const double modInc = twoPi * modFreq / m_sampleRate;
    const double carInc = twoPi * 220.0 / m_sampleRate;  // Base ~A3

    double env = 1.0;
    double modPhase = m_modPhase;
    double carPhase = m_carPhase;

    const float leftGain = 1.0f - std::abs (pan);
    const float rightGain = 1.0f + std::abs (pan);

    for (int i = 0; i < numSamples; ++i)
    {
        // Modulator sine → index → carrier phase deviation
        const double mod = std::sin (modPhase) * modIndex;
        carPhase += carInc * (1.0 + mod * 0.5);

        // Carrier sine
        const double car = std::sin (carPhase);
        const float sample = static_cast<float> (car * env * level);

        outputL[i] += sample * leftGain;
        outputR[i] += sample * rightGain;

        // Exponential decay
        env *= static_cast<float> (decayCoeff);

        modPhase += modInc;
        if (modPhase >= twoPi) modPhase -= twoPi;
        if (carPhase >= twoPi) carPhase -= twoPi;

        if (env < 1e-5f) break;  // Early exit
    }

    m_modPhase = modPhase;
    m_carPhase = carPhase;
}


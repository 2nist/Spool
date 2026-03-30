#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * FmDrumVoice - 2-operator FM layer for DrumVoiceDSP.
 * Simple sine-modulated carrier for bell/metallic percussion.
 */
class FmDrumVoice
{
public:
    FmDrumVoice() = default;

    void prepare (double sampleRate) noexcept { m_sampleRate = sampleRate; }

    /** Render block, adds to output. */
    void renderBlock (float* outputL, float* outputR, int numSamples,
                      float level, float modIndex, float ratio,
                      double decayCoeff, float pan) noexcept;

private:
    double m_sampleRate = 44100.0;
    double m_carPhase = 0.0;
    double m_modPhase = 0.0;
};


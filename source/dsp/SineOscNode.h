#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/**
    SineOscNode — monophonic sine-wave oscillator triggered by MIDI.

    Used as the default synthesis node for testing the DSP chain before
    Surge XT is integrated. Kept as DSP testing fallback.

    Real-time safe:
      - No allocation in process()
      - Gain smoothed per-sample to eliminate clicks on note events
      - Phase accumulated modulo 2π to avoid denormals
*/
class SineOscNode
{
public:
    void prepare (double sampleRate, int /*blockSize*/) noexcept
    {
        m_sampleRate = sampleRate;
        m_phase      = 0.0;
        m_gain       = 0.0f;
        m_targetGain = 0.0f;
    }

    void reset() noexcept
    {
        m_phase      = 0.0;
        m_gain       = 0.0f;
        m_targetGain = 0.0f;
    }

    void process (juce::AudioBuffer<float>& buffer,
                  const juce::MidiBuffer&   midi)
    {
        // Process MIDI events first
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                m_frequency  = msg.getMidiNoteInHertz (msg.getNoteNumber());
                m_targetGain = msg.getFloatVelocity() * 0.3f;
            }
            else if (msg.isNoteOff())
            {
                m_targetGain = 0.0f;
            }
        }

        const int numCh      = juce::jmin (buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();
        if (numCh == 0 || numSamples == 0)
            return;

        const double phaseInc =
            (juce::MathConstants<double>::twoPi * m_frequency) / m_sampleRate;

        auto* left  = buffer.getWritePointer (0);
        auto* right = (numCh > 1) ? buffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            // One-pole smoothing — eliminates clicks on note events
            m_gain += (m_targetGain - m_gain) * 0.001f;

            const float sample = static_cast<float> (std::sin (m_phase)) * m_gain;
            left[i] += sample;
            if (right != nullptr)
                right[i] += sample;

            m_phase += phaseInc;
            if (m_phase >= juce::MathConstants<double>::twoPi)
                m_phase -= juce::MathConstants<double>::twoPi;
        }
    }

private:
    double m_sampleRate = 44100.0;
    double m_frequency  = 440.0;
    double m_phase      = 0.0;
    float  m_gain       = 0.0f;
    float  m_targetGain = 0.0f;
};

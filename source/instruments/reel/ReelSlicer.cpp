#include "ReelSlicer.h"

//==============================================================================
void ReelSlicer::prepare (double sampleRate, int /*blockSize*/) noexcept
{
    m_sampleRate = sampleRate;
    for (auto& v : m_voices)
        v.active = false;
}

//==============================================================================
double ReelSlicer::sliceLength() const noexcept
{
    if (m_buffer == nullptr || !m_buffer->isLoaded())
        return 0.0;

    const double totalSamples = static_cast<double> (m_buffer->getNumSamples());
    const double playLen = (m_params.play.end - m_params.play.start) * totalSamples;
    const int    count   = juce::jmax (1, m_params.slice.count);
    return playLen / static_cast<double> (count);
}

double ReelSlicer::sliceStart (int index) const noexcept
{
    if (m_buffer == nullptr || !m_buffer->isLoaded())
        return 0.0;
    const double totalSamples = static_cast<double> (m_buffer->getNumSamples());
    return m_params.play.start * totalSamples
           + static_cast<double> (index) * sliceLength();
}

double ReelSlicer::sliceEnd (int index) const noexcept
{
    return sliceStart (index) + sliceLength();
}

//==============================================================================
void ReelSlicer::triggerSlice (int sliceIndex, float velocity) noexcept
{
    if (m_buffer == nullptr || !m_buffer->isLoaded())
        return;
    if (sliceIndex < 0 || sliceIndex >= m_params.slice.count)
        return;

    // Find a free voice
    SliceVoice* target = nullptr;
    for (auto& v : m_voices)
    {
        if (!v.active) { target = &v; break; }
    }
    if (target == nullptr)
        target = &m_voices[0];   // steal first

    target->active   = true;
    target->slice    = sliceIndex;
    target->velocity = velocity;
    target->bufPos   = sliceStart (sliceIndex);
    target->bufEnd   = sliceEnd   (sliceIndex);
}

void ReelSlicer::noteOn (int midiNote, float velocity) noexcept
{
    // Map MIDI notes to slices: note 60 = slice 0, 61 = slice 1, etc.
    const int sliceIndex = midiNote - 60;
    if (sliceIndex >= 0 && sliceIndex < m_params.slice.count)
        triggerSlice (sliceIndex, velocity);
}

//==============================================================================
void ReelSlicer::renderBlock (juce::AudioBuffer<float>& buf,
                               int startSample,
                               int numSamples) noexcept
{
    if (m_buffer == nullptr || !m_buffer->isLoaded())
        return;

    const int numOutCh = buf.getNumChannels();

    for (int s = 0; s < numSamples; ++s)
    {
        float outL = 0.0f, outR = 0.0f;

        for (auto& v : m_voices)
        {
            if (!v.active) continue;

            const int numBufCh = m_buffer->getNumChannels();
            const float sampleL = m_buffer->readSample (0, v.bufPos);
            const float sampleR = (numBufCh > 1) ? m_buffer->readSample (1, v.bufPos) : sampleL;

            outL += sampleL * v.velocity;
            outR += sampleR * v.velocity;

            v.bufPos += 1.0;   // slice playback always at original speed

            if (v.bufPos >= v.bufEnd)
                v.active = false;
        }

        const float gain = static_cast<float> (m_params.out.level);
        const float pan  = juce::jlimit (-1.0f, 1.0f, m_params.out.pan);
        const float panL = 1.0f - juce::jmax (0.0f, pan);
        const float panR = 1.0f + juce::jmin (0.0f, pan);
        const int   si   = startSample + s;

        if (numOutCh == 1)
            buf.addSample (0, si, (outL + outR) * 0.5f * gain);
        else
        {
            buf.addSample (0, si, outL * panL * gain);
            buf.addSample (1, si, outR * panR * gain);
        }
    }
}

juce::Array<float> ReelSlicer::getSlicePositions() const noexcept
{
    juce::Array<float> positions;
    if (m_buffer == nullptr || !m_buffer->isLoaded())
        return positions;

    const double totalSamples = static_cast<double> (m_buffer->getNumSamples());
    if (totalSamples <= 0.0)
        return positions;

    const int count = juce::jmax (2, m_params.slice.count);
    for (int i = 0; i <= count; ++i)
        positions.add (static_cast<float> (sliceStart (i) / totalSamples));

    return positions;
}

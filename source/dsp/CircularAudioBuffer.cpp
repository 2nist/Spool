#include "CircularAudioBuffer.h"

//==============================================================================
void CircularAudioBuffer::prepare (double sampleRate, double durationSeconds)
{
    m_sampleRate   = sampleRate;
    m_duration     = juce::jlimit (1.0, MAX_DURATION, durationSeconds);
    m_totalSamples = static_cast<int> (m_sampleRate * m_duration);

    // Pre-allocate — this is the ONLY allocation; writeBlock() must not allocate
    m_buffer.setSize (NUM_CHANNELS, m_totalSamples, false, true, false);
    m_buffer.clear();

    m_writePos.store      (0, std::memory_order_relaxed);
    m_samplesWritten.store (0, std::memory_order_relaxed);

    DBG ("CircularAudioBuffer: prepared " << m_duration << "s @ "
         << sampleRate << " Hz, " << m_totalSamples << " samples");
}

//==============================================================================
void CircularAudioBuffer::writeBlock (const juce::AudioBuffer<float>& src,
                                       int numSamples) noexcept
{
    if (m_totalSamples <= 0 || numSamples <= 0) return;

    const int writePos  = m_writePos.load (std::memory_order_relaxed);
    const int numSrcCh  = src.getNumChannels();
    const int clampedN  = juce::jmin (numSamples, m_totalSamples);

    for (int ch = 0; ch < NUM_CHANNELS; ++ch)
    {
        // If source has fewer channels, repeat last available channel
        const int srcCh = (numSrcCh > 0) ? juce::jmin (ch, numSrcCh - 1) : 0;
        const float* srcPtr = src.getReadPointer (srcCh);
        float*       dstPtr = m_buffer.getWritePointer (ch);

        const int firstPart  = juce::jmin (clampedN, m_totalSamples - writePos);
        const int secondPart = clampedN - firstPart;

        juce::FloatVectorOperations::copy (dstPtr + writePos, srcPtr,             firstPart);
        if (secondPart > 0)
            juce::FloatVectorOperations::copy (dstPtr,        srcPtr + firstPart, secondPart);
    }

    const int newPos = (writePos + clampedN) % m_totalSamples;
    m_writePos.store (newPos, std::memory_order_release);

    const int prevWritten = m_samplesWritten.load (std::memory_order_relaxed);
    if (prevWritten < m_totalSamples)
        m_samplesWritten.store (juce::jmin (prevWritten + clampedN, m_totalSamples),
                                std::memory_order_relaxed);
}

//==============================================================================
int CircularAudioBuffer::readPosFromSecondsAgo (double secondsAgo) const noexcept
{
    const int samplesAgo = static_cast<int> (secondsAgo * m_sampleRate);
    int pos = m_writePos.load (std::memory_order_acquire) - samplesAgo;
    if (pos < 0)
        pos = ((pos % m_totalSamples) + m_totalSamples) % m_totalSamples;
    return pos;
}

double CircularAudioBuffer::getAvailableHistory() const noexcept
{
    if (m_sampleRate <= 0.0) return 0.0;
    return static_cast<double> (m_samplesWritten.load (std::memory_order_relaxed))
           / m_sampleRate;
}

void CircularAudioBuffer::clear()
{
    m_buffer.clear();
    m_writePos.store      (0, std::memory_order_relaxed);
    m_samplesWritten.store (0, std::memory_order_relaxed);
}

//==============================================================================
juce::AudioBuffer<float> CircularAudioBuffer::getRegion (double startSecondsAgo,
                                                          double lengthSeconds) const
{
    const int numSamples = static_cast<int> (lengthSeconds * m_sampleRate);
    juce::AudioBuffer<float> result (NUM_CHANNELS, juce::jmax (1, numSamples));
    result.clear();

    if (m_totalSamples <= 0 || numSamples <= 0) return result;

    const int readPos  = readPosFromSecondsAgo (startSecondsAgo);
    const int copyLen  = juce::jmin (numSamples, m_totalSamples);

    for (int ch = 0; ch < NUM_CHANNELS; ++ch)
    {
        const float* src = m_buffer.getReadPointer (ch);
        float*       dst = result.getWritePointer (ch);

        const int firstPart  = juce::jmin (copyLen, m_totalSamples - readPos);
        const int secondPart = copyLen - firstPart;

        juce::FloatVectorOperations::copy (dst,             src + readPos, firstPart);
        if (secondPart > 0)
            juce::FloatVectorOperations::copy (dst + firstPart, src,       secondPart);
    }

    return result;
}

//==============================================================================
juce::AudioBuffer<float> CircularAudioBuffer::getLastBars (int    numBars,
                                                            double bpm,
                                                            double beatsPerBar,
                                                            double currentBeatPosition) const
{
    if (bpm <= 0.0 || beatsPerBar <= 0.0) return juce::AudioBuffer<float> (NUM_CHANNELS, 1);

    const double beatsPerSecond      = bpm / 60.0;
    const double secondsPerBar       = beatsPerBar / beatsPerSecond;
    const double lengthSeconds       = numBars * secondsPerBar;

    // Snap start to the most recent bar boundary before the write head
    const double secondsIntoCurrentBar = std::fmod (currentBeatPosition, beatsPerBar)
                                         / beatsPerSecond;
    const double startSecondsAgo       = lengthSeconds + secondsIntoCurrentBar;

    return getRegion (startSecondsAgo, lengthSeconds);
}

//==============================================================================
juce::Array<float> CircularAudioBuffer::getWaveformRMS (int    numPoints,
                                                         double windowSeconds) const
{
    juce::Array<float> result;
    result.resize (numPoints);

    if (m_totalSamples <= 0 || numPoints <= 0 || m_sampleRate <= 0.0)
        return result;

    const int writePos      = m_writePos.load (std::memory_order_acquire);
    const int samplesWritten = m_samplesWritten.load (std::memory_order_acquire);
    const int windowSamples  = static_cast<int> (windowSeconds * m_sampleRate);
    const int available      = juce::jmin (windowSamples, samplesWritten);

    if (available <= 0) return result;

    const float samplesPerPoint = static_cast<float> (available) / static_cast<float> (numPoints);

    for (int p = 0; p < numPoints; ++p)
    {
        // p=0 = oldest (left edge), p=numPoints-1 = newest (right/present)
        const int chunkLen    = juce::jmax (1, static_cast<int> (samplesPerPoint));
        const int pointOffset = static_cast<int> ((numPoints - 1 - p) * samplesPerPoint);
        const int startOffset = pointOffset + chunkLen;

        int readPos = writePos - startOffset;
        if (readPos < 0)
            readPos = ((readPos % m_totalSamples) + m_totalSamples) % m_totalSamples;

        double sumSq = 0.0;
        for (int s = 0; s < chunkLen; ++s)
        {
            const int idx = (readPos + s) % m_totalSamples;
            for (int ch = 0; ch < NUM_CHANNELS; ++ch)
            {
                const float v = m_buffer.getSample (ch, idx);
                sumSq += static_cast<double> (v) * static_cast<double> (v);
            }
        }

        result.set (p, static_cast<float> (std::sqrt (sumSq / static_cast<double> (chunkLen * NUM_CHANNELS))));
    }

    return result;
}

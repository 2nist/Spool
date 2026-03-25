#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

//==============================================================================
/**
    CircularAudioBuffer — always-on ring buffer capturing continuous audio history.

    Designed for lock-free audio-thread writes and safe message-thread reads.
    All memory is pre-allocated in prepare() — writeBlock() never allocates.

    Thread safety:
      writeBlock()      — audio thread only
      getWaveformRMS()  — message thread only
      getRegion() / getLastBars() — message thread only (allocates)
      Atomic m_writePos provides safe concurrent access.
*/
class CircularAudioBuffer
{
public:
    static constexpr int    NUM_CHANNELS     = 2;
    static constexpr double DEFAULT_DURATION = 120.0;
    static constexpr double MAX_DURATION     = 600.0;

    /** Allocate internal buffer.  Call from prepareToPlay(), never from processBlock(). */
    void prepare (double sampleRate, double durationSeconds = DEFAULT_DURATION);

    //==========================================================================
    // Audio thread

    /** Copy numSamples from src into ring buffer.  MUST NOT allocate. */
    void writeBlock (const juce::AudioBuffer<float>& src, int numSamples) noexcept;

    //==========================================================================
    // Message thread

    /** Extract a region starting secondsAgo seconds before the current write head. */
    juce::AudioBuffer<float> getRegion (double startSecondsAgo,
                                        double lengthSeconds) const;

    /** Extract the last numBars complete bars, snapped to the bar boundary. */
    juce::AudioBuffer<float> getLastBars (int    numBars,
                                          double bpm,
                                          double beatsPerBar,
                                          double currentBeatPosition) const;

    /** Compute downsampled RMS for waveform display.
        numPoints = pixel width of the display.
        windowSeconds = how much history to cover (left edge = windowSeconds ago). */
    juce::Array<float> getWaveformRMS (int numPoints, double windowSeconds) const;

    //==========================================================================
    // State (thread-safe atomic / trivial)

    bool   isActive()          const noexcept { return m_active;      }
    void   setActive (bool a)        noexcept { m_active = a;         }

    /** Seconds of valid audio currently in the buffer (less than duration at startup). */
    double getAvailableHistory() const noexcept;

    double getDuration()         const noexcept { return m_duration;   }
    void   setSource  (int slot)       noexcept { m_sourceSlot = slot; }
    int    getSource  ()         const noexcept { return m_sourceSlot; }

    void clear();

private:
    juce::AudioBuffer<float> m_buffer;
    std::atomic<int>         m_writePos      { 0 };
    int                      m_totalSamples  = 0;
    std::atomic<int>         m_samplesWritten { 0 };
    double                   m_sampleRate    = 44100.0;
    double                   m_duration      = DEFAULT_DURATION;
    bool                     m_active        = true;
    int                      m_sourceSlot    = -1;   // -1 = master mix

    /** Map secondsAgo → sample read position in ring buffer. */
    int readPosFromSecondsAgo (double secondsAgo) const noexcept;
};

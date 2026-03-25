#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "ReelBuffer.h"
#include "ReelParams.h"

//==============================================================================
/**
    ReelSlicer — equal-division slice playback for REEL's SLICE mode.

    Divides the play region into slice.count equal parts.
    Slices are triggered by the step sequencer (triggerSlice) or by MIDI notes.
    Up to 4 slices can play simultaneously.

    Real-time safe: no allocation in renderBlock / triggerSlice.
    triggerSlice() is safe to call from the audio thread (step sequencer).
*/
class ReelSlicer
{
public:
    void prepare (double sampleRate, int blockSize) noexcept;
    void setBuffer (const ReelBuffer* buf) noexcept { m_buffer = buf; }
    void setParams (const ReelParams& p)   noexcept { m_params = p;   }

    /** Trigger a slice by index.  Thread-safe — may be called from audio thread. */
    void triggerSlice (int sliceIndex, float velocity) noexcept;

    /** MIDI note → slice index (note 60 = slice 0, note 61 = slice 1, etc.). */
    void noteOn (int midiNote, float velocity) noexcept;

    void renderBlock (juce::AudioBuffer<float>& buf,
                      int startSample,
                      int numSamples) noexcept;

    /** Returns normalised 0-1 slice boundary positions for waveform display.
        Array must be at least slice.count + 1 in size. */
    juce::Array<float> getSlicePositions() const noexcept;

private:
    struct SliceVoice
    {
        bool   active   { false };
        int    slice    { -1 };
        double bufPos   { 0.0 };
        double bufEnd   { 0.0 };
        float  velocity { 1.0f };
    };

    static constexpr int MAX_SIMULTANEOUS = 4;
    SliceVoice        m_voices[MAX_SIMULTANEOUS];
    const ReelBuffer* m_buffer      { nullptr };
    ReelParams        m_params;
    double            m_sampleRate  { 44100.0 };

    double sliceStart  (int index) const noexcept;
    double sliceEnd    (int index) const noexcept;
    double sliceLength () const noexcept;
};

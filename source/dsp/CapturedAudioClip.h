#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

/**
    CapturedAudioClip — lightweight transient descriptor for a captured audio region.

    Common transfer object for Tape → destination routing.  PluginEditor constructs
    and owns this; destinations (REEL, LOOPER, TIMELINE) each have their own load path.

    Design note: AudioHistoryStrip must not own or construct this struct directly;
    PluginEditor acts as coordinator, constructing the descriptor when routing
    from capture to a destination.

    Destinations:
      REEL     — loadClipToReelSlot()    full DSP load + Zone A editor panel
      LOOPER   — routeClipToLooper()     stores in m_looperClip; loop engine pending
      TIMELINE — routeClipToTimeline()   places clip in lane model + timeline playback queue
*/
struct CapturedAudioClip
{
    juce::AudioBuffer<float> buffer;
    double                   sampleRate  { 44100.0 };

    /** Human-readable source label, e.g. "MIX", "SLOT 01". */
    juce::String             sourceName;

    /** 0-based source slot index, or -1 for the master mix output. */
    int                      sourceSlot  { -1 };

    /** Session BPM at time of capture (0 = unknown). Used by TIMELINE. */
    double                   tempo       { 0.0 };

    /** Complete bars inferred at capture time (0 = unknown). Used by TIMELINE. */
    int                      bars        { 0 };

    /** Duration in seconds. Returns 0 if sampleRate is invalid. */
    float durationSeconds() const noexcept
    {
        if (sampleRate <= 0.0) return 0.0f;
        return static_cast<float> (buffer.getNumSamples()) / static_cast<float> (sampleRate);
    }
};

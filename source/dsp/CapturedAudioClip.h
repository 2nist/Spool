#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

/**
    CapturedAudioClip — lightweight transient descriptor for a captured audio region.

    Created when a user grabs a region from AudioHistoryStrip (via ◀4/◀8/◀16/FREE).
    Carries buffer ownership, sample rate, and source provenance metadata.

    Phase 1: passed by reference as processorRef.getGrabbedClip() + sampleRate.
    Phase 2: used as the typed payload for drag-and-drop from AudioHistoryStrip
             to Zone B module rows, enabling drop-into-REEL directly from the
             history waveform.

    Design note: AudioHistoryStrip must not own or construct this struct directly;
    PluginEditor acts as coordinator, constructing the descriptor when routing
    from capture to a destination slot.
*/
struct CapturedAudioClip
{
    juce::AudioBuffer<float> buffer;
    double                   sampleRate  { 44100.0 };

    /** Human-readable source label, e.g. "MIX", "REEL · SLOT 01". */
    juce::String             sourceName;

    /** 0-based source slot index, or -1 for the master mix output. */
    int                      sourceSlot  { -1 };

    // Future: tempo, bar length, beat-sync metadata
};

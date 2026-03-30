#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../state/AppState.h"
#include "../structure/StructureState.h"
#include "../PluginProcessor.h"  // For types; impl uses SongManager directly

/**
 * MidiClipExporter - Renders MIDI events from timeline clips, patterns, and structure
 * for DAW output/export (.mid files).
 * 
 * Step 1.2: Core MIDI generation from current app state.
 * Thread-safe for RT use; stateless design.
 */
class MidiClipExporter
{
public:
    /** Render MIDI for a beat range into buffer.
     *  @param songBeatStart Start beat (absolute song position)
     *  @param beatLength    Length in beats
     *  @param outBuffer     Target MidiBuffer (appended)
     */
    static void renderMidiForRange (double songBeatStart, double beatLength,
                                    juce::MidiBuffer& outBuffer,
                                    const SongManager& songMgr,
                                    const StructureEngine& structure,
                                    const AppState& appState,
                                    double ppqPosition = 0.0);

    /** Export slot pattern as standalone MIDI clip (.mid).
     *  @return File write success.
     */
    static bool exportPatternAsMidi (int slotIndex, const SlotPattern& pattern,
                                     double bpm, const juce::File& outputFile,
                                     const juce::String& keyRoot = "C",
                                     const juce::String& keyScale = "Major");

    /** Export current timeline to .mid file. */
    static bool exportTimelineAsMidi (const SongManager& songMgr,
                                      const juce::File& outputFile);

private:
    // Helpers
    static void renderPatternEvents (juce::MidiBuffer& buffer, double beatStart, double ppqStart,
                                     const SlotPattern& pattern, int slotNote,
                                     double bpm, const juce::String& keyRoot, const juce::String& keyScale);
    static void renderStructureChords (juce::MidiBuffer& buffer, double songBeatStart, double beatLength,
                                      const StructureState& structure, const StructureEngine& engine);
    static void renderTimelineMidiClips (juce::MidiBuffer& buffer, double songBeatStart, double beatLength,
                                        const juce::Array<TimelineClipPlacement>& placements);
};


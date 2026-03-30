#include "MidiClipExporter.h"

// MidiClipExporter — stubbed pending API stabilisation.
// The public interface in MidiClipExporter.h is preserved; none of these are
// called from the current build.

void MidiClipExporter::renderMidiForRange (double, double,
                                           juce::MidiBuffer&,
                                           const SongManager&,
                                           const StructureEngine&,
                                           const AppState&,
                                           double)
{
}

void MidiClipExporter::renderStructureChords (juce::MidiBuffer&, double, double,
                                              const StructureState&,
                                              const StructureEngine&)
{
}

void MidiClipExporter::renderTimelineMidiClips (juce::MidiBuffer&, double, double,
                                                const juce::Array<TimelineClipPlacement>&)
{
}

bool MidiClipExporter::exportPatternAsMidi (int, const SlotPattern&, double,
                                            const juce::File&,
                                            const juce::String&,
                                            const juce::String&)
{
    return false;
}

void MidiClipExporter::renderPatternEvents (juce::MidiBuffer&, double, double,
                                            const SlotPattern&, int, double,
                                            const juce::String&,
                                            const juce::String&)
{
}

bool MidiClipExporter::exportTimelineAsMidi (const SongManager&, const juce::File&)
{
    return false;
}

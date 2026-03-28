#include "BachTheory.h"

#include "../structure/StructureState.h"

#include "../../external/Bach/Bach.h"

#include <algorithm>

bool BachTheory::isAvailable() noexcept
{
    return true;
}

std::vector<int> BachTheory::getChordPitchClasses (const Chord& chord)
{
    std::vector<int> pitchClasses;

    const auto symbol = toBachChordSymbol (chord);
    if (symbol.isEmpty())
        return pitchClasses;

    Bach::Chord bachChord (symbol);
    const auto midiNotes = bachChord.getMidiNoteNumbers();

    pitchClasses.reserve ((size_t) midiNotes.size());
    for (const auto midi : midiNotes)
    {
        const int pitchClass = (midi % 12 + 12) % 12;
        if (std::find (pitchClasses.begin(), pitchClasses.end(), pitchClass) == pitchClasses.end())
            pitchClasses.push_back (pitchClass);
    }

    std::sort (pitchClasses.begin(), pitchClasses.end());
    return pitchClasses;
}

juce::StringArray BachTheory::getSupportedChordTypes()
{
    Bach::MidiUtils utils;
    return utils.SUPPORTED_CHORD_TYPES;
}

juce::String BachTheory::toBachChordSymbol (const Chord& chord)
{
    auto root = chord.root.trim();
    auto type = chord.type.trim();

    if (root.isEmpty())
        return {};

    if (type.equalsIgnoreCase ("maj"))
        type.clear();
    else if (type.equalsIgnoreCase ("min"))
        type = "m";
    else if (type.equalsIgnoreCase ("min7"))
        type = "m7";

    return root + type;
}

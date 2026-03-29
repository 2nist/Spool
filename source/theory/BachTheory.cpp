#include "BachTheory.h"

#include "../structure/StructureState.h"

#include <algorithm>
#include <array>

#if __has_include ("../../external/Bach/Bach.h")
 #include "../../external/Bach/Bach.h"
 #define SPOOL_HAS_BACH 1
#else
 #define SPOOL_HAS_BACH 0
#endif

namespace
{
int rootToPitchClass (juce::String root)
{
    root = root.trim().toUpperCase();
    if (root == "CB") return 11;
    if (root == "C" || root == "B#") return 0;
    if (root == "C#" || root == "DB") return 1;
    if (root == "D") return 2;
    if (root == "D#" || root == "EB") return 3;
    if (root == "E" || root == "FB") return 4;
    if (root == "F" || root == "E#") return 5;
    if (root == "F#" || root == "GB") return 6;
    if (root == "G") return 7;
    if (root == "G#" || root == "AB") return 8;
    if (root == "A") return 9;
    if (root == "A#" || root == "BB") return 10;
    if (root == "B") return 11;
    return -1;
}

std::vector<int> intervalsForType (juce::String type)
{
    type = type.trim().toLowerCase();
    if (type.isEmpty() || type == "maj")
        return { 0, 4, 7 };
    if (type == "min" || type == "m")
        return { 0, 3, 7 };
    if (type == "7")
        return { 0, 4, 7, 10 };
    if (type == "maj7")
        return { 0, 4, 7, 11 };
    if (type == "min7" || type == "m7")
        return { 0, 3, 7, 10 };
    if (type == "dim")
        return { 0, 3, 6 };
    if (type == "aug")
        return { 0, 4, 8 };

    return { 0, 4, 7 };
}
}

bool BachTheory::isAvailable() noexcept
{
#if SPOOL_HAS_BACH
    return true;
#else
    return false;
#endif
}

std::vector<int> BachTheory::getChordPitchClasses (const Chord& chord)
{
    std::vector<int> pitchClasses;

#if SPOOL_HAS_BACH
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
#else
    const int root = rootToPitchClass (chord.root);
    if (root < 0)
        return pitchClasses;

    const auto intervals = intervalsForType (chord.type);
    pitchClasses.reserve (intervals.size());
    for (const auto interval : intervals)
        pitchClasses.push_back ((root + interval) % 12);

    std::sort (pitchClasses.begin(), pitchClasses.end());
    pitchClasses.erase (std::unique (pitchClasses.begin(), pitchClasses.end()), pitchClasses.end());
    return pitchClasses;
#endif
}

juce::StringArray BachTheory::getSupportedChordTypes()
{
#if SPOOL_HAS_BACH
    Bach::MidiUtils utils;
    return utils.SUPPORTED_CHORD_TYPES;
#else
    return { "maj", "min", "7", "maj7", "min7", "dim", "aug" };
#endif
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

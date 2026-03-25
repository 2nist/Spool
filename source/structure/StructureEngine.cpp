#include "StructureEngine.h"

#include <array>

namespace
{
constexpr std::array<const char*, 12> kSharpNames = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

int rootToPitchClass (const juce::String& root)
{
    const auto r = root.trim();
    if (r.equalsIgnoreCase ("C")) return 0;
    if (r.equalsIgnoreCase ("C#") || r.equalsIgnoreCase ("Db")) return 1;
    if (r.equalsIgnoreCase ("D")) return 2;
    if (r.equalsIgnoreCase ("D#") || r.equalsIgnoreCase ("Eb")) return 3;
    if (r.equalsIgnoreCase ("E")) return 4;
    if (r.equalsIgnoreCase ("F")) return 5;
    if (r.equalsIgnoreCase ("F#") || r.equalsIgnoreCase ("Gb")) return 6;
    if (r.equalsIgnoreCase ("G")) return 7;
    if (r.equalsIgnoreCase ("G#") || r.equalsIgnoreCase ("Ab")) return 8;
    if (r.equalsIgnoreCase ("A")) return 9;
    if (r.equalsIgnoreCase ("A#") || r.equalsIgnoreCase ("Bb")) return 10;
    if (r.equalsIgnoreCase ("B")) return 11;
    return 0;
}

juce::String pitchClassToRoot (int pitchClass)
{
    const auto wrapped = (pitchClass % 12 + 12) % 12;
    return kSharpNames[(size_t) wrapped];
}
} // namespace

StructureEngine::StructureEngine (StructureState& state) : stateRef (state)
{
}

const Section* StructureEngine::getSectionAtBeat (double beat) const
{
    const auto barFloat = beat / 4.0;

    for (const auto& section : stateRef.sections)
    {
        const auto start = static_cast<double> (section.startBar);
        const auto end = static_cast<double> (section.startBar + section.lengthBars);
        if (barFloat >= start && barFloat < end)
            return &section;
    }

    return nullptr;
}

Chord StructureEngine::getChordAtBeat (double beat) const
{
    if (const auto* section = getSectionAtBeat (beat))
    {
        if (section->progression.empty())
            return { "C", "maj" };

        const auto barFloat = beat / 4.0;
        const auto sectionLocalBars = juce::jmax (0.0, barFloat - static_cast<double> (section->startBar));
        const auto barsPerChord = static_cast<double> (section->lengthBars) / static_cast<double> (section->progression.size());
        const auto slot = juce::jlimit (0,
                                        static_cast<int> (section->progression.size() - 1),
                                        static_cast<int> (sectionLocalBars / juce::jmax (0.0001, barsPerChord)));
        return section->progression[(size_t) slot];
    }

    return { "C", "maj" };
}

std::vector<int> StructureEngine::resolveChord (const Chord& chord) const
{
    const auto rootPc = rootToPitchClass (chord.root);

    auto lowerType = chord.type.toLowerCase();
    if (lowerType.isEmpty())
        lowerType = "maj";

    std::vector<int> intervals { 0, 4, 7 };
    if (lowerType == "min" || lowerType == "m")
        intervals = { 0, 3, 7 };
    else if (lowerType == "dim")
        intervals = { 0, 3, 6 };
    else if (lowerType == "aug")
        intervals = { 0, 4, 8 };
    else if (lowerType == "sus2")
        intervals = { 0, 2, 7 };
    else if (lowerType == "sus4")
        intervals = { 0, 5, 7 };
    else if (lowerType == "7" || lowerType == "dom7")
        intervals = { 0, 4, 7, 10 };
    else if (lowerType == "maj7")
        intervals = { 0, 4, 7, 11 };
    else if (lowerType == "min7" || lowerType == "m7")
        intervals = { 0, 3, 7, 10 };

    std::vector<int> out;
    out.reserve (intervals.size());
    for (const auto i : intervals)
        out.push_back ((rootPc + i) % 12);

    return out;
}

std::vector<int> StructureEngine::resolveChordAtBeat (double beat) const
{
    return resolveChord (getChordAtBeat (beat));
}

void StructureEngine::transpose (int semitones)
{
    for (auto& section : stateRef.sections)
    {
        for (auto& chord : section.progression)
        {
            const auto rootPc = rootToPitchClass (chord.root);
            chord.root = pitchClassToRoot (rootPc + semitones);
        }
    }
}

void StructureEngine::rebuild()
{
    int maxBars = 0;
    for (const auto& section : stateRef.sections)
        maxBars = juce::jmax (maxBars, section.startBar + section.lengthBars);

    stateRef.totalBars = maxBars;
}

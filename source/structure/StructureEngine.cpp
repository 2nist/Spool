#include "StructureEngine.h"

#include <array>
#include <cmath>

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

std::optional<ResolvedSectionInstance> StructureEngine::getSectionAtBeat (double beat) const
{
    if (! structureHasScaffold (stateRef))
        return std::nullopt;

    const auto resolved = buildResolvedStructure (stateRef);
    const auto clampedBeat = juce::jmax (0.0, beat);

    for (const auto& instance : resolved)
    {
        const auto start = static_cast<double> (instance.startBeat);
        const auto end = static_cast<double> (instance.startBeat + instance.totalBeats);
        if (clampedBeat >= start && clampedBeat < end)
            return instance;
    }

    return std::nullopt;
}

Chord StructureEngine::getChordAtBeat (double beat) const
{
    const auto instance = getSectionAtBeat (beat);
    if (! instance.has_value() || instance->section == nullptr)
        return { "C", "maj" };

    if (instance->section->progression.empty())
        return { "C", "maj" };

    const auto perRepeatBeats = juce::jmax (1, instance->barsPerRepeat * instance->beatsPerBar);
    const auto localBeat = juce::jmax (0.0, beat - static_cast<double> (instance->startBeat));
    const auto beatInRepeat = std::fmod (localBeat, static_cast<double> (perRepeatBeats));
    const auto barInRepeat = beatInRepeat / static_cast<double> (instance->beatsPerBar);
    const auto slot = juce::jmax (0, static_cast<int> (barInRepeat)) % static_cast<int> (instance->section->progression.size());
    return instance->section->progression[(size_t) slot];
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
    for (const auto interval : intervals)
        out.push_back ((rootPc + interval) % 12);

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
    stateRef.totalBars = computeStructureTotalBars (stateRef);
    stateRef.totalBeats = computeStructureTotalBeats (stateRef);
    stateRef.estimatedDurationSeconds = computeStructureEstimatedDurationSeconds (stateRef);
}

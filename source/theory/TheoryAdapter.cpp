#include "TheoryAdapter.h"

#include "../structure/StructureEngine.h"
#include "../structure/StructureState.h"

#include <array>

namespace
{
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
} // namespace

std::vector<int> TheoryAdapter::getScalePitchClasses (const juce::String& root, const juce::String& scale) const
{
    static constexpr std::array<int, 7> majorSteps { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr std::array<int, 7> minorSteps { 0, 2, 3, 5, 7, 8, 10 };

    const auto tonic = rootToPitchClass (root);
    const bool isMinor = scale.equalsIgnoreCase ("minor") || scale.equalsIgnoreCase ("min");
    const auto& steps = isMinor ? minorSteps : majorSteps;

    std::vector<int> out;
    out.reserve (steps.size());
    for (auto s : steps)
        out.push_back ((tonic + s) % 12);

    return out;
}

std::vector<int> TheoryAdapter::getChordPitchClasses (const Chord& chord, const StructureEngine* engine) const
{
    if (engine != nullptr)
        return engine->resolveChord (chord);

    // Fallback to a major triad if no engine is available.
    const auto rootPc = rootToPitchClass (chord.root);
    return { rootPc, (rootPc + 4) % 12, (rootPc + 7) % 12 };
}

#include "TheoryAdapter.h"
#include "BachTheory.h"
#include "DiatonicHarmony.h"

#include "../structure/StructureEngine.h"
#include "../structure/StructureState.h"

std::vector<int> TheoryAdapter::getScalePitchClasses (const juce::String& root, const juce::String& scale) const
{
    const auto tonic = DiatonicHarmony::pitchClassForRoot (root);
    const auto steps = DiatonicHarmony::modeIntervals (scale);

    std::vector<int> out;
    out.reserve (steps.size());
    for (auto s : steps)
        out.push_back ((tonic + s) % 12);

    return out;
}

std::vector<int> TheoryAdapter::getChordPitchClasses (const Chord& chord, const StructureEngine* engine) const
{
    if (BachTheory::isAvailable())
    {
        const auto bachPitchClasses = BachTheory::getChordPitchClasses (chord);
        if (! bachPitchClasses.empty())
            return bachPitchClasses;
    }

    if (engine != nullptr)
        return engine->resolveChord (chord);

    // Fallback to a major triad if no engine is available.
    const auto rootPc = DiatonicHarmony::pitchClassForRoot (chord.root);
    return { rootPc, (rootPc + 4) % 12, (rootPc + 7) % 12 };
}

#pragma once

#include <juce_core/juce_core.h>
#include <algorithm>
#include <vector>

struct Chord
{
    juce::String root;
    juce::String type;
};

struct Section
{
    juce::String id;
    juce::String name;
    int bars = 4;
    int beatsPerBar = 4;
    int repeats = 1;
    juce::String keyRoot;
    juce::String mode;
    juce::String harmonicCenter = "I";
    std::vector<Chord> progression;
    juce::String transitionIntent;
};

using SectionDefinition = Section;

struct ArrangementBlock
{
    juce::String id;
    juce::String sectionId;
    int orderIndex = 0;
    int barsOverride = 0;
    int repeatsOverride = 0;
    juce::String transitionIntentOverride;
};

using ArrangementInstance = ArrangementBlock;

struct ResolvedSectionInstance
{
    const Section* section = nullptr;
    const ArrangementBlock* block = nullptr;
    int arrangementIndex = -1;
    int startBar = 0;
    int startBeat = 0;
    int barsPerRepeat = 4;
    int beatsPerBar = 4;
    int repeats = 1;
    int totalBars = 4;
    int totalBeats = 16;
    juce::String transitionIntent;
};

struct StructureState
{
    std::vector<Section> sections;
    std::vector<ArrangementBlock> arrangement;
    int songTempo = 120;
    juce::String songKey = "C";
    juce::String songMode = "Major";
    int totalBars = 0;
    int totalBeats = 0;
    double estimatedDurationSeconds = 0.0;
};

using SongStructureState = StructureState;

inline juce::String makeStructureId()
{
    return juce::Uuid().toString();
}

inline const Section* findStructureSectionById (const StructureState& state, const juce::String& sectionId)
{
    for (const auto& section : state.sections)
        if (section.id == sectionId)
            return &section;

    return nullptr;
}

inline bool structureHasScaffold (const StructureState& state)
{
    return ! state.sections.empty() && ! state.arrangement.empty();
}

inline std::vector<ResolvedSectionInstance> buildResolvedStructure (const StructureState& state)
{
    std::vector<const ArrangementBlock*> sortedBlocks;
    sortedBlocks.reserve (state.arrangement.size());
    for (const auto& block : state.arrangement)
        sortedBlocks.push_back (&block);

    std::sort (sortedBlocks.begin(),
               sortedBlocks.end(),
               [] (const ArrangementBlock* lhs, const ArrangementBlock* rhs)
               {
                   if (lhs->orderIndex != rhs->orderIndex)
                       return lhs->orderIndex < rhs->orderIndex;
                   return lhs->id < rhs->id;
               });

    std::vector<ResolvedSectionInstance> resolved;
    resolved.reserve (sortedBlocks.size());

    int runningBars = 0;
    int runningBeats = 0;
    for (size_t i = 0; i < sortedBlocks.size(); ++i)
    {
        const auto* block = sortedBlocks[i];
        const auto* section = findStructureSectionById (state, block->sectionId);
        if (section == nullptr)
            continue;

        ResolvedSectionInstance instance;
        instance.section = section;
        instance.block = block;
        instance.arrangementIndex = static_cast<int> (i);
        instance.startBar = runningBars;
        instance.startBeat = runningBeats;
        instance.barsPerRepeat = juce::jmax (1, block->barsOverride > 0 ? block->barsOverride : section->bars);
        instance.beatsPerBar = juce::jmax (1, section->beatsPerBar);
        instance.repeats = juce::jmax (1, block->repeatsOverride > 0 ? block->repeatsOverride : section->repeats);
        instance.totalBars = instance.barsPerRepeat * instance.repeats;
        instance.totalBeats = instance.totalBars * instance.beatsPerBar;
        instance.transitionIntent = block->transitionIntentOverride.isNotEmpty()
                                      ? block->transitionIntentOverride
                                      : section->transitionIntent;

        runningBars += instance.totalBars;
        runningBeats += instance.totalBeats;
        resolved.push_back (instance);
    }

    return resolved;
}

inline int computeStructureTotalBars (const StructureState& state)
{
    int totalBars = 0;
    for (const auto& instance : buildResolvedStructure (state))
        totalBars += instance.totalBars;
    return totalBars;
}

inline int computeStructureTotalBeats (const StructureState& state)
{
    int totalBeats = 0;
    for (const auto& instance : buildResolvedStructure (state))
        totalBeats += instance.totalBeats;
    return totalBeats;
}

inline double computeStructureEstimatedDurationSeconds (const StructureState& state)
{
    const auto totalBeats = computeStructureTotalBeats (state);
    const auto tempo = juce::jmax (1, state.songTempo);
    return static_cast<double> (totalBeats) * 60.0 / static_cast<double> (tempo);
}

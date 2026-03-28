#pragma once

#include "StructureState.h"
#include "../dsp/SongManager.h"
#include <optional>
#include <vector>

class StructureEngine
{
public:
    explicit StructureEngine (SongManager& songManager);

    std::optional<ResolvedSectionInstance> getSectionAtBeat (double beat) const;
    Chord getChordAtBeat (double beat) const;
    std::vector<int> resolveChord (const Chord& chord) const;
    std::vector<int> resolveChordAtBeat (double beat) const;
    void transpose (int semitones);
    void rebuild();

private:
    SongManager& songManagerRef;
};

#pragma once

#include "StructureState.h"
#include <vector>

class StructureEngine
{
public:
    explicit StructureEngine (StructureState& state);

    const Section* getSectionAtBeat (double beat) const;
    Chord getChordAtBeat (double beat) const;
    std::vector<int> resolveChord (const Chord& chord) const;
    std::vector<int> resolveChordAtBeat (double beat) const;
    void transpose (int semitones);
    void rebuild();

private:
    StructureState& stateRef;
};

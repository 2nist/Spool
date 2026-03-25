#pragma once

#include "../structure/StructureState.h"
#include "../theory/TheoryAdapter.h"

#include <vector>

class StructureEngine;

class MidiConstraintEngine
{
public:
    enum class NotePolicy
    {
        Constrain,
        Guide,
        Bypass
    };

    struct GuidanceResult
    {
        int inputNote = 60;
        int suggestedNote = 60;
        bool wouldChange = false;
        bool inScale = true;
        bool inChord = true;
    };

    void setStructure (const StructureState* structure);
    void setStructureEngine (const StructureEngine* engine);

    int processNote (int midiNote, double beat);
    GuidanceResult analyzeNote (int midiNote, double beat) const;

    void setScaleLock (bool enabled);
    void setChordLock (bool enabled);
    void setNotePolicy (NotePolicy policy);
    NotePolicy getNotePolicy() const noexcept;

private:
    static int snapToNearest (const std::vector<int>& allowedPitchClasses, int inputNote);
    std::vector<int> buildAllowedPitchClasses (double beat, bool* inScale, bool* inChord, int midiNote) const;

    const StructureState* structureRef { nullptr };
    const StructureEngine* engineRef { nullptr };
    TheoryAdapter theory;

    bool scaleLockEnabled { false };
    bool chordLockEnabled { false };
    NotePolicy notePolicy { NotePolicy::Constrain };
};

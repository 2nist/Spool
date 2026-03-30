#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include "components/ZoneB/DrumMachineData.h"

TEST_CASE ("drum undo/redo restores drum voice steps", "[drum][undo]")
{
    // Unit test of UndoableAction with DrumMachineData (no UI)
    DrumMachineData target;
    DrumMachineData before = DrumMachineData::makeDefault();
    DrumMachineData after  = before;
    REQUIRE (after.voices.size() > 0);
    after.voices[0].toggleStep (0);

    // write initial state
    target = before;
    REQUIRE (target.voices[0].stepBits == before.voices[0].stepBits);

    juce::UndoManager um;
    struct PlainDrumEditAction : public juce::UndoableAction
    {
        DrumMachineData* dst;
        DrumMachineData before, after;
        PlainDrumEditAction (DrumMachineData* d, DrumMachineData b, DrumMachineData a)
            : dst (d), before (std::move (b)), after (std::move (a)) {}
        bool perform() override { *dst = after; return true; }
        bool undo() override    { *dst = before; return true; }
        int getSizeInUnits() override { return 1; }
    };

    um.perform (new PlainDrumEditAction (&target, before, after));
    REQUIRE (target.voices[0].stepBits == after.voices[0].stepBits);

    um.undo();
    REQUIRE (target.voices[0].stepBits == before.voices[0].stepBits);

    um.redo();
    REQUIRE (target.voices[0].stepBits == after.voices[0].stepBits);
}

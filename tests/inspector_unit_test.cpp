#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include "components/ZoneB/StepGridSingle.h"
#include "components/ZoneB/StepGridSingleInspector.h"
#include "components/ZoneB/SlotPattern.h"

TEST_CASE ("StepGridSingleInspector programmatic edits", "[inspector][unit]")
{
    juce::ScopedJuceInitialiser_GUI init;

    SlotPattern pattern;
    pattern.setStepCount (1);
    auto* step = pattern.getStep (0);
    REQUIRE (step != nullptr);
    step->microEventCount = 0;
    step->gateMode = SlotPattern::GateMode::rest;
    step->duration = 1;

    StepGridSingle grid;
    grid.setPattern (&pattern, juce::Colours::white, -1);

    StepGridSingleInspector insp;
    insp.setStepGridSingle (&grid);

    bool beforeCalled = false;
    bool modifiedCalled = false;
    bool gestureEndCalled = false;

    grid.onBeforeEdit = [&]() { beforeCalled = true; };
    grid.onModified = [&]() { modifiedCalled = true; };
    grid.onGestureEnd = [&]() { gestureEndCalled = true; };

    // Velocity
    insp.setVelocityAndCommit (64);
    REQUIRE (beforeCalled);
    REQUIRE (modifiedCalled);
    REQUIRE (gestureEndCalled);
    REQUIRE (pattern.getStep(0)->microEventCount == 1);
    REQUIRE (pattern.getStep(0)->microEvents[0].velocity == 64);

    // Reset flags
    beforeCalled = modifiedCalled = gestureEndCalled = false;

    // Gate on
    insp.setGateAndCommit (true);
    REQUIRE (beforeCalled);
    REQUIRE (modifiedCalled);
    REQUIRE (gestureEndCalled);
    REQUIRE (pattern.getStep(0)->gateMode != SlotPattern::GateMode::rest);

    // Reset flags
    beforeCalled = modifiedCalled = gestureEndCalled = false;

    // Length change
    insp.setLengthAndCommit (4);
    REQUIRE (beforeCalled);
    REQUIRE (modifiedCalled);
    REQUIRE (gestureEndCalled);
    REQUIRE (pattern.getStep(0)->duration == 4);
}

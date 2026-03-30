#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include "components/ZoneB/StepGridDrum.h"
#include "components/ZoneB/DrumMachineData.h"

TEST_CASE ("drum velocity gradient snapshot", "[drum][visual]")
{
    // Prepare drum data with a velocity gradient on voice 0
    DrumMachineData data = DrumMachineData::makeDefault();
    REQUIRE (data.voices.size() > 0);
    auto& v0 = data.voices[0];
    const int n = v0.stepCount;
    for (int s = 0; s < n; ++s)
    {
        const float vel = 0.05f + 0.95f * (float) s / juce::jmax (1, n - 1);
        v0.setStepVelocity (s, vel);
        // activate every other step so gradients are visible
        if ((s % 2) == 0)
            v0.setStepActive (s, true);
        else
            v0.setStepActive (s, false);
    }

    // Create component and layout
    StepGridDrum grid;
    grid.setSize (800, 200);
    grid.setDrumData (&data, juce::Colour (0xFF4B9EDB));

    juce::Component parent;
    parent.setSize (800, 200);
    parent.addChildComponent (grid);
    grid.setVisible (true);

    // Snapshot the grid and write to temp PNG to verify rendering runs
    auto img = parent.createComponentSnapshot (parent.getLocalBounds(), true);
    // Write to a deterministic path inside the repository so tooling can find it
    juce::File outDir ("C:/Dev/DAW/PampleJUCE/Testing");
    outDir.createDirectory();
    juce::File out = outDir.getChildFile ("drum_velocity_snapshot.png");
    if (out.existsAsFile())
        out.deleteFile();
    juce::PNGImageFormat png;
    juce::FileOutputStream stream (out);
    const bool ok = png.writeImageToStream (img, stream);

    std::cout << "Writing snapshot to: " << out.getFullPathName() << std::endl;

    // Also write the path into a small file in the output directory for tooling to find
    juce::File marker = out.getSiblingFile ("last_snapshot_path.txt");
    marker.replaceWithText (out.getFullPathName());

    REQUIRE (ok);
    REQUIRE (out.existsAsFile());
}

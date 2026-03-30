#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>
#include <iostream>
#include "components/ZoneB/StepGridDrum.h"
#include "components/ZoneB/DrumMachineData.h"

static juce::Image renderStepGridSnapshot (DrumMachineData& data, int w = 800, int h = 200)
{
    StepGridDrum grid;
    grid.setSize (w, h);
    grid.setDrumData (&data, juce::Colour (0xFF4B9EDB));

    juce::Component parent;
    parent.setSize (w, h);
    parent.addChildComponent (grid);
    grid.setVisible (true);

    return parent.createComponentSnapshot (parent.getLocalBounds(), true);
}

TEST_CASE ("drum velocity pixel-compare baseline", "[drum][visual][baseline]")
{
    juce::File baselineFile ("tests/visual/baselines/drum_velocity_baseline.png");
    REQUIRE (baselineFile.existsAsFile());

    // prepare drum data same as snapshot test
    DrumMachineData data = DrumMachineData::makeDefault();
    REQUIRE (data.voices.size() > 0);
    auto& v0 = data.voices[0];
    const int n = v0.stepCount;
    for (int s = 0; s < n; ++s)
    {
        const float vel = 0.05f + 0.95f * (float) s / juce::jmax (1, n - 1);
        v0.setStepVelocity (s, vel);
        if ((s % 2) == 0)
            v0.setStepActive (s, true);
        else
            v0.setStepActive (s, false);
    }

    auto img = renderStepGridSnapshot (data, 800, 200);

    // load baseline
    juce::PNGImageFormat png;
    std::unique_ptr<juce::InputStream> in (baselineFile.createInputStream());
    REQUIRE (in != nullptr);
    juce::Image baseline = png.decodeImage (*in);
    REQUIRE (baseline.isValid());

    REQUIRE (img.getWidth() == baseline.getWidth());
    REQUIRE (img.getHeight() == baseline.getHeight());

    const int w = img.getWidth();
    const int h = img.getHeight();
    int diffCount = 0;
    const int total = w * h;
    const int channelThreshold = 6; // per-channel tolerance

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            auto a = img.getPixelAt (x, y);
            auto b = baseline.getPixelAt (x, y);
            if ( std::abs ((int) a.getRed()   - (int) b.getRed())   > channelThreshold ||
                 std::abs ((int) a.getGreen() - (int) b.getGreen()) > channelThreshold ||
                 std::abs ((int) a.getBlue()  - (int) b.getBlue())  > channelThreshold )
            {
                ++diffCount;
            }
        }
    }

    double diffPercent = 100.0 * (double) diffCount / (double) total;
    std::cout << "Pixel diff: " << diffCount << " / " << total << " (" << diffPercent << "%)\n";

    // Allow small rendering differences; fail if >0.5% pixels differ
    REQUIRE (diffPercent <= 0.5);
}

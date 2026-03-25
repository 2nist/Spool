#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    SlotPattern — per-slot step-pattern data.

    8 independent patterns, each with its own step count (1–64 steps).
    Shared by ModuleSlot (synth/sampler/etc.) and DrumVoice (drum machine).
*/
struct SlotPattern
{
    static constexpr int MAX_STEPS    = 64;
    static constexpr int NUM_PATTERNS = 8;

    int  stepCount [NUM_PATTERNS]             = { 16,16,16,16,16,16,16,16 };
    bool active    [NUM_PATTERNS][MAX_STEPS]  = {};
    bool accent    [NUM_PATTERNS][MAX_STEPS]  = {};

    int  currentPattern = 0;

    int  activeStepCount() const noexcept { return stepCount[currentPattern]; }

    bool stepActive (int step) const noexcept
    {
        return step < activeStepCount() && active[currentPattern][step];
    }

    bool stepAccented (int step) const noexcept
    {
        return step < activeStepCount() && accent[currentPattern][step];
    }

    void toggleStep (int step)
    {
        if (step < activeStepCount())
            active[currentPattern][step] = !active[currentPattern][step];
    }

    void setStepCount (int n)
    {
        stepCount[currentPattern] = juce::jlimit (1, MAX_STEPS, n);
    }

    void clearPattern()
    {
        for (int s = 0; s < MAX_STEPS; ++s)
        {
            active [currentPattern][s] = false;
            accent [currentPattern][s] = false;
        }
    }

    SlotPattern copyPattern() const { return *this; }
};

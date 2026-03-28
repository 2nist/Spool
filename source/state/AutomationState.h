#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct AutomationPoint
{
    double beat { 0.0 };
    float value { 0.0f };
};

struct AutomationLane
{
    int id { 0 };
    juce::String targetId;
    juce::String displayName;
    juce::String scope;
    bool enabled { true };
    std::vector<AutomationPoint> points;
};

struct AutomationState
{
    std::vector<AutomationLane> lanes;
    int nextId { 1 };
};

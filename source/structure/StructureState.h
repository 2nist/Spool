#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct Chord
{
    juce::String root;
    juce::String type;
};

struct Section
{
    juce::String name;
    int startBar = 0;
    int lengthBars = 4;
    std::vector<Chord> progression;
};

struct StructureState
{
    std::vector<Section> sections;
    int totalBars = 0;
};

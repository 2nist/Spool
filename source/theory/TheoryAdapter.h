#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct Chord;
class StructureEngine;

class TheoryAdapter
{
public:
    std::vector<int> getScalePitchClasses (const juce::String& root, const juce::String& scale) const;
    std::vector<int> getChordPitchClasses (const Chord& chord, const StructureEngine* engine) const;
};

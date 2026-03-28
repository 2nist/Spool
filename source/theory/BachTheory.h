#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct Chord;

class BachTheory
{
public:
    static bool isAvailable() noexcept;
    static std::vector<int> getChordPitchClasses (const Chord& chord);
    static juce::StringArray getSupportedChordTypes();

private:
    static juce::String toBachChordSymbol (const Chord& chord);
};

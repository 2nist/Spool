#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct LyricStyle
{
    juce::String fontName { "Mono" };
    int sizeIndex { 1 };
    uint32_t colourArgb { 0xFFE6DCCF };
};

struct LyricItem
{
    int id { 0 };
    juce::String text;
    LyricStyle style;
    double beatPosition { -1.0 };
    juce::String sectionId;
    bool pinnedToTimeline { false };
};

struct LyricsState
{
    std::vector<LyricItem> items;
    int nextId { 1 };
};

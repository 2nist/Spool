#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    ClipData — data model for Zone D timeline clips and lanes.
    No audio processing — visual/UI data only.
    Structs only — no .cpp needed.
*/

//==============================================================================
enum class ClipType { audio, midi, output, scaffold };

//==============================================================================
struct Clip
{
    float        startBeat   = 0.0f;
    float        lengthBeats = 0.0f;
    juce::String clipId;
    juce::String name;
    ClipType     type        = ClipType::audio;
    bool         selected    = false;
    bool         armed       = false;
    juce::Colour tint        = juce::Colours::transparentBlack;
    juce::Array<float> waveformPreview; // normalized 0..1 peak envelope bins
};

//==============================================================================
struct LaneData
{
    int          slotIndex    = -1;
    juce::String moduleType;    // "synth", "sampler", "vst3", "output"
    bool         active       = false;
    bool         armed        = false;
    bool         autoExpanded = false;
    juce::Colour dotColor     = juce::Colour (0xFF4a3e2e); // inactive
    float        mixLevel     = 0.8f;
    juce::Array<Clip> clips;
};

//==============================================================================
// Helper: resolve dot colour and clip type from module type string
inline juce::Colour dotColorForModule (const juce::String& moduleType)
{
    if (moduleType == "output")
        return Theme::Colour::error;
    if (moduleType.isNotEmpty())
        return Theme::Signal::audio;
    return Theme::Colour::inactive;
}

inline ClipType clipTypeForModule (const juce::String& moduleType)
{
    if (moduleType == "sampler") return ClipType::midi;
    if (moduleType == "output")  return ClipType::output;
    return ClipType::audio;
}

inline juce::Colour sectionColourForIndex (int index)
{
    static const juce::Colour palette[] = {
        Theme::Zone::a,
        Theme::Zone::b,
        Theme::Zone::c,
        Theme::Zone::d,
        Theme::Colour::accent,
        Theme::Signal::audio
    };
    const int count = static_cast<int> (sizeof (palette) / sizeof (palette[0]));
    const int idx = (index < 0) ? 0 : (index % count);
    return palette[idx];
}

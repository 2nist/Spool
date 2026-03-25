#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

#include "../structure/StructureState.h"

enum class ModuleType
{
    Empty,
    Synth,
    Sampler,
    Sequencer,
    Vst3,
    DrumMachine,
    Output,
    Reel
};

struct SongState
{
    juce::String title;
    int bpm = 120;
    int numerator = 4;
    int denominator = 4;

    juce::String keyRoot = "C";
    juce::String keyScale = "Major";

    juce::String notes;
};

struct TransportState
{
    double bpm = 120.0;
    double playheadBeats = 0.0;
    bool isPlaying = false;
    bool isRecording = false;
};

struct SlotState
{
    int slotIndex = -1;
    ModuleType type = ModuleType::Empty;

    juce::String name;
    juce::String loadedPatch;

    float level = 0.8f;
    bool muted = false;
    bool solo = false;

    juce::ValueTree parameters { "parameters" };
};

struct AppState
{
    SongState song;
    TransportState transport;
    std::vector<SlotState> slots;
    StructureState structure;
};

#pragma once

#include "SlotPattern.h"

//==============================================================================

enum class VoiceMode { sample, midi };

//==============================================================================
/**
    DrumVoice — one voice in a drum machine module.

    Stores its own SlotPattern so each voice has independent step data.
*/
struct DrumVoice
{
    juce::String name        = "VOICE";
    SlotPattern  pattern;
    VoiceMode    mode        = VoiceMode::midi;

    // Sample mode
    juce::File   sampleFile;
    // TODO: actual sample loading in DSP session

    // MIDI mode
    int          midiNote    = 36;   // C2 default
    int          midiChannel = 1;

    // Both modes
    float        velocity    = 1.0f;
    bool         muted       = false;
    juce::Colour color       { 0xFF4B9EDB };
};

#pragma once

#include "DrumVoice.h"

//==============================================================================
/**
    DrumMachineData — all voice data for a DRUM MACHINE module slot.

    Owns an unlimited array of DrumVoice instances.
    Initialises with 4 default voices (KICK / SNARE / HAT / PERC).
*/
struct DrumMachineData
{
    juce::OwnedArray<DrumVoice> voices;
    int                         focusedVoiceIndex = 0;

    void initDefaults()
    {
        struct Preset { const char* name; int midiNote; juce::uint32 col; };

        for (auto& p : { Preset { "KICK",  36, 0xFFDB8E4B },
                         Preset { "SNARE", 38, 0xFF4B9EDB },
                         Preset { "HAT",   42, 0xFF9EDB4B },
                         Preset { "PERC",  39, 0xFFDB4B9E } })
        {
            auto* v      = voices.add (new DrumVoice{});
            v->name      = p.name;
            v->midiNote  = p.midiNote;
            v->color     = juce::Colour (p.col);
        }
    }

    /** Returns the MIDI note name for display (e.g. "C2"). */
    static juce::String midiNoteName (int note)
    {
        static const char* noteNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        const int octave = (note / 12) - 2;
        return juce::String (noteNames[note % 12]) + juce::String (octave);
    }
};

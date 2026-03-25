#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/**
    MidiRouter — distributes an incoming MidiBuffer to per-slot MidiBuffers.

    Routing rules:
      1. If an event's channel matches a slot's assigned channel → that slot only.
      2. Otherwise → the focused slot (default: slot 0).
      3. Sequencer-generated MIDI bypasses the router — goes directly to
         per-slot buffers in PluginProcessor::processBlock().

    Real-time safe: no allocation, no locks.
*/
class MidiRouter
{
public:
    static constexpr int kNumSlots = 8;

    MidiRouter() noexcept = default;

    /** Set which slot has keyboard focus (receives omni-channel MIDI). */
    void setFocusedSlot (int slotIndex) noexcept;

    /** Set the MIDI channel a slot listens on.  -1 = omni (unused internally). */
    void setSlotMidiChannel (int slotIndex, int channel) noexcept;

    /** Route all events from \p input into the appropriate \p slotBuffers entries.
        slotBuffers must point to an array of at least kNumSlots MidiBuffers,
        each already cleared by the caller before this call. */
    void route (const juce::MidiBuffer&  input,
                juce::MidiBuffer*        slotBuffers,
                int                      numSlots) const noexcept;

private:
    int m_focusedSlot                  = 0;
    int m_slotChannels[kNumSlots]      = { -1,-1,-1,-1,-1,-1,-1,-1 };
};

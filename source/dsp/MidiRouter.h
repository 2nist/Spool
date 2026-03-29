#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

//==============================================================================
/**
    MidiRouter — distributes an incoming MidiBuffer to per-slot MidiBuffers.

    Routing rules:
      1. If an event's channel matches a slot's assigned channel → that slot only.
      2. Otherwise → the focused slot (default: slot 0).
      3. Sequencer-generated MIDI bypasses the router — goes directly to
         per-slot buffers in PluginProcessor::processBlock().

    Real-time safe: no allocation, no locks.

    Activity counters (written audio thread, read UI thread via relaxed atomics):
      getInputTick()     — increments on every event entering route()
      getDestTick(slot)  — increments on every event routed to that slot
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

    /** UI-thread readable activity counters.  Wraps at UINT32_MAX — compare
        with != to detect change, not with >. */
    uint32_t getInputTick()           const noexcept { return m_inputTick.load (std::memory_order_relaxed); }
    uint32_t getDestTick (int slot)   const noexcept
    {
        if (slot < 0 || slot >= kNumSlots) return 0;
        return m_destTicks[slot].load (std::memory_order_relaxed);
    }

private:
    int m_focusedSlot                  = 0;
    int m_slotChannels[kNumSlots]      = { -1,-1,-1,-1,-1,-1,-1,-1 };

    // Activity counters — mutable so route() (logically const) can increment them
    mutable std::atomic<uint32_t> m_inputTick   { 0 };
    mutable std::atomic<uint32_t> m_destTicks[kNumSlots] {};
};

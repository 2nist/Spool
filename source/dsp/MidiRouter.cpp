#include "MidiRouter.h"

void MidiRouter::setFocusedSlot (int slotIndex) noexcept
{
    m_focusedSlot = juce::jlimit (0, kNumSlots - 1, slotIndex);
}

void MidiRouter::setSlotMidiChannel (int slotIndex, int channel) noexcept
{
    if (slotIndex >= 0 && slotIndex < kNumSlots)
        m_slotChannels[slotIndex] = channel;
}

void MidiRouter::route (const juce::MidiBuffer& input,
                        juce::MidiBuffer*       slotBuffers,
                        int                     numSlots) const noexcept
{
    if (numSlots <= 0 || slotBuffers == nullptr)
        return;

    for (const auto meta : input)
    {
        const auto msg = meta.getMessage();
        const int  ch  = msg.getChannel();   // 1-16, or 0 for sysex/etc.

        // Check for a slot with a specific channel match
        int targetSlot = m_focusedSlot;
        for (int s = 0; s < numSlots; ++s)
        {
            if (m_slotChannels[s] >= 1 && m_slotChannels[s] == ch)
            {
                targetSlot = s;
                break;
            }
        }

        targetSlot = juce::jlimit (0, numSlots - 1, targetSlot);
        slotBuffers[targetSlot].addEvent (msg, meta.samplePosition);
    }
}

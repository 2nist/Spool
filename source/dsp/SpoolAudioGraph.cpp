#include "SpoolAudioGraph.h"

SpoolAudioGraph::SpoolAudioGraph()
{
    for (int i = 0; i < kNumSlots; ++i)
    {
        m_slotActive    [i].store (i == 0);  // slot 0 active by default for first sound
        m_slotLevel     [i].store (1.0f);
        m_slotSynthType [i].store (0);       // 0 = SineOscNode
        m_slotMeter     [i].store (0.0f);
    }
}

void SpoolAudioGraph::prepare (double sampleRate, int blockSize)
{
    m_blockSize = blockSize;

    for (int i = 0; i < kNumSlots; ++i)
    {
        m_sineNodes   [i].prepare (sampleRate, blockSize);
        m_polySynths  [i].prepare (sampleRate, blockSize);
        m_drumMachines[i].prepare (sampleRate, blockSize);
        m_reels       [i].prepare (sampleRate, blockSize);
        m_fxChains    [i].prepare (sampleRate, blockSize);
        m_slotBuffers[i].setSize (2, blockSize);
        m_slotBuffers[i].clear();
    }
}

void SpoolAudioGraph::process (juce::AudioBuffer<float>& output,
                                juce::MidiBuffer*         slotMidi)
{
    const int numSamples = output.getNumSamples();
    output.clear();

    for (int slot = 0; slot < kNumSlots; ++slot)
    {
        if (!m_slotActive[slot].load())
            continue;

        // Clear the slot's scratch buffer (pre-allocated, no allocation here)
        m_slotBuffers[slot].clear();

        // Synthesise — route to correct DSP based on slot type
        const int synthType = m_slotSynthType[slot].load();
        if (synthType == 3)
            m_reels[slot].process        (m_slotBuffers[slot], slotMidi[slot]);
        else if (synthType == 2)
            m_drumMachines[slot].process (m_slotBuffers[slot], slotMidi[slot]);
        else if (synthType == 1)
            m_polySynths[slot].process   (m_slotBuffers[slot], slotMidi[slot]);
        else
            m_sineNodes[slot].process    (m_slotBuffers[slot], slotMidi[slot]);

        // FX chain
        m_fxChains[slot].process (m_slotBuffers[slot]);

        // RMS metering — post-FX, pre-mute (so the meter shows signal even when muted)
        {
            float sum = 0.0f;
            const float* rd = m_slotBuffers[slot].getReadPointer (0);
            for (int s = 0; s < numSamples; ++s)
                sum += rd[s] * rd[s];
            m_slotMeter[slot].store (std::sqrt (sum / static_cast<float> (numSamples)));
        }

        // Mute / solo gate — skip mixing this slot into output if silenced
        const uint8_t muteMask = m_muteMask.load();
        const uint8_t soloMask = m_soloMask.load();
        const uint8_t bit      = static_cast<uint8_t> (1u << slot);
        const bool    muted    = (muteMask & bit) != 0;
        const bool    soloed   = (soloMask != 0) && ((soloMask & bit) == 0);
        if (muted || soloed)
            continue;

        // Mix into output at this slot's level
        const float level = m_slotLevel[slot].load();
        const int   numCh = juce::jmin (output.getNumChannels(),
                                        m_slotBuffers[slot].getNumChannels());
        for (int ch = 0; ch < numCh; ++ch)
            output.addFrom (ch, 0, m_slotBuffers[slot], ch, 0, numSamples, level);
    }
}

void SpoolAudioGraph::reset()
{
    for (int i = 0; i < kNumSlots; ++i)
    {
        m_sineNodes   [i].reset();
        m_polySynths  [i].reset();
        m_reels       [i].reset();
        m_fxChains    [i].reset();
    }
}

void SpoolAudioGraph::setSlotActive (int slot, bool active) noexcept
{
    if (slot >= 0 && slot < kNumSlots)
        m_slotActive[slot].store (active);
}

bool SpoolAudioGraph::isSlotActive (int slot) const noexcept
{
    if (slot < 0 || slot >= kNumSlots) return false;
    return m_slotActive[slot].load();
}

void SpoolAudioGraph::setSlotLevel (int slot, float gain) noexcept
{
    if (slot >= 0 && slot < kNumSlots)
        m_slotLevel[slot].store (juce::jlimit (0.0f, 2.0f, gain));
}

float SpoolAudioGraph::getSlotLevel (int slot) const noexcept
{
    if (slot < 0 || slot >= kNumSlots) return 1.0f;
    return m_slotLevel[slot].load();
}

void SpoolAudioGraph::setSlotSynthType (int slot, int type) noexcept
{
    if (slot >= 0 && slot < kNumSlots)
        m_slotSynthType[slot].store (type);
}

int SpoolAudioGraph::getSlotSynthType (int slot) const noexcept
{
    if (slot < 0 || slot >= kNumSlots) return 0;
    return m_slotSynthType[slot].load();
}

void SpoolAudioGraph::setSlotMuted (int slot, bool muted) noexcept
{
    if (slot < 0 || slot >= kNumSlots) return;
    const uint8_t bit = static_cast<uint8_t> (1u << slot);
    uint8_t cur = m_muteMask.load();
    m_muteMask.store (muted ? (cur | bit) : (cur & ~bit));
}

bool SpoolAudioGraph::isSlotMuted (int slot) const noexcept
{
    if (slot < 0 || slot >= kNumSlots) return false;
    return (m_muteMask.load() & (1u << slot)) != 0;
}

void SpoolAudioGraph::setSlotSoloed (int slot, bool soloed) noexcept
{
    if (slot < 0 || slot >= kNumSlots) return;
    const uint8_t bit = static_cast<uint8_t> (1u << slot);
    uint8_t cur = m_soloMask.load();
    m_soloMask.store (soloed ? (cur | bit) : (cur & ~bit));
}

bool SpoolAudioGraph::isSlotSoloed (int slot) const noexcept
{
    if (slot < 0 || slot >= kNumSlots) return false;
    return (m_soloMask.load() & (1u << slot)) != 0;
}

float SpoolAudioGraph::getSlotMeter (int slot) const noexcept
{
    if (slot < 0 || slot >= kNumSlots) return 0.0f;
    return m_slotMeter[slot].load();
}

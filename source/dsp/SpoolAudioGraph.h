#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "SineOscNode.h"
#include "FXChainProcessor.h"
#include "PolySynthProcessor.h"
#include "../instruments/drum/DrumMachineProcessor.h"
#include "../instruments/reel/ReelProcessor.h"

//==============================================================================
/**
    SpoolAudioGraph — owns all per-slot synthesis and FX processing.

    Each of the 8 slots has:
      - SineOscNode   — synthesises audio from MIDI (Surge XT replaces this later)
      - FXChainProcessor — processes audio through Zone C's FX chain

    Group bus mixing and master level are handled by PluginProcessor.

    Real-time safe: all buffers are pre-allocated in prepare().
*/
class SpoolAudioGraph
{
public:
    static constexpr int kNumSlots = 8;

    SpoolAudioGraph();
    ~SpoolAudioGraph() = default;

    /** Prepare all nodes.  Must be called before process().
        Allocates per-slot buffers — do NOT call from processBlock(). */
    void prepare (double sampleRate, int blockSize);

    /** Render all active slots into \p output.
        \p slotMidi — one MidiBuffer per slot (index 0..kNumSlots-1).
        Clears \p output before mixing into it. */
    void process (juce::AudioBuffer<float>&  output,
                  juce::MidiBuffer*          slotMidi);

    /** Reset all DSP state (e.g. on transport stop). */
    void reset();

    //--- Per-slot accessors ---------------------------------------------------
    /** Read-only access to the per-slot scratch buffer (valid after process()). */
    const juce::AudioBuffer<float>& getSlotBuffer (int slot) const noexcept
    {
        return m_slotBuffers[slot];
    }

    SineOscNode&           getSineNode      (int slot) noexcept { return m_sineNodes    [slot]; }
    FXChainProcessor&      getFXChain       (int slot) noexcept { return m_fxChains     [slot]; }
    PolySynthProcessor&    getPolySynth     (int slot) noexcept { return m_polySynths   [slot]; }
    DrumMachineProcessor&  getDrumMachine   (int slot) noexcept { return m_drumMachines [slot]; }
    ReelProcessor&         getReel          (int slot) noexcept { return m_reels        [slot]; }

    /** Select which DSP node handles a slot.
        0 = SineOscNode (default / empty)
        1 = PolySynthProcessor (SYNTH)
        2 = DrumMachineProcessor (DRUM MACHINE)
        3 = ReelProcessor (REEL) */
    void setSlotSynthType (int slot, int type) noexcept;
    int  getSlotSynthType (int slot) const noexcept;

    /** Mark a slot as active (true) or silent (false).
        Silent slots still consume no CPU as their buffers are skipped. */
    void setSlotActive (int slot, bool active) noexcept;
    bool isSlotActive  (int slot)        const noexcept;

    /** Per-slot output gain (0..2).  Updated atomically. */
    void  setSlotLevel (int slot, float gain) noexcept;
    float getSlotLevel (int slot)        const noexcept;

    /** Per-slot mute (silences output without stopping DSP). */
    void setSlotMuted (int slot, bool muted) noexcept;
    bool isSlotMuted  (int slot) const noexcept;

    /** Per-slot solo.  When any slot is soloed, un-soloed slots are silenced. */
    void setSlotSoloed (int slot, bool soloed) noexcept;
    bool isSlotSoloed  (int slot) const noexcept;

    /** Post-FX RMS level for the slot (updated each process() call, audio thread).
        Returns 0 when the slot is inactive or silent. */
    float getSlotMeter (int slot) const noexcept;

private:
    SineOscNode          m_sineNodes   [kNumSlots];
    FXChainProcessor     m_fxChains    [kNumSlots];
    PolySynthProcessor   m_polySynths  [kNumSlots];
    DrumMachineProcessor m_drumMachines[kNumSlots];
    ReelProcessor        m_reels       [kNumSlots];

    std::atomic<int>     m_slotSynthType[kNumSlots];

    // Pre-allocated per-slot scratch buffers (no allocation in process())
    juce::AudioBuffer<float> m_slotBuffers[kNumSlots];

    std::atomic<bool>  m_slotActive[kNumSlots];
    std::atomic<float> m_slotLevel [kNumSlots];

    // Mute / solo bitmasks (bit i = slot i)
    std::atomic<uint8_t> m_muteMask { 0 };
    std::atomic<uint8_t> m_soloMask { 0 };

    // Post-FX RMS metering (written on audio thread, read on message thread)
    std::atomic<float> m_slotMeter[kNumSlots];

    int m_blockSize  = 256;
};

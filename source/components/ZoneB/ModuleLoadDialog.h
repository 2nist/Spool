#pragma once

#include "../../Theme.h"
#include "ModuleSlot.h"

//==============================================================================
/**
    ModuleLoadDialog — stub overlay shown when an empty slot is clicked.

    Size:    200 × 200px
    Shows:   Four large buttons (SYNTH, SAMPLER, VST3, CANCEL)
    On pick: fires onTypeChosen(slotIndex, type); on cancel fires onCancel().
*/
class ModuleLoadDialog : public juce::Component
{
public:
    explicit ModuleLoadDialog (int slotIndex);
    ~ModuleLoadDialog() override = default;

    std::function<void (int slotIndex, ModuleSlot::ModuleType type)> onTypeChosen;
    std::function<void()> onCancel;

    void paint   (juce::Graphics&) override;
    void resized () override;

    static constexpr int kWidth  = 200;
    static constexpr int kHeight = 304;

private:
    int m_slotIndex;

    juce::TextButton m_synthBtn    { "SYNTH"        };
    juce::TextButton m_samplerBtn  { "SAMPLER"      };
    juce::TextButton m_seqBtn      { "SEQUENCER"    };
    juce::TextButton m_vst3Btn     { "VST3"         };
    juce::TextButton m_drumBtn     { "DRUM MACHINE" };
    juce::TextButton m_reelBtn     { "REEL"         };
    juce::TextButton m_cancelBtn   { "CANCEL"       };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleLoadDialog)
};

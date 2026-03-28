#pragma once

#include "../../Theme.h"
#include <array>

//==============================================================================
/**
    MacroKnobsStrip — four macro knobs: MORPH · WET · CHAOS · DRIVE.

    Height: 80px (fixed, set by parent)
    All knob values 0–1, default 0.5.
    Macros are visual-only this session — no parameter modulation.
    // TODO: macro modulation routing in future session
*/
class MacroKnobsStrip : public juce::Component
{
public:
    MacroKnobsStrip();
    ~MacroKnobsStrip() override = default;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    static constexpr int kNumKnobs = 4;
    static constexpr int kKnobD    = static_cast<int> (Theme::Space::knobSize);
    static constexpr float kDefault = 0.5f;

    std::array<juce::Slider, kNumKnobs> m_knobs;
    std::array<juce::Label,  kNumKnobs> m_labels;
    const std::array<juce::String, kNumKnobs> m_labelText { "MORPH", "WET", "CHAOS", "DRIVE" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroKnobsStrip)
};

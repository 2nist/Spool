#pragma once

#include "../../Theme.h"

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
    void mouseDown      (const juce::MouseEvent&) override;
    void mouseDrag      (const juce::MouseEvent&) override;
    void mouseUp        (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove      (const juce::MouseEvent&) override;
    void mouseExit      (const juce::MouseEvent&) override;
    juce::MouseCursor getMouseCursor() override;

private:
    static constexpr int kNumKnobs = 4;
    static constexpr int kKnobD    = static_cast<int> (Theme::Space::knobSize); // 32
    static constexpr float kDefault = 0.5f;

    float m_values[kNumKnobs] = { kDefault, kDefault, kDefault, kDefault };
    const juce::String m_labels[kNumKnobs] = { "MORPH", "WET", "CHAOS", "DRIVE" };

    // Drag state
    int   m_draggingKnob { -1 };
    float m_dragStartVal { 0.0f };
    int   m_dragStartY   { 0 };
    bool  m_showValue[kNumKnobs] = {};

    juce::Rectangle<float> knobBounds (int i) const noexcept;
    int hitTestKnob (juce::Point<int> pos) const noexcept;

    void paintKnob (juce::Graphics& g, int i) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroKnobsStrip)
};

#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    OutputStrip  — compact output row focused on master level.

    Left-to-right: [red stripe] [OUTPUT label] [LVL slider]

    Drag on the level slider to adjust; double-click to reset to default.
*/
class OutputStrip : public juce::Component
{
public:
    OutputStrip();
    ~OutputStrip() override = default;

    //==========================================================================
    static constexpr int kHeight = 36;

    //==========================================================================
    float getLevel() const noexcept { return m_level; }
    float getPan()   const noexcept { return m_pan; }

    void setLevel (float v);
    void setPan   (float v);

    //==========================================================================
    std::function<void (float)> onLevelChanged;
    std::function<void (float)> onPanChanged;

    //==========================================================================
    void paint     (juce::Graphics&) override;
    void resized   () override;

private:
    //==========================================================================
    float m_level { 1.0f };
    float m_pan   { 0.0f };
    juce::Slider m_levelSlider;

    //==========================================================================
    // Layout constants
    static constexpr int kStripeW     = 3;
    static constexpr int kPad         = 5;
    static constexpr int kNameW       = 52;   // "OUTPUT" label

    //==========================================================================
    // Region helpers
    juce::Rectangle<int> stripeRect()     const noexcept;
    juce::Rectangle<int> nameLabelRect()  const noexcept;
    juce::Rectangle<int> levelTrackRect() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputStrip)
};

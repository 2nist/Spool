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
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

private:
    //==========================================================================
    float m_level { 1.0f };
    float m_pan   { 0.0f };

    enum class DragTarget { None, Level };
    DragTarget m_drag { DragTarget::None };

    //==========================================================================
    // Layout constants
    static constexpr int kStripeW     = 3;
    static constexpr int kPad         = 5;
    static constexpr int kNameW       = 52;   // "OUTPUT" label
    static constexpr int kThumbD      = 8;

    //==========================================================================
    // Region helpers
    juce::Rectangle<int> stripeRect()     const noexcept;
    juce::Rectangle<int> nameLabelRect()  const noexcept;
    juce::Rectangle<int> levelTrackRect() const noexcept;
    void paintSlider (juce::Graphics& g, juce::Rectangle<int> tk,
                      float t, bool fromCenter, bool active) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OutputStrip)
};

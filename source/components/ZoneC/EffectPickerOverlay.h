#pragma once

#include "../../Theme.h"
#include "ChainState.h"

//==============================================================================
/**
    EffectPickerOverlay — overlay panel shown inside Zone C when + ADD EFFECT
    is clicked.

    Width:   Zone C width (set by parent)
    Height:  200px (fixed)
    Posted as a direct child of ZoneCComponent (not FXChainDisplay).

    Fires onEffectChosen(effectId) when a row is clicked.
    Fires onCancel() when CANCEL or clicking outside.
*/
class EffectPickerOverlay : public juce::Component
{
public:
    EffectPickerOverlay();
    ~EffectPickerOverlay() override = default;

    static constexpr int kHeight = 200;

    std::function<void(const juce::String& effectId)> onEffectChosen;
    std::function<void()>                              onCancel;

    void paint         (juce::Graphics&) override;
    void mouseDown     (const juce::MouseEvent&) override;
    void mouseMove     (const juce::MouseEvent&) override;
    bool hitTest       (int x, int y) override;

private:
    int m_hoverRow { -1 };

    static constexpr int kTitleH  = 24;  // "ADD EFFECT" title + padding
    static constexpr int kRowH    = 28;
    static constexpr int kCancelH = 28;

    juce::Rectangle<int> rowRect    (int i)  const noexcept;
    juce::Rectangle<int> cancelRect ()       const noexcept;

    const juce::Array<EffectTypeDef>& effectDefs() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectPickerOverlay)
};

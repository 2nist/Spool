#pragma once

#include "../Theme.h"

//==============================================================================
/**
    SPOOL Menu Bar
    --------------
    Always-visible 28px system strip at the top of the window.

    Left-side labels are declared explicitly as compact workflow menus.
    Each item acts as an anchor for a focused popup menu instead of relying on
    paint-time placeholders or disconnected shell actions.
*/
class MenuBarComponent : public juce::Component
{
public:
    MenuBarComponent();
    ~MenuBarComponent() override = default;

    std::function<void (const juce::String& itemId, juce::Rectangle<int> anchorBounds)> onMenuClicked;
    std::function<void (float)> onLevelChanged;
    std::function<void (float)> onPanChanged;

    void setLevel (float value) noexcept;
    void setPan (float value) noexcept;
    void setBuildStamp (juce::String stamp);
    float getLevel() const noexcept { return m_level; }
    float getPan() const noexcept { return m_pan; }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;

    juce::Rectangle<int> getAnchorBoundsForItem (const juce::String& itemId) const noexcept;

private:
    struct MenuItem
    {
        juce::String id;
        juce::String label;
        juce::Rectangle<float> bounds;
        bool interactive { false };
    };

    void rebuildItemLayout();
    MenuItem* itemAt (juce::Point<float> pos) noexcept;
    const MenuItem* itemAt (juce::Point<float> pos) const noexcept;

    juce::Array<MenuItem> m_items;
    juce::String m_hoveredItemId;
    juce::Image m_logo;
    juce::Rectangle<float> m_logoBounds;
    juce::String m_buildStamp;
    juce::Rectangle<float> m_buildStampBounds;
    juce::Rectangle<float> m_levelRect;
    juce::Rectangle<float> m_panRect;
    float m_level { 1.0f };
    float m_pan { 0.0f };

    enum class DragTarget
    {
        none,
        level,
        pan
    };

    DragTarget m_dragTarget { DragTarget::none };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenuBarComponent)
};

#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    TabStrip — 28px-wide full-height left-edge navigation strip.

    10 tabs across 6 named groups.  Each group has a 12px header band
    showing the group name.  Active tab uses the group's accent colour.

    Groups (top to bottom):
      SESSION     : song, structure, lyrics        — inkMid
      INSTRUMENT  : instrument                     — Zone::a
      PERFORMANCE : mixer, macro                   — Zone::b
      SIGNAL      : routing                        — Zone::c
      CAPTURE     : tape                           — Zone::d
      TRACK       : tracks, automate               — inkMid

    Active tab  : groupColor at 15% alpha bg + 2px groupColor left border
    Hover       : surfaceEdge at 30% alpha bg
    Text        : inkLight (active), inkMid (hover), inkGhost (default)

    Firing:
      onTabSelected("tabId")  — a new tab was clicked
      onTabSelected("")       — the already-active tab was clicked (collapse)
*/
class TabStrip : public juce::Component
{
public:
    static constexpr int kWidth  = 28;
    // Make the tabs square (height == width) so 22px icons sit centered
    static constexpr int kTabH   = kWidth;
    static constexpr int kGroupH = 12;  // group label header height

    TabStrip();
    ~TabStrip() override = default;

    /** Set the active tab without firing the callback. */
    void setActiveTab (const juce::String& tabId);

    juce::String getActiveTab() const noexcept { return m_activeTabId; }

    /** Fired on click.  Empty string = collapse (active tab re-clicked). */
    std::function<void(const juce::String& tabId)> onTabSelected;

    //--- juce::Component ------------------------------------------------------
    void paint      (juce::Graphics&) override;
    void mouseDown  (const juce::MouseEvent&) override;
    void mouseMove  (const juce::MouseEvent&) override;
    void mouseExit  (const juce::MouseEvent&) override;

private:
    struct Tab
    {
        juce::String id;
        juce::String label;   // ≤4 chars, displayed rotated
    };

    struct Group
    {
        juce::String     name;
        juce::Array<Tab> tabs;
        juce::Colour     color;   // accent color for active state in this group
    };

    juce::Array<Group> m_groups;
    juce::String       m_activeTabId;
    juce::String       m_hoveredTabId;
    int                m_scrollY = 0; // vertical scroll offset for tabs

    void initGroups();

    /** Y offset of the group header band for group gi. */
    int groupHeaderY (int gi) const noexcept;

    /** Y offset of the first tab in group gi. */
    int groupFirstTabY (int gi) const noexcept;

    /** Rectangle for the tab at position (gi, ti). */
    juce::Rectangle<int> tabRect (int gi, int ti) const noexcept;

    /** Return the tab ID at screen position pos; empty if none. */
    juce::String tabAtPos (juce::Point<int> pos) const noexcept;

    /** Return the group color for a given tab ID. */
    juce::Colour groupColorForTab (const juce::String& tabId) const noexcept;

    void paintGroupHeader (juce::Graphics& g,
                           juce::Rectangle<int> r,
                           const juce::String& name) const;

    void paintTab (juce::Graphics& g,
                   const juce::Rectangle<int>& r,
                   const juce::String& label,
                   bool isActive,
                   bool isHovered,
                   juce::Colour groupColor) const;

    // Make the strip vertically scrollable when there isn't enough space
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    int totalHeight() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TabStrip)
};

#include "TabStrip.h"
#include "BinaryData.h"

//==============================================================================
TabStrip::TabStrip()
{
    initGroups();
}

void TabStrip::initGroups()
{
    {
        Group g;
        g.name   = "SESSION";
        g.accent = ZoneAStyle::accentForGroupName (g.name);
        g.tabs.add ({ "song", "SONG", BinaryData::audiowaveform_png, BinaryData::audiowaveform_pngSize,
                     ZoneAStyle::accentForTabId ("song") });
        g.tabs.add ({ "structure", "STR", BinaryData::form_png, BinaryData::form_pngSize,
                     ZoneAStyle::accentForTabId ("structure") });
        g.tabs.add ({ "lyrics", "LYR", BinaryData::notebookpen_png, BinaryData::notebookpen_pngSize,
                     ZoneAStyle::accentForTabId ("lyrics") });
        m_groups.add (std::move (g));
    }

    {
        Group g;
        g.name   = "INSTR";
        g.accent = ZoneAStyle::accentForGroupName (g.name);
        g.tabs.add ({ "instrument", "INST", BinaryData::keyboardmusic_png, BinaryData::keyboardmusic_pngSize,
                     ZoneAStyle::accentForTabId ("instrument") });
        m_groups.add (std::move (g));
    }

    {
        Group g;
        g.name   = "PERF";
        g.accent = ZoneAStyle::accentForGroupName (g.name);
        g.tabs.add ({ "mixer", "MIX", BinaryData::chartarea_png, BinaryData::chartarea_pngSize,
                     ZoneAStyle::accentForTabId ("mixer") });
        g.tabs.add ({ "macro", "MCRO", BinaryData::circlegauge_png, BinaryData::circlegauge_pngSize,
                     ZoneAStyle::accentForTabId ("macro") });
        m_groups.add (std::move (g));
    }

    {
        Group g;
        g.name   = "SIGNAL";
        g.accent = ZoneAStyle::accentForGroupName (g.name);
        g.tabs.add ({ "routing", "RTE", BinaryData::merge_png, BinaryData::merge_pngSize,
                     ZoneAStyle::accentForTabId ("routing") });
        m_groups.add (std::move (g));
    }

    {
        Group g;
        g.name   = "CAPTURE";
        g.accent = ZoneAStyle::accentForGroupName (g.name);
        g.tabs.add ({ "tape", "TAPE", BinaryData::voicemail_png, BinaryData::voicemail_pngSize,
                     ZoneAStyle::accentForTabId ("tape") });
        m_groups.add (std::move (g));
    }

    {
        Group g;
        g.name   = "TRACK";
        g.accent = ZoneAStyle::accentForGroupName (g.name);
        g.tabs.add ({ "tracks", "TRK", BinaryData::brackets_png, BinaryData::brackets_pngSize,
                     ZoneAStyle::accentForTabId ("tracks") });
        g.tabs.add ({ "automate", "AUTO", BinaryData::workflow_png, BinaryData::workflow_pngSize,
                     ZoneAStyle::accentForTabId ("automate") });
        m_groups.add (std::move (g));
    }
}

//==============================================================================
// Layout helpers
//==============================================================================

int TabStrip::groupHeaderY (int gi) const noexcept
{
    int y = 0;
    for (int g = 0; g < gi && g < m_groups.size(); ++g)
        y += kGroupH + m_groups[g].tabs.size() * kTabH;
    return y;
}

int TabStrip::groupFirstTabY (int gi) const noexcept
{
    return groupHeaderY (gi) + kGroupH;
}

juce::Rectangle<int> TabStrip::tabRect (int gi, int ti) const noexcept
{
    const int y = groupFirstTabY (gi) + ti * kTabH - m_scrollY;
    return { 0, y, kWidth, kTabH };
}

juce::String TabStrip::tabAtPos (juce::Point<int> pos) const noexcept
{
    for (int g = 0; g < m_groups.size(); ++g)
        for (int t = 0; t < m_groups[g].tabs.size(); ++t)
            if (tabRect (g, t).contains (pos))
                return m_groups[g].tabs[t].id;
    return {};
}

void TabStrip::setActiveTab (const juce::String& tabId)
{
    m_activeTabId = tabId;
    repaint();
}

//==============================================================================
// Paint
//==============================================================================

void TabStrip::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface0);

    for (auto& group : m_groups)
    {
        group.accent = ZoneAStyle::accentForGroupName (group.name);
        for (auto& tab : group.tabs)
            tab.accent = ZoneAStyle::accentForTabId (tab.id);
    }

    // Right border
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect (kWidth - 1, 0, 1, getHeight());

    for (int gi = 0; gi < m_groups.size(); ++gi)
    {
        const auto& group = m_groups[gi];

        // Group header band (apply scroll offset)
        paintGroupHeader (g,
                          { 0, groupHeaderY (gi) - m_scrollY, kWidth, kGroupH },
                          group);

        for (int ti = 0; ti < group.tabs.size(); ++ti)
        {
            const auto& tab    = group.tabs[ti];
            const bool active  = (tab.id == m_activeTabId);
            const bool hovered = !active && (tab.id == m_hoveredTabId);
            paintTab (g, tabRect (gi, ti), tab, active, hovered);
        }
    }
}

void TabStrip::paintGroupHeader (juce::Graphics& g,
                                  juce::Rectangle<int> r,
                                  const Group& group) const
{
    g.setColour (Theme::Colour::surface1.withAlpha (0.6f));
    g.fillRect (r);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect (2, r.getY(), kWidth - 4, 1);

    g.setFont (juce::Font (juce::FontOptions{}.withHeight (6.0f)));
    g.setColour (group.accent.withAlpha (0.72f));
    g.drawText (group.name, r.reduced (1, 1), juce::Justification::centred, false);
}

void TabStrip::paintTab (juce::Graphics& g,
                          const juce::Rectangle<int>& r,
                          const Tab& tab,
                          bool isActive,
                          bool isHovered) const
{
    const auto accent = tab.accent;

    // Background
    if (isActive)
    {
        g.setColour (accent.withAlpha (0.15f));
        g.fillRect  (r);
        g.setColour (accent);
        g.fillRect  (r.withWidth (2));
    }
    else if (isHovered)
    {
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.30f));
        g.fillRect  (r);
    }

    auto img = (tab.iconData != nullptr && tab.iconSize > 0)
                 ? juce::ImageCache::getFromMemory (tab.iconData, tab.iconSize)
                 : juce::Image();
    if (img.isValid())
    {
        const auto imgR = r.toFloat().withSizeKeepingCentre (16.0f, 16.0f).toNearestInt();
        g.drawImageWithin (img, imgR.getX(), imgR.getY(), imgR.getWidth(), imgR.getHeight(), juce::RectanglePlacement::centred);
    }
    // No text fallback: tabs are icon-only (there are 10 embedded icons)
}

//==============================================================================
// Scrolling support
//==============================================================================

int TabStrip::totalHeight() const noexcept
{
    int h = 0;
    for (const auto& g : m_groups)
        h += kGroupH + g.tabs.size() * kTabH;
    return h;
}

void TabStrip::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& details)
{
    const int maxScroll = juce::jmax (0, totalHeight() - getHeight());
    const int delta = static_cast<int> (details.deltaY * 28.0f); // scroll by approx one tab per tick
    m_scrollY = juce::jlimit (0, maxScroll, m_scrollY + delta);
    repaint();
}

//==============================================================================
// Mouse
//==============================================================================

void TabStrip::mouseDown (const juce::MouseEvent& e)
{
    const auto hit = tabAtPos (e.getPosition());
    if (hit.isEmpty()) return;

    if (hit == m_activeTabId)
    {
        // Re-clicking active tab = collapse signal
        m_activeTabId.clear();
        if (onTabSelected) onTabSelected ({});
    }
    else
    {
        m_activeTabId = hit;
        if (onTabSelected) onTabSelected (hit);
    }
    repaint();
}

void TabStrip::mouseMove (const juce::MouseEvent& e)
{
    const auto hit = tabAtPos (e.getPosition());
    if (hit != m_hoveredTabId)
    {
        m_hoveredTabId = hit;
        repaint();
    }
}

void TabStrip::mouseExit (const juce::MouseEvent&)
{
    if (m_hoveredTabId.isNotEmpty())
    {
        m_hoveredTabId.clear();
        repaint();
    }
}

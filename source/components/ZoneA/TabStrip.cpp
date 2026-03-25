#include "TabStrip.h"
#include "BinaryData.h"

//==============================================================================
TabStrip::TabStrip()
{
    initGroups();
}

void TabStrip::initGroups()
{
    // SESSION group — inkMid
    {
        Group g;
        g.name  = "SESSION";
        g.color = juce::Colour (Theme::Colour::inkMid);
        g.tabs.add ({ "song",      "SONG" });
        g.tabs.add ({ "structure", "STR"  });
        g.tabs.add ({ "lyrics",    "LYR"  });
        m_groups.add (std::move (g));
    }

    // INSTRUMENT group — Zone::a
    {
        Group g;
        g.name  = "INSTR";
        g.color = juce::Colour (Theme::Zone::a);
        g.tabs.add ({ "instrument", "INST" });
        m_groups.add (std::move (g));
    }

    // PERFORMANCE group — Zone::b
    {
        Group g;
        g.name  = "PERF";
        g.color = juce::Colour (Theme::Zone::b);
        g.tabs.add ({ "mixer", "MIX"  });
        g.tabs.add ({ "macro", "MCRO" });
        m_groups.add (std::move (g));
    }

    // SIGNAL group — Zone::c
    {
        Group g;
        g.name  = "SIGNAL";
        g.color = juce::Colour (Theme::Zone::c);
        g.tabs.add ({ "routing", "RTE" });
        m_groups.add (std::move (g));
    }

    // CAPTURE group — Zone::d
    {
        Group g;
        g.name  = "CAPTURE";
        g.color = juce::Colour (Theme::Zone::d);
        g.tabs.add ({ "tape", "TAPE" });
        m_groups.add (std::move (g));
    }

    // TRACK group — inkMid
    {
        Group g;
        g.name  = "TRACK";
        g.color = juce::Colour (Theme::Colour::inkMid);
        g.tabs.add ({ "tracks",   "TRK"  });
        g.tabs.add ({ "automate", "AUTO" });
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

juce::Colour TabStrip::groupColorForTab (const juce::String& tabId) const noexcept
{
    for (const auto& grp : m_groups)
        for (const auto& tab : grp.tabs)
            if (tab.id == tabId)
                return grp.color;
    return juce::Colour (Theme::Zone::a);
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

    // Right border
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect (kWidth - 1, 0, 1, getHeight());

    for (int gi = 0; gi < m_groups.size(); ++gi)
    {
        const auto& group = m_groups[gi];

        // Group header band (apply scroll offset)
        paintGroupHeader (g,
                          { 0, groupHeaderY (gi) - m_scrollY, kWidth, kGroupH },
                          group.name);

        for (int ti = 0; ti < group.tabs.size(); ++ti)
        {
            const auto& tab    = group.tabs[ti];
            const bool active  = (tab.id == m_activeTabId);
            const bool hovered = !active && (tab.id == m_hoveredTabId);
            paintTab (g, tabRect (gi, ti), tab.label, active, hovered, group.color);
        }
    }
}

void TabStrip::paintGroupHeader (juce::Graphics& g,
                                  juce::Rectangle<int> r,
                                  const juce::String& name) const
{
    // Slightly elevated background
    g.setColour (Theme::Colour::surface1.withAlpha (0.6f));
    g.fillRect  (r);

    // Separator line at top of header band
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect  (2, r.getY(), kWidth - 4, 1);

    // Group name — 6px uppercase, centred
    g.setFont   (juce::Font (juce::FontOptions{}.withHeight (6.0f)));
    g.setColour (Theme::Colour::inkGhost.withAlpha (0.6f));
    g.drawText  (name, r.reduced (1, 1), juce::Justification::centred, false);
}

void TabStrip::paintTab (juce::Graphics& g,
                          const juce::Rectangle<int>& r,
                          const juce::String& label,
                          bool isActive,
                          bool isHovered,
                          juce::Colour groupColor) const
{
    // Background
    if (isActive)
    {
        g.setColour (groupColor.withAlpha (0.15f));
        g.fillRect  (r);
        // 2px left accent border
        g.setColour (groupColor);
        g.fillRect  (r.withWidth (2));
    }
    else if (isHovered)
    {
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.30f));
        g.fillRect  (r);
    }

    // Rotated label — 90° CCW (bottom → top)
    g.setFont (Theme::Font::micro());
    g.setColour (isActive  ? groupColor.brighter (0.1f)
               : isHovered ? Theme::Colour::inkMid
                           : Theme::Colour::inkGhost);

    const float cx = static_cast<float> (r.getCentreX());
    const float cy = static_cast<float> (r.getCentreY());

    juce::Graphics::ScopedSaveState saved (g);
    g.addTransform (juce::AffineTransform::rotation (
        -juce::MathConstants<float>::halfPi, cx, cy));

    const auto textR = juce::Rectangle<float>(
        cx - static_cast<float> (kTabH) * 0.5f,
        cy - static_cast<float> (kWidth) * 0.5f,
        static_cast<float> (kTabH),
        static_cast<float> (kWidth));

    // Attempt to draw a 22x22 icon for the tab (BinaryData embedded).
    // There are exactly 10 tabs — no text fallback is used.
    auto getIconForTab = [&]() -> juce::Image
    {
        if (label == "SONG")
            return juce::ImageCache::getFromMemory (BinaryData::audiowaveform_png, BinaryData::audiowaveform_pngSize);
        if (label == "STR")
            return juce::ImageCache::getFromMemory (BinaryData::form_png, BinaryData::form_pngSize);
        if (label == "LYR")
            return juce::ImageCache::getFromMemory (BinaryData::notebookpen_png, BinaryData::notebookpen_pngSize);
        if (label == "INST")
            return juce::ImageCache::getFromMemory (BinaryData::keyboardmusic_png, BinaryData::keyboardmusic_pngSize);
        if (label == "MCRO")
            return juce::ImageCache::getFromMemory (BinaryData::circlegauge_png, BinaryData::circlegauge_pngSize);
        if (label == "TAPE")
            return juce::ImageCache::getFromMemory (BinaryData::voicemail_png, BinaryData::voicemail_pngSize);
        if (label == "TRK")
            return juce::ImageCache::getFromMemory (BinaryData::brackets_png, BinaryData::brackets_pngSize);
        // Icon swap: MIX <- RTE, RTE <- AUTO, AUTO <- MIX (cycle)
        if (label == "MIX")
            return juce::ImageCache::getFromMemory (BinaryData::merge_png, BinaryData::merge_pngSize);       // use routing icon for Mix
        if (label == "RTE")
            return juce::ImageCache::getFromMemory (BinaryData::workflow_png, BinaryData::workflow_pngSize); // use automation icon for Routing
        if (label == "AUTO")
            return juce::ImageCache::getFromMemory (BinaryData::chartarea_png, BinaryData::chartarea_pngSize); // use mix icon for Automation

        return {};
    };

    auto img = getIconForTab();
    if (img.isValid())
    {
        const auto imgR = textR.withSizeKeepingCentre (22.0f, 22.0f).toNearestInt();
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

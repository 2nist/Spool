#include "InstrumentPanel.h"

//==============================================================================
InstrumentPanel::InstrumentPanel()
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

//==============================================================================
// Static helpers
//==============================================================================

juce::Colour InstrumentPanel::typeColor (const juce::String& moduleType)
{
    if (moduleType == "SYNTH")  return juce::Colour (Theme::Zone::a);
    if (moduleType == "DRUMS")  return juce::Colour (Theme::Zone::b);
    if (moduleType == "REEL")   return juce::Colour (Theme::Zone::d);
    if (moduleType == "OUTPUT") return juce::Colour (Theme::Colour::error);
    return juce::Colour (Theme::Colour::inkGhost);
}

juce::String InstrumentPanel::typeShort (const juce::String& moduleType)
{
    if (moduleType == "SYNTH")  return "POLY";
    if (moduleType == "DRUMS")  return "DRUM";
    if (moduleType == "REEL")   return "REEL";
    if (moduleType == "OUTPUT") return "OUT";
    if (moduleType.isNotEmpty()) return moduleType.substring (0, 4);
    return "";
}

//==============================================================================
// Data population
//==============================================================================

void InstrumentPanel::setSlots (const juce::Array<SlotEntry>& slots)
{
    m_slots = slots;
    rebuildRowCache();
    repaint();
}

void InstrumentPanel::rebuildRowCache()
{
    m_rowYCache.clear();

    int y = kBrowserHeaderH;
    juce::String lastGroup;

    for (const auto& entry : m_slots)
    {
        if (entry.groupName != lastGroup)
        {
            lastGroup = entry.groupName;
            y += kGroupHeaderH;
        }
        m_rowYCache.add ({ entry.slotIndex, y });
        y += kSlotRowH;
    }
}

int InstrumentPanel::slotAtBrowserY (int posY) const noexcept
{
    for (const auto& pair : m_rowYCache)
        if (posY >= pair.second && posY < pair.second + kSlotRowH)
            return pair.first;
    return -1;
}

//==============================================================================
// Navigation
//==============================================================================

void InstrumentPanel::showSlot (int slotIndex)
{
    m_activeSlot = slotIndex;
    m_state      = State::editor;
    repaint();
}

void InstrumentPanel::showBrowser()
{
    m_state      = State::browser;
    m_activeSlot = -1;
    repaint();
}

//==============================================================================
// Paint
//==============================================================================

void InstrumentPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);

    if (m_state == State::browser)
        paintBrowser (g);
    else
        paintEditor (g);
}

void InstrumentPanel::paintBrowser (juce::Graphics& g) const
{
    const int w = getWidth();

    // Header
    g.setColour (Theme::Colour::surface1);
    g.fillRect  (0, 0, w, kBrowserHeaderH);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect  (0, kBrowserHeaderH - 1, w, 1);
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Zone::a);
    g.drawText  ("INSTRUMENT",
                 juce::Rectangle<int> (0, 0, w, kBrowserHeaderH),
                 juce::Justification::centred, false);

    if (m_slots.isEmpty())
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("No modules loaded.\nAdd modules in Zone B.",
                     getLocalBounds().withTrimmedTop (kBrowserHeaderH).reduced (8),
                     juce::Justification::centredTop, true);
        return;
    }

    // Slot rows
    juce::String lastGroup;

    for (const auto& entry : m_slots)
    {
        // Group header
        if (entry.groupName != lastGroup)
        {
            lastGroup = entry.groupName;

            int groupY = kBrowserHeaderH;
            for (const auto& pair : m_rowYCache)
                if (pair.first == entry.slotIndex)
                    groupY = pair.second - kGroupHeaderH;

            const juce::Rectangle<int> grpR (0, groupY, w, kGroupHeaderH);
            g.setColour (entry.groupColor.withAlpha (0.12f));
            g.fillRect  (grpR);
            g.setColour (entry.groupColor);
            g.fillRect  (0, groupY, 2, kGroupHeaderH);
            g.setFont   (Theme::Font::micro());
            g.setColour (entry.groupColor);
            g.drawText  (entry.groupName,
                         juce::Rectangle<int> (6, groupY, w - 54, kGroupHeaderH),
                         juce::Justification::centredLeft, false);

            int groupSlotCount = 0;
            for (const auto& s : m_slots)
                if (s.groupName == entry.groupName)
                    ++groupSlotCount;
            g.setColour (Theme::Colour::inkGhost);
            g.drawText  (juce::String (groupSlotCount) + " slots",
                         juce::Rectangle<int> (w - 48, groupY, 44, kGroupHeaderH),
                         juce::Justification::centredRight, false);
        }

        int rowY = -1;
        for (const auto& pair : m_rowYCache)
            if (pair.first == entry.slotIndex)
                rowY = pair.second;

        if (rowY < 0) continue;

        paintBrowserSlot (g, entry,
                          juce::Rectangle<int> (0, rowY, w, kSlotRowH),
                          entry.slotIndex == m_hoveredSlot);
    }
}

void InstrumentPanel::paintBrowserSlot (juce::Graphics& g,
                                         const SlotEntry& entry,
                                         juce::Rectangle<int> r,
                                         bool hovered) const
{
    const int w = r.getWidth();

    g.setColour (hovered ? Theme::Colour::surface3 : Theme::Colour::surface2);
    g.fillRect  (r);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.3f));
    g.fillRect  (r.getX(), r.getBottom() - 1, w, 1);

    if (entry.moduleType.isEmpty())
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.4f));
        g.drawText  ("+ EMPTY",
                     r.withTrimmedLeft (kPad + 8),
                     juce::Justification::centredLeft, false);
        return;
    }

    // Color dot
    g.setColour (entry.groupColor);
    g.fillEllipse (static_cast<float> (r.getX() + kPad),
                   static_cast<float> (r.getCentreY() - 3),
                   6.0f, 6.0f);

    // Slot number
    const juce::String numStr = juce::String (entry.slotIndex + 1).paddedLeft ('0', 2);
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  (numStr,
                 juce::Rectangle<int> (r.getX() + kPad + 10, r.getY(), 18, r.getHeight()),
                 juce::Justification::centredLeft, false);

    // Module name
    g.setFont   (Theme::Font::label());
    g.setColour (Theme::Colour::inkLight);
    g.drawText  (entry.displayName,
                 juce::Rectangle<int> (r.getX() + kPad + 30, r.getY(),
                                       w - kPad - 80, r.getHeight()),
                 juce::Justification::centredLeft, true);

    // Type badge
    const juce::String badge    = typeShort (entry.moduleType);
    const juce::Colour badgeCol = typeColor (entry.moduleType);
    const int badgeW = 36, badgeH = 12;
    const int badgeX = r.getX() + w - badgeW - kPad;
    const int badgeY = r.getCentreY() - badgeH / 2;

    g.setColour (badgeCol.withAlpha (0.15f));
    g.fillRoundedRectangle (juce::Rectangle<int> (badgeX, badgeY, badgeW, badgeH).toFloat(), 2.5f);
    g.setColour (badgeCol.withAlpha (0.5f));
    g.drawRoundedRectangle (juce::Rectangle<int> (badgeX, badgeY, badgeW, badgeH).toFloat(), 2.5f, 0.5f);
    g.setFont   (Theme::Font::micro());
    g.setColour (badgeCol);
    g.drawText  (badge,
                 juce::Rectangle<int> (badgeX, badgeY, badgeW, badgeH),
                 juce::Justification::centred, false);
}

//==============================================================================
// Editor state
//==============================================================================

void InstrumentPanel::paintEditor (juce::Graphics& g) const
{
    const int w = getWidth();

    const SlotEntry* entry = nullptr;
    for (const auto& s : m_slots)
        if (s.slotIndex == m_activeSlot)
            entry = &s;

    // Header
    g.setColour (Theme::Colour::surface1);
    g.fillRect  (0, 0, w, kEditorHeaderH);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect  (0, kEditorHeaderH - 1, w, 1);

    // Back button
    const auto backR = backBtnRect();
    g.setFont   (Theme::Font::label());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  ("\xe2\x86\x90", backR, juce::Justification::centred, false);  // ←

    if (entry != nullptr)
    {
        g.setFont   (Theme::Font::label());
        g.setColour (Theme::Colour::inkLight);
        g.drawText  (entry->displayName,
                     juce::Rectangle<int> (backR.getRight() + 4, 0,
                                           w - backR.getRight() - kPad - 40, kEditorHeaderH),
                     juce::Justification::centredLeft, true);

        // Type badge
        const juce::String badge    = typeShort (entry->moduleType);
        const juce::Colour badgeCol = typeColor (entry->moduleType);
        const int badgeW = 36, badgeH = 12;
        const int badgeX = w - badgeW - kPad;
        const int badgeY = (kEditorHeaderH - badgeH) / 2;

        g.setColour (badgeCol.withAlpha (0.15f));
        g.fillRoundedRectangle (juce::Rectangle<int> (badgeX, badgeY, badgeW, badgeH).toFloat(), 2.5f);
        g.setFont   (Theme::Font::micro());
        g.setColour (badgeCol);
        g.drawText  (badge,
                     juce::Rectangle<int> (badgeX, badgeY, badgeW, badgeH),
                     juce::Justification::centred, false);

        if (entry->moduleType == "SYNTH")
            paintPolySynth (g, entry);
        else
            paintEditorStub (g, entry);
    }
    else
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("No module",
                     getLocalBounds().withTrimmedTop (kEditorHeaderH),
                     juce::Justification::centred, false);
    }
}

void InstrumentPanel::paintEditorStub (juce::Graphics& g,
                                        const SlotEntry* entry) const
{
    const auto area = getLocalBounds().withTrimmedTop (kEditorHeaderH);
    const juce::String typeName = entry ? entry->moduleType : "MODULE";

    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  (typeName + " editor\nexpanding in future session",
                 area.reduced (12), juce::Justification::centred, true);
}

void InstrumentPanel::paintPolySynth (juce::Graphics& g,
                                       const SlotEntry* /*entry*/) const
{
    const int w    = getWidth();
    const int topY = kEditorHeaderH + kPad;

    struct Section { juce::String name; juce::Colour color; };
    const Section sections[] = {
        { "OSCILLATORS", juce::Colour (0xFF4a9eff) },
        { "FILTER",      juce::Colour (Theme::Zone::b) },
        { "ENVELOPES",   juce::Colour (0xFF4a9eff) },
        { "LFO",         juce::Colour (0xFFee4aee) },
        { "VOICE",       juce::Colour (Theme::Colour::inkMid) },
        { "OUTPUT",      juce::Colour (Theme::Colour::inkMid) },
    };

    int y = topY;
    for (const auto& sec : sections)
    {
        const juce::Rectangle<int> hdr (0, y, w, 20);
        g.setColour (Theme::Colour::surface1);
        g.fillRect  (hdr);
        g.setColour (sec.color.withAlpha (0.6f));
        g.fillRect  (0, y, 2, 20);
        g.setFont   (Theme::Font::micro());
        g.setColour (sec.color);
        g.drawText  ("\xe2\x96\xbe " + sec.name,
                     juce::Rectangle<int> (kPad, y, w - kPad * 2, 20),
                     juce::Justification::centredLeft, false);
        y += 20;

        const juce::Rectangle<int> body (0, y, w, 32);
        g.setColour (Theme::Colour::surface0);
        g.fillRect  (body);
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.4f));
        g.drawText  ("— params —", body, juce::Justification::centred, false);
        y += 34;
    }
}

juce::Rectangle<int> InstrumentPanel::backBtnRect() const noexcept
{
    return { kPad, (kEditorHeaderH - 20) / 2, 20, 20 };
}

//==============================================================================
// Resize
//==============================================================================

void InstrumentPanel::resized()
{
    repaint();
}

//==============================================================================
// Mouse
//==============================================================================

void InstrumentPanel::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    if (m_state == State::browser)
    {
        const int hit = slotAtBrowserY (pos.y);
        if (hit >= 0)
        {
            for (const auto& entry : m_slots)
            {
                if (entry.slotIndex == hit && entry.moduleType.isNotEmpty())
                {
                    if (onSlotSelected) onSlotSelected (hit);
                    showSlot (hit);
                    return;
                }
            }
        }
    }
    else
    {
        if (backBtnRect().contains (pos))
            showBrowser();
    }
}

void InstrumentPanel::mouseMove (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();
    const int newHover = (m_state == State::browser) ? slotAtBrowserY (pos.y) : -1;

    if (newHover != m_hoveredSlot)
    {
        m_hoveredSlot = newHover;
        repaint();
    }
}

void InstrumentPanel::mouseExit (const juce::MouseEvent&)
{
    if (m_hoveredSlot != -1)
    {
        m_hoveredSlot = -1;
        repaint();
    }
}

#include "LooperStrip.h"

//==============================================================================

LooperStrip::LooperStrip()
{
    addChildComponent (m_unitB);
}

void LooperStrip::setSourceNames (const juce::StringArray& names)
{
    m_sourceNames = names;
    if (!m_sourceNames.contains ("MIX"))
        m_sourceNames.insert (0, "MIX");
    repaint();
}

void LooperStrip::setHasGrabbedClip (bool hasClip)
{
    m_hasGrabbedClip = hasClip;
    repaint();
}

//==============================================================================
// Layout
//==============================================================================

void LooperStrip::resized()
{
    if (m_showUnitB)
        m_unitB.setBounds (0, 0, getWidth(), getHeight());
}

//==============================================================================
// Paint
//==============================================================================

void LooperStrip::drawPillBtn (juce::Graphics& g,
                                juce::Rectangle<int> r,
                                const juce::String& label,
                                bool active,
                                juce::Colour activeCol)
{
    const juce::Colour fill = active
                              ? (activeCol.isTransparent() ? (juce::Colour) Theme::Colour::surface2
                                                           : activeCol.withAlpha (0.25f))
                              : (juce::Colour) Theme::Colour::surface1;
    g.setColour (fill);
    g.fillRoundedRectangle (r.toFloat(), 2.0f);
    g.setColour (active ? (activeCol.isTransparent() ? (juce::Colour) Theme::Colour::surfaceEdge
                                                     : activeCol)
                        : (juce::Colour) Theme::Colour::surfaceEdge);
    g.drawRoundedRectangle (r.toFloat(), 2.0f, 0.5f);

    g.setFont   (Theme::Font::micro());
    g.setColour (active ? (juce::Colour) Theme::Colour::inkLight
                        : (juce::Colour) Theme::Colour::inkGhost);
    g.drawText  (label, r, juce::Justification::centred, false);
}

void LooperStrip::paint (juce::Graphics& g)
{
    // Background
    g.setColour (Theme::Colour::surface1);
    g.fillRect  (getLocalBounds());

    // Top border
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine  (0.0f, 0.0f, static_cast<float> (getWidth()), 0.0f, 0.8f);

    // Row dividers
    for (int i = 1; i < 3; ++i)
    {
        const float y = static_cast<float> (i * kRowH);
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.4f));
        g.drawLine  (0.0f, y, static_cast<float> (getWidth()), y, 0.5f);
    }

    paintRow1 (g);
    paintRow2 (g);
    paintRow3 (g);
}

void LooperStrip::paintRow1 (juce::Graphics& g) const
{
    const auto r   = row1();
    const int  pad = 4;
    int        x   = r.getX() + pad;

    // SOURCE dropdown
    {
        const juce::String src = (m_selectedSource < m_sourceNames.size())
                                 ? m_sourceNames[m_selectedSource] : "MIX";
        const int w = 70;
        const juce::Rectangle<int> br (x, r.getCentreY() - 7, w, 14);
        g.setColour (Theme::Colour::surface3);
        g.fillRoundedRectangle (br.toFloat(), 2.0f);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRoundedRectangle (br.toFloat(), 2.0f, 0.5f);
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMid);
        g.drawText  ("SRC: " + src + " \xe2\x96\xbe",
                     br.withTrimmedLeft (3), juce::Justification::centredLeft, true);
        x += w + pad;
    }

    // LIVE indicator
    {
        const juce::Colour liveCol = m_isLive ? juce::Colour (0xFF44DD66)
                                              : Theme::Colour::inkGhost;
        g.setColour (liveCol);
        g.fillEllipse (static_cast<float> (x), static_cast<float> (r.getCentreY() - 3), 6.0f, 6.0f);
        g.setFont   (Theme::Font::micro());
        g.setColour (liveCol);
        g.drawText  (m_isLive ? "LIVE" : "PAUSED",
                     juce::Rectangle<int> (x + 8, r.getY(), 36, r.getHeight()),
                     juce::Justification::centredLeft, false);
        x += 50;
    }

    // Grab bar buttons — right-aligned pack
    struct Btn { const char* label; int numBars; };
    const Btn grabBtns[] = { { "\xe2\x97\x80 4", 4 }, { "\xe2\x97\x80 8", 8 }, { "\xe2\x97\x80 16", 16 } };
    int rx = r.getRight() - pad;
    // FREE GRAB (rightmost)
    {
        const int w = 34;
        rx -= w;
        drawPillBtn (g, { rx, r.getCentreY() - 7, w, 14 }, "FREE", false);
        rx -= pad;
    }
    for (int i = 2; i >= 0; --i)
    {
        const int w = static_cast<int> (juce::GlyphArrangement::getStringWidth (Theme::Font::micro(),grabBtns[i].label) + 10);
        rx -= w;
        drawPillBtn (g, { rx, r.getCentreY() - 7, w, 14 }, grabBtns[i].label, true);
        rx -= pad;
    }
}

void LooperStrip::paintRow2 (juce::Graphics& g) const
{
    const auto r   = row2();
    const int  pad = 4;
    int        x   = r.getX() + pad;

    struct Btn { const char* label; bool active; };
    const Btn btns[] =
    {
        { "\xe2\x96\xb6 PLAY",    m_hasGrabbedClip },
        { "\xe2\x8a\x95 LOOP",    m_hasGrabbedClip },
        { "\xe2\x86\x92 REEL",    m_hasGrabbedClip },
        { "\xe2\x86\x92 TMLN",    m_hasGrabbedClip },
    };

    for (const auto& b : btns)
    {
        const int w = static_cast<int> (juce::GlyphArrangement::getStringWidth (Theme::Font::micro(),b.label) + 12);
        drawPillBtn (g, { x, r.getCentreY() - 7, w, 14 }, b.label, b.active,
                     b.active ? (juce::Colour) Theme::Zone::c : juce::Colour{});
        x += w + pad;
    }

    // "+ LOOPER B" at right edge
    {
        const int w = 44;
        drawPillBtn (g, { r.getRight() - pad - w, r.getCentreY() - 7, w, 14 },
                     "+ LOOPER B", m_showUnitB);
    }
}

void LooperStrip::paintRow3 (juce::Graphics& g) const
{
    const auto r   = row3();
    const int  pad = 6;
    int        x   = r.getX() + pad;

    // REC ARM
    {
        const int w = 56;
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("REC ARM", juce::Rectangle<int> (x, r.getY(), w, r.getHeight()),
                     juce::Justification::centredLeft, false);
        x += w;
        const int tw = 26;
        drawPillBtn (g, { x, r.getCentreY() - 7, tw, 14 },
                     m_recArmed ? "ON" : "OFF",
                     m_recArmed, Theme::Colour::error);
        x += tw + pad * 2;
    }

    // SNAP
    {
        static const char* snapLabels[] = { "BAR", "BEAT", "FREE" };
        const int w = 36;
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("SNAP", juce::Rectangle<int> (x, r.getY(), 28, r.getHeight()),
                     juce::Justification::centredLeft, false);
        x += 30;
        drawPillBtn (g, { x, r.getCentreY() - 7, w, 14 },
                     juce::String (snapLabels[m_snapMode]) + " \xe2\x96\xbe", true);
        x += w + pad * 2;
    }

    // AUTO-GRAB
    {
        const int w = 26;
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("AUTO-GRAB", juce::Rectangle<int> (x, r.getY(), 58, r.getHeight()),
                     juce::Justification::centredLeft, false);
        x += 60;
        drawPillBtn (g, { x, r.getCentreY() - 7, w, 14 },
                     m_autoGrab ? "ON" : "OFF",
                     m_autoGrab, Theme::Colour::accent);
    }
}

//==============================================================================
// Hit tests
//==============================================================================

bool LooperStrip::sourceHitAt (juce::Point<int> pos) const noexcept
{
    return row1().contains (pos) && pos.x >= 4 && pos.x <= 78;
}

bool LooperStrip::liveHitAt (juce::Point<int> pos) const noexcept
{
    return row1().contains (pos) && pos.x >= 82 && pos.x <= 130;
}

int LooperStrip::row1BtnAt (juce::Point<int> pos) const noexcept
{
    if (!row1().contains (pos)) return -1;
    const int cy = row1().getCentreY();
    if (pos.y < cy - 9 || pos.y > cy + 9) return -1;

    struct Btn { const char* label; int id; };
    const Btn btns[] = {
        { "\xe2\x97\x80 4", 0 }, { "\xe2\x97\x80 8", 1 }, { "\xe2\x97\x80 16", 2 }, { "FREE", 3 }
    };

    // Layout from right
    int rx = row1().getRight() - 4;
    // FREE
    const int freeW = 34;
    rx -= freeW;
    if (pos.x >= rx && pos.x <= rx + freeW) return 3;
    rx -= 4;
    for (int i = 2; i >= 0; --i)
    {
        const int w = static_cast<int> (juce::GlyphArrangement::getStringWidth (Theme::Font::micro(),btns[i].label) + 10);
        rx -= w;
        if (pos.x >= rx && pos.x <= rx + w) return i;
        rx -= 4;
    }
    return -1;
}

int LooperStrip::row2BtnAt (juce::Point<int> pos) const noexcept
{
    if (!row2().contains (pos)) return -1;
    const int cy = row2().getCentreY();
    if (pos.y < cy - 9 || pos.y > cy + 9) return -1;

    struct Btn { const char* label; };
    const Btn btns[] = {
        { "\xe2\x96\xb6 PLAY" }, { "\xe2\x8a\x95 LOOP" },
        { "\xe2\x86\x92 REEL" }, { "\xe2\x86\x92 TMLN" }
    };

    const int pad = 4;
    int x = row2().getX() + pad;
    for (int i = 0; i < 4; ++i)
    {
        const int w = static_cast<int> (juce::GlyphArrangement::getStringWidth (Theme::Font::micro(),btns[i].label) + 12);
        if (pos.x >= x && pos.x <= x + w) return i;
        x += w + pad;
    }

    // "+B" button at right
    const int bw = 44;
    const int bx = row2().getRight() - pad - bw;
    if (pos.x >= bx && pos.x <= bx + bw) return 4;

    return -1;
}

int LooperStrip::row3BtnAt (juce::Point<int> pos) const noexcept
{
    if (!row3().contains (pos)) return -1;
    // Rough hit areas
    if (pos.x >= 62  && pos.x <= 92)  return 0; // REC ARM toggle
    if (pos.x >= 118 && pos.x <= 158) return 1; // SNAP toggle
    if (pos.x >= 178 && pos.x <= 210) return 2; // AUTO-GRAB toggle
    return -1;
}

//==============================================================================
// Mouse
//==============================================================================

void LooperStrip::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // SOURCE dropdown
    if (sourceHitAt (pos))
    {
        juce::PopupMenu menu;
        for (int i = 0; i < m_sourceNames.size(); ++i)
            menu.addItem (i + 1, m_sourceNames[i]);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
            [this] (int result)
            {
                if (result > 0)
                {
                    m_selectedSource = result - 1;
                    if (onSourceChanged) onSourceChanged (m_selectedSource);
                    repaint();
                }
            });
        return;
    }

    // LIVE toggle
    if (liveHitAt (pos))
    {
        m_isLive = !m_isLive;
        if (onLiveToggled) onLiveToggled (m_isLive);
        repaint();
        return;
    }

    // Row 1 grab buttons
    const int r1btn = row1BtnAt (pos);
    if (r1btn >= 0)
    {
        if (r1btn < 3)
        {
            const int bars[] = { 4, 8, 16 };
            if (onGrabLastBars) onGrabLastBars (bars[r1btn]);
        }
        else if (r1btn == 3 && onGrabFree)
        {
            onGrabFree();
        }
        return;
    }

    // Row 2 buttons
    const int r2btn = row2BtnAt (pos);
    if (r2btn == 1 && onSendToLooper)   { onSendToLooper();   return; }
    if (r2btn == 2 && onSendToReel)     { onSendToReel();     return; }
    if (r2btn == 3 && onSendToTimeline) { onSendToTimeline(); return; }
    if (r2btn == 4)
    {
        m_showUnitB = !m_showUnitB;
        m_unitB.setVisible (m_showUnitB);
        resized();
        repaint();
        return;
    }

    // Row 3 toggles
    const int r3btn = row3BtnAt (pos);
    if (r3btn == 0) { m_recArmed = !m_recArmed; repaint(); return; }
    if (r3btn == 1) { m_snapMode = (m_snapMode + 1) % 3; repaint(); return; }
    if (r3btn == 2) { m_autoGrab = !m_autoGrab; repaint(); return; }
}

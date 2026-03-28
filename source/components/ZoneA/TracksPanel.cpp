#include "TracksPanel.h"
#include "ZoneAStyle.h"

//==============================================================================
TracksPanel::TracksPanel()
{
}

//==============================================================================
// External sync
//==============================================================================

void TracksPanel::setLanes (const juce::Array<LaneInfo>& lanes)
{
    m_lanes = lanes;
    repaint();
}

void TracksPanel::setPlaying (bool playing)
{
    m_playing = playing;
    repaint();
}

void TracksPanel::setRecording (bool recording)
{
    m_recording = recording;
    repaint();
}

void TracksPanel::setBpm (float bpm)
{
    m_bpm = bpm;
    repaint();
}

void TracksPanel::setPosition (float beat)
{
    m_beat = beat;
    repaint();
}

void TracksPanel::setLaneHeightScale (float s)
{
    m_laneHeightScale = juce::jlimit (0.4f, 3.0f, s);
    repaint();
}

//==============================================================================
// Layout helpers
//==============================================================================

juce::Rectangle<int> TracksPanel::topSection()  const noexcept { return { 0, kHeaderH, getWidth(), kTopH }; }
juce::Rectangle<int> TracksPanel::subTabBar()   const noexcept { return { 0, kHeaderH + kTopH, getWidth(), kSubTabBarH }; }
juce::Rectangle<int> TracksPanel::contentArea() const noexcept
{
    return { 0, kHeaderH + kTopH + kSubTabBarH, getWidth(),
             juce::jmax (0, getHeight() - kHeaderH - kTopH - kSubTabBarH) };
}

juce::Rectangle<int> TracksPanel::subTabRect (int t) const noexcept
{
    const int w = getWidth() / kSubTabCount;
    return { t * w, kHeaderH + kTopH, w, kSubTabBarH };
}

juce::Rectangle<int> TracksPanel::stopBtnRect() const noexcept
{
    const int y = kTopH - kPad - kBtnSz;
    return { kPad, y, kBtnSz, kBtnSz };
}

juce::Rectangle<int> TracksPanel::playBtnRect() const noexcept
{
    return stopBtnRect().translated (kBtnSz + 4, 0);
}

juce::Rectangle<int> TracksPanel::recBtnRect() const noexcept
{
    return playBtnRect().translated (kBtnSz + 4, 0);
}

juce::Rectangle<int> TracksPanel::bpmRect() const noexcept
{
    return { kPad, kPad + 28, 60, 20 };
}

//==============================================================================
// Paint
//==============================================================================

void TracksPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);
    ZoneAStyle::drawHeader (g, { 0, 0, getWidth(), kHeaderH }, "TRACKS",
                            ZoneAStyle::accentForTabId ("tracks"));
    paintTop       (g);
    paintSubTabBar (g);

    switch (m_activeSubTab)
    {
        case kLanes: paintLanesTab (g); break;
        case kLoop:  paintLoopTab  (g); break;
        case kView:  paintViewTab  (g); break;
        case kSnap:  paintSnapTab  (g); break;
        default: break;
    }
}

void TracksPanel::paintTransportBtn (juce::Graphics& g,
                                      const juce::Rectangle<int>& r,
                                      const juce::String& label,
                                      bool active) const
{
    g.setColour (active ? Theme::Zone::d.withAlpha (0.3f) : Theme::Colour::surface2);
    g.fillRoundedRectangle (r.toFloat(), Theme::Radius::xs);
    g.setColour (active ? Theme::Zone::d : Theme::Colour::surfaceEdge);
    g.drawRoundedRectangle (r.toFloat(), Theme::Radius::xs, Theme::Stroke::subtle);
    g.setFont (Theme::Font::micro());
    g.setColour (active ? Theme::Zone::d : Theme::Colour::inkMid);
    g.drawText (label, r, juce::Justification::centred, false);
}

void TracksPanel::paintTop (juce::Graphics& g) const
{
    const auto top = topSection();

    g.setColour (Theme::Colour::surface0);
    g.fillRect (top);

    g.setColour (Theme::Colour::surfaceEdge);
    g.fillRect (0, kTopH - 1, getWidth(), 1);

    // Bar.Beat.Tick display
    const int bar  = juce::jmax (1, static_cast<int> (m_beat / 4.0f) + 1);
    const int beat = static_cast<int> (m_beat) % 4 + 1;
    const int tick = static_cast<int> ((m_beat - static_cast<int> (m_beat)) * 100.0f);
    const juce::String pos = juce::String (bar) + " . " + juce::String (beat) + " . "
                           + juce::String (tick).paddedLeft ('0', 2);

    g.setFont (Theme::Font::transport());
    g.setColour (Theme::Zone::d);
    g.drawText (pos, kPad, kPad, getWidth() - kPad * 2, 24,
                juce::Justification::centredLeft, false);

    // BPM
    g.setFont (Theme::Font::value());
    g.setColour (Theme::Colour::accent);
    g.drawText (juce::String (juce::roundToInt (m_bpm)), bpmRect(),
                juce::Justification::centredLeft, false);

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("BPM", bpmRect().withX (bpmRect().getRight() + 2).withWidth (24),
                juce::Justification::centredLeft, false);

    // 4/4
    g.setFont (Theme::Font::value());
    g.setColour (Theme::Colour::inkMid);
    g.drawText ("4/4", bpmRect().withX (bpmRect().getRight() + 30).withWidth (30),
                juce::Justification::centredLeft, false);

    // Transport buttons
    paintTransportBtn (g, stopBtnRect(), "STP", !m_playing);
    paintTransportBtn (g, playBtnRect(), "PLY", m_playing);
    paintTransportBtn (g, recBtnRect(), "REC", m_recording);
}

void TracksPanel::paintSubTabBar (juce::Graphics& g) const
{
    const juce::String names[kSubTabCount] = { "LANES", "LOOP", "VIEW", "SNAP" };

    g.setColour (Theme::Colour::surface2);
    g.fillRect (subTabBar());

    g.setColour (Theme::Colour::surfaceEdge);
    g.fillRect (0, kTopH + kSubTabBarH - 1, getWidth(), 1);

    for (int t = 0; t < kSubTabCount; ++t)
    {
        const auto r = subTabRect (t);
        const bool active = (t == m_activeSubTab);

        if (active)
        {
            g.setColour (Theme::Zone::d);
            g.fillRect (r.withHeight (2).withY (r.getBottom() - 2));
        }

        g.setFont (Theme::Font::micro());
        g.setColour (active ? Theme::Colour::inkLight : Theme::Colour::inkGhost);
        g.drawText (names[t], r, juce::Justification::centred, false);
    }
}

void TracksPanel::paintLanesTab (juce::Graphics& g) const
{
    const auto ca = contentArea();
    int y = ca.getY() + kPad;

    // Header row
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("LANES", kPad, y, 60, 14, juce::Justification::centredLeft, false);
    g.setColour (Theme::Zone::a);
    g.drawText ("+ ADD", getWidth() - 50, y, 44, 14, juce::Justification::centredRight, false);
    y += 18;

    if (m_lanes.isEmpty())
    {
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText ("No active lanes", kPad, y, getWidth() - kPad * 2, 20,
                    juce::Justification::centredLeft, false);
        return;
    }

    for (int i = 0; i < m_lanes.size() && y + kLaneRowH <= ca.getBottom(); ++i)
    {
        const auto& lane = m_lanes.getReference (i);
        const auto rowR  = juce::Rectangle<int> (0, y, getWidth(), kLaneRowH);

        // Colour dot
        g.setColour (lane.colour);
        g.fillEllipse (static_cast<float> (kPad),
                       static_cast<float> (rowR.getCentreY() - 3),
                       6.0f, 6.0f);

        // Name
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMid);
        g.drawText (lane.name, kPad + 10, y, getWidth() - 120, kLaneRowH,
                    juce::Justification::centredLeft, false);

        // M / S / ARM buttons
        const int btnW = 18, btnH = 14;
        const int btnY = rowR.getCentreY() - btnH / 2;
        const auto mR  = juce::Rectangle<int> (getWidth() - 60, btnY, btnW, btnH);
        const auto sR  = mR.translated (btnW + 2, 0);
        const auto aR  = sR.translated (btnW + 2, 0);

        auto paintMiniBtn = [&] (const juce::Rectangle<int>& br, const juce::String& lbl, bool active, juce::Colour activeCol)
        {
            g.setColour (active ? activeCol.withAlpha (0.3f) : Theme::Colour::surface3);
            g.fillRoundedRectangle (br.toFloat(), Theme::Radius::xs);
            g.setColour (active ? activeCol : Theme::Colour::inkGhost);
            g.setFont (Theme::Font::micro());
            g.drawText (lbl, br, juce::Justification::centred, false);
        };

        paintMiniBtn (mR, "M", lane.muted,  Theme::Colour::error);
        paintMiniBtn (sR, "S", lane.soloed, Theme::Zone::d);
        paintMiniBtn (aR, "A", lane.armed,  Theme::Colour::error);

        y += kLaneRowH;
    }
}

void TracksPanel::paintLoopTab (juce::Graphics& g) const
{
    const auto ca = contentArea();
    int y = ca.getY() + kPad;

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("LOOP START", kPad, y, getWidth() - kPad * 2, 14,
                juce::Justification::centredLeft, false);
    y += 16;

    g.setFont (Theme::Font::value());
    g.setColour (Theme::Colour::inkLight);
    g.drawText ("Bar " + juce::String (juce::roundToInt (m_loopStart)),
                kPad, y, getWidth() - kPad * 2, 20, juce::Justification::centredLeft, false);
    y += 24;

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("LOOP LENGTH", kPad, y, getWidth() - kPad * 2, 14,
                juce::Justification::centredLeft, false);
    y += 16;

    g.setFont (Theme::Font::value());
    g.setColour (Theme::Colour::inkLight);
    g.drawText (juce::String (juce::roundToInt (m_loopLength)) + " bars",
                kPad, y, getWidth() - kPad * 2, 20, juce::Justification::centredLeft, false);
    y += 24;

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("LOOP END  Bar " + juce::String (juce::roundToInt (m_loopStart + m_loopLength)),
                kPad, y, getWidth() - kPad * 2, 14, juce::Justification::centredLeft, false);
}

void TracksPanel::paintViewTab (juce::Graphics& g) const
{
    const auto ca = contentArea();
    int y = ca.getY() + kPad;

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);

    g.drawText ("HEIGHT SCALE", kPad, y, getWidth() - kPad * 2, 14,
                juce::Justification::centredLeft, false);
    y += 16;

    g.setFont (Theme::Font::value());
    g.setColour (Theme::Colour::inkLight);
    g.drawText (juce::String (m_laneHeightScale, 2) + "x",
                kPad, y, 60, 20, juce::Justification::centredLeft, false);
    y += 28;

    // Height mode buttons
    g.setFont (Theme::Font::micro());
    const juce::String modes[4] = { "MIN", "NORM", "MAX", "EXPAND" };
    const int modeW = (getWidth() - kPad * 2 - 6) / 4;
    for (int m = 0; m < 4; ++m)
    {
        const auto mr = juce::Rectangle<int> (kPad + m * (modeW + 2), y, modeW, 18);
        g.setColour (Theme::Colour::surface3);
        g.fillRoundedRectangle (mr.toFloat(), Theme::Radius::xs);
        g.setColour (Theme::Colour::inkGhost);
        g.drawRoundedRectangle (mr.toFloat(), Theme::Radius::xs, Theme::Stroke::subtle);
        g.drawText (modes[m], mr, juce::Justification::centred, false);
    }
    y += 26;

    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("HORIZONTAL ZOOM", kPad, y, getWidth() - kPad * 2, 14,
                juce::Justification::centredLeft, false);
    y += 16;
    g.setFont (Theme::Font::value());
    g.setColour (Theme::Colour::inkLight);
    g.drawText (juce::String (m_viewZoom, 1) + "x", kPad, y, 60, 20,
                juce::Justification::centredLeft, false);
}

void TracksPanel::paintSnapTab (juce::Graphics& g) const
{
    const auto ca = contentArea();
    int y = ca.getY() + kPad;

    const juce::String snapNames[6] = { "1/4 note", "1/8 note", "1/16 note",
                                        "1/32 note", "Bar", "Free" };

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("SNAP MODE", kPad, y, getWidth() - kPad * 2, 14,
                juce::Justification::centredLeft, false);
    y += 16;

    for (int i = 0; i < 6; ++i)
    {
        const bool active = (i == m_snapMode);
        const auto rr = juce::Rectangle<int> (kPad, y, getWidth() - kPad * 2, 20);

        // Radio dot
        const int dotD = 10;
        const auto dot = juce::Rectangle<int> (kPad, rr.getCentreY() - dotD / 2, dotD, dotD);
        g.setColour (active ? Theme::Zone::a : Theme::Colour::surface3);
        g.fillEllipse (dot.toFloat());
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawEllipse (dot.toFloat(), 0.5f);

        if (active)
        {
            g.setColour (Theme::Colour::inkDark);
            g.fillEllipse (dot.reduced (3).toFloat());
        }

        g.setFont (Theme::Font::micro());
        g.setColour (active ? Theme::Colour::inkMid : Theme::Colour::inkGhost);
        g.drawText (snapNames[i], kPad + dotD + 6, y, getWidth() - kPad * 2 - dotD - 6, 20,
                    juce::Justification::centredLeft, false);
        y += 22;
    }

    // Swing
    y += 4;
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("SWING  " + juce::String (juce::roundToInt (m_swing * 100)) + "%",
                kPad, y, getWidth() - kPad * 2, 14, juce::Justification::centredLeft, false);
}

//==============================================================================
void TracksPanel::resized()
{
}

//==============================================================================
void TracksPanel::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // Sub-tab clicks
    for (int t = 0; t < kSubTabCount; ++t)
    {
        if (subTabRect (t).contains (pos))
        {
            m_activeSubTab = t;
            repaint();
            return;
        }
    }

    // Transport buttons
    if (stopBtnRect().contains (pos))
    {
        m_playing   = false;
        m_recording = false;
        if (onStop) onStop();
        repaint();
        return;
    }
    if (playBtnRect().contains (pos))
    {
        m_playing = !m_playing;
        if (onPlay) onPlay();
        repaint();
        return;
    }
    if (recBtnRect().contains (pos))
    {
        m_recording = !m_recording;
        if (onRecord) onRecord();
        repaint();
        return;
    }

    // BPM drag start
    if (bpmRect().contains (pos))
    {
        m_draggingBpm  = true;
        m_dragStartY   = static_cast<float> (pos.getY());
        m_dragStartBpm = m_bpm;
        return;
    }

    // LANES tab — M/S/A button clicks
    if (m_activeSubTab == kLanes)
    {
        const auto ca = contentArea();
        int y = ca.getY() + kPad + 18;   // skip header row
        const int btnW = 18, btnH = 14;

        for (int i = 0; i < m_lanes.size() && y + kLaneRowH <= ca.getBottom(); ++i)
        {
            const auto rowR = juce::Rectangle<int> (0, y, getWidth(), kLaneRowH);
            if (rowR.contains (pos))
            {
                auto& lane    = m_lanes.getReference (i);
                const int btnY = rowR.getCentreY() - btnH / 2;
                const auto mR  = juce::Rectangle<int> (getWidth() - 60, btnY, btnW, btnH);
                const auto sR  = mR.translated (btnW + 2, 0);
                const auto aR  = sR.translated (btnW + 2, 0);

                if (mR.contains (pos))
                {
                    lane.muted = !lane.muted;
                    if (onMuteChanged) onMuteChanged (i, lane.muted);
                    repaint();
                    return;
                }
                if (sR.contains (pos))
                {
                    const bool additive = e.mods.isShiftDown();
                    const bool newSolo  = !lane.soloed;

                    if (newSolo && !additive)
                    {
                        for (int j = 0; j < m_lanes.size(); ++j)
                        {
                            auto& other = m_lanes.getReference (j);
                            if (j != i && other.soloed)
                            {
                                other.soloed = false;
                                if (onSoloChanged) onSoloChanged (j, false);
                            }
                        }
                    }

                    lane.soloed = newSolo;
                    if (onSoloChanged) onSoloChanged (i, newSolo);
                    repaint();
                    return;
                }
                if (aR.contains (pos))
                {
                    lane.armed = !lane.armed;
                    if (onArmChanged) onArmChanged (i, lane.armed);
                    repaint();
                    return;
                }
            }
            y += kLaneRowH;
        }
    }

    // SNAP tab radio buttons
    if (m_activeSubTab == kSnap)
    {
        const auto ca = contentArea();
        int y = ca.getY() + kPad + 16;
        for (int i = 0; i < 6; ++i)
        {
            const auto rr = juce::Rectangle<int> (kPad, y, getWidth() - kPad * 2, 20);
            if (rr.contains (pos))
            {
                m_snapMode = i;
                if (onSnapChanged) onSnapChanged (i);
                repaint();
                return;
            }
            y += 22;
        }
    }
}

void TracksPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (!m_draggingBpm) return;

    const float delta = m_dragStartY - static_cast<float> (e.getPosition().getY());
    m_bpm = juce::jlimit (40.0f, 240.0f, m_dragStartBpm + delta * 0.5f);
    if (onBpmChanged) onBpmChanged (m_bpm);
    repaint();
}

void TracksPanel::mouseUp (const juce::MouseEvent&)
{
    m_draggingBpm = false;
}

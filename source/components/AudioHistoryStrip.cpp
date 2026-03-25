#include "AudioHistoryStrip.h"
#include "TapeColors.h"

//==============================================================================
// Tape colours (shared with CylinderBand)
static const juce::Colour kPlayheadColor { 0xFF694412 };
static const juce::Colour kHousingEdge   { 0xFF080604 };

//==============================================================================
AudioHistoryStrip::AudioHistoryStrip()
{
    startTimerHz (15);
}

void AudioHistoryStrip::setCollapsed (bool collapsed) noexcept
{
    // When collapsing, preserve m_savedHeight so double-click can restore it.
    // Only set a default restore height if none is saved yet.
    if (!collapsed && m_savedHeight <= kMinH)
        m_savedHeight = kDefaultH;

    if (onHeightChanged)
        onHeightChanged (collapsed ? kMinH : m_savedHeight);
    resized();
}

AudioHistoryStrip::~AudioHistoryStrip()
{
    stopTimer();
}

//==============================================================================
void AudioHistoryStrip::setSourceInfo (const juce::String& name, juce::Colour color)
{
    m_sourceName  = name;
    m_sourceColor = color;
    repaint();
}

void AudioHistoryStrip::setGrabbedRegion (double startSecondsAgo, double lengthSeconds)
{
    m_hasGrabbedRegion = true;
    m_grabbedStart     = startSecondsAgo;
    m_grabbedLength    = lengthSeconds;
    repaint();
}

void AudioHistoryStrip::clearGrabbedRegion()
{
    m_hasGrabbedRegion = false;
    repaint();
}

//==============================================================================
void AudioHistoryStrip::timerCallback()
{
    if (m_buffer == nullptr || isCollapsed()) return;

    const int waveW = waveRect().getWidth();
    if (waveW > 0)
        m_waveformData = m_buffer->getWaveformRMS (waveW, m_windowSeconds);

    repaint();
}

//==============================================================================
// Layout
//==============================================================================

void AudioHistoryStrip::resized()
{
    const int h = getHeight();
    if (h > kMinH)
        m_savedHeight = h;
}

juce::Rectangle<int> AudioHistoryStrip::tabRect() const noexcept
{
    return { 0, 0, kTabW, getHeight() };
}

juce::Rectangle<int> AudioHistoryStrip::mainRect() const noexcept
{
    return { kTabW, 0, getWidth() - kTabW, getHeight() };
}

juce::Rectangle<int> AudioHistoryStrip::headerRect() const noexcept
{
    return { kTabW, 0, getWidth() - kTabW, kHeaderH };
}

juce::Rectangle<int> AudioHistoryStrip::waveRect() const noexcept
{
    const int h = getHeight();
    if (isCollapsed()) return {};

    const bool showGrab = (h > 60);
    const int  bottom   = showGrab ? h - kGrabRowH : h;
    return { kTabW, kHeaderH, getWidth() - kTabW, bottom - kHeaderH };
}

juce::Rectangle<int> AudioHistoryStrip::grabRect() const noexcept
{
    if (getHeight() <= 60) return {};
    return { kTabW, getHeight() - kGrabRowH, getWidth() - kTabW, kGrabRowH };
}

//==============================================================================
// Coordinate helpers
//==============================================================================

float AudioHistoryStrip::xToSecondsAgo (float x) const noexcept
{
    const auto wr = waveRect();
    if (wr.getWidth() <= 0) return 0.0f;
    const float rimW      = 4.0f;
    const float waveW     = static_cast<float> (wr.getWidth()) - rimW * 2.0f;
    const float relX      = x - static_cast<float> (wr.getX()) - rimW;
    const float normalized = 1.0f - relX / waveW;  // 0=now, 1=oldest
    return juce::jlimit (0.0f, static_cast<float> (m_windowSeconds),
                         normalized * static_cast<float> (m_windowSeconds));
}

float AudioHistoryStrip::secondsAgoToX (float secondsAgo) const noexcept
{
    const auto wr = waveRect();
    if (wr.getWidth() <= 0) return 0.0f;
    const float rimW   = 4.0f;
    const float waveW  = static_cast<float> (wr.getWidth()) - rimW * 2.0f;
    const float norm   = secondsAgo / static_cast<float> (m_windowSeconds);
    return static_cast<float> (wr.getX()) + rimW + (1.0f - norm) * waveW;
}

//==============================================================================
// Paint
//==============================================================================

void AudioHistoryStrip::paint (juce::Graphics& g)
{
    paintTab (g);

    if (isCollapsed()) return;

    paintHeader  (g);
    paintWaveform (g);
    if (getHeight() > 60)
        paintGrabControls (g);
}

void AudioHistoryStrip::paintTab (juce::Graphics& g) const
{
    const auto r = tabRect();

    g.setColour (Theme::Colour::surface0);
    g.fillRect  (r);

    // Right border
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine  (static_cast<float> (r.getRight() - 1), static_cast<float> (r.getY()),
                 static_cast<float> (r.getRight() - 1), static_cast<float> (r.getBottom()), 0.5f);
    // Top border
    g.drawLine  (static_cast<float> (r.getX()), static_cast<float> (r.getY()),
                 static_cast<float> (r.getRight()), static_cast<float> (r.getY()), 0.5f);

    const int cx = r.getCentreX();
    int       y  = r.getY() + 6;

    // Waveform icon (simple lines)
    g.setColour (Theme::Colour::accent);
    for (int i = -2; i <= 2; ++i)
    {
        const int ht = (i == 0) ? 8 : (std::abs (i) == 1 ? 5 : 3);
        g.drawLine (static_cast<float> (cx + i * 3), static_cast<float> (y + 6 - ht / 2),
                    static_cast<float> (cx + i * 3), static_cast<float> (y + 6 + ht / 2), 1.5f);
    }
    y += 14;

    // "HIST" label
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  ("HIST", r.withTop (y).withHeight (10), juce::Justification::centred, false);
    y += 12;

    // Collapse arrow
    const bool collapsed = isCollapsed();
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  (collapsed ? "v" : "^",
                 r.withTop (r.getBottom() - 14).withHeight (12),
                 juce::Justification::centred, false);
}

void AudioHistoryStrip::paintHeader (juce::Graphics& g) const
{
    const auto r = headerRect();

    g.setColour (Theme::Colour::surface0);
    g.fillRect  (r);

    // Top border (shared resize handle boundary)
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine  (static_cast<float> (r.getX()), static_cast<float> (r.getY()),
                 static_cast<float> (r.getRight()), static_cast<float> (r.getY()), 1.0f);
    // Bottom divider
    g.drawLine  (static_cast<float> (r.getX()), static_cast<float> (r.getBottom()),
                 static_cast<float> (r.getRight()), static_cast<float> (r.getBottom()), 0.5f);

    const int pad = 6;

    // Source name (left)
    g.setFont   (Theme::Font::label());
    g.setColour (m_sourceColor);
    g.fillEllipse (static_cast<float> (r.getX() + pad), static_cast<float> (r.getCentreY() - 3),
                   6.0f, 6.0f);
    g.setColour (Theme::Colour::inkLight);
    g.drawText  (m_sourceName,
                 r.withLeft (r.getX() + pad + 10).withRight (r.getCentreX() - 30),
                 juce::Justification::centredLeft, true);

    // LIVE indicator (after source name)
    {
        const int liveX = r.getX() + pad + 10 + 80;
        const juce::Colour liveCol = m_isLive ? juce::Colour (0xFF44DD66)
                                              : Theme::Colour::inkGhost;
        g.setColour (liveCol);
        g.fillEllipse (static_cast<float> (liveX), static_cast<float> (r.getCentreY() - 3),
                       6.0f, 6.0f);
        g.setFont   (Theme::Font::micro());
        g.drawText  (m_isLive ? "LIVE" : "PAUSED",
                     juce::Rectangle<int> (liveX + 9, r.getY(), 40, r.getHeight()),
                     juce::Justification::centredLeft, false);
    }

    // Window size label (centre)
    {
        const double ws = m_windowSeconds;
        juce::String label;
        if (ws < 60.0)      label = juce::String (static_cast<int> (ws)) + "s";
        else if (ws < 120.0) label = "1m";
        else if (ws < 300.0) label = "2m";
        else                  label = "5m";

        g.setFont   (Theme::Font::label());
        g.setColour (Theme::Colour::inkMid);
        g.drawText  (label, r.withLeft (r.getCentreX() - 30).withWidth (60),
                     juce::Justification::centred, false);
    }

    // Buffer duration (right)
    {
        const double dur = m_buffer ? m_buffer->getDuration() : 120.0;
        const int    mins = static_cast<int> (dur) / 60;
        const int    secs = static_cast<int> (dur) % 60;
        const juce::String durLabel = juce::String (mins) + ":"
                                      + juce::String (secs).paddedLeft ('0', 2)
                                      + " buffer";
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  (durLabel, r.withTrimmedRight (pad), juce::Justification::centredRight, false);
    }
}

void AudioHistoryStrip::paintWaveform (juce::Graphics& g) const
{
    const auto wr = waveRect();
    if (wr.isEmpty()) return;

    // Tape base background
    g.setColour (TapeColors::tapeBase);
    g.fillRect  (wr);

    // Hard rims (left and right edges — same as CylinderBand)
    g.setColour (kHousingEdge);
    g.fillRect  (wr.getX(),              wr.getY(), 4, wr.getHeight());
    g.fillRect  (wr.getRight() - 4,      wr.getY(), 4, wr.getHeight());

    // Gradient fades (80px each side)
    const int fadeW = juce::jmin (80, wr.getWidth() / 3);
    {
        juce::ColourGradient leftFade (kHousingEdge.withAlpha (0.7f),
                                       static_cast<float> (wr.getX() + 4), 0.0f,
                                       kHousingEdge.withAlpha (0.0f),
                                       static_cast<float> (wr.getX() + 4 + fadeW), 0.0f,
                                       false);
        g.setGradientFill (leftFade);
        g.fillRect (wr.getX() + 4, wr.getY(), fadeW, wr.getHeight());

        juce::ColourGradient rightFade (kHousingEdge.withAlpha (0.0f),
                                        static_cast<float> (wr.getRight() - 4 - fadeW), 0.0f,
                                        kHousingEdge.withAlpha (0.7f),
                                        static_cast<float> (wr.getRight() - 4), 0.0f,
                                        false);
        g.setGradientFill (rightFade);
        g.fillRect (wr.getRight() - 4 - fadeW, wr.getY(), fadeW, wr.getHeight());
    }

    // Waveform path
    if (m_waveformData.size() > 1)
    {
        const float cx   = static_cast<float> (wr.getCentreY());
        const float maxH = static_cast<float> (wr.getHeight()) * 0.42f;
        const float waveStartX = static_cast<float> (wr.getX() + 4);
        const float waveW      = static_cast<float> (wr.getWidth() - 8);

        juce::Path p;
        const int n = m_waveformData.size();
        p.startNewSubPath (waveStartX, cx);
        for (int i = 0; i < n; ++i)
        {
            const float x = waveStartX + (static_cast<float> (i) / n) * waveW;
            p.lineTo (x, cx - m_waveformData[i] * maxH);
        }
        for (int i = n - 1; i >= 0; --i)
        {
            const float x = waveStartX + (static_cast<float> (i) / n) * waveW;
            p.lineTo (x, cx + m_waveformData[i] * maxH);
        }
        p.closeSubPath();

        g.setColour (m_sourceColor.withAlpha (0.65f));
        g.fillPath  (p);
        g.setColour (m_sourceColor.withAlpha (0.85f));
        g.strokePath (p, juce::PathStrokeType (1.0f));
    }

    // Selection highlight
    if (m_hasSelection && m_selStart != m_selEnd)
    {
        const float x1 = secondsAgoToX (static_cast<float> (juce::jmax (m_selStart, m_selEnd)));
        const float x2 = secondsAgoToX (static_cast<float> (juce::jmin (m_selStart, m_selEnd)));
        const auto  selR = juce::Rectangle<float> (x1, static_cast<float> (wr.getY()),
                                                    x2 - x1, static_cast<float> (wr.getHeight()));
        g.setColour (Theme::Colour::accent.withAlpha (0.18f));
        g.fillRect  (selR);
        g.setColour (Theme::Colour::accent);
        g.drawLine  (x1, static_cast<float> (wr.getY()),     x1, static_cast<float> (wr.getBottom()), 2.0f);
        g.drawLine  (x2, static_cast<float> (wr.getY()),     x2, static_cast<float> (wr.getBottom()), 2.0f);

        // Duration label
        const double len     = std::abs (m_selEnd - m_selStart);
        const juce::String lbl = (len < 60.0) ? juce::String (len, 1) + "s"
                                               : juce::String (len / 60.0, 1) + "m";
        const float lblX = (x1 + x2) * 0.5f - 20.0f;
        const auto  lblR = juce::Rectangle<int> (static_cast<int> (lblX),
                                                 wr.getY() + 2, 40, 12);
        g.setColour (Theme::Colour::surface3);
        g.fillRoundedRectangle (lblR.toFloat().expanded (4.0f, 0.0f), 3.0f);
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkLight);
        g.drawText  (lbl, lblR, juce::Justification::centred, false);
    }

    // Grabbed region highlight
    if (m_hasGrabbedRegion)
    {
        const float x1 = secondsAgoToX (static_cast<float> (m_grabbedStart + m_grabbedLength));
        const float x2 = secondsAgoToX (static_cast<float> (m_grabbedStart));
        const auto  gr = juce::Rectangle<float> (x1, static_cast<float> (wr.getY()),
                                                  juce::jmax (2.0f, x2 - x1),
                                                  static_cast<float> (wr.getHeight()));
        g.setColour (Theme::Zone::c.withAlpha (0.20f));
        g.fillRect  (gr);
        g.setColour (Theme::Zone::c);
        g.drawLine  (gr.getX(),     static_cast<float> (wr.getY()), gr.getX(),     static_cast<float> (wr.getBottom()), 2.0f);
        g.drawLine  (gr.getRight(), static_cast<float> (wr.getY()), gr.getRight(), static_cast<float> (wr.getBottom()), 2.0f);

        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Zone::c);
        g.drawText  ("GRABBED", juce::Rectangle<int> (static_cast<int> (gr.getCentreX()) - 24,
                                                       wr.getY() + 2, 48, 10),
                     juce::Justification::centred, false);
    }

    // Playhead at right edge ("NOW")
    {
        const float px = static_cast<float> (wr.getRight() - 4);
        g.setColour (kPlayheadColor);
        g.drawLine  (px, static_cast<float> (wr.getY()), px, static_cast<float> (wr.getBottom()), 1.0f);

        juce::Path tri;
        tri.addTriangle (px - 4.0f, static_cast<float> (wr.getY()),
                         px + 4.0f, static_cast<float> (wr.getY()),
                         px,        static_cast<float> (wr.getY() + 6));
        g.fillPath (tri);

        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("NOW", juce::Rectangle<int> (static_cast<int> (px) - 14,
                                                   wr.getY() + 8, 28, 8),
                     juce::Justification::centred, false);
    }

    // Time labels at bottom
    {
        const int numLabels = 5;
        g.setFont   (Theme::Font::micro());
        g.setColour (TapeColors::inkOnTape.withAlpha (0.7f));
        for (int i = 1; i < numLabels; ++i)
        {
            const double sAgo = m_windowSeconds * static_cast<double> (i) / numLabels;
            const float  lx   = secondsAgoToX (static_cast<float> (sAgo));
            juce::String lbl;
            if (sAgo < 60.0)  lbl = juce::String (static_cast<int> (sAgo)) + "s";
            else               lbl = juce::String (static_cast<int> (sAgo / 60.0)) + "m";
            g.drawText (lbl, juce::Rectangle<int> (static_cast<int> (lx) - 12,
                                                    wr.getBottom() - 10, 24, 9),
                        juce::Justification::centredLeft, false);
        }
    }
}

void AudioHistoryStrip::paintGrabControls (juce::Graphics& g) const
{
    const auto r = grabRect();
    if (r.isEmpty()) return;

    g.setColour (Theme::Colour::surface0);
    g.fillRect  (r);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine  (static_cast<float> (r.getX()),     static_cast<float> (r.getY()),
                 static_cast<float> (r.getRight()), static_cast<float> (r.getY()), 0.5f);

    struct BtnSpec { const char* label; bool active; };
    const BtnSpec btns[] =
    {
        { "\xe2\x97\x80 4",   true  },
        { "\xe2\x97\x80 8",   true  },
        { "\xe2\x97\x80 16",  true  },
        { "FREE",              m_hasSelection },
        { "\xe2\x86\x92 REEL",     m_hasGrabbedRegion },
        { "\xe2\x86\x92 TMLN",     m_hasGrabbedRegion },
    };

    const int pad  = 4;
    const int btnH = r.getHeight() - 2;
    int       x    = r.getX() + pad;

    for (auto& b : btns)
    {
        const int w = static_cast<int> (juce::GlyphArrangement::getStringWidth (Theme::Font::micro(),b.label) + 12);
        const juce::Rectangle<int> br (x, r.getY() + 1, w, btnH);

        g.setColour (b.active ? (juce::Colour) Theme::Colour::surface2
                              : (juce::Colour) Theme::Colour::surface1);
        g.fillRoundedRectangle (br.toFloat(), 2.0f);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRoundedRectangle (br.toFloat(), 2.0f, 0.5f);

        g.setFont   (Theme::Font::micro());
        g.setColour (b.active ? (juce::Colour) Theme::Colour::inkLight
                              : (juce::Colour) Theme::Colour::inkGhost);
        g.drawText  (b.label, br, juce::Justification::centred, false);

        x += w + pad;
        if (&b == &btns[3]) x += 8;  // gap before routing buttons
    }
}

//==============================================================================
// Mouse
//==============================================================================

juce::MouseCursor AudioHistoryStrip::getMouseCursor()
{
    const auto pos = getMouseXYRelative();

    if (pos.y < kHandleH && !isCollapsed())
        return juce::MouseCursor::UpDownResizeCursor;

    if (!isCollapsed())
    {
        const auto mode = selDragModeAt (pos);
        if (mode == SelDrag::ResizingLeft || mode == SelDrag::ResizingRight)
            return juce::MouseCursor::LeftRightResizeCursor;
    }

    return juce::MouseCursor::NormalCursor;
}

void AudioHistoryStrip::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // Collapse toggle click on left tab
    if (tabRect().contains (pos))
    {
        // handled in mouseDoubleClick
        return;
    }

    // Resize handle — top kHandleH px of main area
    if (!isCollapsed() && pos.y < kHandleH && pos.x >= kTabW)
    {
        m_isResizeDrag     = true;
        m_resizeDragStartY = e.getScreenY();
        m_resizeDragStartH = getHeight();
        return;
    }

    // LIVE dot toggle in header
    if (!isCollapsed() && headerRect().contains (pos))
    {
        // approximate hit on LIVE dot (at ~kTabW+100, centreY)
        const int dotX = kTabW + 6 + 10 + 80;
        if (pos.x >= dotX && pos.x <= dotX + 50)
        {
            m_isLive = !m_isLive;
            if (m_buffer) m_buffer->setActive (m_isLive);
            if (onActiveToggled) onActiveToggled (m_isLive);
            repaint();
            return;
        }

        // Window size click (centre area)
        const int centreX = headerRect().getCentreX();
        if (pos.x >= centreX - 30 && pos.x <= centreX + 30)
        {
            m_windowSizeIndex = (m_windowSizeIndex + 1) % kNumWindowSizes;
            m_windowSeconds   = kWindowSizes[m_windowSizeIndex];
            repaint();
            return;
        }
        return;
    }

    // Grab control buttons
    if (!isCollapsed() && grabRect().contains (pos))
    {
        const int btn = grabBtnAt (pos);
        if      (btn == 0 && onGrabLastBars) onGrabLastBars (4);
        else if (btn == 1 && onGrabLastBars) onGrabLastBars (8);
        else if (btn == 2 && onGrabLastBars) onGrabLastBars (16);
        else if (btn == 3 && m_hasSelection && onGrabRegion)
        {
            const double start = juce::jmax (m_selStart, m_selEnd);
            const double len   = std::abs (m_selEnd - m_selStart);
            onGrabRegion (start, len);
        }
        else if (btn == 4)
        {
            if (e.mods.isShiftDown() && onSendToNewReelSlot) onSendToNewReelSlot();
            else if (onSendToReel)                           onSendToReel();
        }
        else if (btn == 5 && onSendToTimeline) onSendToTimeline();
        return;
    }

    // Waveform area — begin selection or grabbed-region drag
    if (!isCollapsed() && waveRect().contains (pos))
    {
        // If there is a grabbed region and the click lands inside it, prepare a
        // drag-and-drop gesture instead of starting a selection.
        if (m_hasGrabbedRegion)
        {
            const float leftX  = secondsAgoToX (static_cast<float> (m_grabbedStart + m_grabbedLength));
            const float rightX = secondsAgoToX (static_cast<float> (m_grabbedStart));
            const float px     = static_cast<float> (pos.x);
            if (px >= leftX - 8.0f && px <= rightX + 8.0f)
            {
                m_isGrabDragPending = true;
                return;   // don't start selection here
            }
        }

        const double sAgo = static_cast<double> (xToSecondsAgo (static_cast<float> (pos.x)));
        const auto   mode = selDragModeAt (pos);

        if (mode == SelDrag::ResizingLeft || mode == SelDrag::ResizingRight)
        {
            m_selDrag    = mode;
            m_dragAnchor = sAgo;
        }
        else if (mode == SelDrag::Moving)
        {
            m_selDrag    = SelDrag::Moving;
            m_dragAnchor = sAgo;
            m_selMoveOff = sAgo - m_selStart;
        }
        else
        {
            m_selDrag    = SelDrag::Creating;
            m_selStart   = sAgo;
            m_selEnd     = sAgo;
            m_hasSelection = false;
        }
    }
}

void AudioHistoryStrip::mouseDrag (const juce::MouseEvent& e)
{
    // Pending DnD from grabbed region — fire once drag threshold is crossed
    if (m_isGrabDragPending)
    {
        if (e.getDistanceFromDragStart() > 5.0f)
        {
            m_isGrabDragPending = false;

            // Notify PluginEditor first so it can store the CapturedAudioClip
            if (onDragStarted) onDragStarted();

            // Build a small visual thumbnail for the drag cursor
            juce::Image img (juce::Image::ARGB, 80, 20, true);
            {
                juce::Graphics gfx (img);
                gfx.setColour (Theme::Colour::accent.withAlpha (0.85f));
                gfx.fillRoundedRectangle (img.getBounds().toFloat(), 3.0f);
                gfx.setColour (juce::Colours::white);
                gfx.setFont   (Theme::Font::micro());
                gfx.drawText  ("  CLIP", img.getBounds(), juce::Justification::centredLeft, false);
            }

            if (auto* ddc = juce::DragAndDropContainer::findParentDragContainerFor (this))
                ddc->startDragging ("CapturedAudioClip", this, juce::ScaledImage (img));
        }
        return;
    }

    if (m_isResizeDrag)
    {
        const int dy     = m_resizeDragStartY - e.getScreenY();  // drag up = taller
        const int newH   = juce::jlimit (kMinH, kMaxH, m_resizeDragStartH + dy);
        if (onHeightChanged) onHeightChanged (newH);
        return;
    }

    if (m_selDrag == SelDrag::None) return;

    const double sAgo = static_cast<double> (
        xToSecondsAgo (static_cast<float> (e.getPosition().x)));

    if (m_selDrag == SelDrag::Creating)
    {
        m_selEnd       = sAgo;
        m_hasSelection = (std::abs (m_selEnd - m_selStart) > 0.1);
    }
    else if (m_selDrag == SelDrag::Moving)
    {
        const double len = m_selEnd - m_selStart;
        m_selStart = sAgo - m_selMoveOff;
        m_selEnd   = m_selStart + len;
    }
    else if (m_selDrag == SelDrag::ResizingLeft)
    {
        m_selStart = sAgo;
    }
    else if (m_selDrag == SelDrag::ResizingRight)
    {
        m_selEnd = sAgo;
    }

    repaint();
}

void AudioHistoryStrip::mouseUp (const juce::MouseEvent&)
{
    m_isResizeDrag     = false;
    m_selDrag          = SelDrag::None;
    m_isGrabDragPending = false;
}

void AudioHistoryStrip::mouseDoubleClick (const juce::MouseEvent& e)
{
    // Double-click on tab or main area: toggle collapse
    if (!waveRect().contains (e.getPosition()))
    {
        const int newH = isCollapsed() ? m_savedHeight : kMinH;
        if (onHeightChanged) onHeightChanged (newH);
        return;
    }

    // Double-click in waveform: clear selection
    m_hasSelection = false;
    repaint();
}

//==============================================================================
// Hit tests
//==============================================================================

AudioHistoryStrip::SelDrag AudioHistoryStrip::selDragModeAt (juce::Point<int> pos) const noexcept
{
    if (!m_hasSelection) return SelDrag::None;
    if (!waveRect().contains (pos)) return SelDrag::None;

    const float x1 = secondsAgoToX (static_cast<float> (juce::jmax (m_selStart, m_selEnd)));
    const float x2 = secondsAgoToX (static_cast<float> (juce::jmin (m_selStart, m_selEnd)));
    const float px = static_cast<float> (pos.x);
    const int   edge = 8;

    if (px >= x1 - edge && px <= x1 + edge) return SelDrag::ResizingLeft;
    if (px >= x2 - edge && px <= x2 + edge) return SelDrag::ResizingRight;
    if (px >= x1 && px <= x2)               return SelDrag::Moving;
    return SelDrag::None;
}

int AudioHistoryStrip::grabBtnAt (juce::Point<int> pos) const noexcept
{
    const auto r   = grabRect();
    if (r.isEmpty() || !r.contains (pos)) return -1;

    struct BtnSpec { const char* label; };
    const BtnSpec btns[] = {
        { "\xe2\x97\x80 4" }, { "\xe2\x97\x80 8" }, { "\xe2\x97\x80 16" },
        { "FREE" }, { "\xe2\x86\x92 REEL" }, { "\xe2\x86\x92 TMLN" }
    };

    const int pad = 4;
    int x = r.getX() + pad;
    for (int i = 0; i < 6; ++i)
    {
        const int w = static_cast<int> (juce::GlyphArrangement::getStringWidth (Theme::Font::micro(),btns[i].label) + 12);
        const juce::Rectangle<int> br (x, r.getY() + 1, w, r.getHeight() - 2);
        if (br.contains (pos)) return i;
        x += w + pad;
        if (i == 3) x += 8;
    }
    return -1;
}

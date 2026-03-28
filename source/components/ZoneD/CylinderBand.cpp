#include "CylinderBand.h"

namespace
{
const ThemeData& tapeTheme() noexcept
{
    return ThemeManager::get().theme();
}
}

//==============================================================================
CylinderBand::CylinderBand()
{
    addAndMakeVisible (m_laneLabels);
    addAndMakeVisible (m_mixerGutter);

    m_laneLabels.onDotClicked = [this] (int i, bool armed)
    {
        if (onDotClicked) onDotClicked (i, armed);
    };

    m_laneLabels.onAutoArrowClicked = [this] (int i)
    {
        if (i >= 0 && i < m_lanes.size())
        {
            m_lanes.getReference (i).autoExpanded = !m_lanes[i].autoExpanded;
            m_laneLabels.setLanes (m_lanes);
            resized();
            repaint();
        }
        if (onAutoArrowClicked) onAutoArrowClicked (i);
    };
}

//==============================================================================
// DragAndDropTarget
//==============================================================================

bool CylinderBand::isInterestedInDragSource (const SourceDetails& details)
{
    return details.description.toString() == "audio-history-clip";
}

void CylinderBand::itemDropped (const SourceDetails& details)
{
    if (onClipDropped)
    {
        // Identify which lane was hit (if any)
        const auto localPos = details.localPosition;
        int hitLane = -1;
        for (int i = 0; i < m_lanes.size(); ++i)
        {
            const int y = laneTopY (i);
            if (localPos.y >= y && localPos.y < y + laneH())
            {
                hitLane = i;
                break;
            }
        }
        onClipDropped (hitLane);
    }
}

//==============================================================================
// Rotation model
//==============================================================================

float CylinderBand::loopLengthBeats() const noexcept
{
    return m_loopLengthBars * m_beatsPerBar;
}

float CylinderBand::loopLengthPx() const noexcept
{
    return loopLengthBeats() * m_pxPerBeat;
}

float CylinderBand::signedOffsetPxForBeat (float beatPosition) const noexcept
{
    const float loopBeats = loopLengthBeats();
    if (loopBeats <= 0.0f)
        return 0.0f;

    float deltaBeats = std::fmod (beatPosition - m_currentBeat, loopBeats);

    // Canonical timeline rule:
    // earlier beats live to the left of the playhead, later beats to the right.
    if (deltaBeats < -loopBeats * 0.5f)
        deltaBeats += loopBeats;
    else if (deltaBeats >= loopBeats * 0.5f)
        deltaBeats -= loopBeats;

    return deltaBeats * m_pxPerBeat;
}

float CylinderBand::timelineXForBeat (float beatPosition, float centerX) const noexcept
{
    return centerX + signedOffsetPxForBeat (beatPosition);
}

//==============================================================================
// Lane management
//==============================================================================

void CylinderBand::setLaneScale (float s)
{
    m_laneScale = juce::jlimit (0.5f, 3.0f, s);
    m_laneLabels.setLaneScale (m_laneScale);
    m_mixerGutter.setLaneScale (m_laneScale);
    resized();
    repaint();
}

void CylinderBand::setLanes (const juce::Array<LaneData>& lanes)
{
    m_lanes = lanes;
    m_laneLabels.setLanes (lanes);
    m_mixerGutter.setLanes (lanes);
    resized();
    repaint();
}

void CylinderBand::setHighlightLane (int slotIndex)
{
    m_highlightSlot = slotIndex;
    repaint();
}

void CylinderBand::clearHighlight()
{
    m_highlightSlot = -1;
    repaint();
}

void CylinderBand::setLaneArmed (int slotIndex, bool armed)
{
    for (int i = 0; i < m_lanes.size(); ++i)
    {
        if (m_lanes[i].slotIndex == slotIndex)
        {
            m_lanes.getReference (i).armed = armed;
            m_laneLabels.updateArmedState (i, armed);
            repaint();
            return;
        }
    }
}

void CylinderBand::setMixerGutterOpen (bool open)
{
    m_mixerOpen = open;
    m_mixerGutter.setOpen (open);
    resized();
    repaint();
}

//==============================================================================
void CylinderBand::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    // Label column
    m_laneLabels.setBounds (0, 0, kLabelW, h);

    // Mixer gutter slides from right
    const int gutterW = m_mixerOpen ? MixerGutter::kOpenWidth : 0;
    m_mixerGutter.setBounds (w - gutterW, 0, gutterW, h);

    // Tape surface occupies remaining width
    m_tapeRect = { kLabelW, 0, w - kLabelW - gutterW, h };
}

void CylinderBand::paint (juce::Graphics& g)
{
    const int tapeW   = m_tapeRect.getWidth();
    const int tapeH   = m_tapeRect.getHeight();
    const float cx    = static_cast<float> (m_tapeRect.getCentreX());

    // Clip painter to tape surface
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (m_tapeRect);
        g.setOrigin (m_tapeRect.getPosition());

        // Layer 1: tape base fill
        paintTapeBase (g, tapeH);

        // Layer 2: lane content (beat markers, seam, clips — all scrolling)
        paintBeatMarkers (g, cx - static_cast<float> (kLabelW), tapeH);
        paintSeam        (g, cx - static_cast<float> (kLabelW), tapeH);

        for (int i = 0; i < m_lanes.size(); ++i)
        {
            const int laneTop = laneTopY (i);
            paintLaneContent (g, i, laneTop,  cx - static_cast<float> (kLabelW), tapeH);
        }

        // Specular highlight — sits on top of tape content, under rim fades
        paintSpecularHighlight (g, cx - static_cast<float> (kLabelW), tapeH);

        // Rims, fades, shadows, crown lines
        paintRimsAndFades (g, tapeW, tapeH);

        // Layer 10: playhead (on top of everything including fades)
        paintPlayhead (g, cx - static_cast<float> (kLabelW), tapeH);
    }
}

void CylinderBand::mouseDown (const juce::MouseEvent& e)
{
    if (m_transportPlaying)
        return;
    if (!m_tapeRect.contains (e.getPosition()))
        return;
    m_isDragging   = true;
    m_dragStartX   = e.position.x;
    m_dragStartBeat = m_currentBeat;
}

void CylinderBand::mouseDrag (const juce::MouseEvent& e)
{
    if (!m_isDragging)
        return;
    const float delta   = (e.position.x - m_dragStartX) / m_pxPerBeat;
    float newBeat       = m_dragStartBeat + delta;
    const float loopLen = loopLengthBeats();
    newBeat = std::fmod (newBeat, loopLen);
    if (newBeat < 0.0f)
        newBeat += loopLen;
    m_currentBeat = newBeat;
    repaint();
}

void CylinderBand::mouseUp (const juce::MouseEvent&)
{
    m_isDragging = false;
}

juce::MouseCursor CylinderBand::getMouseCursor()
{
    return m_transportPlaying
               ? juce::MouseCursor::NormalCursor
               : juce::MouseCursor::LeftRightResizeCursor;
}

//==============================================================================
// Paint helpers
//==============================================================================

void CylinderBand::paintTapeBase (juce::Graphics& g, int tapeH) const
{
    const int   tapeW = m_tapeRect.getWidth();
    const float fw    = static_cast<float> (tapeW);
    const auto  tapeBase = tapeTheme().tapeBase;

    // Cylindrical diffuse lighting: cos(θ) model.
    // Crown of the cylinder (center) faces the viewer and is brightest.
    // Edges wrap away — surface normal is nearly perpendicular to line of sight.
    juce::ColourGradient grad (
        tapeBase.darker  (0.58f),   // left edge  — deep wrap-around
        0.0f, 0.0f,
        tapeBase.darker  (0.58f),   // right edge — symmetric
        fw, 0.0f,
        false);
    // Shoulders pulled in closer — steeper cos(θ) falloff
    grad.addColour (0.07, tapeBase.darker  (0.42f));
    grad.addColour (0.17, tapeBase.darker  (0.18f));
    grad.addColour (0.40, tapeBase.brighter (0.05f));
    grad.addColour (0.50, tapeBase.brighter (0.24f)); // lit crown
    grad.addColour (0.60, tapeBase.brighter (0.05f));
    grad.addColour (0.83, tapeBase.darker  (0.18f));
    grad.addColour (0.93, tapeBase.darker  (0.42f));

    g.setGradientFill (grad);
    g.fillRect (0, 0, tapeW, tapeH);
}

void CylinderBand::paintSpecularHighlight (juce::Graphics& g, float centerX, int tapeH) const
{
    // The crown of the cylinder catches a concentrated highlight — narrow,
    // warm-white, slightly shorter than the full tape height.
    const float coreY  = 5.0f;   // starts just inside the hard recess shadow line
    const float coreH  = static_cast<float> (tapeH) - coreY * 2.0f;
    if (coreH <= 0.0f)
        return;

    // Soft penumbra: ±10px wide gaussian-like falloff
    const float glowW = 10.0f;
    juce::ColourGradient penumbra (
        juce::Colour::fromRGBA (255, 248, 228, 0),
        centerX - glowW, 0.0f,
        juce::Colour::fromRGBA (255, 248, 228, 0),
        centerX + glowW, 0.0f,
        false);
    penumbra.addColour (0.40, juce::Colour::fromRGBA (255, 248, 228, 18));
    penumbra.addColour (0.50, juce::Colour::fromRGBA (255, 248, 228, 42));
    penumbra.addColour (0.60, juce::Colour::fromRGBA (255, 248, 228, 18));
    g.setGradientFill (penumbra);
    g.fillRect (centerX - glowW, coreY, glowW * 2.0f, coreH);

    // Hard specular core: 1.5px bright line
    g.setColour (juce::Colour::fromRGBA (255, 252, 238, 118));
    g.fillRect (centerX - 0.75f, coreY, 1.5f, coreH);
}

void CylinderBand::paintLaneContent (juce::Graphics& g,
                                      int laneIndex,
                                      int laneTopYVal,
                                      float centerX,
                                      int /*tapeH*/) const
{
    if (laneIndex < 0 || laneIndex >= m_lanes.size())
        return;

    const auto& lane = m_lanes.getReference (laneIndex);

    // Highlight tint (Zone B selection — amber at 5%)
    if (m_highlightSlot == lane.slotIndex)
    {
        g.setColour (Theme::Zone::b.withAlpha (0.05f));
        g.fillRect (0, laneTopYVal, m_tapeRect.getWidth(), laneH());
    }

    // Armed tint (error at 8%)
    if (lane.armed)
    {
        g.setColour (Theme::Colour::error.withAlpha (0.08f));
        g.fillRect (0, laneTopYVal, m_tapeRect.getWidth(), laneH());
    }

    // Lane separator
    const auto tapeBeatTick = tapeTheme().tapeBeatTick;
    g.setColour (tapeBeatTick.withAlpha (0.18f));
    g.drawHorizontalLine (laneTopYVal + laneH() - 1, 0.0f, static_cast<float> (m_tapeRect.getWidth()));

    // Automation sub-lane
    if (lane.autoExpanded)
    {
        const int autoTop = laneTopYVal + laneH();
        // Background: tapeBase + black overlay at 10%
        const auto tapeBase = tapeTheme().tapeBase;
        g.setColour (tapeBase);
        g.fillRect (0, autoTop, m_tapeRect.getWidth(), autoH());
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.fillRect (0, autoTop, m_tapeRect.getWidth(), autoH());
        // Placeholder automation line
        g.setColour (Theme::Zone::b.withAlpha (0.60f));
        g.drawHorizontalLine (autoTop + autoH() / 2, 0.0f, static_cast<float> (m_tapeRect.getWidth()));
        // TODO: real automation curve rendering
    }

    // Clips
    paintClips (g, laneIndex, laneTopYVal, laneH(), centerX);
}

void CylinderBand::paintBeatMarkers (juce::Graphics& g, float centerX, int tapeH) const
{
    const float loopPx  = loopLengthPx();
    const int   numBeats = static_cast<int> (loopLengthBeats());
    const auto  tapeBeatTick = tapeTheme().tapeBeatTick;

    for (int i = 0; i < numBeats; ++i)
    {
        const float x = timelineXForBeat (static_cast<float> (i), centerX);
        if (x <= 4.0f || x >= static_cast<float> (m_tapeRect.getWidth() - 4))
            continue;

        const bool isBar  = (i % static_cast<int> (m_beatsPerBar) == 0);
        const int  tickH  = isBar ? 7 : 3;
        const float alpha = isBar ? 0.46f : 0.30f;

        g.setColour (tapeBeatTick.withAlpha (alpha));
        g.fillRect (x - 0.25f, 0.0f, 0.5f, static_cast<float> (tickH));

        if (isBar)
        {
            // Bar number
            const int barNum = (i / static_cast<int> (m_beatsPerBar)) + 1;
            g.setFont (Theme::Font::micro());
            g.setColour (tapeBeatTick.withAlpha (0.44f));
            g.drawText (juce::String (barNum),
                        juce::Rectangle<float> (x + 2.0f, 0.0f, 16.0f, 9.0f).toNearestInt(),
                        juce::Justification::centredLeft, false);
        }
    }

    juce::ignoreUnused (tapeH, loopPx);
}

void CylinderBand::paintSeam (juce::Graphics& g, float centerX, int tapeH) const
{
    const float x      = timelineXForBeat (0.0f, centerX);
    const float halfW  = static_cast<float> (m_tapeRect.getWidth()) * 0.5f;
    const auto  tapeSeam = tapeTheme().tapeSeam;

    if (x > 4.0f && x < static_cast<float> (m_tapeRect.getWidth() - 4))
    {
        // Seam fades as it wraps toward the edges — on a cylinder it would
        // eventually disappear around the back.
        const float dist      = std::abs (x - centerX);
        const float perspFade = juce::jmax (0.0f, 1.0f - dist / (halfW * 0.85f));

        g.setColour (tapeSeam.withAlpha (perspFade));
        g.fillRect (x - 0.5f, 0.0f, 1.0f, static_cast<float> (tapeH));
    }
}

void CylinderBand::paintClips (juce::Graphics& g,
                                int laneIndex,
                                int laneTopYVal,
                                int laneH,
                                float centerX) const
{
    if (laneIndex < 0 || laneIndex >= m_lanes.size())
        return;

    const auto& lane    = m_lanes.getReference (laneIndex);
    const float loopPx  = loopLengthPx();
    const float clipTop = static_cast<float> (laneTopYVal + 3);
    const float clipH   = static_cast<float> (laneH - 6);

    for (const auto& clip : lane.clips)
    {
        const float clipLeft  = timelineXForBeat (clip.startBeat, centerX);
        const float clipW     = clip.lengthBeats * m_pxPerBeat;
        const float clipRight = clipLeft + clipW;

        auto drawClipAt = [&] (float left, float right)
        {
            // Clamp to tape surface
            const float tW = static_cast<float> (m_tapeRect.getWidth());
            const float l  = juce::jmax (left, 4.0f);
            const float r  = juce::jmin (right, tW - 4.0f);
            if (r > l)
                ClipCard::paint (g, { l, clipTop, r - l, clipH }, clip,
                                 loopLengthBeats(), m_pxPerBeat);
        };

        if (clipW < loopPx)
        {
            // Draw the canonical visible segment plus wrapped neighbours so a
            // clip remains continuous as it crosses the loop seam.
            drawClipAt (clipLeft, clipRight);
            drawClipAt (clipLeft - loopPx, clipRight - loopPx);
            drawClipAt (clipLeft + loopPx, clipRight + loopPx);
        }
        else
        {
            // Full-loop clip — just draw it
            drawClipAt (clipLeft, clipLeft + clipW);
        }
    }
}

void CylinderBand::paintRimsAndFades (juce::Graphics& g, int tapeW, int tapeH) const
{
    const auto housingEdge = tapeTheme().housingEdge;

    // === RIM BEVEL — left (inner highlight suggests housing depth) ===
    g.setColour (juce::Colour (0xFF040302));   // outer very dark
    g.fillRect (0, 0, 2, tapeH);
    g.setColour (juce::Colour (0xFF2E2014));   // mid bevel catch-light
    g.fillRect (2, 0, 1, tapeH);
    g.setColour (juce::Colour (0xFF0C0806));   // inner dark
    g.fillRect (3, 0, 2, tapeH);

    // === RIM BEVEL — right (mirror) ===
    g.setColour (juce::Colour (0xFF0C0806));   // inner dark
    g.fillRect (tapeW - 5, 0, 2, tapeH);
    g.setColour (juce::Colour (0xFF2E2014));   // mid bevel catch-light
    g.fillRect (tapeW - 3, 0, 1, tapeH);
    g.setColour (juce::Colour (0xFF040302));   // outer very dark
    g.fillRect (tapeW - 2, 0, 2, tapeH);

    // === EDGE FADES — intensified to push the cylinder illusion harder ===
    {
        juce::ColourGradient gradL (juce::Colour::fromRGBA (12, 8, 3, 195),
                                    static_cast<float> (kRimW), 0.0f,
                                    juce::Colours::transparentBlack,
                                    static_cast<float> (kRimW + kFadeW), 0.0f,
                                    false);
        gradL.addColour (0.14, juce::Colour::fromRGBA (12, 8, 3, 128));
        gradL.addColour (0.36, juce::Colour::fromRGBA (12, 8, 3,  52));
        gradL.addColour (0.65, juce::Colour::fromRGBA (12, 8, 3,  14));
        g.setGradientFill (gradL);
        g.fillRect (kRimW, 0, kFadeW, tapeH);
    }
    {
        const int x1 = tapeW - kRimW - kFadeW;
        juce::ColourGradient gradR (juce::Colours::transparentBlack,
                                    static_cast<float> (x1), 0.0f,
                                    juce::Colour::fromRGBA (12, 8, 3, 195),
                                    static_cast<float> (tapeW - kRimW), 0.0f,
                                    false);
        gradR.addColour (0.35, juce::Colour::fromRGBA (12, 8, 3,  14));
        gradR.addColour (0.64, juce::Colour::fromRGBA (12, 8, 3,  52));
        gradR.addColour (0.86, juce::Colour::fromRGBA (12, 8, 3, 128));
        g.setGradientFill (gradR);
        g.fillRect (x1, 0, kFadeW, tapeH);
    }

    // === INSET RECESS SHADOW — top ===
    // Hard housing wall edge + steep cast shadow: the cylinder sits IN a slot,
    // not on a curved surface.  No gradual rounding — hard then quick falloff.
    {
        const int recessH = juce::jmin (kShadowH, tapeH / 3);
        // Hard wall shadow: full-opaque line at y=0, steep drop over 3px
        g.setColour (housingEdge);
        g.fillRect (0, 0, tapeW, 1);
        juce::ColourGradient cast;
        cast.isRadial = false;
        cast.point1   = { 0.0f, 1.0f };
        cast.point2   = { 0.0f, static_cast<float> (recessH) };
        cast.addColour (0.00, housingEdge.withAlpha (0.92f));
        cast.addColour (0.14, housingEdge.withAlpha (0.72f));
        cast.addColour (0.30, housingEdge.withAlpha (0.48f));
        cast.addColour (0.52, housingEdge.withAlpha (0.22f));
        cast.addColour (0.74, housingEdge.withAlpha (0.07f));
        cast.addColour (1.00, housingEdge.withAlpha (0.00f));
        g.setGradientFill (cast);
        g.fillRect (0, 1, tapeW, recessH - 1);
    }

    // === INSET RECESS SHADOW — bottom (mirror) ===
    {
        const int recessH = juce::jmin (kShadowH, tapeH / 3);
        g.setColour (housingEdge);
        g.fillRect (0, tapeH - 1, tapeW, 1);
        juce::ColourGradient cast;
        cast.isRadial = false;
        cast.point1   = { 0.0f, static_cast<float> (tapeH - 1) };
        cast.point2   = { 0.0f, static_cast<float> (tapeH - recessH) };
        cast.addColour (0.00, housingEdge.withAlpha (0.92f));
        cast.addColour (0.14, housingEdge.withAlpha (0.72f));
        cast.addColour (0.30, housingEdge.withAlpha (0.48f));
        cast.addColour (0.52, housingEdge.withAlpha (0.22f));
        cast.addColour (0.74, housingEdge.withAlpha (0.07f));
        cast.addColour (1.00, housingEdge.withAlpha (0.00f));
        g.setGradientFill (cast);
        g.fillRect (0, tapeH - recessH, tapeW, recessH - 1);
    }

    // === OUTER RIM LINES ===
    g.setColour (housingEdge.withAlpha (0.99f));
    g.drawHorizontalLine (0,        0.0f, static_cast<float> (tapeW));
    g.drawHorizontalLine (tapeH - 1, 0.0f, static_cast<float> (tapeW));
}

void CylinderBand::paintPlayhead (juce::Graphics& g, float centerX, int tapeH) const
{
    // 1.5px vertical line
    const auto playheadColor = tapeTheme().playheadColor;
    g.setColour (playheadColor);
    g.fillRect (centerX - 0.75f, 0.0f, 1.5f, static_cast<float> (tapeH));

    // Downward triangle at top (5px base, 6px height)
    juce::Path tri;
    tri.addTriangle (centerX - 2.5f, 0.0f,
                     centerX + 2.5f, 0.0f,
                     centerX,        6.0f);
    g.fillPath (tri);
}

//==============================================================================
int CylinderBand::totalLaneHeight() const noexcept
{
    int h = 0;
    for (const auto& lane : m_lanes)
        h += laneH() + (lane.autoExpanded ? autoH() : 0);
    return h;
}

int CylinderBand::laneTopY (int laneIndex) const noexcept
{
    int y = 0;
    for (int i = 0; i < laneIndex; ++i)
        y += laneH() + (m_lanes[i].autoExpanded ? autoH() : 0);
    return y;
}

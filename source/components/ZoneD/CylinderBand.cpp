#include "CylinderBand.h"
#include "TimelineEditMath.h"
#include <limits>

namespace
{
const ThemeData& tapeTheme() noexcept
{
    return ThemeManager::get().theme();
}

float wrapBeat (float beat, float loopBeats) noexcept
{
    return TimelineEditMath::wrapBeat (beat, loopBeats);
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
    const auto desc = details.description.toString();
    return desc == "audio-history-clip" || desc == "CapturedAudioClip";
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

    float deltaBeats = std::fmod (m_currentBeat - beatPosition, loopBeats);

    // Left-to-right transport motion:
    // as current beat increases, fixed tape content moves right.
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

void CylinderBand::clearClipSelection()
{
    for (auto& lane : m_lanes)
        for (auto& clip : lane.clips)
            clip.selected = false;
}

CylinderBand::ClipHit CylinderBand::hitTestEditableClip (juce::Point<float> pos) const
{
    ClipHit best;
    float bestScore = std::numeric_limits<float>::max();

    if (! m_tapeRect.contains (juce::Point<int> (juce::roundToInt (pos.x), juce::roundToInt (pos.y))))
        return best;

    const float localX = pos.x - static_cast<float> (m_tapeRect.getX());
    const float loopPx = loopLengthPx();
    if (loopPx <= 0.0f)
        return best;

    static constexpr float handleTol = 6.0f;
    const float centerX = static_cast<float> (m_tapeRect.getWidth()) * 0.5f;

    for (int laneIndex = 0; laneIndex < m_lanes.size(); ++laneIndex)
    {
        const int top = laneTopY (laneIndex);
        if (pos.y < static_cast<float> (top) || pos.y > static_cast<float> (top + laneH()))
            continue;

        const auto& lane = m_lanes.getReference (laneIndex);
        for (int clipIndex = 0; clipIndex < lane.clips.size(); ++clipIndex)
        {
            const auto& clip = lane.clips.getReference (clipIndex);
            if (clip.type == ClipType::scaffold)
                continue;

            const float clipStartX = timelineXForBeat (clip.startBeat, centerX);
            const float clipW = clip.lengthBeats * m_pxPerBeat;
            const float oneBeatLaterX = timelineXForBeat (clip.startBeat + 1.0f, centerX);
            const float beatAxisDir = (oneBeatLaterX >= clipStartX) ? 1.0f : -1.0f;
            const float clipEndX = clipStartX + beatAxisDir * clipW;
            const float clipLeft = juce::jmin (clipStartX, clipEndX);
            const float clipRight = juce::jmax (clipStartX, clipEndX);

            for (int wrap = -1; wrap <= 1; ++wrap)
            {
                const float l = clipLeft + static_cast<float> (wrap) * loopPx;
                const float r = clipRight + static_cast<float> (wrap) * loopPx;

                if (localX < (l - handleTol) || localX > (r + handleTol))
                    continue;

                const float startEdge = (beatAxisDir >= 0.0f ? l : r);
                const float endEdge = (beatAxisDir >= 0.0f ? r : l);
                const float startDist = std::abs (localX - startEdge);
                const float endDist = std::abs (localX - endEdge);

                ClipHit candidate;
                candidate.laneIndex = laneIndex;
                candidate.clipIndex = clipIndex;
                candidate.beatAxisDir = beatAxisDir;

                float score = 1000.0f + std::abs (localX - 0.5f * (l + r));
                if (startDist <= handleTol || endDist <= handleTol)
                {
                    if (startDist <= endDist)
                    {
                        candidate.mode = DragMode::clipTrimStart;
                        score = startDist;
                    }
                    else
                    {
                        candidate.mode = DragMode::clipTrimEnd;
                        score = endDist;
                    }
                }
                else if (localX >= l && localX <= r)
                {
                    candidate.mode = DragMode::clipMove;
                }
                else
                {
                    continue;
                }

                if (score < bestScore)
                {
                    bestScore = score;
                    best = candidate;
                }
            }
        }
    }

    return best;
}

void CylinderBand::mouseDown (const juce::MouseEvent& e)
{
    if (m_transportPlaying || ! m_tapeRect.contains (e.getPosition()))
        return;

    const auto hit = hitTestEditableClip (e.position);
    if (hit.mode != DragMode::none)
    {
        clearClipSelection();

        m_dragMode = hit.mode;
        m_dragStartX = e.position.x;
        m_dragClipLane = hit.laneIndex;
        m_dragClipIndex = hit.clipIndex;
        m_dragClipAxisDir = (std::abs (hit.beatAxisDir) > 0.01f) ? hit.beatAxisDir : -1.0f;
        m_dragClipChanged = false;

        if (m_dragClipLane >= 0
            && m_dragClipLane < m_lanes.size()
            && m_dragClipIndex >= 0
            && m_dragClipIndex < m_lanes.getReference (m_dragClipLane).clips.size())
        {
            auto& clip = m_lanes.getReference (m_dragClipLane).clips.getReference (m_dragClipIndex);
            clip.selected = true;
            m_dragClipStartBeat = clip.startBeat;
            m_dragClipLengthBeats = clip.lengthBeats;
        }

        repaint();
        updateMouseCursor();
        return;
    }

    m_dragMode = DragMode::scrub;
    m_dragStartX = e.position.x;
    m_dragStartBeat = m_currentBeat;
    updateMouseCursor();
}

void CylinderBand::mouseDrag (const juce::MouseEvent& e)
{
    if (m_dragMode == DragMode::none)
        return;

    const float loopLen = juce::jmax (1.0f, loopLengthBeats());
    const float snapStep = (m_pxPerBeat >= 48.0f) ? 0.125f : 0.25f;
    const bool snapEnabled = ! e.mods.isShiftDown();

    if (m_dragMode == DragMode::scrub)
    {
        const float delta = (e.position.x - m_dragStartX) / m_pxPerBeat;
        m_currentBeat = wrapBeat (m_dragStartBeat + delta, loopLen);
        if (onScrubBeatChanged)
            onScrubBeatChanged (m_currentBeat);
        repaint();
        return;
    }

    if (m_dragClipLane < 0 || m_dragClipLane >= m_lanes.size())
        return;

    auto& lane = m_lanes.getReference (m_dragClipLane);
    if (m_dragClipIndex < 0 || m_dragClipIndex >= lane.clips.size())
        return;

    auto& clip = lane.clips.getReference (m_dragClipIndex);
    if (clip.type == ClipType::scaffold)
        return;

    const float axis = (std::abs (m_dragClipAxisDir) > 0.01f) ? m_dragClipAxisDir : -1.0f;
    const float deltaBeats = (e.position.x - m_dragStartX) / (axis * m_pxPerBeat);
    TimelineEditMath::EditMode editMode = TimelineEditMath::EditMode::move;
    if (m_dragMode == DragMode::clipTrimStart)
        editMode = TimelineEditMath::EditMode::trimStart;
    else if (m_dragMode == DragMode::clipTrimEnd)
        editMode = TimelineEditMath::EditMode::trimEnd;

    TimelineEditMath::ClipSegment originalSegment;
    originalSegment.startBeat = m_dragClipStartBeat;
    originalSegment.lengthBeats = m_dragClipLengthBeats;

    TimelineEditMath::EditRequest request;
    request.mode = editMode;
    request.deltaBeats = deltaBeats;
    request.loopBeats = loopLen;
    request.minLengthBeats = 0.25f;
    request.snapStep = snapStep;
    request.snapEnabled = snapEnabled;

    const auto editedSegment = TimelineEditMath::applyEdit (originalSegment, request);
    const float newStart = editedSegment.startBeat;
    const float newLength = editedSegment.lengthBeats;

    if (std::abs (clip.startBeat - newStart) > 0.0001f
        || std::abs (clip.lengthBeats - newLength) > 0.0001f)
    {
        clip.startBeat = newStart;
        clip.lengthBeats = newLength;
        m_dragClipChanged = true;
    }

    repaint();
}

void CylinderBand::mouseUp (const juce::MouseEvent&)
{
    if ((m_dragMode == DragMode::clipMove
         || m_dragMode == DragMode::clipTrimStart
         || m_dragMode == DragMode::clipTrimEnd)
        && m_dragClipChanged
        && m_dragClipLane >= 0
        && m_dragClipLane < m_lanes.size()
        && m_dragClipIndex >= 0
        && m_dragClipIndex < m_lanes.getReference (m_dragClipLane).clips.size())
    {
        const auto& lane = m_lanes.getReference (m_dragClipLane);
        const auto& clip = lane.clips.getReference (m_dragClipIndex);
        if (onClipEditCommitted)
            onClipEditCommitted (lane.slotIndex, clip.clipId, clip.startBeat, clip.lengthBeats);
    }

    m_dragMode = DragMode::none;
    m_dragClipLane = -1;
    m_dragClipIndex = -1;
    m_dragClipChanged = false;
    updateMouseCursor();
}

void CylinderBand::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (m_transportPlaying || ! m_tapeRect.contains (e.getPosition()))
        return;

    const auto hit = hitTestEditableClip (e.position);
    if (hit.mode == DragMode::none
        || hit.laneIndex < 0
        || hit.laneIndex >= m_lanes.size())
        return;

    const auto& lane = m_lanes.getReference (hit.laneIndex);
    if (hit.clipIndex < 0 || hit.clipIndex >= lane.clips.size())
        return;

    const auto& clip = lane.clips.getReference (hit.clipIndex);
    if (clip.type == ClipType::scaffold)
        return;

    if (onClipSplitRequested)
        onClipSplitRequested (lane.slotIndex, clip.clipId, m_currentBeat);
}

juce::MouseCursor CylinderBand::getMouseCursor()
{
    return m_transportPlaying
               ? juce::MouseCursor::NormalCursor
               : [&]
               {
                   if (m_dragMode == DragMode::clipMove)
                       return juce::MouseCursor::DraggingHandCursor;
                   if (m_dragMode == DragMode::clipTrimStart || m_dragMode == DragMode::clipTrimEnd)
                       return juce::MouseCursor::LeftRightResizeCursor;

                   const auto mousePos = getMouseXYRelative().toFloat();
                   const auto hit = hitTestEditableClip (mousePos);
                   if (hit.mode == DragMode::clipTrimStart || hit.mode == DragMode::clipTrimEnd)
                       return juce::MouseCursor::LeftRightResizeCursor;
                   if (hit.mode == DragMode::clipMove)
                       return juce::MouseCursor::PointingHandCursor;
                   return juce::MouseCursor::LeftRightResizeCursor;
               }();
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

    for (int clipIndex = 0; clipIndex < lane.clips.size(); ++clipIndex)
    {
        const auto& clip = lane.clips.getReference (clipIndex);
        const float clipStartX = timelineXForBeat (clip.startBeat, centerX);
        const float clipW = clip.lengthBeats * m_pxPerBeat;
        const float oneBeatLaterX = timelineXForBeat (clip.startBeat + 1.0f, centerX);
        const float beatAxisDir = (oneBeatLaterX >= clipStartX) ? 1.0f : -1.0f;
        const float clipEndX = clipStartX + beatAxisDir * clipW;
        const float clipLeft = juce::jmin (clipStartX, clipEndX);
        const float clipRight = juce::jmax (clipStartX, clipEndX);
        const float startEdgeBase = (beatAxisDir >= 0.0f ? clipLeft : clipRight);
        const float endEdgeBase = (beatAxisDir >= 0.0f ? clipRight : clipLeft);

        const bool isDraggingThisClip = (m_dragMode == DragMode::clipMove
                                         || m_dragMode == DragMode::clipTrimStart
                                         || m_dragMode == DragMode::clipTrimEnd)
                                        && laneIndex == m_dragClipLane
                                        && clipIndex == m_dragClipIndex;

        auto drawGhostAt = [&] (float left, float right)
        {
            const float tW = static_cast<float> (m_tapeRect.getWidth());
            const float l = juce::jmax (left, 4.0f);
            const float r = juce::jmin (right, tW - 4.0f);
            if (r <= l)
                return;

            g.setColour (Theme::Colour::inkLight.withAlpha (0.35f));
            g.drawRoundedRectangle ({ l, clipTop, r - l, clipH }, Theme::Radius::xs, 1.0f);
        };

        auto drawClipAt = [&] (float left, float right, float startEdge, float endEdge)
        {
            // Clamp to tape surface
            const float tW = static_cast<float> (m_tapeRect.getWidth());
            const float l  = juce::jmax (left, 4.0f);
            const float r  = juce::jmin (right, tW - 4.0f);
            if (r > l)
            {
                ClipCard::paint (g, { l, clipTop, r - l, clipH }, clip,
                                 loopLengthBeats(), m_pxPerBeat);

                if (clip.selected && clip.type != ClipType::scaffold)
                {
                    const auto handleColour = Theme::Zone::d.withAlpha (0.95f);
                    g.setColour (handleColour);

                    if (startEdge >= l && startEdge <= r)
                        g.fillRoundedRectangle ({ startEdge - 1.0f, clipTop + 1.0f, 2.0f, juce::jmax (4.0f, clipH - 2.0f) },
                                                1.0f);
                    if (endEdge >= l && endEdge <= r)
                        g.fillRoundedRectangle ({ endEdge - 1.0f, clipTop + 1.0f, 2.0f, juce::jmax (4.0f, clipH - 2.0f) },
                                                1.0f);
                }
            }
        };

        if (isDraggingThisClip)
        {
            const float ghostStartX = timelineXForBeat (m_dragClipStartBeat, centerX);
            const float ghostW = m_dragClipLengthBeats * m_pxPerBeat;
            const float ghostEndX = ghostStartX + beatAxisDir * ghostW;
            const float ghostLeft = juce::jmin (ghostStartX, ghostEndX);
            const float ghostRight = juce::jmax (ghostStartX, ghostEndX);
            drawGhostAt (ghostLeft, ghostRight);
            drawGhostAt (ghostLeft - loopPx, ghostRight - loopPx);
            drawGhostAt (ghostLeft + loopPx, ghostRight + loopPx);
        }

        if (clipW < loopPx)
        {
            // Draw the canonical visible segment plus wrapped neighbours so a
            // clip remains continuous as it crosses the loop seam.
            drawClipAt (clipLeft, clipRight, startEdgeBase, endEdgeBase);
            drawClipAt (clipLeft - loopPx, clipRight - loopPx, startEdgeBase - loopPx, endEdgeBase - loopPx);
            drawClipAt (clipLeft + loopPx, clipRight + loopPx, startEdgeBase + loopPx, endEdgeBase + loopPx);
        }
        else
        {
            // Full-loop clip — just draw it
            drawClipAt (clipLeft, clipLeft + clipW, startEdgeBase, endEdgeBase);
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

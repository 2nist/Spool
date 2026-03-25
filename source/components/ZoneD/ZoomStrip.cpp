#include "ZoomStrip.h"

//==============================================================================
const juce::Colour ZoomStrip::zoomStripBg     { 0xFFa89870 };
const juce::Colour ZoomStrip::zoomStripBorder { 0xFF7a6a50 };
const juce::Colour ZoomStrip::thumbColor      { 0xFFc8b070 };
const juce::Colour ZoomStrip::textColor       { 0xFF5a4a2a };
const juce::Colour ZoomStrip::activeBtn       { 0xFFc8b070 };
const juce::Colour ZoomStrip::activeBtnBorder { 0xFF988040 };
const juce::Colour ZoomStrip::activeBtnText   { 0xFF3a2a08 };

//==============================================================================
ZoomStrip::ZoomStrip()
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void ZoomStrip::setHeightMode (HeightMode mode)
{
    m_heightMode = mode;
    repaint();
}

void ZoomStrip::setLaneHeightScale (float scale) noexcept
{
    m_laneHeightScale = juce::jlimit (0.4f, 3.0f, scale);
    repaint();
}

//==============================================================================
void ZoomStrip::paint (juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    g.setColour (zoomStripBg);
    g.fillAll();

    // Top border
    g.setColour (zoomStripBorder.withAlpha (0.5f));
    g.drawHorizontalLine (0, 0.0f, static_cast<float> (w));

    // "ZOOM" label
    g.setFont (Theme::Font::micro());
    g.setColour (textColor);
    g.drawText ("ZOOM", 6, 0, 28, h, juce::Justification::centredLeft, false);

    // Zoom slider area
    const juce::Rectangle<int> sliderArea { 38, 3, 100, h - 6 };
    drawZoomSlider (g, sliderArea);

    // Value text
    g.setFont (Theme::Font::micro());
    g.setColour (textColor);
    g.drawText (juce::String (juce::roundToInt (m_pxPerBeat)) + " px/b",
                sliderArea.getRight() + 4, 0, 50, h,
                juce::Justification::centredLeft, false);

    // Right-side controls (right to left):
    // EXPAND button
    const int expandBtnW = 68;
    const int btnH       = h - 4;
    m_expandBtnRect = { w - expandBtnW - 4, 2, expandBtnW, btnH };
    drawExpandButton (g);

    // MIN / NORM / MAX buttons
    const int modeBtnW   = 30;
    const int modeBtnGap = 2;
    for (int i = 0; i < 3; ++i)
    {
        m_modeBtnRects[i] = {
            m_expandBtnRect.getX() - (3 - i) * (modeBtnW + modeBtnGap) - 4,
            2,
            modeBtnW,
            btnH
        };
    }
    drawModeButtons (g);

    // Vertical height handle (replaces H+/H−) — left of MIN/NORM/MAX
    const int handleX = m_modeBtnRects[0].getX() - 20;
    m_heightHandleRect = { handleX, 0, 16, h };
    drawHeightHandle (g);
}

void ZoomStrip::drawZoomSlider (juce::Graphics& g,
                                 const juce::Rectangle<int>& area) const
{
    const int trackH  = 2;
    const int trackY  = area.getCentreY() - trackH / 2;
    const int trackW  = 80;

    m_sliderTrackRect = { area.getX(), trackY, trackW, trackH };

    g.setColour (zoomStripBorder);
    g.fillRect (m_sliderTrackRect);

    const float t    = (m_pxPerBeat - kZoomMin) / (kZoomMax - kZoomMin);
    const int   tx   = area.getX() + juce::roundToInt (t * (trackW - 8));
    const int   ty   = area.getCentreY() - 4;
    const int   td   = 8;

    g.setColour (thumbColor);
    g.fillEllipse (static_cast<float> (tx),
                   static_cast<float> (ty),
                   static_cast<float> (td),
                   static_cast<float> (td));
}

void ZoomStrip::drawModeButtons (juce::Graphics& g) const
{
    const juce::String labels[3] = { "MIN", "NORM", "MAX" };
    const HeightMode modes[3]    = { HeightMode::min, HeightMode::norm, HeightMode::max };

    for (int i = 0; i < 3; ++i)
    {
        const bool active = (m_heightMode == modes[i]);
        const auto& r     = m_modeBtnRects[i];

        if (active)
        {
            g.setColour (activeBtn);
            g.fillRoundedRectangle (r.toFloat(), Theme::Radius::xs);
            g.setColour (activeBtnBorder);
            g.drawRoundedRectangle (r.toFloat(), Theme::Radius::xs, 0.5f);
            g.setColour (activeBtnText);
        }
        else
        {
            g.setColour (zoomStripBorder.withAlpha (0.5f));
            g.drawRoundedRectangle (r.toFloat(), Theme::Radius::xs, 0.5f);
            g.setColour (textColor);
        }

        g.setFont (Theme::Font::micro());
        g.drawText (labels[i], r, juce::Justification::centred, false);
    }
}

void ZoomStrip::drawExpandButton (juce::Graphics& g) const
{
    const bool expanded = (m_heightMode == HeightMode::expand);
    g.setColour (zoomStripBorder.withAlpha (0.5f));
    g.drawRoundedRectangle (m_expandBtnRect.toFloat(), Theme::Radius::xs, 0.5f);
    g.setFont (Theme::Font::micro());
    g.setColour (textColor);
    g.drawText (expanded ? "v COLLAPSE" : "^ EXPAND",
                m_expandBtnRect, juce::Justification::centred, false);
}

void ZoomStrip::drawHeightHandle (juce::Graphics& g) const
{
    const auto& r  = m_heightHandleRect;
    const int   cx = r.getCentreX();
    const int   cy = r.getCentreY();

    // Scale value label above the handle
    g.setFont (Theme::Font::micro());
    g.setColour (m_heightHandleHovered ? Theme::Zone::d : textColor.withAlpha (0.6f));
    g.drawText (juce::String (m_laneHeightScale, 1) + "x",
                r.getX() - 4, r.getY(), 24, 10, juce::Justification::centred, false);

    // Handle tick mark: 2px wide, 14px tall with 3px end caps
    const juce::Colour handleCol = m_heightHandleHovered ? Theme::Zone::d
                                                         : zoomStripBorder;
    // Vertical stem
    g.setColour (handleCol);
    g.fillRect (cx - 1, cy - 7, 2, 14);

    // Top cap
    g.fillRect (cx - 2, cy - 7, 5, 1);

    // Bottom cap
    g.fillRect (cx - 2, cy + 6, 5, 1);
}

//==============================================================================
void ZoomStrip::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // Height handle drag
    if (m_heightHandleRect.expanded (4, 0).contains (pos))
    {
        m_draggingHeight      = true;
        m_heightDragStartY    = static_cast<float> (pos.getY());
        m_heightDragStartScale = m_laneHeightScale;
        return;
    }

    // Mode buttons
    const HeightMode modes[3] = { HeightMode::min, HeightMode::norm, HeightMode::max };
    for (int i = 0; i < 3; ++i)
    {
        if (m_modeBtnRects[i].contains (pos))
        {
            m_heightMode = modes[i];
            if (onHeightModeChanged)
                onHeightModeChanged (m_heightMode);
            repaint();
            return;
        }
    }

    // Expand / Collapse button
    if (m_expandBtnRect.contains (pos))
    {
        if (m_heightMode == HeightMode::expand)
            m_heightMode = HeightMode::norm;
        else
        {
            m_preExpandMode = m_heightMode;
            m_heightMode    = HeightMode::expand;
        }
        if (onHeightModeChanged)
            onHeightModeChanged (m_heightMode);
        repaint();
        return;
    }

    // Zoom slider thumb hit
    if (m_sliderTrackRect.expanded (8, 8).contains (pos))
    {
        m_draggingZoom  = true;
        m_dragStartX    = static_cast<float> (pos.getX());
        m_dragStartZoom = m_pxPerBeat;
    }
}

void ZoomStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (m_draggingHeight)
    {
        // UP = larger scale (drag upward = negative delta = increase)
        const float delta = m_heightDragStartY - static_cast<float> (e.getPosition().getY());
        m_laneHeightScale = juce::jlimit (0.4f, 3.0f, m_heightDragStartScale + delta * 0.01f);
        if (onLaneHeightChanged)
            onLaneHeightChanged (m_laneHeightScale);
        repaint();
        return;
    }

    if (!m_draggingZoom)
        return;

    const float delta = static_cast<float> (e.getPosition().getX()) - m_dragStartX;
    const float range = kZoomMax - kZoomMin;
    const float trackW = 80.0f - 8.0f;

    m_pxPerBeat = juce::jlimit (kZoomMin,
                                kZoomMax,
                                m_dragStartZoom + delta / trackW * range);

    if (onZoomChanged)
        onZoomChanged (m_pxPerBeat);

    repaint();
}

void ZoomStrip::mouseUp (const juce::MouseEvent&)
{
    if (m_draggingHeight)
    {
        // Snap to nearest 0.25
        m_laneHeightScale = juce::roundToInt (m_laneHeightScale * 4.0f) / 4.0f;
        m_laneHeightScale = juce::jlimit (0.4f, 3.0f, m_laneHeightScale);
        if (onLaneHeightChanged)
            onLaneHeightChanged (m_laneHeightScale);
        repaint();
    }
    m_draggingHeight = false;
    m_draggingZoom   = false;
}

void ZoomStrip::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (m_heightHandleRect.expanded (4, 0).contains (e.getPosition()))
    {
        m_laneHeightScale = 1.0f;
        if (onLaneHeightChanged)
            onLaneHeightChanged (m_laneHeightScale);
        repaint();
    }
}

void ZoomStrip::mouseMove (const juce::MouseEvent& e)
{
    const bool hovered = m_heightHandleRect.expanded (4, 0).contains (e.getPosition());
    if (hovered != m_heightHandleHovered)
    {
        m_heightHandleHovered = hovered;
        setMouseCursor (hovered ? juce::MouseCursor::UpDownResizeCursor
                                : juce::MouseCursor::NormalCursor);
        repaint();
    }
}

void ZoomStrip::mouseExit (const juce::MouseEvent&)
{
    if (m_heightHandleHovered)
    {
        m_heightHandleHovered = false;
        setMouseCursor (juce::MouseCursor::NormalCursor);
        repaint();
    }
}

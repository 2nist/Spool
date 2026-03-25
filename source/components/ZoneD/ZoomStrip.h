#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    ZoomStrip — 20px fixed-height zoom and height-mode control strip.

    Background: warm mid-tan (#a89870), outside espresso palette.

    Left side:  ZOOM label + slider (10–80 px/beat)
    Right side: MIN/NORM/MAX buttons + EXPAND/COLLAPSE button
*/
class ZoomStrip : public juce::Component
{
public:
    static constexpr int kHeight = 20;

    // Zoom range
    static constexpr float kZoomMin     = 10.0f;
    static constexpr float kZoomMax     = 80.0f;
    static constexpr float kZoomDefault = 28.0f;

    // Local colours
    static const juce::Colour zoomStripBg;
    static const juce::Colour zoomStripBorder;
    static const juce::Colour thumbColor;
    static const juce::Colour textColor;
    static const juce::Colour activeBtn;
    static const juce::Colour activeBtnBorder;
    static const juce::Colour activeBtnText;

    enum class HeightMode { min, norm, max, expand };

    ZoomStrip();
    ~ZoomStrip() override = default;

    float currentZoom() const noexcept { return m_pxPerBeat; }
    HeightMode currentHeightMode() const noexcept { return m_heightMode; }
    bool isExpanded() const noexcept { return m_heightMode == HeightMode::expand; }

    // Sync external state into UI
    void setHeightMode (HeightMode mode);

    // Callbacks
    std::function<void(float pxPerBeat)> onZoomChanged;
    std::function<void(HeightMode)>      onHeightModeChanged;
    /** Fired with the new scale factor (0.4..3.0) when the vertical handle is dragged. */
    std::function<void(float scale)>     onLaneHeightChanged;

    /** Sync the handle to an externally driven scale (e.g. from TracksPanel VIEW tab). */
    void setLaneHeightScale (float scale) noexcept;
    float laneHeightScale() const noexcept { return m_laneHeightScale; }

    void paint           (juce::Graphics&) override;
    void mouseDown       (const juce::MouseEvent&) override;
    void mouseDrag       (const juce::MouseEvent&) override;
    void mouseUp         (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    float m_pxPerBeat        = kZoomDefault;
    float m_laneHeightScale  = 1.0f;
    HeightMode m_heightMode    = HeightMode::norm;
    HeightMode m_preExpandMode = HeightMode::norm;

    // Drag state for zoom slider
    bool  m_draggingZoom  = false;
    float m_dragStartX    = 0.0f;
    float m_dragStartZoom = 0.0f;

    // Layout rects — set in paint
    mutable juce::Rectangle<int> m_sliderTrackRect;
    mutable juce::Rectangle<int> m_expandBtnRect;
    mutable juce::Rectangle<int> m_modeBtnRects[3];      // 0=MIN 1=NORM 2=MAX
    mutable juce::Rectangle<int> m_heightHandleRect;     // vertical drag handle (replaces H+/H-)

    // Drag state for height handle
    bool  m_draggingHeight      = false;
    float m_heightDragStartY    = 0.0f;
    float m_heightDragStartScale = 1.0f;
    bool  m_heightHandleHovered  = false;

    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    void drawZoomSlider    (juce::Graphics&, const juce::Rectangle<int>& area) const;
    void drawModeButtons   (juce::Graphics&) const;
    void drawExpandButton  (juce::Graphics&) const;
    void drawHeightHandle  (juce::Graphics&) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZoomStrip)
};

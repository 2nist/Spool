#pragma once

#include "../../Theme.h"
#include "ClipData.h"
#include "LaneLabels.h"
#include "ClipCard.h"
#include "MixerGutter.h"

//==============================================================================
/**
    CylinderBand — the scrolling tape surface plus label column and mixer gutter.

    Contains:
      - LaneLabels (36px left)
      - Tape surface (paint only — no child components)
      - MixerGutter (60px right, slides in/out)

    The cylinder rotation model lives here:
      - m_loopLengthBars, m_beatsPerBar, m_pxPerBeat, m_currentBeat
      - timelineXForBeat() — ALL element positions go through this
      - signedOffsetPxForBeat() — canonical left-to-right beat mapping around the playhead
*/
class CylinderBand : public juce::Component,
                     public juce::DragAndDropTarget
{
public:
    CylinderBand();
    ~CylinderBand() override = default;

    //==========================================================================
    // Rotation model
    float m_loopLengthBars = 8.0f;
    float m_beatsPerBar    = 4.0f;
    float m_pxPerBeat      = 28.0f;
    float m_currentBeat    = 0.0f;

    float loopLengthBeats() const noexcept;
    float loopLengthPx()    const noexcept;
    float signedOffsetPxForBeat (float beatPosition) const noexcept;
    float timelineXForBeat      (float beatPosition, float centerX) const noexcept;

    //==========================================================================
    // Lane management
    void setLanes (const juce::Array<LaneData>& lanes);
    void setHighlightLane (int slotIndex);      // amber tint
    void clearHighlight();
    void setLaneArmed (int slotIndex, bool armed);
    void setMixerGutterOpen (bool open);
    bool isMixerGutterOpen() const noexcept { return m_mixerOpen; }

    //==========================================================================
    // Rotation model setters (called from ZoneDComponent)
    void setPxPerBeat       (float pxPerBeat) noexcept { m_pxPerBeat = pxPerBeat; repaint(); }
    void setLoopLengthBars  (float bars)      noexcept { m_loopLengthBars = bars; repaint(); }
    void setCurrentBeat     (float beat)      noexcept { m_currentBeat = beat;    repaint(); }
    /** Called by ZoneDComponent when transport starts/stops — controls scrub behaviour. */
    void setTransportPlaying (bool playing)    noexcept { m_transportPlaying = playing; updateMouseCursor(); }

    /** Set vertical zoom: 1.0 = normal, 0.5 = half height, 2.0 = double. Range clamped [0.5, 3.0]. */
    void setLaneScale (float s);

    //==========================================================================
    void paint              (juce::Graphics&) override;
    void resized            () override;
    void mouseDown          (const juce::MouseEvent&) override;
    void mouseDrag          (const juce::MouseEvent&) override;
    void mouseUp            (const juce::MouseEvent&) override;
    juce::MouseCursor getMouseCursor() override;

    // Callbacks
    std::function<void(int slotIndex, bool armed)> onDotClicked;
    std::function<void(int slotIndex)>              onAutoArrowClicked;
    /** Fired when an audio-history-clip is dropped onto the tape. laneIndex = -1 if no lane hit. */
    std::function<void(int laneIndex)>              onClipDropped;

    //==========================================================================
    // DragAndDropTarget
    bool isInterestedInDragSource (const SourceDetails& details) override;
    void itemDropped               (const SourceDetails& details) override;

private:
    static constexpr int kLabelW  = 36;
    static constexpr int kRimW    = 5;
    static constexpr int kFadeW   = 160;
    static constexpr int kShadowH = 20;
    static constexpr int kLaneH   = 27;   // reference height (1.0 scale)
    static constexpr int kAutoH   = 14;   // reference auto-lane height

    float m_laneScale = 1.0f;
    int   laneH() const noexcept { return juce::roundToInt (static_cast<float> (kLaneH) * m_laneScale); }
    int   autoH() const noexcept { return juce::roundToInt (static_cast<float> (kAutoH) * m_laneScale); }

    juce::Array<LaneData> m_lanes;
    int   m_highlightSlot    = -1;
    bool  m_mixerOpen        = false;
    bool  m_transportPlaying = false;

    // Scrub drag state
    bool  m_isDragging     = false;
    float m_dragStartX     = 0.0f;
    float m_dragStartBeat  = 0.0f;

    LaneLabels  m_laneLabels;
    MixerGutter m_mixerGutter;

    // Tape surface rect (computed in resized)
    juce::Rectangle<int> m_tapeRect;

    void paintTapeBase          (juce::Graphics&, int tapeH) const;
    void paintSpecularHighlight (juce::Graphics&, float centerX, int tapeH) const;
    void paintLaneContent       (juce::Graphics&, int laneIndex,
                                 int laneTopY, float centerX, int tapeH) const;
    void paintBeatMarkers       (juce::Graphics&, float centerX, int tapeH) const;
    void paintSeam              (juce::Graphics&, float centerX, int tapeH) const;
    void paintClips             (juce::Graphics&, int laneIndex,
                                 int laneTopY, int laneH,
                                 float centerX) const;
    void paintRimsAndFades      (juce::Graphics&, int tapeW, int tapeH) const;
    void paintPlayhead          (juce::Graphics&, float centerX, int tapeH) const;

    int  totalLaneHeight()  const noexcept;
    int  laneTopY (int laneIndex) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CylinderBand)
};

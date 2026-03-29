#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    TracksPanel — full timeline control surface opened via the TRK tab.

    Layout (top to bottom):
      TOP SECTION (~100px fixed):
        Row 1 — Bar.Beat.Tick position display (drag to scrub)
        Row 2 — BPM + Time signature
        Row 3 — STOP / PLAY / REC buttons (28×28px)

      BOTTOM SECTION (fills remaining):
        Sub-tab bar: LANES | LOOP | VIEW | SNAP
        Content area (switches per sub-tab)

    LANES — per-lane rows with M/S/ARM toggles
    LOOP  — mini timeline, loop start/length
    VIEW  — zoom + height sliders, height mode buttons
    SNAP  — snap mode radio buttons, swing slider
*/
class TracksPanel : public juce::Component
{
public:
    TracksPanel();
    ~TracksPanel() override = default;

    //--- External sync --------------------------------------------------------
    struct LaneInfo
    {
        juce::String name;
        juce::Colour colour;
        bool         muted  { false };
        bool         soloed { false };
        bool         armed  { false };
    };

    void setLanes      (const juce::Array<LaneInfo>& lanes);
    void setPlaying    (bool playing);
    void setRecording  (bool recording);
    void setBpm        (float bpm);
    void setPosition   (float beat);
    void setLaneArmed  (int laneIndex, bool armed);
    struct TimelinePlacement
    {
        int          laneIndex   { 0 };
        juce::String laneName;
        juce::String clipName;
        float        startBeat   { 0.0f };
        float        lengthBeats { 0.0f };
    };
    void addTimelinePlacement (const TimelinePlacement& placement);
    void clearTimelinePlacements();

    /** Current lane height scale (0.4..3.0). */
    void setLaneHeightScale (float s);
    float getLaneHeightScale() const noexcept { return m_laneHeightScale; }

    //--- Callbacks ------------------------------------------------------------
    std::function<void()>                   onPlay;
    std::function<void()>                   onStop;
    std::function<void()>                   onRecord;
    std::function<void(float bpm)>          onBpmChanged;
    std::function<void(int lane, bool m)>   onMuteChanged;
    std::function<void(int lane, bool s)>   onSoloChanged;
    std::function<void(int lane, bool a)>   onArmChanged;
    std::function<void(float bars)>         onLoopLengthChanged;
    std::function<void(float bars)>         onLoopStartChanged;
    std::function<void(float zoom)>         onViewZoomChanged;
    std::function<void(float scale)>        onViewHeightChanged;
    std::function<void(int snapMode)>       onSnapChanged;

    //--- juce::Component ------------------------------------------------------
    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

private:
    enum SubTab { kLanes, kLoop, kView, kSnap, kSubTabCount };

    //--- State ----------------------------------------------------------------
    juce::Array<LaneInfo> m_lanes;
    bool  m_playing      { false };
    bool  m_recording    { false };
    float m_bpm          { 120.0f };
    float m_beat         { 0.0f };
    int   m_activeSubTab { kLanes };
    juce::Array<TimelinePlacement> m_timelinePlacements;

    float m_laneHeightScale { 1.0f };
    float m_loopStart       { 1.0f };
    float m_loopLength      { 8.0f };
    float m_viewZoom        { 1.0f };
    int   m_snapMode        { 2 };   // 0=1/4, 1=1/8, 2=1/16, 3=1/32, 4=bar, 5=free
    float m_swing           { 0.0f };

    // Drag state
    bool  m_draggingBpm  { false };
    float m_dragStartY   { 0.0f };
    float m_dragStartBpm { 120.0f };

    //--- Layout constants -----------------------------------------------------
    static constexpr int kPad        = 5;
    static constexpr int kHeaderH    = 22;
    static constexpr int kTopH       = 88;  // transport section height
    static constexpr int kSubTabBarH = 18;
    static constexpr int kBtnSz      = 22;
    static constexpr int kLaneRowH   = 24;

    //--- Layout helpers -------------------------------------------------------
    juce::Rectangle<int> topSection()    const noexcept;
    juce::Rectangle<int> subTabBar()     const noexcept;
    juce::Rectangle<int> contentArea()   const noexcept;
    juce::Rectangle<int> subTabRect (int t) const noexcept;
    juce::Rectangle<int> stopBtnRect()   const noexcept;
    juce::Rectangle<int> playBtnRect()   const noexcept;
    juce::Rectangle<int> recBtnRect()    const noexcept;
    juce::Rectangle<int> bpmRect()       const noexcept;

    //--- Painters -------------------------------------------------------------
    void paintTop        (juce::Graphics& g) const;
    void paintSubTabBar  (juce::Graphics& g) const;
    void paintLanesTab   (juce::Graphics& g) const;
    void paintLoopTab    (juce::Graphics& g) const;
    void paintViewTab    (juce::Graphics& g) const;
    void paintSnapTab    (juce::Graphics& g) const;

    void paintTransportBtn (juce::Graphics& g,
                            const juce::Rectangle<int>& r,
                            const juce::String& label,
                            bool active) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TracksPanel)
};

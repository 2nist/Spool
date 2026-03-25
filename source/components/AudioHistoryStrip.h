#pragma once

#include "../Theme.h"
#include "../dsp/CircularAudioBuffer.h"

//==============================================================================
/**
    AudioHistoryStrip — full-width resizable strip between Zone B/C and Zone D.

    Displays the continuous circular audio history as a rolling waveform.
    Present moment is pinned to the right edge; past scrolls left.

    Layout (left to right):
      Left tab:   28px  — icon, "HIST" label, collapse arrow
      Main area:  remaining — header row + waveform + grab controls

    Interaction:
      Drag top edge     — resize (fires onHeightChanged)
      Double-click      — collapse to 20px / restore saved height
      Drag in waveform  — create selection region
      Double-click wave — clear selection

    Collapsed state (height == kMinH = 20px):
      Only the left tab visible, no waveform.
*/
class AudioHistoryStrip : public juce::Component,
                          public juce::Timer
{
public:
    AudioHistoryStrip();
    ~AudioHistoryStrip() override;

    //==========================================================================
    // Data source

    /** Must be called before the strip becomes visible. */
    void setCircularBuffer (CircularAudioBuffer* buf) noexcept { m_buffer = buf; }

    /** Programmatically collapse/restore the history strip. */
    void setCollapsed (bool collapsed) noexcept;

    /** Update the source slot display (called when selection changes in Zone B). */
    void setSourceInfo (const juce::String& name, juce::Colour color);

    /** Mark a grabbed region for highlight.  Call after grabRegion/grabLastBars. */
    void setGrabbedRegion (double startSecondsAgo, double lengthSeconds);
    void clearGrabbedRegion();

    //==========================================================================
    // Layout constants

    static constexpr int kTabW      = 28;
    static constexpr int kMinH      = 20;
    static constexpr int kMaxH      = 200;
    static constexpr int kDefaultH  = 80;
    static constexpr int kHandleH   = 6;    // top resize handle zone
    static constexpr int kHeaderH   = 20;
    static constexpr int kGrabRowH  = 18;

    //==========================================================================
    // Callbacks

    /** Fired when strip is resized by dragging the top edge or collapsed/expanded. */
    std::function<void(int height)>               onHeightChanged;

    /** Fired when a grab button is pressed. */
    std::function<void(int numBars)>              onGrabLastBars;
    std::function<void(double start, double len)> onGrabRegion;

    /** Fired by → routing buttons. */
    std::function<void()> onSendToReel;

    /** Fired by → REEL when Shift is held — load into a new (non-focused) REEL slot.
        PluginEditor resolves which slot to target (typically focusedSlot + 1). */
    std::function<void()> onSendToNewReelSlot;

    std::function<void()> onSendToTimeline;

    /** Fired when the LIVE dot is clicked. */
    std::function<void(bool active)> onActiveToggled;

    /** Fired at the start of a drag gesture originating from a grabbed region.
        PluginEditor should use this to populate its m_dragClip before the JUCE
        drag-and-drop system takes over. */
    std::function<void()> onDragStarted;

    //==========================================================================
    // juce::Component
    void paint            (juce::Graphics&)            override;
    void resized          ()                            override;
    void mouseDown        (const juce::MouseEvent&)    override;
    void mouseDrag        (const juce::MouseEvent&)    override;
    void mouseUp          (const juce::MouseEvent&)    override;
    void mouseDoubleClick (const juce::MouseEvent&)    override;
    juce::MouseCursor getMouseCursor()                 override;

    // juce::Timer
    void timerCallback() override;

private:
    //==========================================================================
    // Data

    CircularAudioBuffer* m_buffer      = nullptr;
    juce::Array<float>   m_waveformData;

    juce::String m_sourceName  { "MIX" };
    juce::Colour m_sourceColor { 0xFF4B9EDB };
    bool         m_isLive      { true };

    // Grabbed region (seconds ago from write head)
    bool   m_hasGrabbedRegion  { false };
    double m_grabbedStart      { 0.0 };
    double m_grabbedLength     { 0.0 };

    // Selection state (seconds ago)
    bool   m_hasSelection { false };
    double m_selStart     { 0.0 };
    double m_selEnd       { 0.0 };

    // Window size cycling
    static constexpr double kWindowSizes[]   = { 10.0, 30.0, 60.0, 120.0, 300.0 };
    static constexpr int    kNumWindowSizes  = 5;
    int    m_windowSizeIndex  { 1 };   // default 30s
    double m_windowSeconds    { 30.0 };

    // Resize / collapse state
    int  m_savedHeight       { kDefaultH };
    bool m_isResizeDrag      { false };
    int  m_resizeDragStartY  { 0 };
    int  m_resizeDragStartH  { 0 };

    // Selection drag state
    enum class SelDrag { None, Creating, Moving, ResizingLeft, ResizingRight };
    SelDrag m_selDrag      { SelDrag::None };
    double  m_dragAnchor   { 0.0 };
    double  m_selMoveOff   { 0.0 };

    // Set while the user is dragging from within a grabbed region — triggers DnD
    bool m_isGrabDragPending { false };

    //==========================================================================
    // Layout helpers

    juce::Rectangle<int> tabRect()    const noexcept;
    juce::Rectangle<int> mainRect()   const noexcept;
    juce::Rectangle<int> headerRect() const noexcept;
    juce::Rectangle<int> waveRect()   const noexcept;
    juce::Rectangle<int> grabRect()   const noexcept;

    bool isCollapsed()  const noexcept { return getHeight() <= kMinH; }

    // Coordinate mapping
    float xToSecondsAgo   (float x)           const noexcept;
    float secondsAgoToX   (float secondsAgo)  const noexcept;

    // Paint
    void paintTab         (juce::Graphics&) const;
    void paintHeader      (juce::Graphics&) const;
    void paintWaveform    (juce::Graphics&) const;
    void paintGrabControls(juce::Graphics&) const;

    // Hit-test for grab buttons — returns 0-based index or -1
    // 0=◀4  1=◀8  2=◀16  3=FREE  4=→REEL  5=→TIMELINE
    int grabBtnAt (juce::Point<int> pos) const noexcept;

    // Selection edge hit-test
    SelDrag selDragModeAt (juce::Point<int> pos) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioHistoryStrip)
};

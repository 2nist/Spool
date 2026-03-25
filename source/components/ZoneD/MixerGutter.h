#pragma once

#include "../../Theme.h"
#include "ClipData.h"

//==============================================================================
/**
    MixerGutter — 60px panel that slides in from the right of the cylinder band.

    Shows one vertical fader strip per lane.
    Animated open/close via width change.
    When open: parent should reduce tape surface width by 60px.
*/
class MixerGutter : public juce::Component
{
public:
    static constexpr int kOpenWidth = 60;

    MixerGutter();
    ~MixerGutter() override = default;

    void setLanes (const juce::Array<LaneData>& lanes);
    bool isOpen() const noexcept { return m_open; }
    void setOpen  (bool open) noexcept { m_open = open; }

    /** Update lane row heights for vertical zoom (1.0 = normal). */
    void setLaneScale (float s) noexcept { m_laneH = juce::roundToInt (27.0f * s); repaint(); }

    void paint    (juce::Graphics&) override;
    void resized  () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    static constexpr int kFaderW   = 8;

    juce::Array<LaneData> m_lanes;
    bool  m_open           = false;
    int   m_laneH          = 27;   // updated by setLaneScale
    int   m_draggingLane   = -1;
    float m_dragStartY     = 0.0f;
    float m_dragStartLevel = 0.0f;

    int laneTopY (int laneIndex) const noexcept;
    juce::Rectangle<int> faderRect (int laneIndex) const noexcept;
    float levelToY (int laneIndex, float level) const noexcept;
    int hitTestLane (juce::Point<int> pos) const noexcept;

    friend class ZoneDComponent;  // ZoneDComponent sets m_open directly before animation

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerGutter)
};

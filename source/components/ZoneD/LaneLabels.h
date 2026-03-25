#pragma once

#include "../../Theme.h"
#include "ClipData.h"

//==============================================================================
/**
    LaneLabels — the 36px dark column on the left of the cylinder band.

    Contains one cell per active lane (stacked vertically).
    Outside all cylinder gradient fades — purely housing colour.

    Inherits juce::Timer for the armed-state dot pulse animation.
*/
class LaneLabels : public juce::Component,
                   public juce::Timer
{
public:
    static constexpr int kWidth   = 36;

    LaneLabels();
    ~LaneLabels() override;

    /** Rebuild from lane data array. Called after setLaneInfo changes. */
    void setLanes (const juce::Array<LaneData>& lanes);

    /** Called when a lane's armed state changes — starts/stops timer. */
    void updateArmedState (int laneIndex, bool armed);

    /** Fires when dot clicked → parent calls armLane(). */
    std::function<void(int laneIndex, bool armed)> onDotClicked;

    /** Fires when automation arrow clicked. */
    std::function<void(int laneIndex)> onAutoArrowClicked;

    /** Update lane row heights for vertical zoom (1.0 = normal). */
    void setLaneScale (float s) noexcept
    {
        m_laneH = juce::roundToInt (27.0f * s);
        m_autoH = juce::roundToInt (14.0f * s);
        repaint();
    }

    void paint    (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void timerCallback() override;

private:
    juce::Array<LaneData> m_lanes;

    // Dynamic row heights (updated via setLaneScale)
    int m_laneH = 27;
    int m_autoH = 14;

    // Pulse animation state
    float m_pulseAlpha     = 1.0f;
    bool  m_pulseDirection = false; // false = decreasing, true = increasing

    int laneTopY (int laneIndex) const noexcept;
    juce::Rectangle<int> dotRect (int laneIndex) const noexcept;
    juce::Rectangle<int> arrowRect (int laneIndex) const noexcept;

    bool anyArmed() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaneLabels)
};

#include "LaneLabels.h"

//==============================================================================
LaneLabels::LaneLabels()
{
}

LaneLabels::~LaneLabels()
{
    stopTimer();
}

//==============================================================================
void LaneLabels::setLanes (const juce::Array<LaneData>& lanes)
{
    m_lanes = lanes;
    repaint();
}

void LaneLabels::updateArmedState (int laneIndex, bool armed)
{
    if (laneIndex >= 0 && laneIndex < m_lanes.size())
    {
        m_lanes.getReference (laneIndex).armed = armed;

        if (armed && !isTimerRunning())
            startTimerHz (2);
        else if (!anyArmed())
            stopTimer();

        repaint();
    }
}

//==============================================================================
void LaneLabels::timerCallback()
{
    // Pulse alpha 0.6 → 1.0
    const float step = 0.4f / 4.0f; // ~2 ticks per half-cycle at 2Hz
    if (m_pulseDirection)
    {
        m_pulseAlpha += step;
        if (m_pulseAlpha >= 1.0f)
        {
            m_pulseAlpha     = 1.0f;
            m_pulseDirection = false;
        }
    }
    else
    {
        m_pulseAlpha -= step;
        if (m_pulseAlpha <= 0.6f)
        {
            m_pulseAlpha     = 0.6f;
            m_pulseDirection = true;
        }
    }
    repaint();
}

//==============================================================================
void LaneLabels::paint (juce::Graphics& g)
{
    const int w = getWidth();

    // Background
    g.fillAll (Theme::Colour::surface0);

    // Right border
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect (w - 1, 0, 1, getHeight());

    for (int i = 0; i < m_lanes.size(); ++i)
    {
        const auto& lane = m_lanes.getReference (i);
        const int cellY  = laneTopY (i);
        const int cellH  = m_laneH + (lane.autoExpanded ? m_autoH : 0);
        const juce::Rectangle<int> cellR { 0, cellY, w, cellH };

        // Lane separator
        g.setColour (juce::Colour (0xFF41300e).withAlpha (0.18f));
        g.fillRect (cellR.withHeight (1).translated (0, cellH - 1));

        // Color dot
        const auto dr       = dotRect (i);
        const bool isArmed  = lane.armed;
        const float alpha   = isArmed ? m_pulseAlpha : 1.0f;
        const auto dotCol   = lane.dotColor.withAlpha (isArmed ? alpha : 0.85f);

        // Outer glow
        g.setColour (dotCol.withAlpha (0.25f * alpha));
        g.fillEllipse (dr.expanded (2).toFloat());

        // Inner glow
        g.setColour (dotCol.withAlpha (0.55f * alpha));
        g.fillEllipse (dr.toFloat());

        // Core dot
        g.setColour (dotCol);
        const auto coreR = dr.reduced (1);
        g.fillEllipse (coreR.toFloat());

        // Track number
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        const juce::String num = juce::String::formatted ("%02d", lane.slotIndex + 1);
        g.drawText (num,
                    juce::Rectangle<int> (0, dr.getBottom() + 2, w, 9),
                    juce::Justification::centred, false);

        // Automation arrow (if lane has clips / is expandable)
        if (!lane.clips.isEmpty())
        {
            const auto ar = arrowRect (i);
            g.setColour (Theme::Colour::inkGhost);
            g.drawText (lane.autoExpanded ? "^" : "v",
                        ar, juce::Justification::centred, false);
        }
    }
}

void LaneLabels::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    for (int i = 0; i < m_lanes.size(); ++i)
    {
        // Dot hit
        if (dotRect (i).expanded (4).contains (pos))
        {
            const bool newArmed = !m_lanes.getReference (i).armed;
            if (onDotClicked)
                onDotClicked (i, newArmed);
            return;
        }

        // Arrow hit
        if (!m_lanes.getReference (i).clips.isEmpty()
            && arrowRect (i).expanded (4).contains (pos))
        {
            if (onAutoArrowClicked)
                onAutoArrowClicked (i);
            return;
        }
    }
}

//==============================================================================
int LaneLabels::laneTopY (int laneIndex) const noexcept
{
    int y = 0;
    for (int i = 0; i < laneIndex; ++i)
        y += m_laneH + (m_lanes[i].autoExpanded ? m_autoH : 0);
    return y;
}

juce::Rectangle<int> LaneLabels::dotRect (int laneIndex) const noexcept
{
    const int diam = 5;
    const int cellY = laneTopY (laneIndex);
    const int cx    = getWidth() / 2;
    const int cy    = cellY + 8;
    return { cx - diam / 2, cy - diam / 2, diam, diam };
}

juce::Rectangle<int> LaneLabels::arrowRect (int laneIndex) const noexcept
{
    const int cellY  = laneTopY (laneIndex);
    return { 0, cellY + m_laneH - 10, getWidth(), 10 };
}

bool LaneLabels::anyArmed() const noexcept
{
    for (const auto& lane : m_lanes)
        if (lane.armed) return true;
    return false;
}

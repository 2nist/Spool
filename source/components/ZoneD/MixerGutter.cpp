#include "MixerGutter.h"

//==============================================================================
MixerGutter::MixerGutter()
{
}

void MixerGutter::setLanes (const juce::Array<LaneData>& lanes)
{
    m_lanes = lanes;
    repaint();
}

//==============================================================================
void MixerGutter::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface0);

    // Left border
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect (0, 0, 1, getHeight());

    for (int i = 0; i < m_lanes.size(); ++i)
    {
        const auto& lane = m_lanes.getReference (i);
        const auto fr    = faderRect (i);
        const float lv   = lane.mixLevel;

        // Track background
        g.setColour (Theme::Colour::surface2);
        g.fillRect (fr);

        // Fill from bottom to level
        const int fillTop = juce::roundToInt (fr.getBottom() - lv * fr.getHeight());
        g.setColour (lane.dotColor.withAlpha (0.40f));
        g.fillRect (fr.withTop (fillTop));

        // Track border
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.4f));
        g.drawRect (fr.toFloat(), 0.5f);
    }
}

void MixerGutter::resized()
{
}

void MixerGutter::mouseDown (const juce::MouseEvent& e)
{
    m_draggingLane = hitTestLane (e.getPosition());
    if (m_draggingLane >= 0)
    {
        m_dragStartY     = static_cast<float> (e.getPosition().getY());
        m_dragStartLevel = m_lanes[m_draggingLane].mixLevel;
    }
}

void MixerGutter::mouseDrag (const juce::MouseEvent& e)
{
    if (m_draggingLane < 0 || m_draggingLane >= m_lanes.size())
        return;

    const auto fr     = faderRect (m_draggingLane);
    const float delta = (m_dragStartY - static_cast<float> (e.getPosition().getY()))
                        / static_cast<float> (fr.getHeight());

    m_lanes.getReference (m_draggingLane).mixLevel =
        juce::jlimit (0.0f, 1.0f, m_dragStartLevel + delta);

    repaint();
}

//==============================================================================
int MixerGutter::laneTopY (int laneIndex) const noexcept
{
    int y = 0;
    for (int i = 0; i < laneIndex; ++i)
        y += m_laneH;
    return y;
}

juce::Rectangle<int> MixerGutter::faderRect (int laneIndex) const noexcept
{
    const int y  = laneTopY (laneIndex);
    const int cx = getWidth() / 2;
    return { cx - kFaderW / 2, y + 2, kFaderW, m_laneH - 4 };
}

float MixerGutter::levelToY (int laneIndex, float level) const noexcept
{
    const auto fr = faderRect (laneIndex);
    return fr.getBottom() - level * fr.getHeight();
}

int MixerGutter::hitTestLane (juce::Point<int> pos) const noexcept
{
    for (int i = 0; i < m_lanes.size(); ++i)
    {
        const int top    = laneTopY (i);
        const int bottom = top + m_laneH;
        if (pos.getY() >= top && pos.getY() < bottom)
            return i;
    }
    return -1;
}

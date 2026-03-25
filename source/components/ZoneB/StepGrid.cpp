#include "StepGrid.h"

StepGrid::StepGrid()
{
    setRepaintsOnMouseActivity (false);
    // Set step 0 active by default to verify active state renders
    m_active[0] = true;
}

void StepGrid::setArmedSlot (int slotIndex, const juce::String& abbreviation)
{
    m_armedSlot  = slotIndex;
    m_armedLabel = abbreviation;
    repaint();
}

void StepGrid::setPlayhead (int stepIndex)
{
    m_playhead = stepIndex;
    repaint();
}

//==============================================================================
// Layout helpers
//==============================================================================

juce::Rectangle<int> StepGrid::stepRect (int idx) const noexcept
{
    if (idx < 0 || idx >= kNumSteps) return {};

    const int pad     = static_cast<int> (Theme::Space::sm);
    const int gap     = static_cast<int> (Theme::Space::stepGap);
    const int rowGap  = static_cast<int> (Theme::Space::sm);
    const int stepH   = static_cast<int> (Theme::Space::stepHeight);
    const int row     = idx / kStepsPerRow;
    const int col     = idx % kStepsPerRow;

    const int availW  = getWidth() - pad * 2;
    const int stepW   = (availW - (kStepsPerRow - 1) * gap) / kStepsPerRow;

    const int x = pad + col * (stepW + gap);
    const int y = kHeaderH + pad + row * (stepH + rowGap);

    return { x, y, stepW, stepH };
}

int StepGrid::stepAtPosition (juce::Point<int> pos) const noexcept
{
    const int pad     = static_cast<int> (Theme::Space::sm);
    const int gap     = static_cast<int> (Theme::Space::stepGap);
    const int rowGap  = static_cast<int> (Theme::Space::sm);
    const int stepH   = static_cast<int> (Theme::Space::stepHeight);

    const int availW  = getWidth() - pad * 2;
    const int stepW   = (availW - (kStepsPerRow - 1) * gap) / kStepsPerRow;

    // Which row?
    int row = -1;
    for (int r = 0; r < 2; ++r)
    {
        const int rowY = kHeaderH + pad + r * (stepH + rowGap);
        if (pos.y >= rowY && pos.y < rowY + stepH)
        {
            row = r;
            break;
        }
    }
    if (row < 0) return -1;

    // Which column?
    const int relX = pos.x - pad;
    if (relX < 0) return -1;
    const int col = relX / (stepW + gap);
    if (col < 0 || col >= kStepsPerRow) return -1;

    // Check not in gap
    if (relX - col * (stepW + gap) >= stepW) return -1;

    return row * kStepsPerRow + col;
}

//==============================================================================
// Paint
//==============================================================================

void StepGrid::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    paintHeader (g);

    for (int i = 0; i < kNumSteps; ++i)
        paintStep (g, i);

    // Beat-group dividers — visible 1px lines between groups of 4 steps
    const int pad  = static_cast<int> (Theme::Space::sm);
    const int gap  = static_cast<int> (Theme::Space::stepGap);
    const int stepH = static_cast<int> (Theme::Space::stepHeight);
    const int rowGap = static_cast<int> (Theme::Space::sm);
    const int availW = getWidth() - pad * 2;
    const int stepW  = (availW - (kStepsPerRow - 1) * gap) / kStepsPerRow;

    g.setColour (juce::Colour (0xFF5a5040));
    for (int r = 0; r < 2; ++r)
    {
        const int rowY = kHeaderH + pad + r * (stepH + rowGap);
        for (int group = 1; group < 4; ++group)
        {
            const int col   = group * 4;
            const int lineX = pad + col * (stepW + gap) - gap;
            g.fillRect (juce::Rectangle<int> (lineX, rowY, 1, stepH));
        }
    }
}

void StepGrid::paintHeader (juce::Graphics& g) const
{
    const auto headR = juce::Rectangle<int> (0, 0, getWidth(), kHeaderH);
    g.setColour (Theme::Colour::surface0);
    g.fillRect  (headR);

    const int gap    = static_cast<int> (Theme::Space::stepGap);
    const int pad    = static_cast<int> (Theme::Space::sm);
    const int availW = getWidth() - pad * 2;
    const int stepW  = (availW - (kStepsPerRow - 1) * gap) / kStepsPerRow;

    g.setFont   (Theme::Font::micro());

    // Armed label (far left)
    if (m_armedSlot >= 0)
    {
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  (m_armedLabel,
                     juce::Rectangle<int> (pad, 0, 24, kHeaderH),
                     juce::Justification::centred, false);
    }

    // Beat markers "1" "2" "3" "4" above beat-1 of each group
    const juce::StringArray beats { "1", "2", "3", "4" };
    for (int beat = 0; beat < 4; ++beat)
    {
        const int col = beat * 4;
        const int x   = pad + col * (stepW + gap);
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  (beats[beat],
                     juce::Rectangle<int> (x, 0, stepW, kHeaderH),
                     juce::Justification::centred, false);
    }

    // Step count "32" — right side
    g.setColour (Theme::Colour::accent);
    g.drawText  ("32",
                 headR.withTrimmedRight (static_cast<int> (Theme::Space::xxl) + pad),
                 juce::Justification::centredRight, false);

    // Pattern label
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  ("PATTERN 1",
                 headR.withTrimmedRight (pad),
                 juce::Justification::centredRight, false);
}

void StepGrid::paintStep (juce::Graphics& g, int idx) const
{
    const auto r = stepRect (idx);
    if (r.isEmpty()) return;

    const bool isActive   = m_active[idx];
    const bool isAccented = m_accented[idx];
    const bool isBeat1Pos = isBeat1 (idx % kStepsPerRow);
    const bool isPlaying  = (idx == m_playhead);

    // Determine fill and whether to use beat-1 variant
    bool activeArg  = isActive;
    bool accentArg  = isAccented || (isActive && isBeat1Pos);

    // Beat-1 inactive gets beat-1 tinted background
    juce::Colour fill;
    if (!isActive && isBeat1Pos)
        fill = juce::Colour (0xFF635748);                                // beat-1 inactive
    else if (isActive && isBeat1Pos && !isAccented)
        fill = Theme::Colour::accent.brighter (0.15f);                   // beat-1 active
    else
        fill = isActive ? (isAccented ? Theme::Colour::accentWarm : Theme::Colour::accent)
                        : juce::Colour (0xFF524840);                     // regular inactive

    g.setColour (fill);
    g.fillRoundedRectangle (r.toFloat(), Theme::Radius::xs);

    // Border — clearly visible between each step
    g.setColour (juce::Colour (0xFF80705a));
    g.drawRoundedRectangle (r.toFloat(), Theme::Radius::xs, Theme::Stroke::subtle);

    // Playhead — 1px red line above and below
    if (isPlaying)
    {
        g.setColour (Theme::Colour::error);
        g.fillRect  (r.getX(), r.getY() - 1, r.getWidth(), 1);
        g.fillRect  (r.getX(), r.getBottom(),  r.getWidth(), 1);
    }

    juce::ignoreUnused (activeArg, accentArg);
}

void StepGrid::resized() {}

//==============================================================================
// Mouse
//==============================================================================

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    const int idx = stepAtPosition (e.getPosition());
    if (idx < 0) return;

    if (e.mods.isRightButtonDown())
    {
        // Right-click cycles: inactive → active → accent → inactive
        if (!m_active[idx])
        {
            m_active[idx]   = true;
            m_accented[idx] = false;
        }
        else if (!m_accented[idx])
        {
            m_accented[idx] = true;
        }
        else
        {
            m_active[idx]   = false;
            m_accented[idx] = false;
        }
        repaint();
        return;
    }

    // Left-click toggle + set drag-fill mode
    m_active[idx] = !m_active[idx];
    if (!m_active[idx]) m_accented[idx] = false;
    m_paintMode       = m_active[idx];   // mode matches new state
    m_lastPaintedStep = idx;
    repaint();
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;

    const int idx = stepAtPosition (e.getPosition());
    if (idx < 0 || idx == m_lastPaintedStep) return;

    m_active[idx] = m_paintMode;
    if (!m_paintMode) m_accented[idx] = false;
    m_lastPaintedStep = idx;
    repaint();
}

void StepGrid::mouseUp (const juce::MouseEvent&)
{
    m_lastPaintedStep = -1;
}

juce::MouseCursor StepGrid::getMouseCursor()
{
    return juce::MouseCursor (juce::MouseCursor::CrosshairCursor);
}

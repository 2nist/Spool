#include "StepGridSingle.h"

//==============================================================================

StepGridSingle::StepGridSingle()
{
    setRepaintsOnMouseActivity (false);
}

//==============================================================================
// State
//==============================================================================

void StepGridSingle::setPattern (SlotPattern* pattern, juce::Colour groupColor)
{
    m_pattern    = pattern;
    m_groupColor = groupColor;
    repaint();
}

void StepGridSingle::clearPattern()
{
    m_pattern  = nullptr;
    m_playhead = -1;
    repaint();
}

void StepGridSingle::setPlayhead (int stepIndex)
{
    m_playhead = stepIndex;
    repaint();
}

//==============================================================================
// Layout helpers
//==============================================================================

int StepGridSingle::numRows() const noexcept
{
    if (m_pattern == nullptr) return 2;
    const int n = m_pattern->activeStepCount();
    if (n <= 16)  return 1;
    if (n <= 32)  return 2;
    return 3;   // 33–64 steps: 3 rows (TODO: inner scroll for >48)
}

int StepGridSingle::stepsPerRow() const noexcept
{
    if (m_pattern == nullptr) return 16;
    const int n = m_pattern->activeStepCount();
    return juce::jmin (n, 16);   // at most 16 per row
}

int StepGridSingle::stepW() const noexcept
{
    // Constrain to the smaller of width-based and height-based cell size so steps stay square
    const int spr  = stepsPerRow();
    const int byW  = (gridArea().getWidth()  - (spr - 1) * kGap) / spr;
    const int rows = numRows();
    const int byH  = (gridArea().getHeight() - (rows - 1) * kGap) / rows;
    return juce::jmax (4, juce::jmin (byW, byH));
}

int StepGridSingle::stepH() const noexcept
{
    return stepW();  // always square
}

juce::Rectangle<int> StepGridSingle::gridArea() const noexcept
{
    return { kPad, kPad, getWidth() - kPad * 2 - kCtrlW - kPad, getHeight() - kPad * 2 };
}

juce::Rectangle<int> StepGridSingle::ctrlArea() const noexcept
{
    return { getWidth() - kCtrlW - kPad, kPad, kCtrlW, getHeight() - kPad * 2 };
}

juce::Rectangle<int> StepGridSingle::decBtnRect() const noexcept
{
    const auto a = ctrlArea();
    return { a.getX(), a.getY(), kBtnW, kBtnH };
}

juce::Rectangle<int> StepGridSingle::incBtnRect() const noexcept
{
    const auto d = decBtnRect();
    return d.translated (kBtnW + 2, 0);
}

juce::Rectangle<int> StepGridSingle::countLblRect() const noexcept
{
    const auto a = ctrlArea();
    return { a.getX(), a.getY() + kBtnH + 3, kCtrlW, a.getHeight() - kBtnH - 3 };
}

juce::Rectangle<int> StepGridSingle::stepRect (int idx) const noexcept
{
    if (m_pattern == nullptr || idx < 0 || idx >= m_pattern->activeStepCount())
        return {};

    const int spr = stepsPerRow();
    const int sw  = stepW();
    const int sh  = stepH();
    const int row = idx / spr;
    const int col = idx % spr;
    const auto ga = gridArea();

    const int x = ga.getX() + col * (sw + kGap);
    const int y = ga.getY() + row * (sh + kGap);
    return { x, y, sw, sh };
}

int StepGridSingle::stepAtPos (juce::Point<int> pos) const noexcept
{
    if (m_pattern == nullptr) return -1;
    const int n   = m_pattern->activeStepCount();
    const int spr = stepsPerRow();
    const int sw  = stepW();
    const int sh  = stepH();
    const auto ga = gridArea();

    if (!ga.contains (pos)) return -1;

    const int col = (pos.x - ga.getX()) / (sw + kGap);
    if (col < 0 || col >= spr) return -1;

    // Check not in column gap
    if ((pos.x - ga.getX()) - col * (sw + kGap) >= sw) return -1;

    const int row = (pos.y - ga.getY()) / (sh + kGap);
    const int nr  = numRows();
    if (row < 0 || row >= nr) return -1;

    // Check not in row gap
    if ((pos.y - ga.getY()) - row * (sh + kGap) >= sh) return -1;

    const int idx = row * spr + col;
    return (idx < n) ? idx : -1;
}

//==============================================================================
// Paint
//==============================================================================

void StepGridSingle::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    if (m_pattern == nullptr)
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("click a slot to edit steps",
                     getLocalBounds(), juce::Justification::centred, false);
        return;
    }

    paintGrid     (g);
    paintDividers (g);
    paintCtrl     (g);
}

void StepGridSingle::paintGrid (juce::Graphics& g) const
{
    const int n    = m_pattern->activeStepCount();
    const int sw   = stepW();
    const int sh   = stepH();
    const int spr  = stepsPerRow();

    for (int i = 0; i < n; ++i)
    {
        const auto r = stepRect (i);
        if (r.isEmpty()) continue;

        const bool active   = m_pattern->stepActive   (i);
        const bool accented = m_pattern->stepAccented (i);

        juce::Colour col;
        if (active)
            col = accented ? m_groupColor.brighter (0.25f) : m_groupColor;
        else
            col = Theme::Colour::surface3;

        g.setColour (col);
        g.fillRect  (r);

        // Playhead marker
        if (i == m_playhead)
        {
            g.setColour (Theme::Colour::error);
            g.fillRect  (r.withWidth (1));
        }
    }

    juce::ignoreUnused (sw, sh, spr);
}

void StepGridSingle::paintDividers (juce::Graphics& g) const
{
    if (m_pattern == nullptr) return;

    const int n   = m_pattern->activeStepCount();
    const int spr = stepsPerRow();
    const int sw  = stepW();
    const int sh  = stepH();
    const int nr  = numRows();
    const auto ga = gridArea();

    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.6f));

    for (int row = 0; row < nr; ++row)
    {
        const int rowY = ga.getY() + row * (sh + kGap);

        for (int beat = 1; beat * 4 < spr; ++beat)
        {
            const int col = beat * 4;
            if (col >= n) break;
            const int lineX = ga.getX() + col * (sw + kGap) - kGap;
            g.fillRect (lineX, rowY, 1, sh);
        }
    }
}

void StepGridSingle::paintCtrl (juce::Graphics& g) const
{
    // Buttons bar
    const auto decR = decBtnRect();
    const auto incR = incBtnRect();

    // "-" button
    g.setColour (Theme::Colour::surface2);
    g.fillRect  (decR);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRect  (decR, 1);
    g.setFont   (Theme::Font::heading());
    g.setColour (Theme::Colour::inkMid);
    g.drawText  ("-", decR, juce::Justification::centred, false);

    // "+" button
    g.setColour (Theme::Colour::surface2);
    g.fillRect  (incR);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRect  (incR, 1);
    g.setFont   (Theme::Font::heading());
    g.setColour (Theme::Colour::inkMid);
    g.drawText  ("+", incR, juce::Justification::centred, false);

    // Count label
    g.setFont   (Theme::Font::value());
    g.setColour (Theme::Colour::inkLight);
    g.drawText  ("[" + juce::String (m_pattern->activeStepCount()) + "]",
                 countLblRect(), juce::Justification::centredTop, false);

    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  ("steps",
                 countLblRect().withTrimmedTop (12), juce::Justification::centredTop, false);
}

//==============================================================================
// Mouse
//==============================================================================

void StepGridSingle::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // Step count buttons
    if (m_pattern != nullptr)
    {
        if (decBtnRect().contains (pos))
        {
            m_pattern->setStepCount (m_pattern->activeStepCount() - 1);
            repaint();
            if (onModified) onModified();
            return;
        }
        if (incBtnRect().contains (pos))
        {
            m_pattern->setStepCount (m_pattern->activeStepCount() + 1);
            repaint();
            if (onModified) onModified();
            return;
        }
    }

    // Step toggle
    const int idx = stepAtPos (pos);
    if (idx >= 0 && m_pattern != nullptr)
    {
        m_paintMode       = !m_pattern->stepActive (idx);
        m_lastPaintedStep = idx;
        m_pattern->toggleStep (idx);
        repaint();
        if (onModified) onModified();
    }
}

void StepGridSingle::mouseDrag (const juce::MouseEvent& e)
{
    if (m_pattern == nullptr) return;
    const int idx = stepAtPos (e.getPosition());
    if (idx >= 0 && idx != m_lastPaintedStep)
    {
        m_lastPaintedStep = idx;
        if (m_pattern->stepActive (idx) != m_paintMode)
        {
            m_pattern->toggleStep (idx);
            repaint();
            if (onModified) onModified();
        }
    }
}

void StepGridSingle::mouseUp (const juce::MouseEvent&)
{
    m_lastPaintedStep = -1;
}

juce::MouseCursor StepGridSingle::getMouseCursor()
{
    return juce::MouseCursor::CrosshairCursor;
}

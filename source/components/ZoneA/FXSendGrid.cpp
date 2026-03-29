#include "FXSendGrid.h"

namespace
{
static const char* kColLabels[] = { "BUS A", "BUS B", "BUS C", "BUS D", "MASTER" };
} // namespace

//==============================================================================
const char* FXSendGrid::colLabel (int col)
{
    if (col >= 0 && col < kCols) return kColLabels[col];
    return "";
}

//==============================================================================
FXSendGrid::FXSendGrid()
{
    m_matrix = FXSendMatrix::makeDefault();
    setRepaintsOnMouseActivity (false);
}

FXSendGrid::~FXSendGrid()
{
    stopTimer();
}

//==============================================================================
void FXSendGrid::setSlotNames (const juce::StringArray& names)
{
    m_slotNames = names;
    m_numRows = juce::jmin (names.size(), kFXSlots);
    repaint();
}

void FXSendGrid::setMatrix (const FXSendMatrix& matrix)
{
    m_matrix = matrix;
    repaint();
}

void FXSendGrid::setAudioTick (const std::atomic<uint32_t>* tick)
{
    m_audioTickPtr = tick;
    if (tick)
        m_lastAudioTick = tick->load (std::memory_order_relaxed);

    if (isVisible() && tick)
        startTimerHz (30);
    else
        stopTimer();
}

void FXSendGrid::visibilityChanged()
{
    if (isVisible() && m_audioTickPtr)
        startTimerHz (30);
    else
        stopTimer();
}

//==============================================================================
void FXSendGrid::timerCallback()
{
    if (!m_audioTickPtr) return;

    const uint32_t tick = m_audioTickPtr->load (std::memory_order_relaxed);
    bool needRepaint = false;

    if (tick != m_lastAudioTick)
    {
        m_lastAudioTick = tick;
        // Pulse all enabled cells
        for (int s = 0; s < m_numRows; ++s)
            for (int t = 0; t < kCols; ++t)
                if (m_matrix.enabled[s][t])
                    m_cellFlash[s][t] = 1.0f;
        needRepaint = true;
    }

    // Decay
    for (int s = 0; s < kFXSlots; ++s)
    {
        for (int t = 0; t < kCols; ++t)
        {
            if (m_cellFlash[s][t] > 0.0f)
            {
                m_cellFlash[s][t] = juce::jmax (0.0f, m_cellFlash[s][t] - kFlashDecay);
                needRepaint = true;
            }
        }
    }

    if (needRepaint)
        repaint();
}

//==============================================================================
int FXSendGrid::cellW() const noexcept
{
    const int avail = getWidth() - kLabelW - (kCols - 1) * kCellGap;
    return juce::jmax (kMinCellW, avail / kCols);
}

int FXSendGrid::cellH() const noexcept
{
    const int rows = juce::jmax (1, m_numRows);
    const int avail = getHeight() - kHeaderH - (rows - 1) * kCellGap;
    return juce::jmax (kMinCellH, avail / rows);
}

juce::Rectangle<int> FXSendGrid::cellRect (int row, int col) const noexcept
{
    const int cw = cellW();
    const int ch = cellH();
    const int x  = kLabelW + col * (cw + kCellGap);
    const int y  = kHeaderH + row * (ch + kCellGap);
    return { x, y, cw, ch };
}

juce::Rectangle<int> FXSendGrid::headerRect (int col) const noexcept
{
    const int cw = cellW();
    const int x  = kLabelW + col * (cw + kCellGap);
    return { x, 0, cw, kHeaderH };
}

std::pair<int,int> FXSendGrid::hitTest (juce::Point<int> p) const noexcept
{
    // Header zone
    if (p.y < kHeaderH && p.x >= kLabelW)
    {
        const int cw  = cellW();
        const int col = (p.x - kLabelW) / (cw + kCellGap);
        if (col >= 0 && col < kCols && headerRect (col).contains (p))
            return { -1, col };
        return { -1, -1 };
    }

    if (p.x < kLabelW || p.y < kHeaderH)
        return { -1, -1 };

    const int cw  = cellW();
    const int ch  = cellH();
    const int col = (p.x - kLabelW) / (cw + kCellGap);
    const int row = (p.y - kHeaderH) / (ch + kCellGap);

    if (col < 0 || col >= kCols || row < 0 || row >= m_numRows)
        return { -1, -1 };

    if (!cellRect (row, col).contains (p))
        return { -1, -1 };

    return { row, col };
}

//==============================================================================
void FXSendGrid::paint (juce::Graphics& g)
{
    const auto& theme  = ThemeManager::get().theme();
    const auto  accent = ZoneAStyle::accentForTabId ("routing");
    const auto  font   = Theme::Font::micro();

    g.fillAll (Theme::Zone::bgA);

    const int cw = cellW();
    const float radius = juce::jmin (3.0f, theme.sliderCornerRadius);

    // --- Column header labels ---
    g.setFont (font);
    for (int c = 0; c < kCols; ++c)
    {
        const auto  hr = headerRect (c);
        const bool  isHover = (m_hoverRow == -1 && m_hoverCol == c);
        auto        col = juce::Colour (Theme::Colour::inkGhost);
        if (isHover) col = col.brighter (0.25f);
        g.setColour (col);
        g.drawFittedText (colLabel (c), hr, juce::Justification::centred, 1);
    }

    // --- Row labels ---
    for (int r = 0; r < m_numRows; ++r)
    {
        const int ch = cellH();
        const auto labelR = juce::Rectangle<int> (0, kHeaderH + r * (ch + kCellGap), kLabelW - kCellGap, ch);
        const bool isHover = (m_hoverRow == r);
        auto col = juce::Colour (Theme::Colour::inkGhost);
        if (isHover) col = col.brighter (0.2f);
        g.setColour (col);
        const auto& name = (r < m_slotNames.size()) ? m_slotNames[r] : juce::String ("SLOT " + juce::String (r + 1));
        g.drawFittedText (name, labelR, juce::Justification::centredRight, 1);
    }

    // --- Cells ---
    for (int r = 0; r < m_numRows; ++r)
    {
        for (int c = 0; c < kCols; ++c)
        {
            const auto  cr      = cellRect (r, c).toFloat();
            const bool  active  = m_matrix.enabled[r][c];
            const bool  hovering = (r == m_hoverRow && c == m_hoverCol);
            const float flash   = m_cellFlash[r][c];

            if (active)
            {
                const float lv = juce::jlimit (0.0f, 1.0f, m_matrix.level[r][c]);
                // Brightness encodes level: min alpha 0.40, max 0.95
                const float alpha = 0.40f + lv * 0.55f;
                auto fillCol = accent.withAlpha (alpha);
                if (flash > 0.0f) fillCol = fillCol.brighter (flash * 0.45f);
                if (hovering)     fillCol = fillCol.brighter (0.15f);
                g.setColour (fillCol);
                g.fillRoundedRectangle (cr, radius);

                // Level text (small, inside cell)
                if (cr.getWidth() >= 28)
                {
                    g.setFont (font);
                    g.setColour (Theme::Helper::inkFor (fillCol).withAlpha (0.9f));
                    const juce::String lvStr = juce::String (juce::roundToInt (lv * 100)) + "%";
                    g.drawFittedText (lvStr, cellRect (r, c), juce::Justification::centred, 1);
                }
            }
            else
            {
                auto bgCol = theme.controlBg.withAlpha (hovering ? 0.5f : 0.30f);
                if (flash > 0.0f)
                    bgCol = bgCol.interpolatedWith (accent.withAlpha (0.20f), flash);
                g.setColour (bgCol);
                g.fillRoundedRectangle (cr, radius);
                g.setColour (theme.surfaceEdge.withAlpha (hovering ? 0.55f : 0.28f));
                g.drawRoundedRectangle (cr.reduced (0.5f), radius, 1.0f);
            }
        }
    }

    // --- Divider before MASTER column (col 4) ---
    if constexpr (kCols > 1)
    {
        const int divX = kLabelW + (kCols - 1) * (cw + kCellGap) - (kCellGap / 2 + 1);
        g.setColour (theme.surfaceEdge.withAlpha (0.4f));
        const int divTop = kHeaderH;
        const int divBot = kHeaderH + m_numRows * cellH() + (m_numRows - 1) * kCellGap;
        g.drawLine ((float) divX, (float) divTop, (float) divX, (float) divBot, 1.0f);
    }
}

//==============================================================================
void FXSendGrid::mouseDown (const juce::MouseEvent& e)
{
    const auto [row, col] = hitTest (e.getPosition());

    if (row == -1 && col >= 0)
    {
        // Header tap -> fire bus-selected callback
        if (onBusSelected)
            onBusSelected (juce::String (FXBusID::fromColumn (col)));
        return;
    }

    if (row < 0 || col < 0) return;

    if (m_matrix.enabled[row][col])
    {
        // Already enabled — start a level drag
        m_dragRow        = row;
        m_dragCol        = col;
        m_dragStartLevel = m_matrix.level[row][col];
        m_dragStartY     = e.getPosition().y;
    }
    else
    {
        // Enable the send at level 1.0
        m_matrix.enabled[row][col] = true;
        m_matrix.level  [row][col] = 1.0f;
        if (onMatrixChanged) onMatrixChanged (m_matrix);
        repaint();
    }
}

void FXSendGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (m_dragRow < 0 || m_dragCol < 0) return;

    // Vertical drag: up = increase level, down = decrease
    const int   dy       = m_dragStartY - e.getPosition().y;
    const float range    = (float) juce::jmax (1, getHeight());
    const float delta    = (float) dy / range * 2.0f;   // full height = 2x range for fine control
    const float newLevel = juce::jlimit (0.0f, 1.0f, m_dragStartLevel + delta);
    m_matrix.level[m_dragRow][m_dragCol] = newLevel;

    if (onMatrixChanged) onMatrixChanged (m_matrix);
    repaint();
}

void FXSendGrid::mouseMove (const juce::MouseEvent& e)
{
    const auto [row, col] = hitTest (e.getPosition());
    if (row != m_hoverRow || col != m_hoverCol)
    {
        m_hoverRow = row;
        m_hoverCol = col;
        repaint();
    }
}

void FXSendGrid::mouseExit (const juce::MouseEvent&)
{
    m_dragRow  = -1;
    m_dragCol  = -1;
    m_hoverRow = -1;
    m_hoverCol = -1;
    repaint();
}

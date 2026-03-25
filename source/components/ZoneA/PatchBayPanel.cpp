#include "PatchBayPanel.h"

//==============================================================================
PatchBayPanel::PatchBayPanel()
{
    // Default module names — replaced when Zone B reports its module list
    m_names.add ("MIX");
    m_names.add ("OUT");
}

void PatchBayPanel::setModuleNames (const juce::StringArray& names)
{
    m_names = names;
    // Clear connections when topology changes
    for (int r = 0; r < kMaxModules; ++r)
        for (int c = 0; c < kMaxModules; ++c)
            m_conn[r][c] = false;
    repaint();
}

//==============================================================================
juce::Rectangle<int> PatchBayPanel::cellRect (int row, int col) const noexcept
{
    return { kLabelW + col * kCellSize,
             kHeaderH + row * kCellSize,
             kCellSize, kCellSize };
}

//==============================================================================
void PatchBayPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface0);
    paintHeader (g);
    paintGrid   (g);
}

void PatchBayPanel::paintHeader (juce::Graphics& g) const
{
    // Background
    g.setColour (Theme::Colour::surface1);
    g.fillRect  (0, 0, getWidth(), kHeaderH);

    // Title
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkMid);
    g.drawText  ("PATCH BAY", 8, 0, kLabelW - 8, kHeaderH,
                 juce::Justification::centredLeft, false);

    // Column headers (rotated — label reads bottom → top)
    const int n = moduleCount();
    for (int col = 0; col < n; ++col)
    {
        const float cx = static_cast<float> (kLabelW + col * kCellSize + kCellSize / 2);
        const float cy = static_cast<float> (kHeaderH / 2);

        juce::Graphics::ScopedSaveState saved (g);
        g.addTransform (juce::AffineTransform::rotation (
            -juce::MathConstants<float>::halfPi, cx, cy));

        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  (m_names[col],
                     juce::Rectangle<float> (cx - kHeaderH * 0.5f,
                                             cy - kCellSize * 0.5f,
                                             static_cast<float> (kHeaderH),
                                             static_cast<float> (kCellSize))
                         .toNearestInt(),
                     juce::Justification::centred, false);
    }

    // Divider
    g.setColour (Theme::Colour::surfaceEdge);
    g.fillRect  (0, kHeaderH - 1, getWidth(), 1);
}

void PatchBayPanel::paintGrid (juce::Graphics& g) const
{
    const int n = moduleCount();

    for (int row = 0; row < n; ++row)
    {
        // Row label
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMid);
        g.drawText  (m_names[row],
                     2, kHeaderH + row * kCellSize,
                     kLabelW - 4, kCellSize,
                     juce::Justification::centredLeft, false);

        // Cells
        for (int col = 0; col < n; ++col)
        {
            const auto cell = cellRect (row, col);

            if (row == col)
            {
                // Diagonal — no self-routing
                g.setColour (Theme::Colour::surface1);
                g.fillRect  (cell.reduced (1));
                continue;
            }

            // Cell background
            g.setColour (m_conn[row][col]
                             ? Theme::Zone::a.withAlpha (0.18f)
                             : Theme::Colour::surface1);
            g.fillRect (cell.reduced (1));

            // Connection indicator
            if (m_conn[row][col])
            {
                g.setColour (Theme::Zone::a);
                g.fillEllipse (cell.reduced (5).toFloat());
            }

            // Cell border
            g.setColour (Theme::Colour::surfaceEdge);
            g.drawRect  (cell.reduced (1));
        }
    }
}

//==============================================================================
void PatchBayPanel::mouseDown (const juce::MouseEvent& e)
{
    const int n   = moduleCount();
    const auto pos = e.getPosition();

    for (int row = 0; row < n; ++row)
    {
        for (int col = 0; col < n; ++col)
        {
            if (row == col) continue;
            if (cellRect (row, col).contains (pos))
            {
                m_conn[row][col] = !m_conn[row][col];
                if (onConnectionChanged)
                    onConnectionChanged (row, col, m_conn[row][col]);
                repaint();
                return;
            }
        }
    }
}

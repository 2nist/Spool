#include "MidiRoutingGrid.h"

namespace
{
static const char* kSourceNames[]  = { "Keyboard In", "Sequencer", "Midi FX" };
static const char* kSourceLabels[] = { "KBD", "SEQ", "MFX" };
static const char* kDestNames[]    = { "Slot 1", "Slot 2", "Slot 3", "Slot 4",
                                        "Slot 5", "Slot 6", "Slot 7", "Slot 8",
                                        "Drum Rack" };
static const char* kDestLabels[]   = { "S1", "S2", "S3", "S4",
                                        "S5", "S6", "S7", "S8", "DR" };
} // namespace

//==============================================================================
juce::String MidiRoutingGrid::sourceName (int row)
{
    if (row >= 0 && row < kSources) return kSourceNames[row];
    return {};
}

juce::String MidiRoutingGrid::destName (int col)
{
    if (col >= 0 && col < kDests) return kDestNames[col];
    return {};
}

juce::String MidiRoutingGrid::sourceLabel (int row)
{
    if (row >= 0 && row < kSources) return kSourceLabels[row];
    return {};
}

juce::String MidiRoutingGrid::destLabel (int col)
{
    if (col >= 0 && col < kDests) return kDestLabels[col];
    return {};
}

int MidiRoutingGrid::sourceIndex (const juce::String& name)
{
    for (int i = 0; i < kSources; ++i)
        if (name == kSourceNames[i]) return i;
    return -1;
}

int MidiRoutingGrid::destIndex (const juce::String& name)
{
    for (int i = 0; i < kDests; ++i)
        if (name == kDestNames[i]) return i;
    return -1;
}

//==============================================================================
MidiRoutingGrid::MidiRoutingGrid()
{
    setRepaintsOnMouseActivity (false);
}

MidiRoutingGrid::~MidiRoutingGrid()
{
    stopTimer();
}

//==============================================================================
void MidiRoutingGrid::setMidiRouter (const MidiRouter* router)
{
    m_router = router;

    // Reset last-seen ticks so we don't flash on first poll
    if (m_router)
    {
        m_lastInputTick = m_router->getInputTick();
        for (int i = 0; i < MidiRouter::kNumSlots; ++i)
            m_lastDestTicks[i] = m_router->getDestTick (i);
    }

    if (isVisible() && m_router)
        startTimerHz (30);
    else
        stopTimer();
}

void MidiRoutingGrid::visibilityChanged()
{
    if (isVisible() && m_router)
        startTimerHz (30);
    else
        stopTimer();
}

void MidiRoutingGrid::timerCallback()
{
    bool needRepaint = false;

    // --- Input tick → flash source rows that have at least one active cell ---
    const uint32_t inputTick = m_router->getInputTick();
    if (inputTick != m_lastInputTick)
    {
        m_lastInputTick = inputTick;
        for (int r = 0; r < kSources; ++r)
            for (int c = 0; c < kDests; ++c)
                if (m_cells[r][c]) { m_sourceFlash[r] = 1.0f; break; }
        needRepaint = true;
    }

    // --- Dest ticks → flash destination columns (slots 0-7 only) ---
    for (int c = 0; c < MidiRouter::kNumSlots; ++c)
    {
        const uint32_t tick = m_router->getDestTick (c);
        if (tick != m_lastDestTicks[c])
        {
            m_lastDestTicks[c] = tick;
            m_destFlash[c]     = 1.0f;
            needRepaint = true;
        }
    }

    // --- Decay all flash values ---
    for (auto& f : m_sourceFlash)
        if (f > 0.0f) { f = juce::jmax (0.0f, f - kFlashDecay); needRepaint = true; }
    for (auto& f : m_destFlash)
        if (f > 0.0f) { f = juce::jmax (0.0f, f - kFlashDecay); needRepaint = true; }

    if (needRepaint)
        repaint();
}

//==============================================================================
void MidiRoutingGrid::setCell (int row, int col, bool on)
{
    if (row < 0 || row >= kSources || col < 0 || col >= kDests) return;
    m_cells[row][col] = on;
    repaint();
}

bool MidiRoutingGrid::getCell (int row, int col) const noexcept
{
    if (row < 0 || row >= kSources || col < 0 || col >= kDests) return false;
    return m_cells[row][col];
}

void MidiRoutingGrid::clearAll()
{
    for (int r = 0; r < kSources; ++r)
        for (int c = 0; c < kDests; ++c)
            m_cells[r][c] = false;
    repaint();
}

//==============================================================================
int MidiRoutingGrid::cellW() const noexcept
{
    const int avail = getWidth() - kLabelW - (kDests - 1) * kCellGap;
    return juce::jmax (12, avail / kDests);
}

int MidiRoutingGrid::cellH() const noexcept
{
    const int avail = getHeight() - kHeaderH - (kSources - 1) * kCellGap;
    return juce::jmax (14, avail / kSources);
}

juce::Rectangle<int> MidiRoutingGrid::cellRect (int row, int col) const noexcept
{
    const int cw = cellW();
    const int ch = cellH();
    const int x  = kLabelW + col * (cw + kCellGap);
    const int y  = kHeaderH + row * (ch + kCellGap);
    return { x, y, cw, ch };
}

std::pair<int, int> MidiRoutingGrid::cellAtPoint (juce::Point<int> p) const noexcept
{
    if (p.x < kLabelW || p.y < kHeaderH)
        return { -1, -1 };

    const int cw  = cellW();
    const int ch  = cellH();
    const int col = (p.x - kLabelW) / (cw + kCellGap);
    const int row = (p.y - kHeaderH) / (ch + kCellGap);

    if (col >= kDests || row >= kSources)
        return { -1, -1 };

    if (!cellRect (row, col).contains (p))
        return { -1, -1 };

    return { row, col };
}

//==============================================================================
void MidiRoutingGrid::paint (juce::Graphics& g)
{
    const auto& theme  = ThemeManager::get().theme();
    const auto  accent = juce::Colour (Theme::Zone::c);
    const auto  font   = Theme::Font::micro();

    g.fillAll (Theme::Zone::bgA);

    const int cw = cellW();
    const int ch = cellH();

    // --- Destination header labels ---
    g.setFont (font);
    for (int c = 0; c < kDests; ++c)
    {
        const auto r = juce::Rectangle<int> (kLabelW + c * (cw + kCellGap), 0, cw, kHeaderH);

        // Activity flash blends inkGhost → accent, hover pushes brighter
        const bool isHoverCol = (c == m_hoverCol);
        const float flash = (c < MidiRouter::kNumSlots) ? m_destFlash[c] : 0.0f;
        auto col = juce::Colour (Theme::Colour::inkGhost);
        if (flash > 0.0f) col = col.interpolatedWith (accent, flash);
        if (isHoverCol)   col = col.brighter (0.2f);
        g.setColour (col);
        g.drawFittedText (destLabel (c), r, juce::Justification::centred, 1);
    }

    // --- Source row labels ---
    for (int r = 0; r < kSources; ++r)
    {
        const auto labelR = juce::Rectangle<int> (0, kHeaderH + r * (ch + kCellGap), kLabelW - kCellGap, ch);
        const bool isHoverRow = (r == m_hoverRow);
        const float flash = m_sourceFlash[r];
        auto col = juce::Colour (Theme::Colour::inkGhost);
        if (flash > 0.0f) col = col.interpolatedWith (accent, flash);
        if (isHoverRow)   col = col.brighter (0.2f);
        g.setColour (col);
        g.drawFittedText (sourceLabel (r), labelR, juce::Justification::centredRight, 1);
    }

    // --- Cells ---
    const float radius = juce::jmin (3.0f, theme.sliderCornerRadius);

    for (int r = 0; r < kSources; ++r)
    {
        for (int c = 0; c < kDests; ++c)
        {
            const auto  cr       = cellRect (r, c).toFloat();
            const bool  active   = m_cells[r][c];
            const bool  hovering = (r == m_hoverRow && c == m_hoverCol);
            const float dFlash   = (c < MidiRouter::kNumSlots) ? m_destFlash[c] : 0.0f;
            const float sFlash   = m_sourceFlash[r];
            const float cellFlash = juce::jmax (dFlash, sFlash);

            if (active)
            {
                // Active cells brighten with activity, dim on hover only if not active
                auto fillCol = accent.withAlpha (0.85f);
                if (cellFlash > 0.0f) fillCol = fillCol.brighter (cellFlash * 0.5f);
                if (hovering)         fillCol = fillCol.brighter (0.15f);
                g.setColour (fillCol);
                g.fillRoundedRectangle (cr, radius);
            }
            else
            {
                // Inactive cells show a faint activity pulse
                auto bgCol = theme.controlBg.withAlpha (hovering ? 0.5f : 0.35f);
                if (cellFlash > 0.0f)
                    bgCol = bgCol.interpolatedWith (accent.withAlpha (0.25f), cellFlash);
                g.setColour (bgCol);
                g.fillRoundedRectangle (cr, radius);
                g.setColour (theme.surfaceEdge.withAlpha (hovering ? 0.6f : 0.3f));
                g.drawRoundedRectangle (cr.reduced (0.5f), radius, 1.0f);
            }
        }
    }

    // --- Divider before Drum Rack column ---
    const int divX = kLabelW + 8 * (cw + kCellGap) - (kCellGap / 2 + 1);
    g.setColour (theme.surfaceEdge.withAlpha (0.4f));
    g.drawLine ((float) divX, (float) kHeaderH,
                (float) divX, (float) (kHeaderH + kSources * ch + (kSources - 1) * kCellGap),
                1.0f);
}

void MidiRoutingGrid::mouseDown (const juce::MouseEvent& e)
{
    const auto [row, col] = cellAtPoint (e.getPosition());
    if (row < 0) return;
    const bool newState = !m_cells[row][col];
    setCell (row, col, newState);
    if (onCellToggled)
        onCellToggled (row, col, newState);
}

void MidiRoutingGrid::mouseMove (const juce::MouseEvent& e)
{
    const auto [row, col] = cellAtPoint (e.getPosition());
    if (row != m_hoverRow || col != m_hoverCol)
    {
        m_hoverRow = row;
        m_hoverCol = col;
        repaint();
    }
}

void MidiRoutingGrid::mouseExit (const juce::MouseEvent&)
{
    m_hoverRow = -1;
    m_hoverCol = -1;
    repaint();
}

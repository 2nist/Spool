#pragma once

#include "../../source/Theme.h"

//==============================================================================
/**
    ZoneResizer — a thin drag handle placed on the boundary between zones.

    Placed on zone borders by PluginEditor. The component is deliberately thin
    (6px) and subtle so it doesn't intrude on zone content. On hover the dots
    become slightly brighter. Double-click collapses the zone to its minimum
    and double-clicking again restores the previous size.
*/
class ZoneResizer  : public juce::Component
{
public:
    enum class Direction { horizontal, vertical };

    ZoneResizer (Direction dir) : m_dir (dir) {}

    //--------------------------------------------------------------------------
    void paint (juce::Graphics& g) override
    {
        const juce::Colour bg   = m_hovered ? Theme::Colour::surface2 : Theme::Colour::surface0;
        const juce::Colour dots = m_hovered ? Theme::Colour::inkMuted  : Theme::Colour::inkGhost;

        g.fillAll (bg);
        g.setColour (dots);

        const float cx = static_cast<float> (getWidth())  * 0.5f;
        const float cy = static_cast<float> (getHeight()) * 0.5f;

        if (m_dir == Direction::horizontal)
        {
            for (int i = -1; i <= 1; ++i)
                g.fillEllipse (cx - 2.0f, cy + static_cast<float> (i) * 5.0f - 2.0f, 4.0f, 4.0f);
        }
        else
        {
            for (int i = -1; i <= 1; ++i)
                g.fillEllipse (cx + static_cast<float> (i) * 5.0f - 2.0f, cy - 2.0f, 4.0f, 4.0f);
        }
    }

    //--------------------------------------------------------------------------
    void mouseEnter (const juce::MouseEvent&) override
    {
        setMouseCursor (m_dir == Direction::horizontal
            ? juce::MouseCursor::LeftRightResizeCursor
            : juce::MouseCursor::UpDownResizeCursor);
        m_hovered = true;
        repaint();
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        m_hovered = false;
        repaint();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        m_dragStart  = (m_dir == Direction::horizontal) ? e.getScreenX() : e.getScreenY();
        m_startSize  = m_currentSize;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        const int rawDelta = (m_dir == Direction::horizontal)
            ? e.getScreenX() - m_dragStart
            : e.getScreenY() - m_dragStart;
        const int delta = m_invertDrag ? -rawDelta : rawDelta;

        m_currentSize = juce::jlimit (m_minSize, m_maxSize, m_startSize + delta);

        if (onResize)
            onResize (m_currentSize);
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        if (m_currentSize > m_minSize + 10)
        {
            m_savedSize   = m_currentSize;
            m_currentSize = m_minSize;
        }
        else
        {
            m_currentSize = (m_savedSize > m_minSize) ? m_savedSize : m_defaultSize;
        }

        if (onResize)
            onResize (m_currentSize);
    }

    //--------------------------------------------------------------------------
    /** Called whenever the drag produces a new size. */
    std::function<void (int newSize)> onResize;

    void setConstraints (int minSize, int maxSize, int defaultSize)
    {
        m_minSize     = minSize;
        m_maxSize     = maxSize;
        m_defaultSize = defaultSize;
        m_currentSize = defaultSize;
    }

    /** Invert drag direction — use for resizers on the top edge of a bottom-anchored zone. */
    void setInvertDrag (bool invert) noexcept { m_invertDrag = invert; }

    int getCurrentSize() const noexcept { return m_currentSize; }

private:
    Direction m_dir;
    bool      m_hovered     = false;
    bool      m_invertDrag  = false;
    int       m_dragStart   = 0;
    int       m_startSize   = 0;
    int       m_savedSize   = 0;
    int       m_currentSize = 0;
    int       m_minSize     = 40;
    int       m_maxSize     = 600;
    int       m_defaultSize = 160;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZoneResizer)
};

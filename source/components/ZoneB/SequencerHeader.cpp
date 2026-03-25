#include "SequencerHeader.h"

//==============================================================================

SequencerHeader::SequencerHeader()
{
    setInterceptsMouseClicks (true, false);
}

//==============================================================================
// State
//==============================================================================

void SequencerHeader::setFocusedSlot (const juce::String& slotName,
                                       const juce::String& groupName,
                                       juce::Colour        groupColor)
{
    m_slotName  = slotName;
    m_groupName = groupName;
    m_groupColor = groupColor;
    m_hasFocus  = true;
    repaint();
}

void SequencerHeader::clearFocus()
{
    m_slotName   = {};
    m_groupName  = {};
    m_groupColor = juce::Colour (0x00000000);
    m_hasFocus   = false;
    m_pattern    = nullptr;
    repaint();
}

void SequencerHeader::setPattern (SlotPattern* pattern)
{
    m_pattern = pattern;
    repaint();
}

void SequencerHeader::setClipboard (const SlotPattern* /*cb*/, bool hasData)
{
    m_hasClipboard = hasData;
    repaint();
}

//==============================================================================
// Region helpers
//==============================================================================

juce::Rectangle<int> SequencerHeader::dotRect() const noexcept
{
    return { kPad, (kHeight - 6) / 2, 6, 6 };
}

juce::Rectangle<int> SequencerHeader::labelRect() const noexcept
{
    const int x = dotRect().getRight() + 4;
    const int w = getWidth() / 2 - x;
    return { x, 0, w, kHeight };
}

// Pattern selector centred in the remaining right half
juce::Rectangle<int> SequencerHeader::patPrevRect() const noexcept
{
    const int cx = getWidth() / 2;
    return { cx - kPatW / 2, 0, kArrowW, kHeight };
}

juce::Rectangle<int> SequencerHeader::patLabelRect() const noexcept
{
    const int cx = getWidth() / 2;
    return { cx - kPatW / 2 + kArrowW, 0, kPatW - 2 * kArrowW, kHeight };
}

juce::Rectangle<int> SequencerHeader::patNextRect() const noexcept
{
    const int cx = getWidth() / 2;
    return { cx + kPatW / 2 - kArrowW, 0, kArrowW, kHeight };
}

juce::Rectangle<int> SequencerHeader::clearRect() const noexcept
{
    return { getWidth() - kPad - kBtnW, 0, kBtnW, kHeight };
}

juce::Rectangle<int> SequencerHeader::pasteRect() const noexcept
{
    return clearRect().translated (-(kBtnW + 2), 0);
}

juce::Rectangle<int> SequencerHeader::copyRect() const noexcept
{
    return pasteRect().translated (-(kBtnW + 2), 0);
}

//==============================================================================
// Paint
//==============================================================================

void SequencerHeader::paint (juce::Graphics& g)
{
    g.setColour (Theme::Colour::surface0);
    g.fillAll();

    if (m_hasFocus)
    {
        // Colour dot
        g.setColour (m_groupColor.isTransparent() ? (juce::Colour) Theme::Colour::accent : m_groupColor);
        g.fillEllipse (dotRect().toFloat());

        // "SLOT · GROUP" label
        const juce::String label = m_slotName
            + (m_groupName.isNotEmpty() ? juce::String (" \xc2\xb7 ") + m_groupName : juce::String());
        g.setFont   (Theme::Font::label());
        g.setColour (Theme::Colour::inkLight);
        g.drawText  (label, labelRect(), juce::Justification::centredLeft, true);
    }
    else
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("click a slot to focus", labelRect(), juce::Justification::centredLeft, false);
    }

    // Pattern selector (only when pattern is active)
    if (m_pattern != nullptr)
    {
        const int pat = m_pattern->currentPattern;

        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("<", patPrevRect(), juce::Justification::centred, false);

        g.setFont   (Theme::Font::value());
        g.setColour (Theme::Colour::inkMid);
        g.drawText  ("PAT " + juce::String (pat + 1),
                     patLabelRect(), juce::Justification::centred, false);

        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  (">", patNextRect(), juce::Justification::centred, false);
    }

    // Action buttons
    g.setFont (Theme::Font::micro());

    auto paintBtn = [&] (juce::Rectangle<int> r, const juce::String& label, bool enabled)
    {
        g.setColour (enabled ? (juce::Colour) Theme::Colour::inkMid : (juce::Colour) Theme::Colour::inkGhost);
        g.drawText (label, r, juce::Justification::centred, false);
    };

    paintBtn (copyRect(),  "COPY",  m_hasFocus);
    paintBtn (pasteRect(), "PASTE", m_hasFocus && m_hasClipboard);
    paintBtn (clearRect(), "CLR",   m_hasFocus);

    // Bottom border
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine (0.0f, static_cast<float> (kHeight - 1),
                static_cast<float> (getWidth()), static_cast<float> (kHeight - 1), 0.7f);
}

//==============================================================================
// Mouse
//==============================================================================

void SequencerHeader::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    if (m_pattern != nullptr)
    {
        if (patPrevRect().contains (pos))
        {
            m_pattern->currentPattern = (m_pattern->currentPattern - 1 + SlotPattern::NUM_PATTERNS)
                                        % SlotPattern::NUM_PATTERNS;
            repaint();
            if (onPatternChanged) onPatternChanged (m_pattern->currentPattern);
            return;
        }

        if (patNextRect().contains (pos))
        {
            m_pattern->currentPattern = (m_pattern->currentPattern + 1) % SlotPattern::NUM_PATTERNS;
            repaint();
            if (onPatternChanged) onPatternChanged (m_pattern->currentPattern);
            return;
        }
    }

    if (copyRect().contains (pos) && m_hasFocus)
    {
        if (onCopy) onCopy();
        return;
    }

    if (pasteRect().contains (pos) && m_hasFocus && m_hasClipboard)
    {
        if (onPaste) onPaste();
        return;
    }

    if (clearRect().contains (pos) && m_hasFocus)
    {
        if (onClear) onClear();
        return;
    }
}

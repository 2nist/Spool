#include "LooperUnit.h"

//==============================================================================

LooperUnit::LooperUnit()
{
    m_sourceNames.add ("MIX");
}

//==============================================================================

void LooperUnit::setSourceNames (const juce::StringArray& names)
{
    m_sourceNames = names;
    if (!m_sourceNames.contains ("MIX"))
        m_sourceNames.insert (0, "MIX");
    m_selectedSource = 0;
    repaint();
}

//==============================================================================
// Region helpers
//==============================================================================

juce::Rectangle<int> LooperUnit::ctrlRect() const noexcept
{
    return { 0, 0, kCtrlW, getHeight() };
}

juce::Rectangle<int> LooperUnit::waveformRect() const noexcept
{
    return { kCtrlW, 0, getWidth() - kCtrlW - kHandleW, getHeight() };
}

juce::Rectangle<int> LooperUnit::handleRect() const noexcept
{
    return { getWidth() - kHandleW, 0, kHandleW, getHeight() };
}

juce::Rectangle<int> LooperUnit::sourceDropRect() const noexcept
{
    return { 4, 2, kCtrlW - 8, 14 };
}

juce::Rectangle<int> LooperUnit::modeToggleRect() const noexcept
{
    return { 4, 18, kCtrlW - 8, 12 };
}

juce::Rectangle<int> LooperUnit::recBtnRect() const noexcept
{
    return { 4, 32, kBtnSz, kBtnSz };
}

juce::Rectangle<int> LooperUnit::playBtnRect() const noexcept
{
    return { 4 + kBtnSz + 2, 32, kBtnSz, kBtnSz };
}

juce::Rectangle<int> LooperUnit::dubBtnRect() const noexcept
{
    return { 4 + 2 * (kBtnSz + 2), 32, kBtnSz, kBtnSz };
}

juce::Rectangle<int> LooperUnit::clearBtnRect() const noexcept
{
    return { 4 + 3 * (kBtnSz + 2), 32, kBtnSz, kBtnSz };
}

//==============================================================================
// Layout
//==============================================================================

void LooperUnit::resized() {}

//==============================================================================
// Paint
//==============================================================================

void LooperUnit::paint (juce::Graphics& g)
{
    // Background (surface2 = slightly lighter than surface1, visible against zone bgB)
    g.setColour (Theme::Colour::surface2);
    g.fillRect  (getLocalBounds());

    // Left border accent
    g.setColour (Theme::Zone::b);
    g.fillRect  (0, 0, 2, getHeight());

    paintCtrl     (g);
    paintWaveform (g);
    paintHandle   (g);

    // Divider between ctrl and waveform
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine  (static_cast<float> (kCtrlW), 0.0f,
                 static_cast<float> (kCtrlW), static_cast<float> (getHeight()), 0.7f);
}

void LooperUnit::paintCtrl (juce::Graphics& g) const
{
    // SOURCE label
    {
        const auto r = sourceDropRect();
        g.setColour (Theme::Colour::surface3);
        g.fillRect  (r);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRect  (r, 1);

        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMid);
        const juce::String src = (m_selectedSource < m_sourceNames.size())
                                 ? m_sourceNames[m_selectedSource] : "MIX";
        g.drawText  ("SRC: " + src, r.withTrimmedLeft (3), juce::Justification::centredLeft, true);
    }

    // MODE toggle
    {
        const auto r = modeToggleRect();
        g.setColour (Theme::Colour::surface3);
        g.fillRect  (r);
        g.setColour (m_freeMode ? (juce::Colour) Theme::Colour::accentWarm
                                : (juce::Colour) Theme::Colour::accent);
        g.drawRect  (r, 1);
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkLight);
        g.drawText  (m_freeMode ? "FREE" : "QUANTIZED", r, juce::Justification::centred, false);
    }

    // Transport buttons: ● ▶ ⊕ ⊗
    struct BtnSpec { juce::Rectangle<int> rect; const char* label; juce::Colour col; bool state; };
    const BtnSpec btns[] =
    {
        { recBtnRect(),   "\xe2\x97\x8f",  Theme::Colour::error,         m_recording },
        { playBtnRect(),  "\xe2\x96\xb6",  Theme::Colour::accent,        m_playing   },
        { dubBtnRect(),   "\xe2\x8a\x95",  Theme::Colour::accentWarm,    m_dubbing   },
        { clearBtnRect(), "\xe2\x8a\x97",  Theme::Colour::inkGhost,      false       },
    };

    for (const auto& b : btns)
    {
        g.setColour (b.state ? b.col : (juce::Colour) Theme::Colour::surface3);
        g.fillRect  (b.rect);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRect  (b.rect, 1);
        g.setFont   (Theme::Font::label());
        g.setColour (b.state ? (juce::Colour) Theme::Colour::inkDark : b.col);
        g.drawText  (b.label, b.rect, juce::Justification::centred, false);
    }
}

void LooperUnit::paintWaveform (juce::Graphics& g) const
{
    const auto r = waveformRect();

    // Background
    g.setColour (Theme::Colour::surface0);
    g.fillRect  (r);

    // Empty state label  — TODO: DSP session will replace with actual waveform
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  ("HOLD REC TO CAPTURE", r, juce::Justification::centred, false);
}

void LooperUnit::paintHandle (juce::Graphics& g) const
{
    const auto r = handleRect();

    g.setColour (m_handleHovered ? (juce::Colour) Theme::Colour::inkMid
                                 : (juce::Colour) Theme::Colour::inkGhost);
    g.fillRect  (r);

    // "···" drag dots
    const float dotD  = 3.0f;
    const float dotGap = 4.0f;
    const float startX = r.getCentreX() - dotD * 0.5f;
    const float startY = r.getCentreY() - (3 * dotD + 2 * dotGap) * 0.5f;

    g.setColour (Theme::Colour::inkMid);
    for (int i = 0; i < 3; ++i)
        g.fillEllipse (startX, startY + i * (dotD + dotGap), dotD, dotD);
}

//==============================================================================
// Mouse
//==============================================================================

void LooperUnit::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    if (sourceDropRect().contains (pos))
    {
        juce::PopupMenu menu;
        for (int i = 0; i < m_sourceNames.size(); ++i)
            menu.addItem (i + 1, m_sourceNames[i]);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
            [this] (int result)
            {
                if (result > 0)
                {
                    m_selectedSource = result - 1;
                    repaint();
                }
            });
        return;
    }

    if (modeToggleRect().contains (pos))
    {
        m_freeMode = !m_freeMode;
        repaint();
        return;
    }

    if (recBtnRect().contains (pos))
    {
        m_recording = !m_recording;
        // TODO: DSP session — start/stop recording
        repaint();
        return;
    }

    if (playBtnRect().contains (pos))
    {
        m_playing = !m_playing;
        // TODO: DSP session — start/stop playback
        repaint();
        return;
    }

    if (dubBtnRect().contains (pos))
    {
        m_dubbing = !m_dubbing;
        // TODO: DSP session — dub mode
        repaint();
        return;
    }

    if (clearBtnRect().contains (pos))
    {
        m_recording = false;
        m_playing   = false;
        m_dubbing   = false;
        // TODO: DSP session — clear loop buffer
        repaint();
        return;
    }

    if (handleRect().contains (pos))
    {
        // TODO: DSP session — DragAndDropContainer drag to Zone D for clip creation
    }
}

juce::MouseCursor LooperUnit::getMouseCursor()
{
    // TODO: DSP session — use DragCopyCursor when hovering handle
    return juce::MouseCursor::NormalCursor;
}

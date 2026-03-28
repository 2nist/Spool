#include "OutputStrip.h"

//==============================================================================

OutputStrip::OutputStrip()
{
    setInterceptsMouseClicks (true, false);
}

//==============================================================================
// Setters
//==============================================================================

void OutputStrip::setLevel (float v)
{
    m_level = juce::jlimit (0.0f, 1.0f, v);
    repaint();
}

void OutputStrip::setPan (float v)
{
    m_pan = juce::jlimit (-1.0f, 1.0f, v);
    repaint();
}

//==============================================================================
// Region helpers
//==============================================================================

juce::Rectangle<int> OutputStrip::stripeRect() const noexcept
{
    return { 0, 0, kStripeW, getHeight() };
}

juce::Rectangle<int> OutputStrip::nameLabelRect() const noexcept
{
    return { kStripeW + kPad, 0, kNameW, getHeight() };
}

juce::Rectangle<int> OutputStrip::levelTrackRect() const noexcept
{
    const int x  = nameLabelRect().getRight() + kPad;
    const int w  = getWidth() - x - kPad;
    const int cy = getHeight() / 2;
    const int h  = 6;
    return { x, cy - h/2, juce::jmax (0, w), h };
}

//==============================================================================
// paintSlider
//==============================================================================

void OutputStrip::paintSlider (juce::Graphics& g,
                                juce::Rectangle<int> tk,
                                float t,
                                bool  fromCenter,
                                bool  active) const
{
    const auto& theme = ThemeManager::get().theme();
    const auto col = theme.sliderTrack.interpolatedWith ((juce::Colour) Theme::Colour::error, 0.25f);
    const auto trackCol = theme.controlBg.withAlpha (0.96f);
    const auto thumbCol = theme.sliderThumb.interpolatedWith ((juce::Colour) Theme::Colour::error, 0.18f);

    // Track background
    g.setColour (trackCol);
    g.fillRoundedRectangle (tk.toFloat(), 3.0f);

    if (fromCenter)
    {
        const int cx   = tk.getX() + tk.getWidth() / 2;
        const int thumbX = tk.getX() + static_cast<int> (t * (float) tk.getWidth());
        const int fillX  = juce::jmin (cx, thumbX);
        const int fillW  = std::abs (thumbX - cx);
        if (fillW > 0)
        {
            g.setColour (col.withAlpha (0.7f));
            g.fillRoundedRectangle (juce::Rectangle<int> (fillX, tk.getY(), fillW, tk.getHeight()).toFloat(), 3.0f);
        }
        // Center tick
        g.setColour (theme.surfaceEdge.withAlpha (0.55f));
        g.fillRect  (cx, tk.getY() - 1, 1, tk.getHeight() + 2);
        // Thumb
        const float ty = static_cast<float> (tk.getCentreY()) - kThumbD * 0.5f;
        g.setColour (active ? thumbCol.brighter (0.2f) : thumbCol);
        g.fillEllipse (static_cast<float> (thumbX) - kThumbD * 0.5f, ty, kThumbD, kThumbD);
    }
    else
    {
        const int fillW = static_cast<int> (t * (float) tk.getWidth());
        if (fillW > 0)
        {
            g.setColour (col.withAlpha (0.7f));
            g.fillRoundedRectangle (tk.withWidth (fillW).toFloat(), 3.0f);
        }
        // Thumb
        const float ty = static_cast<float> (tk.getCentreY()) - kThumbD * 0.5f;
        g.setColour (active ? thumbCol.brighter (0.2f) : thumbCol);
        g.fillEllipse (static_cast<float> (tk.getX() + fillW) - kThumbD * 0.5f, ty, kThumbD, kThumbD);
    }
}

//==============================================================================
// Paint
//==============================================================================

void OutputStrip::paint (juce::Graphics& g)
{
    // Background
    g.setColour (Theme::Colour::surface1);
    g.fillAll();

    // Left stripe (error/output red)
    g.setColour (Theme::Colour::error);
    g.fillRect  (stripeRect());

    // "OUTPUT" label
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::error.withAlpha (0.9f));
    g.drawText  ("OUTPUT", nameLabelRect(), juce::Justification::centredLeft, false);

    // Level slider
    paintSlider (g, levelTrackRect(), m_level, false, m_drag == DragTarget::Level);

    const auto tk = levelTrackRect();
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Helper::inkFor (Theme::Colour::error).withAlpha (0.96f));
    g.drawText ("LEVEL", tk.reduced (8, 0), juce::Justification::centredLeft, false);

    // Top border
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine  (0.0f, 0.0f, static_cast<float> (getWidth()), 0.0f, 0.7f);
}

//==============================================================================
// Mouse
//==============================================================================

void OutputStrip::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    const bool hitLevel = levelTrackRect().expanded (0, kThumbD).contains (pos);
    // Double-click resets to default
    if (e.getNumberOfClicks() == 2)
    {
        if (hitLevel) { setLevel (1.0f); if (onLevelChanged) onLevelChanged (m_level); }
        return;
    }

    if (hitLevel)
    {
        m_drag = DragTarget::Level;
        const auto tk = levelTrackRect();
        if (tk.getWidth() > 0)
        {
            m_level = juce::jlimit (0.0f, 1.0f,
                        (float)(pos.x - tk.getX()) / (float) tk.getWidth());
            repaint();
        }
    }
}

void OutputStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (m_drag == DragTarget::Level)
    {
        const auto tk = levelTrackRect();
        if (tk.getWidth() > 0)
        {
            m_level = juce::jlimit (0.0f, 1.0f,
                        (float)(e.x - tk.getX()) / (float) tk.getWidth());
            repaint();
            if (onLevelChanged) onLevelChanged (m_level);
        }
    }
}

void OutputStrip::mouseUp (const juce::MouseEvent&)
{
    if (m_drag != DragTarget::None)
    {
        m_drag = DragTarget::None;
        repaint();
    }
}

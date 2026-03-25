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

juce::Rectangle<int> OutputStrip::lvlLabelRect() const noexcept
{
    const int x = nameLabelRect().getRight() + kPad;
    return { x, 0, kMiniLabelW, getHeight() };
}

juce::Rectangle<int> OutputStrip::levelTrackRect() const noexcept
{
    const int x  = lvlLabelRect().getRight() + kPad;
    const int w  = getWidth() - x - kPad - kMiniLabelW - kPad - kPanTrackW - kPad;
    const int cy = getHeight() / 2;
    const int h  = 6;
    return { x, cy - h/2, juce::jmax (0, w), h };
}

juce::Rectangle<int> OutputStrip::panLabelRect() const noexcept
{
    const int x = getWidth() - kPad - kPanTrackW - kPad - kMiniLabelW;
    return { x, 0, kMiniLabelW, getHeight() };
}

juce::Rectangle<int> OutputStrip::panTrackRect() const noexcept
{
    const int x  = getWidth() - kPad - kPanTrackW;
    const int cy = getHeight() / 2;
    const int h  = 6;
    return { x, cy - h/2, kPanTrackW, h };
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
    const auto col   = (juce::Colour) Theme::Colour::error;
    const auto trackCol = (juce::Colour) Theme::Colour::surface0;

    // Track background
    g.setColour (trackCol);
    g.fillRect  (tk);

    if (fromCenter)
    {
        const int cx   = tk.getX() + tk.getWidth() / 2;
        const int thumbX = tk.getX() + static_cast<int> (t * (float) tk.getWidth());
        const int fillX  = juce::jmin (cx, thumbX);
        const int fillW  = std::abs (thumbX - cx);
        if (fillW > 0)
        {
            g.setColour (col.withAlpha (0.7f));
            g.fillRect  (fillX, tk.getY(), fillW, tk.getHeight());
        }
        // Center tick
        g.setColour (col.withAlpha (0.4f));
        g.fillRect  (cx, tk.getY() - 1, 1, tk.getHeight() + 2);
        // Thumb
        const float ty = static_cast<float> (tk.getCentreY()) - kThumbD * 0.5f;
        g.setColour (active ? col.brighter (0.3f) : col);
        g.fillEllipse (static_cast<float> (thumbX) - kThumbD * 0.5f, ty, kThumbD, kThumbD);
    }
    else
    {
        const int fillW = static_cast<int> (t * (float) tk.getWidth());
        if (fillW > 0)
        {
            g.setColour (col.withAlpha (0.7f));
            g.fillRect  (tk.withWidth (fillW));
        }
        // Thumb
        const float ty = static_cast<float> (tk.getCentreY()) - kThumbD * 0.5f;
        g.setColour (active ? col.brighter (0.3f) : col);
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

    // "LVL" mini-label
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkMuted);
    g.drawText  ("LVL", lvlLabelRect(), juce::Justification::centredLeft, false);

    // Level slider
    paintSlider (g, levelTrackRect(), m_level, false, m_drag == DragTarget::Level);

    // "PAN" mini-label
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkMuted);
    g.drawText  ("PAN", panLabelRect(), juce::Justification::centredLeft, false);

    // Pan slider (t = 0..1 where 0.5 = center)
    const float panT = (m_pan + 1.0f) * 0.5f;
    paintSlider (g, panTrackRect(), panT, true, m_drag == DragTarget::Pan);

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
    const bool hitPan   = panTrackRect().expanded (0, kThumbD).contains (pos);

    // Double-click resets to default
    if (e.getNumberOfClicks() == 2)
    {
        if (hitLevel) { setLevel (1.0f); if (onLevelChanged) onLevelChanged (m_level); }
        if (hitPan)   { setPan   (0.0f); if (onPanChanged)   onPanChanged   (m_pan);   }
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
    else if (hitPan)
    {
        m_drag = DragTarget::Pan;
        const auto tk = panTrackRect();
        if (tk.getWidth() > 0)
        {
            const float t = juce::jlimit (0.0f, 1.0f,
                                (float)(pos.x - tk.getX()) / (float) tk.getWidth());
            m_pan = t * 2.0f - 1.0f;
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
    else if (m_drag == DragTarget::Pan)
    {
        const auto tk = panTrackRect();
        if (tk.getWidth() > 0)
        {
            const float t = juce::jlimit (0.0f, 1.0f,
                                (float)(e.x - tk.getX()) / (float) tk.getWidth());
            m_pan = t * 2.0f - 1.0f;
            repaint();
            if (onPanChanged) onPanChanged (m_pan);
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

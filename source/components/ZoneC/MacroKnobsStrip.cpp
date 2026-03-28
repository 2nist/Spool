#include "MacroKnobsStrip.h"

//==============================================================================

MacroKnobsStrip::MacroKnobsStrip()
{
    setRepaintsOnMouseActivity (true);
}

//==============================================================================

juce::Rectangle<float> MacroKnobsStrip::knobBounds (int i) const noexcept
{
    const float slotW = static_cast<float> (getWidth()) / kNumKnobs;
    const float cx    = slotW * i + slotW * 0.5f;
    const float cy    = 10.0f + kKnobD * 0.5f;  // 10px top padding before knob
    return { cx - kKnobD * 0.5f, cy - kKnobD * 0.5f,
             static_cast<float> (kKnobD), static_cast<float> (kKnobD) };
}

int MacroKnobsStrip::hitTestKnob (juce::Point<int> pos) const noexcept
{
    for (int i = 0; i < kNumKnobs; ++i)
    {
        if (knobBounds (i).contains (pos.toFloat()))
            return i;
    }
    return -1;
}

//==============================================================================
// Paint
//==============================================================================

void MacroKnobsStrip::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface0);

    // Top border
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine (0.0f, 0.0f, static_cast<float> (getWidth()), 0.0f, Theme::Stroke::subtle);

    for (int i = 0; i < kNumKnobs; ++i)
        paintKnob (g, i);
}

void MacroKnobsStrip::paintKnob (juce::Graphics& g, int i) const
{
    const auto& theme = ThemeManager::get().theme();
    const auto  r   = knobBounds (i);
    const float val = m_values[i];
    const float cx  = r.getCentreX();
    const float cy  = r.getCentreY();
    const float rad = r.getWidth() * 0.5f;

    // Background circle
    g.setColour (theme.controlBg.withAlpha (0.95f));
    g.fillEllipse (r);
    g.setColour (theme.surfaceEdge.withAlpha (0.85f));
    g.drawEllipse (r.reduced (0.5f), Theme::Stroke::subtle);

    // Arc angles: start 210° clockwise from top = 210°-90° = 120° in JUCE coords
    // JUCE angles: 0 = 3 o'clock, positive = clockwise, in radians
    // "210° clockwise from top" = 210° + 270° = 480° from 3 o'clock = 120° from 3 o'clock
    const float kStartAngle = juce::MathConstants<float>::pi * 4.0f / 6.0f;   // 120° from 3-clock → 210° from top
    const float kSweep      = juce::MathConstants<float>::pi * 5.0f / 3.0f;   // 300° sweep

    const float arcR = rad - 3.0f;

    // Arc track (full sweep in surface0 contrast)
    {
        juce::Path track;
        track.addCentredArc (cx, cy, arcR, arcR, 0.0f, kStartAngle, kStartAngle + kSweep, true);
        g.setColour (theme.surfaceEdge.withAlpha (0.45f));
        g.strokePath (track, juce::PathStrokeType (2.0f));
    }

    // Arc fill (Zone::c up to value)
    {
        const float fillAngle = kStartAngle + val * kSweep;
        juce::Path fill;
        fill.addCentredArc (cx, cy, arcR, arcR, 0.0f, kStartAngle, fillAngle, true);
        g.setColour (theme.sliderTrack);
        g.strokePath (fill, juce::PathStrokeType (2.0f));
    }

    // Indicator dot at value position
    {
        const float angle = kStartAngle + val * kSweep;
        const float dotX  = cx + arcR * std::cos (angle);
        const float dotY  = cy + arcR * std::sin (angle);
        g.setColour (theme.sliderThumb);
        g.fillEllipse (dotX - 2.5f, dotY - 2.5f, 5.0f, 5.0f);
    }

    // Value text (on hover or drag)
    if (m_showValue[i] || m_draggingKnob == i)
    {
        const juce::String valStr = juce::String (val, 2);
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkLight);
        g.drawText (valStr,
                    static_cast<int> (r.getX()) - 4,
                    static_cast<int> (r.getY()) - 12,
                    static_cast<int> (r.getWidth()) + 8, 10,
                    juce::Justification::centred, false);
    }

    // Label below knob
    const float slotW = static_cast<float> (getWidth()) / kNumKnobs;
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText (m_labels[i],
                static_cast<int> (slotW * i), static_cast<int> (r.getBottom()) + 2,
                static_cast<int> (slotW), 10,
                juce::Justification::centred, false);
}

//==============================================================================
// resized
//==============================================================================

void MacroKnobsStrip::resized() {}

//==============================================================================
// Mouse
//==============================================================================

void MacroKnobsStrip::mouseDown (const juce::MouseEvent& e)
{
    m_draggingKnob = hitTestKnob (e.getPosition());
    if (m_draggingKnob >= 0)
    {
        m_dragStartVal = m_values[m_draggingKnob];
        m_dragStartY   = e.getScreenY();
        m_showValue[m_draggingKnob] = true;
    }
}

void MacroKnobsStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (m_draggingKnob < 0) return;
    const float delta = static_cast<float> (m_dragStartY - e.getScreenY()) / 150.0f;
    m_values[m_draggingKnob] = juce::jlimit (0.0f, 1.0f, m_dragStartVal + delta);
    repaint();
}

void MacroKnobsStrip::mouseUp (const juce::MouseEvent&)
{
    if (m_draggingKnob >= 0)
    {
        m_showValue[m_draggingKnob] = false;
        m_draggingKnob = -1;
        repaint();
    }
}

void MacroKnobsStrip::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int idx = hitTestKnob (e.getPosition());
    if (idx >= 0)
    {
        m_values[idx] = kDefault;
        repaint();
    }
}

void MacroKnobsStrip::mouseMove (const juce::MouseEvent& e)
{
    const int hover = hitTestKnob (e.getPosition());
    bool changed = false;
    for (int i = 0; i < kNumKnobs; ++i)
    {
        const bool shouldShow = (i == hover);
        if (m_showValue[i] != shouldShow)
        {
            m_showValue[i] = shouldShow;
            changed = true;
        }
    }
    if (changed) repaint();
}

void MacroKnobsStrip::mouseExit (const juce::MouseEvent&)
{
    bool changed = false;
    for (int i = 0; i < kNumKnobs; ++i)
    {
        if (m_showValue[i]) { m_showValue[i] = false; changed = true; }
    }
    if (changed) repaint();
}

juce::MouseCursor MacroKnobsStrip::getMouseCursor()
{
    if (hitTestKnob (getMouseXYRelative()) >= 0)
        return juce::MouseCursor::UpDownResizeCursor;
    return juce::MouseCursor::NormalCursor;
}

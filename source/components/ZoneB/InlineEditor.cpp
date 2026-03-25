#include "InlineEditor.h"

InlineEditor::InlineEditor()
{
    setInterceptsMouseClicks (true, false);
}

void InlineEditor::setParams (const juce::Array<Param>& params,
                               const juce::Colour&       signalColour,
                               const juce::String&       typeName,
                               const juce::StringArray&  inPorts,
                               const juce::StringArray&  outPorts)
{
    m_params       = params;
    m_signalColour = signalColour;
    m_moduleType   = typeName;
    m_inPorts      = inPorts;
    m_outPorts     = outPorts;
    repaint();
}

//==============================================================================
// Layout helpers
//==============================================================================

juce::Rectangle<int> InlineEditor::paramAreaBounds() const noexcept
{
    return getLocalBounds()
        .withTrimmedTop    (kTopBorderH + kHeaderH)
        .withTrimmedBottom (kBottomH);
}

juce::Rectangle<int> InlineEditor::rowBounds (int i) const noexcept
{
    auto area = paramAreaBounds();
    return { area.getX(),
             area.getY() + kPadV + i * kRowH,
             area.getWidth(),
             kRowH };
}

juce::Rectangle<int> InlineEditor::trackBounds (juce::Rectangle<int> rowR) const noexcept
{
    return { rowR.getX() + kLabelW,
             rowR.getCentreY() - 1,
             rowR.getWidth() - kLabelW - kValueW,
             2 };
}

//==============================================================================
// Paint
//==============================================================================

void InlineEditor::paint (juce::Graphics& g)
{
    // Top border — signal type colour
    g.setColour (m_signalColour);
    g.fillRect  (0, 0, getWidth(), kTopBorderH);

    // Editor header
    {
        const auto headR = juce::Rectangle<int> (0, kTopBorderH, getWidth(), kHeaderH);
        g.setColour (Theme::Colour::surface0);
        g.fillRect  (headR);

        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("PARAMETERS",
                     headR.withTrimmedLeft (static_cast<int> (Theme::Space::sm)),
                     juce::Justification::centredLeft, false);
        g.setColour (m_signalColour);
        g.drawText  (m_moduleType,
                     headR.withTrimmedRight (static_cast<int> (Theme::Space::sm)),
                     juce::Justification::centredRight, false);
    }

    // Parameter area background
    g.setColour (Theme::Colour::surface2);
    g.fillRect  (paramAreaBounds());

    for (int i = 0; i < m_params.size(); ++i)
        paintRow (g, i);

    // Bottom strip — port type badges
    {
        const auto botR = getLocalBounds().removeFromBottom (kBottomH);
        g.setColour (Theme::Colour::surface0);
        g.fillRect  (botR);

        const int badgeH = static_cast<int> (Theme::Space::md) + static_cast<int> (Theme::Space::xs);
        const int badgeW = 30;
        const int badgeY = botR.getCentreY() - badgeH / 2;
        const int padX   = static_cast<int> (Theme::Space::sm);

        // IN badges — left-aligned
        int lx = botR.getX() + padX;
        for (const auto& p : m_inPorts)
        {
            const juce::Rectangle<int> br { lx, badgeY, badgeW, badgeH };
            const auto col = Theme::Signal::forType (p.toLowerCase());
            g.setColour (col.withAlpha (0.15f));
            g.fillRoundedRectangle (br.toFloat(), Theme::Radius::xs);
            g.setColour (col.withAlpha (0.30f));
            g.drawRoundedRectangle (br.toFloat(), Theme::Radius::xs, Theme::Stroke::subtle);
            g.setFont   (Theme::Font::micro());
            g.setColour (col);
            g.drawText  (p, br, juce::Justification::centred, false);
            lx += badgeW + static_cast<int> (Theme::Space::xs);
        }

        // OUT badges — right-aligned
        int rx = botR.getRight() - padX;
        for (int i = m_outPorts.size() - 1; i >= 0; --i)
        {
            rx -= badgeW;
            const juce::Rectangle<int> br { rx, badgeY, badgeW, badgeH };
            const auto col = Theme::Signal::forType (m_outPorts[i].toLowerCase());
            g.setColour (col.withAlpha (0.15f));
            g.fillRoundedRectangle (br.toFloat(), Theme::Radius::xs);
            g.setColour (col.withAlpha (0.30f));
            g.drawRoundedRectangle (br.toFloat(), Theme::Radius::xs, Theme::Stroke::subtle);
            g.setFont   (Theme::Font::micro());
            g.setColour (col);
            g.drawText  (m_outPorts[i], br, juce::Justification::centred, false);
            rx -= static_cast<int> (Theme::Space::xs);
        }
    }
}

void InlineEditor::paintRow (juce::Graphics& g, int i) const
{
    const auto rowR = rowBounds (i);
    const auto& p   = m_params.getReference (i);
    const auto  tk  = trackBounds (rowR);

    // Label
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  (p.label,
                 juce::Rectangle<int> (rowR.getX(), rowR.getY(), kLabelW, rowR.getHeight()),
                 juce::Justification::centredLeft, false);

    // Track background
    g.setColour (Theme::Colour::surface0);
    g.fillRect  (tk);

    // Track fill
    const float t     = (p.max > p.min) ? (p.value - p.min) / (p.max - p.min) : 0.0f;
    const int   fillW = static_cast<int> (t * static_cast<float> (tk.getWidth()));
    if (fillW > 0)
    {
        g.setColour (m_signalColour);
        g.fillRect  (tk.withWidth (fillW));
    }

    // Thumb
    const float thumbCx = static_cast<float> (tk.getX() + fillW);
    const float thumbY  = static_cast<float> (rowR.getCentreY()) - kThumbD * 0.5f;
    g.setColour (m_draggingParamIdx == i ? m_signalColour.brighter (0.2f) : m_signalColour);
    g.fillEllipse (thumbCx - kThumbD * 0.5f, thumbY, kThumbD, kThumbD);

    // Value
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  (juce::String (p.value, 2),
                 juce::Rectangle<int> (rowR.getRight() - kValueW, rowR.getY(),
                                       kValueW, rowR.getHeight()),
                 juce::Justification::centredRight, false);
}

void InlineEditor::resized() {}

//==============================================================================
// Mouse
//==============================================================================

void InlineEditor::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) return;

    const auto area = paramAreaBounds();
    for (int i = 0; i < m_params.size(); ++i)
    {
        auto tk = trackBounds (rowBounds (i));
        // Expanded hit zone — includes thumb radius vertically
        if (tk.expanded (0, kThumbD / 2).contains (e.x, e.y))
        {
            m_draggingParamIdx = i;
            m_dragStartValue   = m_params[i].value;
            m_dragStartX       = e.x;
            return;
        }
    }
    juce::ignoreUnused (area);
}

void InlineEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (m_draggingParamIdx < 0) return;
    auto& p  = m_params.getReference (m_draggingParamIdx);
    auto  tk = trackBounds (rowBounds (m_draggingParamIdx));
    if (tk.getWidth() <= 0) return;

    const float ratio = static_cast<float> (e.x - m_dragStartX) / static_cast<float> (tk.getWidth());
    p.value = juce::jlimit (p.min, p.max, m_dragStartValue + ratio * (p.max - p.min));

    if (onParamChanged)
        onParamChanged (m_draggingParamIdx, p.value);

    repaint();
}

void InlineEditor::mouseUp (const juce::MouseEvent&)
{
    m_draggingParamIdx = -1;
    repaint();
}

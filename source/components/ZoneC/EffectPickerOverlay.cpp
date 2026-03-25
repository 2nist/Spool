#include "EffectPickerOverlay.h"

//==============================================================================

EffectPickerOverlay::EffectPickerOverlay()
{
    setRepaintsOnMouseActivity (true);
}

const juce::Array<EffectTypeDef>& EffectPickerOverlay::effectDefs() const
{
    static const auto defs = EffectRegistry::all();
    return defs;
}

//==============================================================================
// Geometry
//==============================================================================

juce::Rectangle<int> EffectPickerOverlay::rowRect (int i) const noexcept
{
    return { 0, kTitleH + i * kRowH, getWidth(), kRowH };
}

juce::Rectangle<int> EffectPickerOverlay::cancelRect() const noexcept
{
    return { 0, getHeight() - kCancelH, getWidth(), kCancelH };
}

bool EffectPickerOverlay::hitTest (int x, int y)
{
    return getLocalBounds().contains (x, y);
}

//==============================================================================
// Paint
//==============================================================================

void EffectPickerOverlay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds();

    // Background + border
    g.setColour (Theme::Colour::surface2);
    g.fillRoundedRectangle (bounds.toFloat(), Theme::Radius::md);
    g.setColour (Theme::Zone::c);
    g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f),
                            Theme::Radius::md, Theme::Stroke::normal);

    // Title
    g.setFont (Theme::Font::label());
    g.setColour (Theme::Colour::inkLight);
    g.drawText ("ADD EFFECT",
                8, 8, bounds.getWidth() - 16, 14,
                juce::Justification::centredLeft, false);

    const auto& defs = effectDefs();

    for (int i = 0; i < defs.size(); ++i)
    {
        const auto& d = defs.getReference (i);
        const auto  r = rowRect (i);

        // Hover background
        if (i == m_hoverRow)
        {
            g.setColour (Theme::Colour::surface3);
            g.fillRect (r);
        }

        // Colour dot
        const int dotD = 8;
        const int dotX = 8;
        const int dotY = r.getY() + (kRowH - dotD) / 2;
        g.setColour (d.colour);
        g.fillEllipse (static_cast<float> (dotX), static_cast<float> (dotY),
                       static_cast<float> (dotD), static_cast<float> (dotD));

        // Effect name
        g.setFont (Theme::Font::label());
        g.setColour (Theme::Colour::inkLight);
        g.drawText (d.displayName,
                    dotX + dotD + 6, r.getY(), 90, kRowH,
                    juce::Justification::centredLeft, false);

        // Subtype tag (right)
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText (d.tag,
                    bounds.getRight() - 50, r.getY(), 44, kRowH,
                    juce::Justification::centredRight, false);
    }

    // Cancel row
    const auto cr = cancelRect();
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("CANCEL", cr, juce::Justification::centred, false);
}

//==============================================================================
// Mouse
//==============================================================================

void EffectPickerOverlay::mouseMove (const juce::MouseEvent& e)
{
    const auto& defs = effectDefs();
    int newHover = -1;
    for (int i = 0; i < defs.size(); ++i)
    {
        if (rowRect (i).contains (e.getPosition()))
        {
            newHover = i;
            break;
        }
    }
    if (newHover != m_hoverRow)
    {
        m_hoverRow = newHover;
        repaint();
    }
}

void EffectPickerOverlay::mouseDown (const juce::MouseEvent& e)
{
    const auto& defs = effectDefs();
    for (int i = 0; i < defs.size(); ++i)
    {
        if (rowRect (i).contains (e.getPosition()))
        {
            if (onEffectChosen)
                onEffectChosen (defs[i].id);
            return;
        }
    }

    // Cancel row or outside
    if (onCancel)
        onCancel();
}

#pragma once

#include "../Theme.h"
#include "LucideIcons.h"

//==============================================================================
/**
    IconTabButton — 28×28px toggle button that draws a Lucide SVG path icon.

    Used in ThemeEditorPanel's tab bar to replace text abbreviation buttons.
    The button is a toggle: getToggleState() == true means this tab is active.

    Visual states:
      Active  : Zone::a at 20% alpha background, Zone::a bottom underline, Zone::a icon tint
      Hover   : surfaceEdge at 30% alpha background, inkMid icon tint
      Default : transparent, inkGhost icon tint

    Tooltip: set via setTooltip() with the full category name.
*/
class IconTabButton final : public juce::Button
{
public:
    /** @param svgPath  Lucide SVG path string (24×24 viewBox).
        @param tooltip  Category name shown in tooltip. */
    IconTabButton (const juce::String& svgPath, const juce::String& tooltip)
        : juce::Button (tooltip)
    {
        m_svgPath = svgPath;
        setTooltip (tooltip);
        setClickingTogglesState (false);   // caller manages active state
    }

    ~IconTabButton() override = default;

    void paintButton (juce::Graphics& g,
                      bool isHighlighted,
                      bool /*isDown*/) override
    {
        const bool active = getToggleState();

        // Background
        if (active)
        {
            g.setColour (Theme::Zone::a.withAlpha (0.20f));
            g.fillRect (getLocalBounds());
        }
        else if (isHighlighted)
        {
            g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.30f));
            g.fillRect (getLocalBounds());
        }

        // Active bottom underline
        if (active)
        {
            g.setColour (Theme::Zone::a);
            g.fillRect (0, getHeight() - 2, getWidth(), 2);
        }

        // Draw icon path (scale to 16×16, centred)
        juce::Path path = juce::Drawable::parseSVGPath (m_svgPath);

        const auto pb = path.getBounds();
        if (!pb.isEmpty())
        {
            const float s  = 16.0f / juce::jmax (pb.getWidth(), pb.getHeight());
            const float ox = (static_cast<float> (getWidth())  - pb.getWidth()  * s) * 0.5f - pb.getX() * s;
            const float oy = (static_cast<float> (getHeight()) - pb.getHeight() * s) * 0.5f - pb.getY() * s;
            path.applyTransform (juce::AffineTransform::scale (s).translated (ox, oy));
        }

        g.setColour (active       ? Theme::Zone::a
                   : isHighlighted ? Theme::Colour::inkMid
                                   : Theme::Colour::inkGhost);
        g.strokePath (path, juce::PathStrokeType (1.5f));
    }

private:
    juce::String m_svgPath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IconTabButton)
};

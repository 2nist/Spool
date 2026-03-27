#pragma once

#include "../../Theme.h"

namespace ZoneAControlStyle
{
inline constexpr int kSectionHeaderH = 16;
inline constexpr int kBarHeight = 14;
inline constexpr int kPad = 6;
inline constexpr int kGap = 6;

class BarSliderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float /*minSliderPos*/,
                           float /*maxSliderPos*/,
                           const juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearBar && style != juce::Slider::LinearBarVertical)
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
            return;
        }

        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (0.5f);
        const auto fill = slider.findColour (juce::Slider::trackColourId);
        const auto bg = slider.findColour (juce::Slider::backgroundColourId);
        const bool isVertical = style == juce::Slider::LinearBarVertical;
        const float proportion = (float) slider.valueToProportionOfLength (slider.getValue());

        g.setColour (bg);
        g.fillRoundedRectangle (bounds, 4.0f);

        if (isVertical)
        {
            const float fillH = bounds.getHeight() * juce::jlimit (0.0f, 1.0f, proportion);
            auto fillBounds = bounds.withY (bounds.getBottom() - fillH).withHeight (fillH);
            if (fillBounds.getHeight() > 0.0f)
            {
                g.setColour (fill);
                g.fillRoundedRectangle (fillBounds, 4.0f);
            }
        }
        else
        {
            const float posX = juce::jlimit (bounds.getX(), bounds.getRight(), sliderPos);
            auto fillBounds = bounds.withWidth (juce::jmax (0.0f, posX - bounds.getX()));
            if (fillBounds.getWidth() > 0.0f)
            {
                g.setColour (fill);
                g.fillRoundedRectangle (fillBounds, 4.0f);
            }
        }

        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.85f));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        const auto label = slider.getName();
        if (label.isEmpty())
            return;

        const bool overFill = proportion >= (isVertical ? 0.58f : 0.62f);
        const auto labelBg = overFill ? fill : bg;
        const auto textColour = Theme::Helper::inkFor (labelBg).withAlpha (0.98f);
        const auto shadowColour = textColour.getPerceivedBrightness() > 0.5f
                                    ? Theme::Colour::inkDark.withAlpha (0.28f)
                                    : Theme::Colour::inkLight.withAlpha (0.18f);

        g.setFont (Theme::Font::microMedium());

        if (isVertical)
        {
            juce::Graphics::ScopedSaveState state (g);
            const auto centre = bounds.getCentre();
            g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, centre.x, centre.y));

            auto textBounds = bounds.toNearestInt().reduced (0, 5);
            g.setColour (shadowColour);
            g.drawText (label, textBounds.translated (1, 1), juce::Justification::centred, false);
            g.setColour (textColour);
            g.drawText (label, textBounds, juce::Justification::centred, false);
        }
        else
        {
            auto textBounds = bounds.toNearestInt().reduced (8, 0);
            g.setColour (shadowColour);
            g.drawText (label, textBounds.translated (1, 1), juce::Justification::centredLeft, false);
            g.setColour (textColour);
            g.drawText (label, textBounds, juce::Justification::centredLeft, false);
        }
    }
};

inline BarSliderLookAndFeel& barLookAndFeel()
{
    static BarSliderLookAndFeel lf;
    return lf;
}

inline void initBarSlider (juce::Slider& slider, const juce::String& label)
{
    slider.setName (label);
    slider.setSliderStyle (juce::Slider::LinearBar);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel (&barLookAndFeel());
    slider.setColour (juce::Slider::backgroundColourId, Theme::Colour::surface3.withAlpha (0.95f));
    slider.setColour (juce::Slider::trackColourId, Theme::Colour::accentWarm);
    slider.setColour (juce::Slider::thumbColourId, Theme::Colour::accentWarm.brighter (0.08f));
    slider.setColour (juce::Slider::rotarySliderFillColourId, Theme::Colour::accentWarm);
}

inline void tintBarSlider (juce::Slider& slider, juce::Colour colour)
{
    slider.setColour (juce::Slider::trackColourId, colour);
    slider.setColour (juce::Slider::thumbColourId, colour.brighter (0.08f));
    slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
}

inline void styleTextButton (juce::TextButton& button, juce::Colour accent = Theme::Colour::surfaceEdge.withAlpha (0.7f))
{
    button.setColour (juce::TextButton::buttonColourId, Theme::Colour::surface2);
    button.setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.22f));
    button.setColour (juce::TextButton::textColourOffId, Theme::Colour::inkGhost);
    button.setColour (juce::TextButton::textColourOnId, Theme::Helper::inkFor (accent).withAlpha (0.96f));
}

inline void styleTextEditor (juce::TextEditor& editor)
{
    editor.setColour (juce::TextEditor::backgroundColourId, Theme::Colour::surface2);
    editor.setColour (juce::TextEditor::outlineColourId, Theme::Colour::surfaceEdge.withAlpha (0.55f));
    editor.setColour (juce::TextEditor::focusedOutlineColourId, Theme::Zone::a.withAlpha (0.8f));
    editor.setColour (juce::TextEditor::textColourId, Theme::Colour::inkLight);
    editor.setColour (juce::TextEditor::highlightColourId, Theme::Zone::a.withAlpha (0.25f));
    editor.setFont (Theme::Font::label());
}
}

#pragma once

#include "../../Theme.h"

namespace ZoneAControlStyle
{
inline constexpr int kSectionHeaderH = 16;
inline constexpr int kBarHeight = 14;
inline constexpr int kPad = 6;
inline constexpr int kGap = 6;

inline int sectionHeaderHeight() noexcept
{
    return juce::roundToInt (ThemeManager::get().theme().zoneASectionHeaderHeight);
}

inline int controlBarHeight() noexcept
{
    return juce::roundToInt (ThemeManager::get().theme().zoneAControlBarHeight);
}

inline int controlHeight() noexcept
{
    return juce::roundToInt (ThemeManager::get().theme().zoneAControlHeight);
}

inline int compactGap() noexcept
{
    return juce::roundToInt (ThemeManager::get().theme().zoneACompactGap);
}

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
            if (slider.getName() == "BARS")
            {
                const double maxV = slider.getMaximum();
                for (int marker = 4; marker < static_cast<int> (maxV); marker += 4)
                {
                    const float markerPos = (float) slider.valueToProportionOfLength ((double) marker);
                    if (markerPos <= 0.0f || markerPos >= 1.0f)
                        continue;

                    const float xPos = bounds.getX() + markerPos * bounds.getWidth();
                    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.55f));
                    g.fillRect (xPos, bounds.getY() + 2.0f, 1.0f, bounds.getHeight() - 4.0f);
                }
            }

            const float posX = juce::jlimit (bounds.getX(), bounds.getRight(), sliderPos);
            auto fillBounds = bounds.withWidth (juce::jmax (0.0f, posX - bounds.getX()));
            if (fillBounds.getWidth() > 0.0f)
            {
                g.setColour (fill);
                g.fillRoundedRectangle (fillBounds, 4.0f);
            }
        }

        g.setColour (ThemeManager::get().theme().surfaceEdge.withAlpha (0.85f));
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

class CompactButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        return buttonHeight <= 18 ? Theme::Font::microMedium() : Theme::Font::label();
    }

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour&,
                               bool isHighlighted,
                               bool isDown) override
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const auto& theme = ThemeManager::get().theme();
        const auto accent = button.findColour (juce::TextButton::buttonOnColourId);
        auto fill = button.getToggleState() ? theme.controlOnBg : theme.controlBg;

        if (isDown)
            fill = fill.brighter (0.08f);
        else if (isHighlighted)
            fill = fill.interpolatedWith (theme.rowHover, 0.4f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 4.0f);

        g.setColour (accent.withAlpha (button.getToggleState() ? 0.7f : 0.35f));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
    }
};

class CompactComboLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return Theme::Font::micro();
    }
};

inline BarSliderLookAndFeel& barLookAndFeel()
{
    static BarSliderLookAndFeel lf;
    return lf;
}

inline CompactButtonLookAndFeel& buttonLookAndFeel()
{
    static CompactButtonLookAndFeel lf;
    return lf;
}

inline CompactComboLookAndFeel& comboLookAndFeel()
{
    static CompactComboLookAndFeel lf;
    return lf;
}

inline void initBarSlider (juce::Slider& slider, const juce::String& label)
{
    slider.setName (label);
    slider.setSliderStyle (juce::Slider::LinearBar);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setSliderSnapsToMousePosition (false);
    slider.setScrollWheelEnabled (false);
    slider.setVelocityBasedMode (false);
    slider.setMouseDragSensitivity (900);
    slider.setChangeNotificationOnlyOnRelease (true);
    slider.setLookAndFeel (&barLookAndFeel());
    slider.setColour (juce::Slider::backgroundColourId, ThemeManager::get().theme().controlBg.withAlpha (0.95f));
    slider.setColour (juce::Slider::trackColourId, ThemeManager::get().theme().sliderTrack);
    slider.setColour (juce::Slider::thumbColourId, ThemeManager::get().theme().sliderThumb);
    slider.setColour (juce::Slider::rotarySliderFillColourId, ThemeManager::get().theme().sliderTrack);
}

inline void tintBarSlider (juce::Slider& slider, juce::Colour colour)
{
    slider.setColour (juce::Slider::trackColourId, colour);
    slider.setColour (juce::Slider::thumbColourId, colour.brighter (0.08f));
    slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
}

inline void styleTextButton (juce::TextButton& button, juce::Colour accent = Theme::Colour::surfaceEdge.withAlpha (0.7f))
{
    button.setLookAndFeel (&buttonLookAndFeel());
    button.setColour (juce::TextButton::buttonColourId, ThemeManager::get().theme().controlBg);
    button.setColour (juce::TextButton::buttonOnColourId, accent);
    button.setColour (juce::TextButton::textColourOffId, ThemeManager::get().theme().controlText);
    button.setColour (juce::TextButton::textColourOnId, ThemeManager::get().theme().controlTextOn);
}

inline void styleComboBox (juce::ComboBox& combo, juce::Colour accent = Theme::Zone::a)
{
    combo.setLookAndFeel (&comboLookAndFeel());
    combo.setColour (juce::ComboBox::backgroundColourId, ThemeManager::get().theme().controlBg);
    combo.setColour (juce::ComboBox::textColourId, ThemeManager::get().theme().controlTextOn);
    combo.setColour (juce::ComboBox::outlineColourId, ThemeManager::get().theme().surfaceEdge.withAlpha (0.55f));
    combo.setColour (juce::ComboBox::arrowColourId, accent.withAlpha (0.9f));
}

inline void styleTextEditor (juce::TextEditor& editor)
{
    editor.setColour (juce::TextEditor::backgroundColourId, ThemeManager::get().theme().controlBg);
    editor.setColour (juce::TextEditor::outlineColourId, ThemeManager::get().theme().surfaceEdge.withAlpha (0.55f));
    editor.setColour (juce::TextEditor::focusedOutlineColourId, ThemeManager::get().theme().focusOutline.withAlpha (0.8f));
    editor.setColour (juce::TextEditor::textColourId, ThemeManager::get().theme().controlTextOn);
    editor.setColour (juce::TextEditor::highlightColourId, Theme::Zone::a.withAlpha (0.25f));
    editor.setFont (Theme::Font::label());
}

inline void styleToggleButton (juce::ToggleButton& button, juce::Colour accent = Theme::Zone::a)
{
    button.setColour (juce::ToggleButton::textColourId, ThemeManager::get().theme().controlText);
    button.setColour (juce::ToggleButton::tickColourId, accent);
    button.setColour (juce::ToggleButton::tickDisabledColourId, ThemeManager::get().theme().surfaceEdge);
}
}

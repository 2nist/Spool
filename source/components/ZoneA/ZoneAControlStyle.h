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
        const auto& theme = ThemeManager::get().theme();
        const bool isVertical = style == juce::Slider::LinearBarVertical;
        const float proportion = (float) slider.valueToProportionOfLength (slider.getValue());
        const float sliderR = theme.sliderCornerRadius;

        // Read bg always from ThemeManager so colour changes are live.
        // For fill: use a per-slider tint property if set, otherwise theme default.
        const auto bg = theme.controlBg.withAlpha (0.95f);
        const auto tintProp = slider.getProperties()["tint"];
        const auto fill = tintProp.isVoid() ? theme.sliderTrack
                                             : juce::Colour::fromString (tintProp.toString());

        // sliderTrackThickness: shrink the drawn bar within the allocated bounds.
        const float trackH = juce::jmin (theme.sliderTrackThickness, bounds.getHeight());
        const float trackW = juce::jmin (theme.sliderTrackThickness, bounds.getWidth());
        const auto drawBounds = isVertical
                                    ? bounds.withSizeKeepingCentre (trackW, bounds.getHeight())
                                    : bounds.withSizeKeepingCentre (bounds.getWidth(), trackH);

        g.setColour (bg);
        g.fillRoundedRectangle (drawBounds, sliderR);

        if (isVertical)
        {
            const float fillH = drawBounds.getHeight() * juce::jlimit (0.0f, 1.0f, proportion);
            auto fillBounds = drawBounds.withY (drawBounds.getBottom() - fillH).withHeight (fillH);
            if (fillBounds.getHeight() > 0.0f)
            {
                g.setColour (fill);
                g.fillRoundedRectangle (fillBounds, sliderR);
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

                    const float xPos = drawBounds.getX() + markerPos * drawBounds.getWidth();
                    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.55f));
                    g.fillRect (xPos, drawBounds.getY() + 2.0f, 1.0f, drawBounds.getHeight() - 4.0f);
                }
            }

            const float posX = juce::jlimit (drawBounds.getX(), drawBounds.getRight(), sliderPos);
            auto fillBounds = drawBounds.withWidth (juce::jmax (0.0f, posX - drawBounds.getX()));
            if (fillBounds.getWidth() > 0.0f)
            {
                g.setColour (fill);
                g.fillRoundedRectangle (fillBounds, sliderR);
            }
        }

        g.setColour (theme.surfaceEdge.withAlpha (0.85f));
        g.drawRoundedRectangle (drawBounds, sliderR, Theme::Stroke::normal);

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

        const float fillAlpha = button.getToggleState() ? theme.btnOnFillStrength : theme.btnFillStrength;
        g.setColour (fill.withMultipliedAlpha (fillAlpha));
        g.fillRoundedRectangle (bounds, theme.btnCornerRadius);

        const float borderAlpha = button.getToggleState() ? theme.btnBorderStrength
                                                           : theme.btnBorderStrength * 0.5f;
        g.setColour (accent.withAlpha (borderAlpha));
        g.drawRoundedRectangle (bounds, theme.btnCornerRadius, Theme::Stroke::normal);
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool /*isHighlighted*/,
                         bool /*isDown*/) override
    {
        const auto& theme = ThemeManager::get().theme();
        g.setFont (getTextButtonFont (button, button.getHeight()));
        g.setColour (button.getToggleState() ? theme.controlTextOn : theme.controlText);
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds().reduced (4, 2),
                          juce::Justification::centred, 1, 1.0f);
    }
};

class CompactComboLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Font getComboBoxFont (juce::ComboBox&) override { return Theme::Font::micro(); }

    void drawComboBox (juce::Graphics& g,
                       int width, int height,
                       bool /*isButtonDown*/,
                       int /*buttonX*/, int /*buttonY*/,
                       int /*buttonW*/, int /*buttonH*/,
                       juce::ComboBox& box) override
    {
        const auto& theme = ThemeManager::get().theme();
        const auto bounds = juce::Rectangle<float> (0.f, 0.f, (float) width, (float) height).reduced (0.5f);

        g.setColour (theme.controlBg);
        g.fillRoundedRectangle (bounds, Theme::Radius::sm);

        g.setColour (theme.surfaceEdge.withAlpha (0.55f));
        g.drawRoundedRectangle (bounds, Theme::Radius::sm, Theme::Stroke::subtle);

        // Arrow
        const float arrowX = (float) width - 14.f;
        const float arrowY = (float) height * 0.5f;
        juce::Path arrow;
        arrow.addTriangle (arrowX, arrowY - 2.f, arrowX + 6.f, arrowY - 2.f, arrowX + 3.f, arrowY + 2.f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha (0.8f));
        g.fillPath (arrow);
    }

    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        const auto& theme = ThemeManager::get().theme();
        g.fillAll (juce::Colours::transparentBlack);
        g.setColour (theme.controlTextOn);
        g.setFont (Theme::Font::micro());
        g.drawFittedText (label.getText(),
                          label.getLocalBounds().reduced (4, 0),
                          label.getJustificationType(), 1, 1.0f);
    }

    // Popup menu — rows and background
    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        const auto& theme = ThemeManager::get().theme();
        g.setColour (theme.surface2);
        g.fillRoundedRectangle (0.f, 0.f, (float) width, (float) height, Theme::Radius::sm);
        g.setColour (theme.surfaceEdge.withAlpha (0.6f));
        g.drawRoundedRectangle (0.5f, 0.5f, (float) width - 1.f, (float) height - 1.f,
                                Theme::Radius::sm, Theme::Stroke::subtle);
    }

    void drawPopupMenuItem (juce::Graphics& g,
                            const juce::Rectangle<int>& area,
                            bool /*isSeparator*/,
                            bool isActive,
                            bool isHighlighted,
                            bool isTicked,
                            bool /*hasSubMenu*/,
                            const juce::String& text,
                            const juce::String& /*shortcutKeyText*/,
                            const juce::Drawable* /*icon*/,
                            const juce::Colour* /*textColour*/) override
    {
        const auto& theme = ThemeManager::get().theme();
        auto r = area.toFloat().reduced (2.f, 1.f);

        if (isHighlighted && isActive)
        {
            g.setColour (theme.rowHover);
            g.fillRoundedRectangle (r, Theme::Radius::xs);
        }

        g.setFont (Theme::Font::micro());
        g.setColour (isActive ? (isTicked ? theme.controlTextOn : theme.controlText)
                              : theme.inkGhost);
        g.drawFittedText (text, area.reduced (8, 0), juce::Justification::centredLeft, 1);

        if (isTicked)
        {
            const float dotD = 4.f;
            g.setColour (theme.controlTextOn);
            g.fillEllipse ((float) area.getRight() - 12.f,
                           (float) area.getCentreY() - dotD * 0.5f,
                           dotD, dotD);
        }
    }

    juce::Font getPopupMenuFont() override { return Theme::Font::micro(); }

    int getPopupMenuBorderSize() override { return 4; }
};

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
    // No setLookAndFeel needed — inherits SpoolLookAndFeel from PluginEditor root
}

inline void tintBarSlider (juce::Slider& slider, juce::Colour colour)
{
    // Store tint as a component property so drawLinearSlider can read it live.
    slider.getProperties().set ("tint", colour.toString());
}

inline void styleTextButton (juce::TextButton& button, juce::Colour accent = Theme::Colour::surfaceEdge.withAlpha (0.7f))
{
    // Only the per-instance accent colour is needed — bg, text, border all read
    // from ThemeManager live via the inherited SpoolLookAndFeel.
    button.setColour (juce::TextButton::buttonOnColourId, accent);
}

inline void styleComboBox (juce::ComboBox& combo, juce::Colour accent = Theme::Zone::a)
{
    // Only the per-instance arrow accent is needed — bg/text/outline read
    // from ThemeManager live via the inherited SpoolLookAndFeel.
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

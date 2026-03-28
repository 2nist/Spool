#pragma once

#include "../Theme.h"
#include <cmath>

/**
 * SpoolLookAndFeel — single application-wide LookAndFeel.
 *
 * Set on PluginEditor so every child component inherits it automatically.
 * All draw methods read from ThemeManager live, so theme editor changes
 * update the entire UI on the next repaint with no per-component setup.
 *
 * Per-component accent colours (zone tints, button highlights) are still
 * set via component colour IDs (buttonOnColourId, arrowColourId) since
 * those vary per instance.
 */
class SpoolLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SpoolLookAndFeel() = default;

    //==========================================================================
    // Slider — LinearBar / LinearBarVertical
    //==========================================================================

    void drawLinearSlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float /*minSliderPos*/,
                           float /*maxSliderPos*/,
                           const juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        const bool isLinearBarStyle = (style == juce::Slider::LinearBar || style == juce::Slider::LinearBarVertical);
        const bool isLinearClassicStyle = (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearVertical);
        if (!isLinearBarStyle && !isLinearClassicStyle)
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.f, 0.f, style, slider);
            return;
        }

        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (0.5f);
        const auto& theme = ThemeManager::get().theme();
        const bool isVertical = (style == juce::Slider::LinearBarVertical || style == juce::Slider::LinearVertical);
        const float proportion = (float) slider.valueToProportionOfLength (slider.getValue());
        const float sliderR = theme.sliderCornerRadius;

        const auto bg = theme.controlBg.withAlpha (0.95f);
        const auto tintProp = slider.getProperties()["tint"];
        const auto fill = tintProp.isVoid() ? theme.sliderTrack
                                             : juce::Colour::fromString (tintProp.toString());

        const float trackH = juce::jmin (theme.sliderTrackThickness, bounds.getHeight());
        const float trackW = juce::jmin (theme.sliderTrackThickness, bounds.getWidth());
        const auto drawBounds = isVertical
                                    ? bounds.withSizeKeepingCentre (trackW, bounds.getHeight())
                                    : bounds.withSizeKeepingCentre (bounds.getWidth(), trackH);

        g.setColour (bg);
        g.fillRoundedRectangle (drawBounds, sliderR);

        if (isVertical)
        {
            const float fillH = drawBounds.getHeight() * juce::jlimit (0.f, 1.f, proportion);
            auto fillBounds = drawBounds.withY (drawBounds.getBottom() - fillH).withHeight (fillH);
            if (fillBounds.getHeight() > 0.f)
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
                    if (markerPos <= 0.f || markerPos >= 1.f)
                        continue;

                    const float xPos = drawBounds.getX() + markerPos * drawBounds.getWidth();
                    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.55f));
                    g.fillRect (xPos, drawBounds.getY() + 2.f, 1.f, drawBounds.getHeight() - 4.f);
                }
            }

            const float posX = juce::jlimit (drawBounds.getX(), drawBounds.getRight(), sliderPos);
            auto fillBounds = drawBounds.withWidth (juce::jmax (0.f, posX - drawBounds.getX()));
            if (fillBounds.getWidth() > 0.f)
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

    //==========================================================================
    // Slider — Rotary
    //==========================================================================

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        const auto& theme = ThemeManager::get().theme();
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (0.5f);
        bounds = bounds.withSizeKeepingCentre (juce::jmin (bounds.getWidth(), bounds.getHeight()),
                                               juce::jmin (bounds.getWidth(), bounds.getHeight()));

        const auto tintProp = slider.getProperties()["tint"];
        const auto accent = tintProp.isVoid() ? theme.sliderTrack
                                              : juce::Colour::fromString (tintProp.toString());
        const bool isAssigned = static_cast<bool> (slider.getProperties().getWithDefault ("slotAssigned", true));
        const float alphaScale = slider.isEnabled() ? 1.0f : 0.6f;
        const float assignmentScale = isAssigned ? 1.0f : 0.65f;

        g.setColour (theme.controlBg.withAlpha (0.95f * alphaScale));
        g.fillEllipse (bounds);
        g.setColour (theme.surfaceEdge.withAlpha (0.85f * alphaScale));
        g.drawEllipse (bounds, Theme::Stroke::subtle);

        const auto centre = bounds.getCentre();
        const float radius = bounds.getWidth() * 0.5f;
        const float ringRadius = juce::jmax (1.0f, radius - 3.0f);
        const float ringThickness = juce::jmax (0.5f, theme.knobRingThickness);

        {
            juce::Path track;
            track.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                                 rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (theme.surfaceEdge.withAlpha (0.45f * alphaScale * assignmentScale));
            g.strokePath (track, juce::PathStrokeType (ringThickness));
        }

        const float toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        {
            juce::Path fill;
            fill.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                                rotaryStartAngle, toAngle, true);
            g.setColour (accent.withAlpha (0.95f * alphaScale * assignmentScale));
            g.strokePath (fill, juce::PathStrokeType (ringThickness));
        }

        const float capRadius = juce::jmax (2.0f, radius * theme.knobCapSize);
        const auto capColour = theme.controlBg.brighter (0.1f);
        g.setColour (capColour.withAlpha (0.98f * alphaScale));
        g.fillEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);
        g.setColour (theme.surfaceEdge.withAlpha (0.9f * alphaScale));
        g.drawEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, Theme::Stroke::subtle);

        const float dotRadius = juce::jmax (1.0f, theme.knobDotSize * 0.5f);
        const float dotOrbit = juce::jmax (1.0f, ringRadius - ringThickness * 0.5f);
        const float dotX = centre.x + dotOrbit * std::cos (toAngle);
        const float dotY = centre.y + dotOrbit * std::sin (toAngle);
        g.setColour (theme.sliderThumb.interpolatedWith (accent, 0.25f).withAlpha (0.98f * alphaScale * assignmentScale));
        g.fillEllipse (dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }

    //==========================================================================
    // TextButton
    //==========================================================================

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

    //==========================================================================
    // ComboBox
    //==========================================================================

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

        const float arrowX = (float) width - 14.f;
        const float arrowY = (float) height * 0.5f;
        juce::Path arrow;
        arrow.addTriangle (arrowX, arrowY - 2.f, arrowX + 6.f, arrowY - 2.f, arrowX + 3.f, arrowY + 2.f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha (0.8f));
        g.fillPath (arrow);
    }

    //==========================================================================
    // Popup menu
    //==========================================================================

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

        if (isHighlighted && isActive)
        {
            g.setColour (theme.rowHover);
            g.fillRoundedRectangle (area.toFloat().reduced (2.f, 1.f), Theme::Radius::xs);
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

    //==========================================================================
    // Scrollbar
    //==========================================================================

    int getDefaultScrollbarWidth() override { return 10; }

    void drawScrollbar (juce::Graphics& g,
                        juce::ScrollBar& /*scrollbar*/,
                        int x, int y, int width, int height,
                        bool isScrollbarVertical,
                        int thumbStartPosition,
                        int thumbSize,
                        bool isMouseOver,
                        bool isMouseDown) override
    {
        const auto& theme = ThemeManager::get().theme();
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (1.0f);
        if (bounds.isEmpty())
            return;

        // Dark gray track by default.
        const auto track = theme.surface1.darker (0.18f).withAlpha (0.94f);
        g.setColour (track);
        g.fillRoundedRectangle (bounds, Theme::Radius::xs);

        if (thumbSize <= 0)
            return;

        juce::Rectangle<float> thumb;
        if (isScrollbarVertical)
        {
            thumb = { bounds.getX() + 1.0f,
                      (float) thumbStartPosition + 1.0f,
                      juce::jmax (2.0f, bounds.getWidth() - 2.0f),
                      juce::jmax (8.0f, (float) thumbSize - 2.0f) };
        }
        else
        {
            thumb = { (float) thumbStartPosition + 1.0f,
                      bounds.getY() + 1.0f,
                      juce::jmax (8.0f, (float) thumbSize - 2.0f),
                      juce::jmax (2.0f, bounds.getHeight() - 2.0f) };
        }

        thumb = thumb.getIntersection (bounds.reduced (0.5f));
        auto thumbColour = theme.surface3.withAlpha (0.92f);
        if (isMouseDown)
            thumbColour = thumbColour.brighter (0.14f);
        else if (isMouseOver)
            thumbColour = thumbColour.brighter (0.08f);

        g.setColour (thumbColour);
        g.fillRoundedRectangle (thumb, Theme::Radius::xs);
        g.setColour (theme.surfaceEdge.withAlpha (0.85f));
        g.drawRoundedRectangle (thumb, Theme::Radius::xs, Theme::Stroke::subtle);
    }

    void drawScrollbarButton (juce::Graphics& g,
                              juce::ScrollBar& /*scrollbar*/,
                              int width, int height,
                              int /*buttonDirection*/,
                              bool /*isScrollbarVertical*/,
                              bool /*isMouseOverButton*/,
                              bool /*isButtonDown*/) override
    {
        // Keep compact scrollbars clean: no arrow buttons.
        g.setColour (juce::Colours::transparentBlack);
        g.fillRect (0, 0, width, height);
    }
};

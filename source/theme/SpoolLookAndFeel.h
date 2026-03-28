#pragma once

#include "../Theme.h"
#include <cmath>

class SpoolLookAndFeel : public juce::LookAndFeel_V4
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
        const bool barStyle = (style == juce::Slider::LinearBar || style == juce::Slider::LinearBarVertical);
        const bool classicStyle = (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearVertical);

        if (! barStyle && ! classicStyle)
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
            return;
        }

        const bool isVertical = (style == juce::Slider::LinearBarVertical || style == juce::Slider::LinearVertical);
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (0.5f);

        const float corner = Theme::Radius::sm;
        const float trackThickness = 6.0f;
        const auto track = isVertical ? bounds.withSizeKeepingCentre (juce::jmin (trackThickness, bounds.getWidth()), bounds.getHeight())
                                      : bounds.withSizeKeepingCentre (bounds.getWidth(), juce::jmin (trackThickness, bounds.getHeight()));

        const auto tintProp = slider.getProperties()["tint"];
        const auto fill = tintProp.isVoid() ? (juce::Colour) Theme::Colour::accent
                                             : juce::Colour::fromString (tintProp.toString());
        const auto bg = Theme::Colour::surface2.withAlpha (0.95f);

        g.setColour (bg);
        g.fillRoundedRectangle (track, corner);

        const float proportion = (float) slider.valueToProportionOfLength (slider.getValue());
        if (isVertical)
        {
            const float fillH = track.getHeight() * juce::jlimit (0.0f, 1.0f, proportion);
            auto fillBounds = track.withY (track.getBottom() - fillH).withHeight (fillH);
            if (fillBounds.getHeight() > 0.0f)
            {
                g.setColour (fill);
                g.fillRoundedRectangle (fillBounds, corner);
            }
        }
        else
        {
            const float posX = juce::jlimit (track.getX(), track.getRight(), sliderPos);
            auto fillBounds = track.withWidth (juce::jmax (0.0f, posX - track.getX()));
            if (fillBounds.getWidth() > 0.0f)
            {
                g.setColour (fill);
                g.fillRoundedRectangle (fillBounds, corner);
            }
        }

        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.85f));
        g.drawRoundedRectangle (track, corner, Theme::Stroke::normal);

        const auto label = slider.getName();
        if (label.isEmpty())
            return;

        g.setFont (Theme::Font::microMedium());
        g.setColour (Theme::Helper::inkFor (fill));
        if (isVertical)
        {
            juce::Graphics::ScopedSaveState state (g);
            const auto centre = bounds.getCentre();
            g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, centre.x, centre.y));
            g.drawText (label, bounds.toNearestInt().reduced (0, 5), juce::Justification::centred, false);
        }
        else
        {
            g.drawText (label, bounds.toNearestInt().reduced (8, 0), juce::Justification::centredLeft, false);
        }
    }

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (0.5f);
        bounds = bounds.withSizeKeepingCentre (juce::jmin (bounds.getWidth(), bounds.getHeight()),
                                               juce::jmin (bounds.getWidth(), bounds.getHeight()));

        const auto tintProp = slider.getProperties()["tint"];
        const auto accent = tintProp.isVoid() ? (juce::Colour) Theme::Colour::accent
                                              : juce::Colour::fromString (tintProp.toString());
        const bool isAssigned = (bool) slider.getProperties().getWithDefault ("slotAssigned", true);
        const float assignAlpha = isAssigned ? 1.0f : 0.6f;

        g.setColour (Theme::Colour::surface2.withAlpha (0.95f));
        g.fillEllipse (bounds);
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.85f));
        g.drawEllipse (bounds, Theme::Stroke::subtle);

        const auto centre = bounds.getCentre();
        const float radius = bounds.getWidth() * 0.5f;
        const float ringRadius = juce::jmax (1.0f, radius - 3.0f);
        constexpr float ringThickness = 2.0f;

        juce::Path trackArc;
        trackArc.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f * assignAlpha));
        g.strokePath (trackArc, juce::PathStrokeType (ringThickness));

        const float toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        juce::Path fillArc;
        fillArc.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f, rotaryStartAngle, toAngle, true);
        g.setColour (accent.withAlpha (0.95f * assignAlpha));
        g.strokePath (fillArc, juce::PathStrokeType (ringThickness));

        const float capRadius = juce::jmax (2.0f, radius * 0.35f);
        g.setColour (Theme::Colour::surface3.withAlpha (0.95f));
        g.fillEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.9f));
        g.drawEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, Theme::Stroke::subtle);

        const float dotRadius = 2.0f;
        const float dotOrbit = juce::jmax (1.0f, ringRadius - ringThickness * 0.5f);
        const float dotX = centre.x + dotOrbit * std::cos (toAngle);
        const float dotY = centre.y + dotOrbit * std::sin (toAngle);
        g.setColour (Theme::Colour::inkLight.withAlpha (0.95f * assignAlpha));
        g.fillEllipse (dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }

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
        juce::Colour fill = button.getToggleState() ? (juce::Colour) Theme::Colour::surface3
                                                     : (juce::Colour) Theme::Colour::surface2;

        if (isDown)
            fill = fill.brighter (0.08f);
        else if (isHighlighted)
            fill = fill.brighter (0.05f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, Theme::Radius::sm);

        g.setColour (button.findColour (juce::TextButton::buttonOnColourId).withAlpha (0.45f));
        g.drawRoundedRectangle (bounds, Theme::Radius::sm, Theme::Stroke::normal);
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool /*isHighlighted*/,
                         bool /*isDown*/) override
    {
        g.setFont (getTextButtonFont (button, button.getHeight()));
        g.setColour (button.getToggleState() ? (juce::Colour) Theme::Colour::inkLight
                                             : (juce::Colour) Theme::Colour::inkMid);
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (4, 2), juce::Justification::centred, 1, 1.0f);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return Theme::Font::micro(); }

    void drawComboBox (juce::Graphics& g,
                       int width, int height,
                       bool /*isButtonDown*/,
                       int /*buttonX*/, int /*buttonY*/,
                       int /*buttonW*/, int /*buttonH*/,
                       juce::ComboBox& box) override
    {
        const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        g.setColour (Theme::Colour::surface2);
        g.fillRoundedRectangle (bounds, Theme::Radius::sm);
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.65f));
        g.drawRoundedRectangle (bounds, Theme::Radius::sm, Theme::Stroke::subtle);

        const float arrowX = (float) width - 14.0f;
        const float arrowY = (float) height * 0.5f;
        juce::Path arrow;
        arrow.addTriangle (arrowX, arrowY - 2.0f, arrowX + 6.0f, arrowY - 2.0f, arrowX + 3.0f, arrowY + 2.0f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha (0.8f));
        g.fillPath (arrow);
    }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        g.setColour (Theme::Colour::surface2);
        g.fillRoundedRectangle (0.0f, 0.0f, (float) width, (float) height, Theme::Radius::sm);
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.6f));
        g.drawRoundedRectangle (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f, Theme::Radius::sm, Theme::Stroke::subtle);
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
        if (isHighlighted && isActive)
        {
            g.setColour (Theme::Colour::surface3.withAlpha (0.85f));
            g.fillRoundedRectangle (area.toFloat().reduced (2.0f, 1.0f), Theme::Radius::xs);
        }

        g.setFont (Theme::Font::micro());
        g.setColour (isActive ? (isTicked ? (juce::Colour) Theme::Colour::inkLight : (juce::Colour) Theme::Colour::inkMid)
                              : (juce::Colour) Theme::Colour::inkGhost);
        g.drawFittedText (text, area.reduced (8, 0), juce::Justification::centredLeft, 1);

        if (isTicked)
        {
            g.setColour (Theme::Colour::inkLight);
            g.fillEllipse ((float) area.getRight() - 12.0f, (float) area.getCentreY() - 2.0f, 4.0f, 4.0f);
        }
    }

    juce::Font getPopupMenuFont() override { return Theme::Font::micro(); }
    int getPopupMenuBorderSize() override { return 4; }

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
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (1.0f);
        if (bounds.isEmpty())
            return;

        g.setColour (Theme::Colour::surface1.darker (0.18f).withAlpha (0.94f));
        g.fillRoundedRectangle (bounds, Theme::Radius::xs);

        if (thumbSize <= 0)
            return;

        juce::Rectangle<float> thumb;
        if (isScrollbarVertical)
            thumb = { bounds.getX() + 1.0f, (float) thumbStartPosition + 1.0f, juce::jmax (2.0f, bounds.getWidth() - 2.0f), juce::jmax (8.0f, (float) thumbSize - 2.0f) };
        else
            thumb = { (float) thumbStartPosition + 1.0f, bounds.getY() + 1.0f, juce::jmax (8.0f, (float) thumbSize - 2.0f), juce::jmax (2.0f, bounds.getHeight() - 2.0f) };

        thumb = thumb.getIntersection (bounds.reduced (0.5f));
        auto thumbColour = Theme::Colour::surface3.withAlpha (0.92f);
        if (isMouseDown)
            thumbColour = thumbColour.brighter (0.14f);
        else if (isMouseOver)
            thumbColour = thumbColour.brighter (0.08f);

        g.setColour (thumbColour);
        g.fillRoundedRectangle (thumb, Theme::Radius::xs);
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.85f));
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
        g.setColour (juce::Colours::transparentBlack);
        g.fillRect (0, 0, width, height);
    }
};

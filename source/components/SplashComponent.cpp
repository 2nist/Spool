#include "SplashComponent.h"

namespace
{
static float smoothstep (float t)
{
    t = juce::jlimit (0.0f, 1.0f, t);
    return t * t * (3.0f - 2.0f * t);
}
}

SplashComponent::SplashComponent()
{
    if (const auto logo = juce::ImageCache::getFromMemory (BinaryData::spoollogo_png, BinaryData::spoollogo_pngSize);
        logo.isValid())
    {
        frames.add (logo);
    }

    setInterceptsMouseClicks (true, true);
    setAlwaysOnTop (true);
    startTimerHz (60);
}

void SplashComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto& theme = ThemeManager::get().theme();

    // Fade in quickly at startup.
    const float fadeIn = juce::jlimit (0.0f, 1.0f, currentFrameIndex / 16.0f);
    g.setColour (juce::Colour (theme.surface0).withAlpha (0.94f * fadeIn));
    g.fillRect (bounds);

    // Subtle spotlight behind the logo.
    {
        juce::ColourGradient glow (juce::Colour (theme.accentWarm).withAlpha (0.17f * fadeIn),
                                   bounds.getCentreX(), bounds.getCentreY(),
                                   juce::Colour (theme.surface0).withAlpha (0.0f),
                                   bounds.getCentreX(), bounds.getCentreY() + juce::jmax (120.0f, bounds.getHeight() * 0.35f),
                                   true);
        g.setGradientFill (glow);
        g.fillEllipse (bounds.getCentreX() - 180.0f, bounds.getCentreY() - 140.0f, 360.0f, 280.0f);
    }

    if (!frames.isEmpty() && frames[0].isValid())
    {
        const float anim = smoothstep ((float) juce::jmin (currentFrameIndex, totalFrames) / (float) totalFrames);
        const float logoScale = 0.74f + 0.26f * anim;
        const float logoAlpha = juce::jlimit (0.0f, 1.0f, fadeIn * (0.75f + 0.25f * anim));

        const auto logo = frames[0];
        const auto maxLogoW = juce::jmin (bounds.getWidth() * 0.42f, 340.0f);
        const float srcW = (float) logo.getWidth();
        const float srcH = (float) logo.getHeight();
        const float baseW = maxLogoW;
        const float baseH = baseW * (srcH / juce::jmax (1.0f, srcW));
        const float w = baseW * logoScale;
        const float h = baseH * logoScale;
        const float x = bounds.getCentreX() - w * 0.5f;
        const float y = bounds.getCentreY() - h * 0.62f;

        g.setOpacity (logoAlpha);
        g.drawImage (logo, juce::Rectangle<float> (x, y, w, h));
    }

    const auto titleY = juce::roundToInt (bounds.getCentreY() + juce::jmax (56.0f, bounds.getHeight() * 0.15f));
    g.setColour (juce::Colour (theme.inkLight).withAlpha (0.93f * fadeIn));
    g.setFont (Theme::Font::display().boldened().withHeight (26.0f));
    g.drawText ("SPOOL", getLocalBounds().withY (titleY).withHeight (30), juce::Justification::centredTop, false);

    g.setColour (juce::Colour (theme.inkGhost).withAlpha (0.85f * fadeIn));
    g.setFont (Theme::Font::micro().withHeight (11.0f));
    g.drawText ("Loading creative session...", getLocalBounds().withY (titleY + 30).withHeight (18), juce::Justification::centredTop, false);
}

void SplashComponent::timerCallback()
{
    ++currentFrameIndex;
    repaint();

    if (currentFrameIndex >= totalFrames + holdFrames)
        finish();
}

void SplashComponent::mouseDown (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    finish();
}

void SplashComponent::finish()
{
    if (finished)
        return;

    finished = true;
    stopTimer();
    setVisible (false);
    if (onFinished)
    {
        auto callback = onFinished;
        juce::MessageManager::callAsync ([callback] () mutable
        {
            if (callback)
                callback();
        });
    }
}

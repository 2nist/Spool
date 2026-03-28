#include "OutputStrip.h"

//==============================================================================

OutputStrip::OutputStrip()
{
    setInterceptsMouseClicks (true, false);

    m_levelSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    m_levelSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    m_levelSlider.setRange (0.0, 1.0, 0.001);
    m_levelSlider.setValue (m_level, juce::dontSendNotification);
    m_levelSlider.setDoubleClickReturnValue (true, 1.0);
    m_levelSlider.setName ("LEVEL");
    m_levelSlider.getProperties().set ("tint", juce::Colour (Theme::Colour::error).toString());
    m_levelSlider.onValueChange = [this]
    {
        m_level = (float) m_levelSlider.getValue();
        if (onLevelChanged)
            onLevelChanged (m_level);
    };
    addAndMakeVisible (m_levelSlider);
}

//==============================================================================
// Setters
//==============================================================================

void OutputStrip::setLevel (float v)
{
    m_level = juce::jlimit (0.0f, 1.0f, v);
    m_levelSlider.setValue (m_level, juce::dontSendNotification);
    repaint();
}

void OutputStrip::setPan (float v)
{
    m_pan = juce::jlimit (-1.0f, 1.0f, v);
    repaint();
}

//==============================================================================
// Region helpers
//==============================================================================

juce::Rectangle<int> OutputStrip::stripeRect() const noexcept
{
    return { 0, 0, kStripeW, getHeight() };
}

juce::Rectangle<int> OutputStrip::nameLabelRect() const noexcept
{
    return { kStripeW + kPad, 0, kNameW, getHeight() };
}

juce::Rectangle<int> OutputStrip::levelTrackRect() const noexcept
{
    const int x  = nameLabelRect().getRight() + kPad;
    const int w  = getWidth() - x - kPad;
    const int cy = getHeight() / 2;
    const int h  = 6;
    return { x, cy - h/2, juce::jmax (0, w), h };
}

void OutputStrip::paint (juce::Graphics& g)
{
    // Background
    g.setColour (Theme::Colour::surface1);
    g.fillAll();

    // Left stripe (error/output red)
    g.setColour (Theme::Colour::error);
    g.fillRect  (stripeRect());

    // "OUTPUT" label
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::error.withAlpha (0.9f));
    g.drawText  ("OUTPUT", nameLabelRect(), juce::Justification::centredLeft, false);

    // Top border
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine  (0.0f, 0.0f, static_cast<float> (getWidth()), 0.0f, 0.7f);
}

//==============================================================================
void OutputStrip::resized()
{
    m_levelSlider.setBounds (levelTrackRect());
}

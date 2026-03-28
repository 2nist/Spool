#include "MacroKnobsStrip.h"

//==============================================================================

MacroKnobsStrip::MacroKnobsStrip()
{
    for (int i = 0; i < kNumKnobs; ++i)
    {
        auto& knob = m_knobs[(size_t) i];
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knob.setRange (0.0, 1.0, 0.001);
        knob.setValue (kDefault, juce::dontSendNotification);
        knob.setDoubleClickReturnValue (true, kDefault);
        knob.setScrollWheelEnabled (false);
        knob.setPopupDisplayEnabled (true, true, this);
        knob.getProperties().set ("tint", juce::Colour (Theme::Zone::c).toString());
        addAndMakeVisible (knob);

        auto& label = m_labels[(size_t) i];
        label.setText (m_labelText[(size_t) i], juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
    }
}

//==============================================================================

void MacroKnobsStrip::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface0);

    // Top border
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine (0.0f, 0.0f, static_cast<float> (getWidth()), 0.0f, Theme::Stroke::subtle);
}

//==============================================================================
void MacroKnobsStrip::resized()
{
    const int w = getWidth();
    const int slotW = w / kNumKnobs;
    const int knobTop = 6;
    const int knobH = kKnobD + 4;

    for (int i = 0; i < kNumKnobs; ++i)
    {
        auto slot = juce::Rectangle<int> (i * slotW, 0,
                                          i == kNumKnobs - 1 ? w - i * slotW : slotW,
                                          getHeight());

        const int controlW = juce::jmin (48, juce::jmax (36, slot.getWidth() - 4));
        m_knobs[(size_t) i].setBounds (slot.getCentreX() - controlW / 2, knobTop, controlW, knobH);

        auto& label = m_labels[(size_t) i];
        label.setFont (Theme::Font::micro());
        label.setColour (juce::Label::textColourId, Theme::Colour::inkGhost);
        label.setBounds (slot.getX(), knobTop + knobH + 2, slot.getWidth(), 12);
    }
}

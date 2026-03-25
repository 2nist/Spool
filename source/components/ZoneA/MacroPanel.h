#pragma once
#include "../../Theme.h"

//==============================================================================
/** MacroPanel — stub. 8 macro knob assignments and MIDI learn. */
class MacroPanel : public juce::Component
{
public:
    MacroPanel() = default;
    ~MacroPanel() override = default;

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Theme::Zone::bgA);
        const auto b = getLocalBounds();
        g.setFont   (Theme::Font::label());
        g.setColour (Theme::Zone::b);
        g.drawText  ("MACRO", b.withHeight (24), juce::Justification::centred, false);
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("8 macro knob assignments and MIDI learn\n\xe2\x80\x94 coming soon",
                     b.reduced (12).withTrimmedTop (32),
                     juce::Justification::centredTop, true);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroPanel)
};

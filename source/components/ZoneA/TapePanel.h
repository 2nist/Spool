#pragma once
#include "../../Theme.h"

//==============================================================================
/** TapePanel — stub. Circular buffer capture controls and looper settings.
    Intentionally uses flat dark matte background (#1a1208). */
class TapePanel : public juce::Component
{
public:
    TapePanel() = default;
    ~TapePanel() override = default;

    void paint (juce::Graphics& g) override
    {
        // Intentional departure from theme — flat dark matte
        g.fillAll (juce::Colour (0xFF1a1208));
        const auto b = getLocalBounds();
        g.setFont   (Theme::Font::label());
        g.setColour (Theme::Zone::d);
        g.drawText  ("TAPE", b.withHeight (24), juce::Justification::centred, false);
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.6f));
        g.drawText  ("Circular buffer capture controls\nand looper settings \xe2\x80\x94 coming soon",
                     b.reduced (12).withTrimmedTop (32),
                     juce::Justification::centredTop, true);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapePanel)
};

#pragma once
#include "../../Theme.h"
#include "ZoneAStyle.h"

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
        g.fillAll (Theme::Zone::bgA);
        const auto accent = ZoneAStyle::accentForTabId ("tape");
        const auto b = getLocalBounds();
        ZoneAStyle::drawHeader (g, b.withHeight (24), "TAPE", accent);
        ZoneAStyle::drawCard (g, b.reduced (8).withTrimmedTop (30), accent);

        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.6f));
        g.drawText  ("Circular buffer capture controls\nand looper settings are still being folded into the new workflow.",
                     b.reduced (18).withTrimmedTop (44),
                     juce::Justification::centredTop, true);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapePanel)
};

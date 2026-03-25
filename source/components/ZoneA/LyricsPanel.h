#pragma once
#include "../../Theme.h"

//==============================================================================
/** LyricsPanel — stub. Timestamped lyric blocks with LRC import. */
class LyricsPanel : public juce::Component
{
public:
    LyricsPanel() = default;
    ~LyricsPanel() override = default;

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Theme::Zone::bgA);
        const auto b = getLocalBounds();
        g.setFont   (Theme::Font::label());
        g.setColour (Theme::Colour::inkMid);
        g.drawText  ("LYRICS", b.withHeight (24), juce::Justification::centred, false);
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("Timestamped lyric blocks with LRC import\nand playhead sync \xe2\x80\x94 coming soon",
                     b.reduced (12).withTrimmedTop (32),
                     juce::Justification::centredTop, true);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LyricsPanel)
};

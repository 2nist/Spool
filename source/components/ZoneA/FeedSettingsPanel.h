#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    FeedSettingsPanel — settings for the SystemFeedCylinder.

    Opened via the FEED tab in the Zone A tab strip.

    Content:
      "SYSTEM FEED" header
      Toggle list (MIDI in, MIDI out, System info, Transport, Link, Warnings)
      Scroll speed slider
      CLEAR FEED button
*/
class FeedSettingsPanel : public juce::Component
{
public:
    FeedSettingsPanel();
    ~FeedSettingsPanel() override = default;

    /** Called when the feed filter toggles change. */
    std::function<void(const juce::String& item, bool enabled)> onFeedToggleChanged;

    /** Called when the user presses CLEAR FEED. */
    std::function<void()> onClearFeed;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    struct FeedItem
    {
        juce::String name;
        bool enabled { true };
    };

    juce::Array<FeedItem> m_items;

    juce::Slider     m_speedSlider;
    juce::TextButton m_clearBtn { "CLEAR FEED" };

    static constexpr int kPad  = 8;
    static constexpr int kRowH = 24;

    juce::Rectangle<int> itemRect (int index) const noexcept;
    void                 onItemClicked (int index, juce::Point<int> localPos);

    void mouseDown (const juce::MouseEvent&) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeedSettingsPanel)
};

#pragma once

#include "../../Theme.h"
#include "../../theme/ThemeEditorPanel.h"
#include "RoutingPanel.h"
#include "ButtonSlot.h"

//==============================================================================
/**
    PanelContentView — content area for a Zone A panel slot.

    When panelId == "theme" it hosts a ThemeEditorPanel; otherwise it shows
    a stub (panel name + "coming soon") until real content is built.
*/
class PanelContentView : public juce::Component
{
public:
    PanelContentView();
    ~PanelContentView() override = default;

    /** Configure which panel this view represents.
        @param fullName   Display name (e.g. "THEME EDITOR")
        @param panelId    Short routing key (e.g. "theme", "mixer", …)
        @param colour     Zone accent colour for stub display
    */
    void setPanel (const juce::String& fullName,
                   const juce::String& panelId,
                   const juce::Colour& colour);

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Returns a small, display-friendly routing summary (e.g. "Stereo"). */
    juce::String getRoutingSummary() const;

    /** Callback fired when the routing matrix changes in the UI. */
    std::function<void (const std::array<uint8_t, 8>&)> onRoutingChanged;

    /** Programmatically set the routing matrix (8 bits). */
    void setRoutingMatrix (const std::array<uint8_t, 8>& m);

private:
    juce::String  m_fullName;
    juce::String  m_panelId;
    juce::Colour  m_colour { Theme::Colour::inkMid };

    std::unique_ptr<ThemeEditorPanel> m_themePanel;
    std::unique_ptr<RoutingPanel>     m_routingPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PanelContentView)
};

#pragma once

#include "../Theme.h"

//==============================================================================
/**
    SPOOL Menu Bar
    --------------
    Always-visible 28px system strip at the top of the window.
    Static paint-only component — no interactivity, no state.

    Height is exactly Theme::Space::menuBarHeight (28px).
    All visual values trace back to Theme.h.
*/
class MenuBarComponent : public juce::Component
{
public:
    MenuBarComponent();
    ~MenuBarComponent() override = default;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenuBarComponent)
};

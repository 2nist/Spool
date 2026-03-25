#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    BreadcrumbBar — renders the navigation path when Zone A is in panel view.

    Shows:  "ZONE A  ›  PANEL NAME  ›  SUB PANEL"
    Clicking a crumb fires onCrumbClicked(depth) so ZoneAComponent can
    pop the active slot's stack back to that depth.

    Height: 16px fixed.
*/
class BreadcrumbBar : public juce::Component
{
public:
    BreadcrumbBar();
    ~BreadcrumbBar() override = default;

    /** Supply the crumb labels to display (shallowest first).
        The last entry is the current panel and is drawn in activeColour.
        All earlier entries are drawn in inkGhost. */
    void setCrumbs (const juce::StringArray& labels,
                    const juce::Colour&      activeColour);

    /** Called when the user clicks a breadcrumb.
        Argument is 0-based depth index (0 = root). */
    std::function<void (int depth)> onCrumbClicked;

    /** Small one-line routing summary drawn at the right. */
    void setRoutingSummary (const juce::String& s);

    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::StringArray  m_labels;
    juce::Colour       m_activeColour { Theme::Colour::inkLight };
    juce::String       m_routingSummary;

    /** Cached hit-test rectangles for each crumb, built in paint(). */
    juce::Array<juce::Rectangle<float>> m_crumbRects;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BreadcrumbBar)
};

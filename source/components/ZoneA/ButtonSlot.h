#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    ButtonSlot — data model for one of the 16 Zone A navigation slots.

    No UI code lives here. ZoneAComponent reads this struct to paint each
    slot and to manage per-slot independent navigation stacks.
*/
struct ButtonSlot
{
    //==========================================================================
    // Identity

    /** Empty string means the slot is unassigned. */
    juce::String panelId;

    /** Four-character uppercase display label, e.g. "STR", "PAT". */
    juce::String label;

    /** Full human name, e.g. "STRUCTURE", "PATTERNS". */
    juce::String fullName;

    /** Single Unicode character used as an icon glyph. */
    juce::String iconChar;

    /** The accent colour associated with this panel. */
    juce::Colour colour { Theme::Colour::inkMid };

    //==========================================================================
    // Navigation stack

    /** Ordered list of panel IDs visited, shallowest first.
        When the slot is first pressed, panelId is pushed onto this stack.
        Navigating deeper inside a panel pushes additional IDs.
        The back button pops from the top. */
    juce::StringArray navigationStack;

    /** Index into navigationStack that is currently displayed.
        -1 means the slot has never been entered (stack empty). */
    int currentDepth { -1 };

    //==========================================================================
    // Helpers

    bool isAssigned()  const noexcept { return panelId.isNotEmpty(); }
    bool hasHistory()  const noexcept { return currentDepth > 0; }

    /** What panel ID is currently shown for this slot (top of visible stack). */
    juce::String currentPanelId() const
    {
        if (navigationStack.isEmpty() || currentDepth < 0)
            return panelId;
        return navigationStack[currentDepth];
    }

    /** Push a new panel onto this slot's stack and advance depth. */
    void pushPanel (const juce::String& id)
    {
        // Truncate any forward history above current position
        while (navigationStack.size() > currentDepth + 1)
            navigationStack.remove (navigationStack.size() - 1);

        navigationStack.add (id);
        currentDepth = navigationStack.size() - 1;
    }

    /** Pop back one level. Returns true if there is still a panel to show. */
    bool popPanel()
    {
        if (currentDepth > 0)
        {
            --currentDepth;
            return true;
        }
        currentDepth = -1;
        return false;
    }

    /** Reset stack to just the root panel. */
    void resetStack()
    {
        navigationStack.clear();
        currentDepth = -1;
        if (panelId.isNotEmpty())
        {
            navigationStack.add (panelId);
            currentDepth = 0;
        }
    }

    /** Build a display array of full names for breadcrumb rendering.
        The caller must map panel IDs to full names externally. */
    juce::StringArray breadcrumbIds() const
    {
        if (currentDepth < 0)
            return {};
        juce::StringArray crumbs;
        for (int i = 0; i <= currentDepth; ++i)
            crumbs.add (navigationStack[i]);
        return crumbs;
    }
};

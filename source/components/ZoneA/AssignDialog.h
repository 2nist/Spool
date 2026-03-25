#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    AssignDialog — stub overlay for assigning a panel to a button slot.

    Full functionality (search, categories, drag) deferred to a later session.
    This stub shows the slot index, a list of available panel IDs, and
    ASSIGN / CANCEL buttons.
*/
class AssignDialog : public juce::Component
{
public:
    explicit AssignDialog (int slotIndex);
    ~AssignDialog() override = default;

    /** Called when the user confirms an assignment.
        Arguments: slotIndex, chosen panelId. */
    std::function<void (int slotIndex, const juce::String& panelId)> onAssign;

    /** Called when the dialog is dismissed without assigning. */
    std::function<void()> onCancel;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    int              m_slotIndex;
    juce::ListBox    m_list;
    juce::TextButton m_assignBtn { "ASSIGN" };
    juce::TextButton m_cancelBtn { "CANCEL" };

    // Available panels (id, display name pairs)
    struct PanelEntry { juce::String id, name; };
    juce::Array<PanelEntry> m_entries;

    // ListBoxModel inline implementation
    struct Model : public juce::ListBoxModel
    {
        juce::Array<PanelEntry>* entries { nullptr };
        int getNumRows() override { return entries ? entries->size() : 0; }
        void paintListBoxItem (int row, juce::Graphics& g,
                               int w, int h, bool selected) override
        {
            if (!entries || row >= entries->size()) return;
            g.fillAll (selected ? Theme::Colour::surface4 : Theme::Colour::surface3);
            g.setFont (Theme::Font::label());
            g.setColour (selected ? Theme::Colour::inkLight : Theme::Colour::inkMuted);
            g.drawText ((*entries)[row].name,
                        juce::Rectangle<int> (static_cast<int> (Theme::Space::sm), 0, w, h),
                        juce::Justification::centredLeft, false);
        }
    } m_model;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AssignDialog)
};

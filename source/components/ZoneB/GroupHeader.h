#pragma once

#include "ModuleGroup.h"
#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
/**
    GroupHeader — 20px-tall coloured divider for one module group.

    Left side:  12×12 colour swatch (click → ColourSelector popup)
                Group name in group colour (double-click → inline rename)
                Slot count in inkGhost

    Right side: ADD SLOT "+"  |  COLLAPSE "▾/▴"  |  DELETE "×"
*/
class GroupHeader : public juce::Component,
                    public juce::ChangeListener
{
public:
    explicit GroupHeader (ModuleGroup& group);
    ~GroupHeader() override;

    /** Called externally (e.g. after ADD GROUP) to begin inline rename. */
    void startRename();

    /** Refresh visuals (e.g. after slot count changes). */
    void refresh() { repaint(); }

    std::function<void()>             onAddSlot;
    std::function<void()>             onToggleCollapse;
    std::function<void()>             onDelete;
    std::function<void(juce::Colour)> onColorChanged;
    std::function<void(juce::String)> onNameChanged;

    void paint            (juce::Graphics&) override;
    void resized          () override;
    void mouseDown        (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    // juce::ChangeListener for ColourSelector
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    static constexpr int kHeight  = 20;

private:
    ModuleGroup&          m_group;
    juce::ColourSelector* m_selectorPtr { nullptr };

    std::unique_ptr<juce::TextEditor> m_nameEditor;
    juce::TextButton                  m_addSlotBtn { "+" };
    juce::TextButton                  m_collapseBtn { "\xe2\x96\xbd" };
    juce::TextButton                  m_menuBtn { "..." };

    static constexpr int kStripeW = 3;

    juce::Rectangle<int> nameRect() const noexcept;
    juce::Rectangle<int> slotCountRect() const noexcept;

    void openColorPicker();
    void commitRename();
    void showContextMenu();
    void toggleCollapsed();
    void syncButtonState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GroupHeader)
};

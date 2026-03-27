#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../preset/PresetManager.h"
#include "../../Theme.h"
#include "ZoneAControlStyle.h"

//==============================================================================
/**
    PresetBar — compact preset browser / name editor mounted in the
    InstrumentPanel editor header.

    Layout (left to right, height = kHeight):
      [ preset name (editable TextEditor) | ▾ browse | INIT | SAVE ]

    Name field:
      - Editable inline.  Committed on Return or focus-lost.
      - If the name changed on commit, auto-saves as a new user preset.
      - Displays draft indicator ("*") in text colour when dirty.

    ▾ button:  opens PopupMenu grouping factory and user presets.
    INIT:      loads the module-specific init preset (first factory entry).
    SAVE:      saves current state with the current name.
*/
class PresetBar  : public juce::Component,
                   private juce::TextEditor::Listener
{
public:
    static constexpr int kHeight = 26;

    PresetBar();
    ~PresetBar() override = default;

    //==========================================================================
    // Configuration

    void setModuleType (PresetManager::ModuleType type);

    /** Update displayed name (called by InstrumentPanel after a load). */
    void displayPreset (const juce::String& name, bool isFactory);

    juce::String getCurrentName() const;

    //==========================================================================
    // Callbacks wired by InstrumentPanel

    /** Called when a preset is about to be applied.
        Return the current processor state so it can be captured for save. */
    std::function<juce::var()>             onRequestState;

    /** Called with pre-parsed JSON when the user selects a preset to load. */
    std::function<void(const juce::var&)>  onLoadPreset;

    //==========================================================================
    // Component overrides

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::TextEditor m_nameEditor;
    juce::TextButton m_browseBtn  { "\xe2\x96\xbe" };   // ▾
    juce::TextButton m_initBtn    { "INIT" };
    juce::TextButton m_saveBtn    { "SAVE" };

    PresetManager::ModuleType m_moduleType { PresetManager::ModuleType::Synth };

    /** Name that was last committed/saved — used to detect dirty state. */
    juce::String m_committedName;

    bool isDirty() const;

    //==========================================================================
    // Actions

    void showBrowsePopup();
    void saveCurrentPreset();
    void loadInitPreset();
    void commitNameChanges();

    //==========================================================================
    // TextEditor::Listener

    void textEditorReturnKeyPressed (juce::TextEditor&) override;
    void textEditorFocusLost        (juce::TextEditor&) override;

    static void styleButton (juce::TextButton& b);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};

#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    InstrumentPanel — two-state panel in Zone A INSTRUMENT tab.

    STATE A — Browser: lists all loaded module slots grouped by Zone B groups.
      Click a slot row → switches to STATE B.

    STATE B — Editor: shows instrument controls for the focused slot.
      Module-type-specific editor (SYNTH, DRUMS, REEL — others stub).
      Back arrow → returns to browser.

    The panel is populated via setModuleList() which accepts the same
    "<Group>:<Type>" strings fired by ZoneBComponent::onModuleListChanged.

    When showSlot(slotIndex) is called (from PluginEditor::slotSelected)
    the panel switches to editor STATE B for that slot.
*/
class InstrumentPanel : public juce::Component
{
public:
    InstrumentPanel();
    ~InstrumentPanel() override = default;

    //==========================================================================
    // Data population

    struct SlotEntry
    {
        int          slotIndex  { -1 };
        juce::String groupName;
        juce::Colour groupColor { 0xFF4B9EDB };
        juce::String moduleType;   // "SYNTH", "DRUMS", "REEL", "OUTPUT", ""
        juce::String displayName;  // e.g. "SYNTH 01"
    };

    /** Rebuild the browser list from slot entries. */
    void setSlots (const juce::Array<SlotEntry>& slots);

    //==========================================================================
    // Navigation

    /** Show editor state for this slot.  Creates editor if needed. */
    void showSlot (int slotIndex);

    /** Return to browser. */
    void showBrowser();

    //==========================================================================
    // Callbacks

    /** Fired when user selects a slot from the browser. */
    std::function<void(int slotIndex)> onSlotSelected;

    //==========================================================================
    // juce::Component

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    enum class State { browser, editor };

    State        m_state       { State::browser };
    int          m_activeSlot  { -1 };

    juce::Array<SlotEntry> m_slots;

    //==========================================================================
    // Browser layout helpers

    static constexpr int kBrowserHeaderH = 24;
    static constexpr int kGroupHeaderH   = 18;
    static constexpr int kSlotRowH       = 32;
    static constexpr int kPad            = 6;

    /** Calculates the Y position of each slot row in browser mode.
        Fills m_rowYCache (slotIndex → y in browser space). */
    void rebuildRowCache();
    juce::Array<std::pair<int,int>> m_rowYCache;  // {slotIndex, y}

    int slotAtBrowserY (int y) const noexcept;

    void paintBrowser      (juce::Graphics& g) const;
    void paintBrowserSlot  (juce::Graphics& g,
                            const SlotEntry& entry,
                            juce::Rectangle<int> r,
                            bool hovered) const;

    //==========================================================================
    // Editor

    static constexpr int kEditorHeaderH = 32;

    void paintEditor       (juce::Graphics& g) const;
    void paintEditorStub   (juce::Graphics& g,
                            const SlotEntry* entry) const;
    void paintPolySynth    (juce::Graphics& g,
                            const SlotEntry* entry) const;

    juce::Rectangle<int> backBtnRect() const noexcept;

    //==========================================================================
    // Hover tracking

    int m_hoveredSlot { -1 };

    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    //==========================================================================
    // Type badge color

    static juce::Colour typeColor (const juce::String& moduleType);
    static juce::String typeShort (const juce::String& moduleType);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstrumentPanel)
};

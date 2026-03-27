#pragma once

#include "../../Theme.h"
#include "../ZoneB/DrumMachineData.h"
#include "PresetBar.h"

class PluginProcessor;
class PolySynthEditorComponent;
class DrumEditorComponent;

//==============================================================================
/**
    InstrumentPanel — slot-browser + mounted editor host in Zone A INSTRUMENT tab.

    STATE A — Browser: lists all loaded module slots grouped by Zone B group.
      Click a slot row → fires onSlotSelected, switches to STATE B.

    STATE B — Editor: mounts the correct child editor for the slot family:
      SYNTH  → PolySynthEditorComponent (fully wired to PolySynthProcessor)
      DRUMS  → DrumEditorComponent      (real component, editor in next pass)
      other  → generic stub (painted inline)

    Populated via setSlots() which accepts the same "<Group>:<Type>" strings
    fired by ZoneBComponent::onModuleListChanged (parsed by ZoneAComponent).

    REEL slots appear in the browser list but clicking them fires onSlotSelected
    → ZoneAComponent::switchToModulePanel("REEL") which opens the dedicated
    reel_N panel.  REEL editing lives there, not here.
*/
class InstrumentPanel : public juce::Component
{
public:
    explicit InstrumentPanel (PluginProcessor& p);
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

    /** Show editor state for this slot.  Mounts the correct editor child. */
    void showSlot (int slotIndex);

    /** Return to browser. */
    void showBrowser();

    //==========================================================================
    // Callbacks

    /** Fired when the user selects a slot from the browser.
        Passes both the slot index and the module-type string so the caller
        (ZoneAComponent / PluginEditor) can perform full slot-selection. */
    std::function<void(int slotIndex, const juce::String& moduleType)> onSlotSelected;

    /** Fetch the authoritative drum state for a given slot when mounting
        the Zone A drum editor or capturing/loading drum presets. */
    std::function<DrumMachineData* (int slotIndex)> onRequestDrumState;

    /** Called after the mounted drum editor or a preset load mutates the shared
        drum state. The owner should apply the state to DSP/sequencing. */
    std::function<void (int slotIndex, const DrumMachineData& state)> onDrumStateChanged;

    /** Trigger a preview hit for the given drum voice from the mounted editor. */
    std::function<void (int slotIndex, int voiceIndex, float velocity)> onPreviewDrumVoice;

    //==========================================================================
    // juce::Component

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    PluginProcessor& processorRef;

    enum class State { browser, editor };

    State        m_state       { State::browser };
    int          m_activeSlot  { -1 };

    juce::Array<SlotEntry> m_slots;

    //==========================================================================
    // Editor host

    juce::Viewport m_editorViewport;

    std::unique_ptr<PolySynthEditorComponent> m_polySynthEditor;
    std::unique_ptr<DrumEditorComponent>      m_drumEditor;

    void mountEditor   (const SlotEntry& entry);
    void unmountEditor ();

    //==========================================================================
    // Preset bar (visible only in editor state for SYNTH / DRUMS)

    PresetBar m_presetBar;

    static constexpr int kPresetBarH = PresetBar::kHeight;

    void wirePresetBar (const SlotEntry& entry);

    //==========================================================================
    // Browser layout helpers

    static constexpr int kBrowserHeaderH = 24;
    static constexpr int kGroupHeaderH   = 18;
    static constexpr int kSlotRowH       = 32;
    static constexpr int kPad            = 6;

    void rebuildRowCache();
    juce::Array<std::pair<int,int>> m_rowYCache;  // {slotIndex, y}

    int  slotAtBrowserY (int y) const noexcept;
    void paintBrowser     (juce::Graphics& g) const;
    void paintBrowserSlot (juce::Graphics& g,
                           const SlotEntry& entry,
                           juce::Rectangle<int> r,
                           bool hovered) const;

    //==========================================================================
    // Editor header

    static constexpr int kEditorHeaderH = 32;

    void paintEditorHeader (juce::Graphics& g) const;

    juce::Rectangle<int> backBtnRect()     const noexcept;
    const SlotEntry*     getActiveEntry()  const noexcept;

    //==========================================================================
    // Hover tracking

    int m_hoveredSlot { -1 };

    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    //==========================================================================
    // Type badge helpers

    static juce::Colour typeColor (const juce::String& moduleType);
    static juce::String typeShort (const juce::String& moduleType);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstrumentPanel)
};

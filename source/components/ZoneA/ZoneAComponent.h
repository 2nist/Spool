#pragma once

#include "../../Theme.h"
#include "../ZoneB/OutputStrip.h"
#include "PatchBayPanel.h"
#include "RoutingPanel.h"
#include "../../theme/ThemeEditorPanel.h"
#include "TracksPanel.h"
#include "FeedSettingsPanel.h"
#include "SongInfoPanel.h"
#include "TransportSettingsPanel.h"
#include "ReelEditorPanel.h"
// New panels
#include "InstrumentPanel.h"
#include "MixerPanel.h"
#include "StructurePanel.h"
#include "LyricsPanel.h"
#include "MacroPanel.h"
#include "TapePanel.h"
#include "AutoPanel.h"

class PluginProcessor;

//==============================================================================
/**
    ZoneAComponent — content panel for the Zone A tab strip.

    Sits to the right of the TabStrip (28px).  Width is managed by PluginEditor —
    it collapses to zero when no tab is selected.

    setActivePanel(tabId) switches the visible panel.  All panels are lazily
    created on first activation.

    Tab IDs (matches TabStrip):
      SESSION     : song, structure, lyrics
      INSTRUMENT  : instrument
      PERFORMANCE : mixer, macro
      SIGNAL      : routing
      CAPTURE     : tape
      TRACK       : tracks, automate

    The OutputStrip is always visible at the bottom of the content panel.
*/
class ZoneAComponent : public juce::Component
{
public:
  ZoneAComponent (PluginProcessor& p);
  ~ZoneAComponent() override = default;

    //--- Tab panel switching --------------------------------------------------

    //--- Tab panel switching --------------------------------------------------
    void setActivePanel (const juce::String& tabId);
    juce::String getActivePanelId() const noexcept { return m_activePanelId; }

    //--- Output strip callbacks -----------------------------------------------
    void setOnLevelChanged (std::function<void (float)> cb) { m_outputStrip.onLevelChanged = std::move (cb); }
    void setOnPanChanged   (std::function<void (float)> cb) { m_outputStrip.onPanChanged   = std::move (cb); }

    //--- Routing panel callbacks ----------------------------------------------
    void setOnRoutingChanged (std::function<void (const std::array<uint8_t, 8>&)> cb);
    void setRoutingMatrix    (const std::array<uint8_t, 8>& m);

    //--- Patch bay ------------------------------------------------------------
    void setPatchModuleNames (const juce::StringArray& names);
    PatchBayPanel* getPatchBayPanel() noexcept { return m_patchPanel.get(); }

    //--- Module auto-switch (called from PluginEditor::slotSelected) ----------
    /** Switch Zone A to the appropriate editor panel for the given module type.
        moduleType strings: "REEL", "SYNTH", "DRUMS", "VST3", "OUTPUT", "".
        slotIndex is used to restore per-slot panel state. */
    void switchToModulePanel (const juce::String& moduleType, int slotIndex);

    ReelEditorPanel* getReelPanel (int slotIndex) noexcept;

    //--- InstrumentPanel interface --------------------------------------------
    InstrumentPanel* getInstrumentPanel() noexcept { return m_instrumentPanel.get(); }

    /** Fired when the user selects a slot from the InstrumentPanel browser.
        Wire in PluginEditor to perform full slot selection across all zones. */
    std::function<void(int slotIndex, const juce::String& moduleType)> onInstrumentSlotSelected;

    /** Update InstrumentPanel browser list from Zone B module list.
        Format: "GroupName:ModuleType" e.g. "SYNTHS:SYNTH" */
    void setInstrumentSlots (const juce::StringArray& moduleList);

    /** Update MixerPanel strip list from Zone B module list (same format). */
    void setMixerSlots (const juce::StringArray& moduleList);

    /** Update TracksPanel lane list from Zone B module list (same format). */
    void setTrackLanes (const juce::StringArray& moduleList);

    //--- Direct panel access for PluginEditor wiring --------------------------
    TracksPanel*            getTracksPanel()    noexcept { return m_tracksPanel.get();    }
    FeedSettingsPanel*      getFeedPanel()      noexcept { return m_feedPanel.get();      }
    SongInfoPanel*          getSongPanel()      noexcept { return m_songPanel.get();      }
    TransportSettingsPanel* getTransportPanel() noexcept { return m_transportPanel.get(); }
    StructurePanel*         getStructurePanel() noexcept { return m_structurePanel.get(); }

    /** Preferred expanded width (content only, not counting the 28px tab strip). */
    static constexpr int getPreferredWidth() noexcept
    {
        return static_cast<int> (Theme::Space::zoneAWidth) - 28;
    }

    //--- Kept for backward compat with PluginEditor ---------------------------
    void navigateToPanel (const juce::String& panelId) { setActivePanel (panelId); }

    //--- juce::Component ------------------------------------------------------
    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    PluginProcessor& processorRef;
    juce::String m_activePanelId;

    //--- Panels (lazily created) ----------------------------------------------

    // Existing panels
    std::unique_ptr<PatchBayPanel>          m_patchPanel;
    std::unique_ptr<ThemeEditorPanel>       m_themePanel;
    std::unique_ptr<RoutingPanel>           m_routingPanel;
    std::unique_ptr<TracksPanel>            m_tracksPanel;
    std::unique_ptr<FeedSettingsPanel>      m_feedPanel;
    std::unique_ptr<SongInfoPanel>          m_songPanel;
    std::unique_ptr<TransportSettingsPanel> m_transportPanel;

    // New panels
    std::unique_ptr<InstrumentPanel>  m_instrumentPanel;
    std::unique_ptr<MixerPanel>       m_mixerPanel;
    std::unique_ptr<StructurePanel>   m_structurePanel;
    std::unique_ptr<LyricsPanel>      m_lyricsPanel;
    std::unique_ptr<MacroPanel>       m_macroPanel;
    std::unique_ptr<TapePanel>        m_tapePanel;
    std::unique_ptr<AutoPanel>        m_autoPanel;

    // Per-slot REEL editors (one per slot, created on demand)
    std::unique_ptr<ReelEditorPanel> m_reelPanels[8];

    OutputStrip m_outputStrip;

    std::function<void (const std::array<uint8_t, 8>&)> m_onRoutingChanged;

    //--- Helpers --------------------------------------------------------------
    juce::Component* activePanel() const noexcept;
    void             ensurePanelCreated (const juce::String& tabId);
    void             hideAllPanels();

    juce::Rectangle<int> panelArea()  const noexcept;
    juce::Rectangle<int> outputArea() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZoneAComponent)
};

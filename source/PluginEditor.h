#pragma once

#include "PluginProcessor.h"
#include "Theme.h"
#include "dsp/CapturedAudioClip.h"
#include "components/ZoneC/ChainState.h"
#include "components/ZoneB/SlotPattern.h"
#include "theme/ThemeManager.h"
#include "components/MenuBarComponent.h"
#include "components/SongHeader/SongHeaderStrip.h"
#include "components/SystemFeed/SystemFeedCylinder.h"
#include "components/ZoneA/TabStrip.h"
#include "components/ZoneA/ZoneAComponent.h"
#include "components/ZoneB/ZoneBComponent.h"
#include "components/ZoneC/ZoneCComponent.h"
#include "components/ZoneD/ZoneDComponent.h"
#include "components/ZoneResizer.h"
#include "components/AudioHistoryStrip.h"
#include "components/SettingsPanel.h"
#include "components/SplashComponent.h"
#include "state/AppPreferences.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "theme/SpoolLookAndFeel.h"
#include <juce_audio_formats/juce_audio_formats.h>

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor,
                     public juce::DragAndDropContainer,
                     public ZoneBComponent::Listener,
                     public TransportListener,
                     public ThemeManager::Listener,
                     private AppPreferences::Listener
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint   (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized () override;
    bool keyPressed (const juce::KeyPress& key) override;
    void refreshFromAuthoredSong();

    // ZoneBComponent::Listener
    void slotSelected   (int slotIndex, const juce::String& moduleType) override;
    void slotDeselected () override;

    // TransportListener
    void playStateChanged   (bool playing)   override;
    void positionChanged    (float beat)     override;
    void recordStateChanged (bool recording) override;

    // ThemeManager::Listener
    void themeChanged() override;

private:
    void appPreferencesChanged() override;
    static void repaintAll (juce::Component& root);
    PluginProcessor& processorRef;
    MenuBarComponent   menuBar;
    SongHeaderStrip    songHeader;
    SystemFeedCylinder systemFeed;
    TabStrip           m_tabStrip;
    ZoneAComponent     zoneA;
    ZoneBComponent     zoneB;
    ZoneCComponent     zoneC;
    ZoneDComponent     zoneD;
    AudioHistoryStrip  historyStrip;

    // Zone resizers — placed on boundaries, drawn on top
    ZoneResizer m_resizerAB   { ZoneResizer::Direction::horizontal };   // Zone A | Zone B
    ZoneResizer m_resizerBC   { ZoneResizer::Direction::horizontal };   // Zone B | Zone C
    ZoneResizer m_resizerBD   { ZoneResizer::Direction::vertical   };   // Zone B/C | Zone D
    ZoneResizer m_resizerSysB { ZoneResizer::Direction::vertical   };   // SystemFeed | zones

    // Layout member variables — driven by resizers
    int  m_menuBarHeight = 28;
    int  m_songHeaderH   = 28;
    int  m_songHeaderCompactH = 22;
    int  m_sysFeedHeight = 32;
    int  m_zoneAWidth    = 220;  // total Zone A width (28px tab strip + 192px content)
    int  m_zoneCWidth    = 200;  // resizable 40–400
    int  m_zoneDHeight   = 180;  // resizable 80–400

    int  m_historyStripHeight { AudioHistoryStrip::kDefaultH };

    /** True when the Zone A content panel is open (a tab is selected). */
    bool m_contentPanelOpen { false };

    // Derived helpers
    int songHeaderHeight() const;
    int systemFeedHeight() const;
    int historyTapeHeight() const;
    int topOffset() const;
    int contentPanelW() const { return m_contentPanelOpen
                                           ? juce::jmax (0, m_zoneAWidth - TabStrip::kWidth)
                                           : 0; }
    int effectiveZoneAW() const { return TabStrip::kWidth + contentPanelW(); }
    int zoneBWidth()    const
    {
        static constexpr int kDefaultZoneCW = static_cast<int> (Theme::Space::zoneCWidth);
        return juce::jmax (40, getWidth() - effectiveZoneAW() - 12 - kDefaultZoneCW);
    }

    // Called from PluginProcessor::onStepAdvanced (message thread)
    void onStepAdvanced (int step);

    // Tracks the most recently selected Zone B slot index (-1 = none)
    int m_focusedSlotIndex { -1 };

    // Cached module list — updated on every onModuleListChanged; used to populate
    // lazy panels (e.g. MixerPanel) when their tab is first opened.
    juce::StringArray m_lastModuleNames;

    // Transient clip held during an AudioHistoryStrip → ModuleRow drag operation.
    // Populated in historyStrip.onDragStarted, consumed in zoneB.onClipDropped.
    std::optional<CapturedAudioClip> m_dragClip;

    // Clip handed off to the LOOPER destination.
    // Owned here until a loop DSP engine is ready to consume it.
    std::optional<CapturedAudioClip> m_looperClip;

    void openSettingsPanel (juce::Rectangle<int> anchorBounds = {});
    void openThemeDesigner (juce::Rectangle<int> anchorBounds = {});
    void showMenuPopup (const juce::String& menuId, juce::Rectangle<int> anchorBounds);
    void showFileMenu (juce::Rectangle<int> anchorBounds);
    void showEditMenu (juce::Rectangle<int> anchorBounds);
    void showViewMenu (juce::Rectangle<int> anchorBounds);
    void showSettingsMenu (juce::Rectangle<int> anchorBounds);
    void toggleTimelineDebugOverlay();
    void paintTimelineDebugOverlay (juce::Graphics& g) const;
    void newSong();
    void openSong();
    void saveSong();
    void saveSongAs();
    void importLyricsText();
    void importThemeFile();
    void exportThemeFile();
    void applyAuthoredSongToUi();

    /** Build a CapturedAudioClip from the current grabbed buffer + session metadata. */
    CapturedAudioClip buildGrabClip() const;

    /** Load the current grabbed clip into slotIndex as a REEL instrument.
        Handles DSP type, slot focus, Zone A panel creation, and callback wiring. */
    void loadGrabbedClipToReelSlot (int slotIndex);

    /** Generalised REEL destination — loads any CapturedAudioClip into a slot.
        Called for both the grab-button path and the drag-and-drop path. */
    void loadClipToReelSlot (const CapturedAudioClip& clip, int slotIndex);

    /** LOOPER destination — hands clip ownership to the looper workspace.
        Updates LooperStrip UI; loop playback DSP is stubbed pending engine work. */
    void routeClipToLooper (const CapturedAudioClip& clip);

    /** TIMELINE destination — places clip metadata onto the timeline lane model.
        Opens Tracks panel and mirrors placement in Zone D + TracksPanel summary. */
    void routeClipToTimeline (const CapturedAudioClip& clip, std::optional<int> targetSlotOverride = std::nullopt);

    DrumMachineData* getDrumStateForSlot (int slotIndex) noexcept;
    const DrumMachineData* getDrumStateForSlot (int slotIndex) const noexcept;
    void syncSlotRuntimeFromModuleType (int slotIndex, const juce::String& moduleType);
    void syncAllSlotRuntimeFromModuleList();
    void applyDrumStateToRuntime (int slotIndex);
    void replaceDrumStateForSlot (int slotIndex, const DrumMachineData& state);
    void updateSequencerStructureContext (double structureBeat);
    juce::String moduleTypeForSlot (int slotIndex) const;
    void refreshTimelinePlacementsUi();
    bool writeTimelineAudioAssetsForSong (const juce::File& songFile);
    juce::String ensureTimelineClipId (TimelineClipPlacement& placement);
    juce::File timelineAssetFileFor (const juce::File& songFile, const juce::String& clipId) const;
    const CapturedAudioClip* findTimelineClipAudio (const juce::String& clipId) const;
    void upsertTimelineClipAudio (const juce::String& clipId, const CapturedAudioClip& clip);
    bool loadTimelineClipAudioFromFile (const TimelineClipPlacement& placement, CapturedAudioClip& outClip);
    bool saveTimelineClipAudioToFile (const CapturedAudioClip& clip, const juce::File& file) const;
    void applyTimelineLaneArmStateUi();
    int preferredArmedTimelineLane (const CapturedAudioClip& clip) const;
    juce::String serialiseZoneCFxStateJson() const;
    void applyZoneCFxStateJson (const juce::String& json, bool pushToDsp);
    void persistZoneCFxStateToSong();

    juce::Component::SafePointer<juce::CallOutBox> m_settingsCallout;
    juce::Component::SafePointer<juce::DialogWindow> m_themeDesignerWindow;
    juce::File m_currentSongFile;
    struct TimelineClipAudioCacheEntry
    {
        juce::String clipId;
        CapturedAudioClip clip;
    };
    juce::Array<TimelineClipAudioCacheEntry> m_timelineClipAudioCache;
    juce::AudioFormatManager m_audioFormatManager;
    SpoolLookAndFeel m_lookAndFeel;
    std::unique_ptr<SplashComponent> m_splash;
    std::unique_ptr<melatonin::Inspector> m_inspector;
    double m_lastObservedProcessorBeat { 0.0 };
    bool m_hasObservedProcessorBeat { false };
    juce::int64 m_lastBackstepWarningMs { 0 };
    bool m_timelineDebugOverlayEnabled { false };
    std::array<bool, SpoolAudioGraph::kNumSlots> m_timelineLaneArmed {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

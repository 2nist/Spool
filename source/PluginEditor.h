#pragma once

#include "PluginProcessor.h"
#include "Theme.h"
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

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor,
                     public juce::DragAndDropContainer,
                     public ZoneBComponent::Listener,
                     public TransportListener,
                     public ThemeManager::Listener
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint   (juce::Graphics&) override;
    void resized () override;

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
    int  m_sysFeedHeight = 32;
    int  m_zoneAWidth    = 220;  // total Zone A width (28px tab strip + 192px content)
    int  m_zoneCWidth    = 200;  // resizable 40–400
    int  m_zoneDHeight   = 180;  // resizable 80–400

    int  m_historyStripHeight { AudioHistoryStrip::kDefaultH };

    /** True when the Zone A content panel is open (a tab is selected). */
    bool m_contentPanelOpen { false };

    // Derived helpers
    int topOffset()     const { return m_menuBarHeight + m_songHeaderH + m_sysFeedHeight; }
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

    // Open the TPRT panel (from gear button in transport strip)
    void openTransportSettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

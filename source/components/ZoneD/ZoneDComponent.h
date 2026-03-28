#pragma once

#include "../../Theme.h"
#include "CylinderBand.h"
#include "ZoomStrip.h"
#include "TransportStrip.h"
#include "ClipData.h"
#include "StructureTimelineLane.h"
#include "../../structure/StructureState.h"
#include "../../state/LyricsState.h"
#include "../../state/AutomationState.h"

//==============================================================================
/**
    ZoneDComponent — tape-cylinder timeline panel (bottom row of the SPOOL layout).

    Children (top-to-bottom):
      - 8px zone stripe
      - CylinderBand  (height = m_cylinderBandH, default 132)
      - ZoomStrip     (20px)
      - TransportStrip (28px)

    Total height = structure lane + m_cylinderBandH + 20 + 28  (currentHeight()).
*/
class ZoneDComponent  : public juce::Component
{
public:
    ZoneDComponent();
    ~ZoneDComponent() override = default;

    //--- Layout ---------------------------------------------------------------
    /** Height seen by PluginEditor — grows/shrinks with HeightMode. */
    int  currentHeight() const noexcept { return kStructureLaneH + m_cylinderBandH + 20 + 28; }

    /** Fired (with new height) whenever height changes; caller should call resized(). */
    std::function<void(int newHeight)> onHeightChanged;

    //--- Lane management (called from PluginEditor / ZoneB interaction) -------
    void setLaneInfo  (int slotIndex, bool active, const juce::String& moduleType);
    void clearLaneInfo (int slotIndex);
    void armLane      (int slotIndex, bool armed);

    //--- Highlight (from ZoneB slot selection) --------------------------------
    void highlightLane (int slotIndex);
    void clearHighlight();

    //--- Transport listener registration (proxy to internal TransportStrip) ----
    void addTransportListener    (TransportListener* l);
    void removeTransportListener (TransportListener* l);

    /** Fired when the ⚙ gear button in the transport strip is clicked. */
    std::function<void()> onSettingsClicked;

    //--- Transport control (from PluginEditor / TransportListener) -----------
    void setPlaying   (bool playing);
    void setRecording (bool recording);
    void setBpm       (float bpm);
    void setStructureView (const StructureState* state, const StructureEngine* engine);
    void setAuthoredTimelineData (const LyricsState* lyricsState, const AutomationState* automationState);
    void setStructureBeat (double beat);
    void setStructureFollowState (TransportStrip::StructureFollowState state);
    void seedStructureRails (const StructureState& structure);

    /** Set vertical zoom for lane rows (0.5..3.0; 1.0 = normal). */
    void setLaneHeightScale (float s) { m_cylinderBand.setLaneScale (s); }

    /** Called by PluginEditor BD resizer — sets the overall height directly. */
    void setTotalHeight (int totalH)
    {
        m_cylinderBandH = juce::jmax (0, totalH - kStructureLaneH - 20 - 28);
        resized();
        if (onHeightChanged)
            onHeightChanged (currentHeight());
    }

    //--- juce::Component ------------------------------------------------------
    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    static constexpr int kStructureLaneH = 40;

    //--- Children ------------------------------------------------------------
    StructureTimelineLane m_structureLane;
    CylinderBand   m_cylinderBand;
    ZoomStrip      m_zoomStrip;
    TransportStrip m_transportStrip;

    //--- State ----------------------------------------------------------------
    int   m_cylinderBandH  = 132;
    float m_loopLengthBars = 8.0f;
    float m_bpm            = 120.0f;

    /** Up to 8 lanes — indexed 0..7 matching slot indices from Zone B. */
    std::array<LaneData, 8> m_lanes{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZoneDComponent)
};

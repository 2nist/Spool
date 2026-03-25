#pragma once
#include "../../Theme.h"

class PluginProcessor;

//==============================================================================
/**
    MixerPanel — vertical per-slot level control surface for the MIX tab.

    Each active slot renders as a 48px strip containing:
      - Group colour left-edge stripe (3px)
      - Type colour dot + slot name
      - Solo [S] and Mute [M] pill buttons
      - Horizontal level fader bar (drag to set 0..1 gain)
      - Vertical live-RMS meter bar (7px right edge)

    A master strip at the bottom controls master gain and pan.

    Interaction:
      - Click + drag on a slot's fader bar (lower half) → set slot level
      - Click [M] → toggle mute (slot audio silenced, DSP keeps running)
      - Click [S] → exclusive solo; Shift+click → additive solo
      - Click + drag on master fader → set master gain
      - Click + drag on pan bar → set master pan

    Live metering is driven by a 33ms juce::Timer polling the audio graph's
    per-slot RMS atomics.  The panel is populated via setSlots() whenever
    Zone B's module list changes.
*/
class MixerPanel : public juce::Component,
                   private juce::Timer
{
public:
    struct SlotInfo
    {
        int          slotIndex  { -1 };
        juce::String name;
        juce::String groupName;
        juce::Colour groupColor { 0xFF4B9EDB };
        float        level      { 0.8f };   // gain 0..1 (UI range)
        float        meter      { 0.0f };   // live RMS from audio graph
        bool         muted      { false };
        bool         soloed     { false };
        bool         isEmpty    { true };
    };

    explicit MixerPanel (PluginProcessor& proc);
    ~MixerPanel() override;

    /** Rebuild the strip list from active module slots. */
    void setSlots (const juce::Array<SlotInfo>& slots);

    //--- Callbacks ------------------------------------------------------------
    std::function<void(int slotIndex, float level)> onLevelChanged;
    std::function<void(int slotIndex, bool muted)>  onMuteChanged;
    std::function<void(int slotIndex, bool soloed)> onSoloChanged;

    //--- juce::Component ------------------------------------------------------
    void paint     (juce::Graphics&) override;
    void resized   () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    PluginProcessor&      m_proc;
    juce::Array<SlotInfo> m_slots;
    juce::Array<int>      m_activeIndices;   // m_slots indices of non-empty slots

    //--- Drag state -----------------------------------------------------------
    int   m_dragSlotArrIdx    { -1 };   // m_slots array index being dragged
    float m_dragBarLeft       { 0.0f };
    float m_dragBarWidth      { 1.0f };
    bool  m_draggingSlotFader { false };
    bool  m_draggingMasterLvl { false };
    bool  m_draggingMasterPan { false };
    float m_masterDragBarLeft { 0.0f };
    float m_masterDragBarW    { 1.0f };
    float m_masterPanBarCx    { 0.0f };
    float m_masterPanBarHalfW { 1.0f };

    //--- Hover ----------------------------------------------------------------
    int m_hoveredRenderIdx { -1 };

    //--- Layout constants -----------------------------------------------------
    static constexpr int kHeaderH  = 20;
    static constexpr int kStripH   = 48;
    static constexpr int kMasterH  = 60;
    static constexpr int kPad      = 5;
    static constexpr int kBtnW     = 14;
    static constexpr int kBtnH     = 14;
    static constexpr int kStripeW  = 3;
    static constexpr int kMeterW   = 7;
    static constexpr int kDotDiam  = 8;
    static constexpr int kFaderH      = 8;
    static constexpr int kDbThreshold = 240;   // panel width at which dB label appears
    static constexpr int kDbReadoutW  = 34;    // pixels reserved to the right of fader

    //--- Index helpers --------------------------------------------------------
    void rebuildActiveIndices();
    int  activeSlotCount()            const noexcept;
    int  renderIndexAtY (int y)       const noexcept;

    /** Returns &m_slots[m_activeIndices[ri]], or nullptr if out of range. */
    const SlotInfo* slotAtRI (int ri) const noexcept;
    SlotInfo*       slotAtRI (int ri)       noexcept;

    //--- Rect helpers ---------------------------------------------------------
    int  stripTopY     (int ri)   const noexcept;
    juce::Rectangle<int> stripRect      (int ri) const noexcept;
    juce::Rectangle<int> soloRect       (int ri) const noexcept;
    juce::Rectangle<int> muteRect       (int ri) const noexcept;
    juce::Rectangle<int> faderBarRect   (int ri) const noexcept;
    juce::Rectangle<int> meterBarRect   (int ri) const noexcept;
    juce::Rectangle<int> masterRect     ()       const noexcept;
    juce::Rectangle<int> masterFaderRect()       const noexcept;
    juce::Rectangle<int> masterPanRect  ()       const noexcept;

    //--- Paint helpers --------------------------------------------------------
    void paintHeader (juce::Graphics&) const;
    void paintStrip  (juce::Graphics&, int ri, const SlotInfo&) const;
    void paintMaster (juce::Graphics&) const;

    //--- Timer ----------------------------------------------------------------
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerPanel)
};

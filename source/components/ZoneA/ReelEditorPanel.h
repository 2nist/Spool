#pragma once

#include "../../Theme.h"
#include "../TapeColors.h"
#include "../../instruments/reel/ReelProcessor.h"

//==============================================================================
/**
    ReelEditorPanel — Zone A deep editor for a REEL slot.

    Opens automatically when a REEL Zone B row is clicked.

    Layout (top to bottom):
      Load strip    (32px) — [LOAD FILE] source name [CLEAR]
      Mode strip    (32px) — [LOOP][SAMPLE][GRAIN][SLICE] pills
      Waveform area (fills ~40%) — tape-aesthetic waveform + handles
      Position row  (24px) — START / END / DURATION values
      Mode params   (fills rest) — per-mode parameter sliders

    Sync:
      Drag start/end handles → fires onStartChanged / onEndChanged
      Grain position handle  → fires onParamChanged("grain.position", val)
      Timer polls processor grain positions for cloud visualisation
*/
class ReelEditorPanel  : public juce::Component,
                         private juce::Timer
{
public:
    explicit ReelEditorPanel (int slotIndex);
    ~ReelEditorPanel() override;

    //==========================================================================
    // Bind to a live processor (called from PluginEditor on slot selection)

    void setProcessor (ReelProcessor* proc);

    //==========================================================================
    // Callbacks (fired on message thread — wire in PluginEditor)

    /** Fired when start or end handle is dragged.  paramId = "play.start" or "play.end". */
    std::function<void (const juce::String& paramId, float normValue)> onParamChanged;

    //==========================================================================
    // juce::Component

    void paint    (juce::Graphics&) override;
    void resized  () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

private:
    //==========================================================================
    int             m_slotIndex  { 0 };
    ReelProcessor*  m_proc       { nullptr };

    // Grain cloud visualisation (updated by timer)
    static constexpr int kMaxGrainDots = 64;
    float m_grainPositions[kMaxGrainDots] {};
    int   m_numGrainDots { 0 };

    //==========================================================================
    // Layout rects (computed in resized)
    juce::Rectangle<int> m_loadRect;
    juce::Rectangle<int> m_modeRect;
    juce::Rectangle<int> m_waveRect;
    juce::Rectangle<int> m_posRow;
    juce::Rectangle<int> m_paramsRect;

    //==========================================================================
    // Drag state for waveform handles
    enum class HandleDrag { None, Start, End, GrainPos };
    HandleDrag m_dragHandle { HandleDrag::None };
    bool       m_syncing    { false };   // prevent feedback when updating handle

    //==========================================================================
    // Paint helpers
    void paintLoadStrip   (juce::Graphics&) const;
    void paintModeStrip   (juce::Graphics&) const;
    void paintWaveform    (juce::Graphics&) const;
    void paintPosRow      (juce::Graphics&) const;
    void paintModeParams  (juce::Graphics&) const;

    // Waveform helpers
    float normalToWaveX  (float norm) const noexcept;
    float waveXToNormal  (float x)    const noexcept;
    bool  hitTestHandle  (float x, float handleNorm) const noexcept;

    // Mode pill hit test (returns 0-3 or -1)
    int modePillAt (juce::Point<int> pos) const noexcept;

    // Load file button hit test
    bool hitLoadBtn  (juce::Point<int> pos) const noexcept;
    bool hitClearBtn (juce::Point<int> pos) const noexcept;

    //==========================================================================
    // juce::Timer
    void timerCallback() override;

    //==========================================================================
    // Layout constants
    static constexpr int kLoadH    = 32;
    static constexpr int kModeH    = 32;
    static constexpr int kPosRowH  = 24;
    static constexpr int kWaveFrac = 40;    // % of remaining height for waveform
    static constexpr int kHandleHit = 8;   // px tolerance for handle drag hit

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReelEditorPanel)
};

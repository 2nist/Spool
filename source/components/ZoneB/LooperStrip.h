#pragma once

#include "LooperUnit.h"

//==============================================================================
/**
    LooperStrip — 72px control surface for the circular audio buffer.

    Three rows of 24px each:
      Row 1: SOURCE ▾  ● LIVE  [◀4] [◀8] [◀16] [FREE]
      Row 2: [▶ PLAY]  [⊕ LOOP]  [→ REEL]  [→ TIMELINE]  [+ LOOPER B]
      Row 3: REC ARM ○  SNAP [BAR▾]  AUTO-GRAB ○

    Callbacks are wired by PluginEditor to the processor.
*/
class LooperStrip : public juce::Component
{
public:
    LooperStrip();
    ~LooperStrip() override = default;

    /** Source names for the SOURCE dropdown. */
    void setSourceNames (const juce::StringArray& names);

    /** Show/hide the secondary looper unit ("+B" button). */
    void setHasGrabbedClip (bool hasClip);

    static constexpr int kHeight = 72;
    static constexpr int kRowH   = 24;

    //==========================================================================
    // Callbacks — wired by PluginEditor

    std::function<void(int numBars)>              onGrabLastBars;
    std::function<void()>                         onGrabFree;
    std::function<void()>                         onSendToReel;
    std::function<void()>                         onSendToLooper;
    std::function<void()>                         onSendToTimeline;
    std::function<void(bool active)>              onLiveToggled;
    std::function<void(int sourceIndex)>          onSourceChanged;

    //==========================================================================
    void paint    (juce::Graphics&) override;
    void resized  () override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::StringArray m_sourceNames;
    int               m_selectedSource { 0 };

    bool m_isLive       { true  };
    bool m_hasGrabbedClip { false };
    bool m_recArmed     { false };
    bool m_autoGrab     { false };
    bool m_showUnitB    { false };
    int  m_snapMode     { 0 };    // 0=BAR 1=BEAT 2=FREE

    LooperUnit m_unitB;

    //==========================================================================
    // Row regions
    juce::Rectangle<int> row1() const noexcept { return { 0, 0,       getWidth(), kRowH }; }
    juce::Rectangle<int> row2() const noexcept { return { 0, kRowH,   getWidth(), kRowH }; }
    juce::Rectangle<int> row3() const noexcept { return { 0, 2*kRowH, getWidth(), kRowH }; }

    // Button hit-test helpers (return -1 if no hit)
    // Row 1: 0=◀4  1=◀8  2=◀16  3=FREE  (source/live handled separately)
    int  row1BtnAt    (juce::Point<int> pos) const noexcept;
    // Row 2: 0=PLAY 1=LOOP 2=REEL 3=TMLN 4=+B
    int  row2BtnAt    (juce::Point<int> pos) const noexcept;
    // Row 3: 0=RECARM 1=SNAP 2=AUTOGRAB
    int  row3BtnAt    (juce::Point<int> pos) const noexcept;

    bool liveHitAt   (juce::Point<int> pos) const noexcept;
    bool sourceHitAt (juce::Point<int> pos) const noexcept;

    void paintRow1 (juce::Graphics&) const;
    void paintRow2 (juce::Graphics&) const;
    void paintRow3 (juce::Graphics&) const;

    // Small pill button helper
    static void drawPillBtn (juce::Graphics& g,
                              juce::Rectangle<int> r,
                              const juce::String& label,
                              bool active,
                              juce::Colour activeCol = {});

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperStrip)
};

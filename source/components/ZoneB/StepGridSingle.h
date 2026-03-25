#pragma once

#include "SlotPattern.h"

//==============================================================================
/**
    StepGridSingle — step grid for one focused slot.

    Shows only the active step count (1–64 steps).
    Two rows when stepCount > 16; three rows when > 32 (with inner scroll).

    Step count control [N] − + on the right side.
    Beat group dividers every 4 steps.
    Click / drag to toggle steps.
*/
class StepGridSingle : public juce::Component
{
public:
    StepGridSingle();
    ~StepGridSingle() override = default;

    /** Attach to a slot pattern.  Pointer must remain valid while displayed. */
    void setPattern   (SlotPattern* pattern, juce::Colour groupColor);
    void clearPattern ();

    /** Advance playhead (−1 = none). */
    void setPlayhead  (int stepIndex);

    /** Fired after any step is toggled or step count changes.
        ZoneBComponent sets this to sync the pattern to the processor. */
    std::function<void()> onModified;

    void paint    (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    juce::MouseCursor getMouseCursor() override;

    static constexpr int kHeight = 80;

private:
    SlotPattern* m_pattern    { nullptr };
    juce::Colour m_groupColor { 0xFF4B9EDB };
    int          m_playhead   { -1 };

    bool m_paintMode       { false };
    int  m_lastPaintedStep { -1 };

    // Layout constants
    static constexpr int kPad      = 6;   // outer padding
    static constexpr int kGap      = 3;   // between steps
    static constexpr int kCtrlW    = 60;  // right-hand step count controls width
    static constexpr int kBtnW     = 16;  // square buttons
    static constexpr int kBtnH     = 16;

    /** Rows needed for given step count. */
    int  numRows()         const noexcept;
    int  stepsPerRow()     const noexcept;
    int  stepW()           const noexcept;
    int  stepH()           const noexcept;

    /** Available rect for the step grid (excluding control area). */
    juce::Rectangle<int> gridArea()      const noexcept;
    /** Right-side step count control area. */
    juce::Rectangle<int> ctrlArea()      const noexcept;
    juce::Rectangle<int> decBtnRect()    const noexcept;
    juce::Rectangle<int> incBtnRect()    const noexcept;
    juce::Rectangle<int> countLblRect()  const noexcept;

    juce::Rectangle<int> stepRect        (int stepIndex) const noexcept;
    int                  stepAtPos       (juce::Point<int> pos) const noexcept;

    void paintGrid      (juce::Graphics& g) const;
    void paintDividers  (juce::Graphics& g) const;
    void paintCtrl      (juce::Graphics& g) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepGridSingle)
};

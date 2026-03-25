#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    StepGrid — 32-step trigger grid for Zone B.

    Single component, paints all steps itself. No sub-components.
    Handles all mouse interaction internally (toggle, drag-fill, right-click cycle).

    Layout:
      Grid header:  16px (beat markers, step count, armed slot label)
      Row 1:        steps 0–15
      Row 2:        steps 16–31
      Row gap:      Theme::Space::sm (8px)
      Outer padding: Theme::Space::sm all sides

    Beat-group dividers are cosmetic painted lines — no extra pixel gap.
*/
class StepGrid : public juce::Component
{
public:
    StepGrid();
    ~StepGrid() override = default;

    /** Set the currently armed slot label (e.g. "SYN", "SMP"). */
    void setArmedSlot (int slotIndex, const juce::String& abbreviation);

    /** Advance playhead to stepIndex (-1 = none). */
    void setPlayhead (int stepIndex);

    void paint         (juce::Graphics&) override;
    void resized       () override;
    void mouseDown     (const juce::MouseEvent&) override;
    void mouseDrag     (const juce::MouseEvent&) override;
    void mouseUp       (const juce::MouseEvent&) override;
    juce::MouseCursor getMouseCursor() override;

private:
    bool m_active[32]   = {};
    bool m_accented[32] = {};
    int  m_playhead     { -1 };
    int  m_armedSlot    { -1 };
    juce::String m_armedLabel;

    // Drag-fill state
    bool m_paintMode        { false };  // true = painting active; false = erasing
    int  m_lastPaintedStep  { -1 };

    static constexpr int kHeaderH  = 16;
    static constexpr int kNumSteps = 32;
    static constexpr int kStepsPerRow = 16;

    /** Returns the bounding rect of step [0,31] in local coordinates, or empty if not in grid. */
    juce::Rectangle<int> stepRect (int stepIndex) const noexcept;

    /** Returns the step index at local position, or -1 if outside grid. */
    int stepAtPosition (juce::Point<int> pos) const noexcept;

    /** Is this step the beat-1 of a 4-step group? */
    static constexpr bool isBeat1 (int step) noexcept
    {
        return (step % 4) == 0;
    }

    void paintHeader (juce::Graphics& g) const;
    void paintStep   (juce::Graphics& g, int index) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepGrid)
};

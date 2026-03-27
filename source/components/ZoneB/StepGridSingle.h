#pragma once

#include "SlotPattern.h"

//==============================================================================
/**
    StepGridSingle — hybrid step sequencer editor for one focused slot.

    Top: variable-length step containers.
    Bottom: compact micro piano-roll for the selected step.
*/
class StepGridSingle : public juce::Component
{
public:
    StepGridSingle();
    ~StepGridSingle() override = default;

    void setPattern   (SlotPattern* pattern, juce::Colour groupColor);
    void clearPattern ();
    void setPlayhead  (int stepIndex);

    std::function<void()> onModified;

    void paint      (juce::Graphics&) override;
    void mouseMove  (const juce::MouseEvent&) override;
    void mouseDown  (const juce::MouseEvent&) override;
    void mouseDrag  (const juce::MouseEvent&) override;
    void mouseUp    (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    juce::MouseCursor getMouseCursor() override;

    static constexpr int kHeight = 116;

private:
    SlotPattern* m_pattern    { nullptr };
    juce::Colour m_groupColor { 0xFF4B9EDB };
    int          m_playhead   { -1 };
    int          m_selectedStep { 0 };
    int          m_selectedEvent { -1 };
    int          m_pitchBase { 48 };
    SlotPattern::Step m_stepClipboard;
    bool         m_hasStepClipboard { false };
    bool         m_detailOpen { false };

    enum class DragMode
    {
        none = 0,
        resizeStep,
        moveEvent,
        resizeEvent
    };

    DragMode m_dragMode { DragMode::none };
    int      m_hoverStep { -1 };
    int      m_hoverEvent { -1 };
    bool     m_hoverStepResizeHandle { false };
    float    m_dragEventOffsetStart { 0.0f };
    float    m_dragEventLengthStart { 0.0f };
    int      m_dragEventPitchStart  { 60 };
    int      m_dragStepDurationStart { 1 };
    juce::Point<int> m_dragStartPos;

    static constexpr int kPad       = 6;
    static constexpr int kGap       = 4;
    static constexpr int kBtnH      = 18;
    static constexpr int kBtnGap    = 4;
    static constexpr int kLaneH     = 58;
    static constexpr int kPitchRows = 12;

    juce::Rectangle<int> stepLaneRect() const noexcept;
    juce::Rectangle<int> compactBarRect() const noexcept;
    juce::Rectangle<int> detailToolbarRect() const noexcept;
    juce::Rectangle<int> detailRect() const noexcept;
    juce::Rectangle<int> editRect() const noexcept;
    juce::Rectangle<int> closeDetailRect() const noexcept;
    juce::Rectangle<int> modeRect() const noexcept;
    juce::Rectangle<int> roleRect() const noexcept;
    juce::Rectangle<int> followRect() const noexcept;
    juce::Rectangle<int> moreRect() const noexcept;
    juce::Rectangle<float> stepResizeHandleRect (int stepIndex) const noexcept;

    int stepAt (juce::Point<int> pos) const noexcept;
    juce::Rectangle<float> stepRect (int index) const noexcept;
    juce::Rectangle<float> microEventRect (int eventIndex) const noexcept;
    int microEventAt (juce::Point<int> pos) const noexcept;

    SlotPattern::Step* selectedStepData() noexcept;
    const SlotPattern::Step* selectedStepData() const noexcept;

    void ensureSelectionValid();
    void paintEmpty (juce::Graphics& g) const;
    void paintStepLane (juce::Graphics& g) const;
    void paintDetail (juce::Graphics& g) const;
    void paintCompactBar (juce::Graphics& g) const;
    void paintDetailToolbar (juce::Graphics& g) const;
    void commitChange();
    void showStepContextMenu (int stepIndex);
    void showNoteContextMenu (int eventIndex);
    void showModeMenu();
    void showRoleMenu();
    void deleteSelection();
    void copySelectedStep();
    void pasteStepToSelection();
    void duplicateStep (int stepIndex);
    void duplicateNote (int eventIndex);
    void splitStep (int stepIndex);
    void joinStepWithNext (int stepIndex);
    void clearStepContents (int stepIndex);
    void transposeSelectedStepContents (int semitones);
    void nudgeSelectedEvent (float delta);
    void adjustSelectedEventLength (float delta);
    void adjustSelectedEventVelocity (int delta);
    void updateHoverState (juce::Point<int> pos);
    int valueForRow (int row) const noexcept;
    int rowForValue (int value) const noexcept;
    juce::String noteValueLabel (int value) const;

    float normalizedTimeFromDetailX (int x) const noexcept;
    int pitchFromDetailY (int y) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepGridSingle)
};

#pragma once

#include "SlotPattern.h"
#include <vector>

class StepGridSingle : public juce::Component
{
public:
    StepGridSingle();
    ~StepGridSingle() override = default;

    void setPattern (SlotPattern* pattern, juce::Colour groupColor, int ownerSlotIndex = -1);
    void clearPattern();
    void setPlayhead (int stepIndex);
    void setMusicalContext (const juce::String& keyRoot,
                            const juce::String& keyScale,
                            const juce::String& currentChord,
                            const juce::String& nextChord,
                            bool followingStructure,
                            bool locallyOverriding);
    bool isSelectedStepFollowingStructure() const noexcept;

    std::function<void()> onModified;
    /** Fired once at the start of each edit gesture (mouseDown or destructive key).
        Hook this to snapshot the pattern before the mutation. */
    std::function<void()> onBeforeEdit;
    /** Fired once at the end of each edit gesture (mouseUp or after keyboard edit).
        Hook this to commit a before/after UndoableAction. */
    std::function<void()> onGestureEnd;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    juce::MouseCursor getMouseCursor() override;

    static constexpr int kHeight = 124;

private:
    bool m_inMouseGesture { false };

    enum class LaneView : uint8_t
    {
        notes = 0,
        chord,
        velocity,
        midiFx
    };

    struct UnitEventRef
    {
        int stepIndex { -1 };
        int eventIndex { -1 };
    };

    SlotPattern* m_pattern { nullptr };
    juce::Colour m_groupColor { 0xFF4B9EDB };
    int m_ownerSlotIndex { -1 };
    int m_playhead { -1 };
    int m_selectedUnit { 0 };
    int m_selectedStep { 0 };
    int m_selectedEvent { -1 };
    int m_pitchBase { 48 };
    int m_pageIndex { 0 };
    LaneView m_view { LaneView::notes };
    SlotPattern::Step m_stepClipboard;
    bool m_hasStepClipboard { false };
    int m_hoverColumn { -1 };
    int m_hoverRow { -1 };
    int m_hoverResizeStep  { -1 };   // step whose right edge the cursor is near
    int m_resizingStep     { -1 };   // step currently being resize-dragged
    int m_resizeDragStartX { 0 };    // mouse X when resize drag began
    int m_resizeDragOrigDur { 0 };   // step duration when resize drag began
    bool m_inResizeDrag    { false };
    std::array<bool, SlotPattern::kRouteRows> m_rowMuted {};
    std::array<std::array<SlotPattern::GateMode, SlotPattern::kRouteUnitsPerRow>, SlotPattern::kRouteRows> m_rowMuteGateSnapshot {};
    std::array<std::array<bool, SlotPattern::kRouteUnitsPerRow>, SlotPattern::kRouteRows> m_rowMuteHadEvent {};

    juce::String m_keyRoot { "C" };
    juce::String m_keyScale { "Major" };
    juce::String m_currentChord { "Cmaj" };
    juce::String m_nextChord { "Gmaj" };
    bool m_followingStructure { true };
    bool m_locallyOverriding { false };

    static constexpr int kPad = 4;
    static constexpr int kHeaderH = 16;
    static constexpr int kInspectorH = 16;
    static constexpr int kResizeZoneW = 5;  // px from step right edge that triggers resize cursor
    static constexpr int kOverviewRows = 4;
    static constexpr int kOverviewRowH = 11;
    static constexpr int kOverviewPadY = 2;
    static constexpr int kRows = 12;
    static constexpr int kCols = 16;
    static constexpr int kLabelW = 26;

    juce::Rectangle<int> contentRect() const noexcept;
    juce::Rectangle<int> headerRect() const noexcept;
    juce::Rectangle<int> gridRect() const noexcept;
    juce::Rectangle<int> overviewRect() const noexcept;
    juce::Rectangle<int> noteGridRect() const noexcept;
    juce::Rectangle<int> inspectorRect() const noexcept;
    int overviewRowCount() const noexcept;
    int overviewHeight() const noexcept;
    int overviewLabelWidth() const noexcept;
    juce::Rectangle<int> overviewLabelsRect() const noexcept;
    juce::Rectangle<int> overviewCellsRect() const noexcept;
    juce::Rectangle<int> overviewRowRect (int row) const noexcept;
    juce::Rectangle<int> overviewRowLabelRect (int row) const noexcept;
    juce::Rectangle<int> overviewRowRouteRect (int row) const noexcept;
    juce::Rectangle<int> overviewRowFollowRect (int row) const noexcept;
    juce::Rectangle<int> overviewRowMuteRect (int row) const noexcept;

    juce::Rectangle<int> pagePrevRect() const noexcept;
    juce::Rectangle<int> pageLabelRect() const noexcept;
    juce::Rectangle<int> pageNextRect() const noexcept;
    juce::Rectangle<int> rowChipRect (int rowPage) const noexcept;
    juce::Rectangle<int> tabRect (LaneView view) const noexcept;
    juce::Rectangle<int> modeRect() const noexcept;
    juce::Rectangle<int> roleRect() const noexcept;
    juce::Rectangle<int> followRect() const noexcept;
    juce::Rectangle<int> moreRect() const noexcept;

    int totalUnits() const noexcept;
    int pageCount() const noexcept;
    int pageStartUnit() const noexcept;
    int pageForUnit (int unit) const noexcept;
    int columnForUnit (int unit) const noexcept;
    int unitForColumn (int column) const noexcept;

    int cellColumnAt (juce::Point<int> pos) const noexcept;
    int cellRowAt (juce::Point<int> pos) const noexcept;
    juce::Rectangle<int> cellRect (int row, int col) const noexcept;
    int stepIndexForResizeAt (juce::Point<int> pos) const noexcept;

    int midiForRow (int row) const noexcept;
    int rowForMidi (int midiNote) const noexcept;
    int velocityForRow (int row) const noexcept;
    int rowForVelocity (int velocity) const noexcept;

    const SlotPattern::Step* selectedStepData() const noexcept;
    SlotPattern::Step* selectedStepData() noexcept;
    int findStepIndexForUnit (int unit) const noexcept;

    int eventUnitFor (const SlotPattern::Step& step, const SlotPattern::MicroEvent& event) const noexcept;
    int eventMidiForDisplay (const SlotPattern::Step& step, const SlotPattern::MicroEvent& event) const noexcept;

    UnitEventRef findEventAtCell (int unit, int row) const noexcept;
    UnitEventRef firstEventInUnit (int unit) const noexcept;

    int storedPitchForMode (const SlotPattern::Step& step, int midiNote) const noexcept;
    int addEventAtCell (int unit, int row, int velocity = 100);
    void addChordAtUnit (int unit, int rootRow, int velocity = 100);
    void removeEventRef (const UnitEventRef& ref);
    void setVelocityAtCell (int unit, int row);

    void ensureSelectionValid();
    void commitChange();

    void showModeMenu();
    void showRoleMenu();
    void showMoreMenu();
    void showEventContextMenu (const UnitEventRef& ref);
    void showRowRouteMenu (int row);
    void toggleRowFollow (int row);
    void toggleRowMute (int row);

    void paintHeader (juce::Graphics& g) const;
    void paintGrid (juce::Graphics& g) const;
    void paintInspector (juce::Graphics& g) const;

    juce::String rowLabelForRow (int row) const;
    juce::String laneViewLabel (LaneView view) const;
    juce::String rowRouteLabel (int row) const;

    juce::String noteName (int midiNote) const;
    int rootToPitchClass (const juce::String& root) const noexcept;
    juce::String chordRootString (const juce::String& chordLabel) const;
    juce::String chordTypeString (const juce::String& chordLabel) const;
    std::vector<int> chordIntervalsForLabel (const juce::String& chordLabel) const;
    std::vector<int> scalePitchClassesForKey() const;
    int nearestNoteForPitchClass (int aroundNote, int targetPc) const noexcept;
    int realizedPitchForPreview (const SlotPattern::Step& step, const SlotPattern::MicroEvent& event) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepGridSingle)
public:
    // Public accessor to selected step for inspector/tests
    const SlotPattern::Step* getSelectedStepData() const noexcept { return selectedStepData(); }
    SlotPattern::Step* getSelectedStepData() noexcept { return selectedStepData(); }
};

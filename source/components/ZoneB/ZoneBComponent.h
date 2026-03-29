#pragma once

#include "../../Theme.h"
#include "ModuleGroup.h"
#include "GroupHeader.h"
#include "ModuleLoadDialog.h"
#include "SequencerHeader.h"
#include "StepGridSingle.h"
#include "StepGridDrum.h"
#include "LooperStrip.h"

//==============================================================================
/**
    ZoneBComponent — module row list + sequencer + looper.

    Zone B vertical layout (top to bottom):
      Zone stripe:         3px  (Theme::Space::zoneStripeHeight)
      Module rows area:    elastic — fills middle, scrollable via Viewport
        Group header:      20px  (GroupHeader::kHeight)  ← inline divider
        Module row pair:   188px expanded / 20px collapsed (2× ModuleRow side-by-side)
      Split handle:        6px   (draggable)
      Sequencer header:    16px  (SequencerHeader::kHeight)
      Step grid:           140px synth / 200px drum  (dynamic)
      Looper strip:        72px  (LooperStrip::kHeight)
*/
class ZoneBComponent : public juce::Component
{
public:
    //==========================================================================
    // Listener (backward-compatible with PluginEditor)

    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void slotSelected   (int slotIndex, const juce::String& moduleType) = 0;
        virtual void slotDeselected () = 0;
    };

    void addListener    (Listener* l) { m_listeners.add (l);    }
    void removeListener (Listener* l) { m_listeners.remove (l); }

    //==========================================================================
    // DSP sync callbacks

    /** Fired after any step is edited in the focused row's step grid. */
    std::function<void (int slotIdx, const SlotPattern& pattern)> onPatternModified;

    /** Fired when a drum machine row is focused.  Use to load kit into DSP. */
    std::function<void (int slotIdx, DrumMachineData*)> onDrumSlotFocused;

    /** Fired after any drum step or step-count changes. */
    std::function<void (int slotIdx, DrumMachineData*)> onDrumPatternModified;

    /** Called by PluginEditor when the sequencer advances a step. */
    void setPlayheadStep (int step);
    void setStructureContext (const juce::String& sectionName,
                              const juce::String& positionLabel,
                              const juce::String& keyRoot,
                              const juce::String& keyScale,
                              const juce::String& currentChord,
                              const juce::String& nextChord,
                              const juce::String& transitionIntent,
                              bool followingStructure,
                              bool locallyOverriding);
    void setFocusedPatternFollowStructure (bool enabled, bool activeOnly = true);
    void seedPatternForSlot (int flatSlotIndex, int stepCount, const std::initializer_list<int>& activeSteps);

    /** Fired when the module list changes — passes flat "<Group>:<Type>" list. */
    std::function<void (const juce::StringArray&)> onModuleListChanged;

    /** Fired when a CapturedAudioClip is dropped onto a module row.
        The row is already focused (slotSelected fired) before this fires.
        flatSlotIndex is the 0-based absolute slot index across all groups. */
    std::function<void (int flatSlotIndex)> onClipDropped;

    /** Fired when any fader on any module row changes.
        actualValue is the mapped (min..max) value, ready to forward to the processor.
        label is the param's display label (e.g. "CUT", "REL") — use for routing, not paramIdx,
        so the mapping survives future changes to the Zone B param list. */
    std::function<void (int slotIdx, const juce::String& label, float actualValue)> onQuickParamChanged;

    //==========================================================================

    ZoneBComponent();
    ~ZoneBComponent() override;

    LooperStrip& getLooperStrip() noexcept { return m_looperStrip; }
    void setLooperVisible (bool shouldShow);

    void paint   (juce::Graphics&) override;
    void resized () override;

    /** Re-fire onModuleListChanged with the current slot list.
        Call after wiring onModuleListChanged to catch the initial state that
        was broadcast during construction before the callback was connected. */
    void broadcastModuleList();
    DrumMachineData* getDrumDataForSlot (int flatSlotIndex) noexcept;
    const DrumMachineData* getDrumDataForSlot (int flatSlotIndex) const noexcept;
    bool setDrumDataForSlot (int flatSlotIndex, const DrumMachineData& data);

private:
    //==========================================================================
    // Data

    juce::OwnedArray<ModuleGroup>  m_groups;
    juce::OwnedArray<GroupHeader>  m_groupHeaders;

    ModuleRow* m_focusedRow      { nullptr };
    int        m_focusedGroupIdx { -1 };

    SlotPattern m_patternClipboard;
    bool        m_hasClipboard { false };

    //==========================================================================
    // Scrollable row-list content component

    struct RowsContent : public juce::Component
    {
        std::function<void()> onResized;
        void resized() override { if (onResized) onResized(); }
    };

    RowsContent      m_rowsContent;
    juce::Viewport   m_rowsViewport;
    juce::TextButton m_addGroupBtn { "+  GROUP" };

    //==========================================================================
    // Fixed-bottom components

    SequencerHeader  m_seqHeader;
    StepGridSingle   m_stepGridSingle;
    StepGridDrum     m_stepGridDrum;
    LooperStrip      m_looperStrip;
    bool             m_showLooper { true };

    //==========================================================================
    // Split handle — drag to resize rows area vs. bottom section

    struct SplitHandle : public juce::Component
    {
        std::function<void()>    onDragStarted;
        std::function<void(int)> onDrag;    // arg = delta-from-drag-start px

        void paint      (juce::Graphics& g) override;
        void mouseDown  (const juce::MouseEvent& e) override;
        void mouseDrag  (const juce::MouseEvent& e) override;
        juce::MouseCursor getMouseCursor() override
        {
            return juce::MouseCursor::UpDownResizeCursor;
        }
    };

    SplitHandle m_splitHandle;
    int         m_splitY          { -1 };  // -1 = uninitialized
    int         m_splitDragStartY {  0 };
    int         m_splitBeforeSequencerFocus { -1 };
    bool        m_sequencerFocusMode { false };

    //==========================================================================
    // Load dialog

    std::unique_ptr<ModuleLoadDialog> m_loadDialog;
    juce::ListenerList<Listener>      m_listeners;

    //==========================================================================
    // Layout constants

    static constexpr int kStripeH     = static_cast<int> (Theme::Space::zoneStripeHeight);
    static constexpr int kSeqHdrH     = SequencerHeader::kHeight;
    static constexpr int kMinGridH    = StepGridSingle::kHeight;  // minimum step grid height
    static constexpr int kLooperH     = LooperStrip::kHeight;
    static constexpr int kHandleH     = 6;
    static constexpr int kMinRowsH    = 40;
    static constexpr int kMinBottomBaseH = kSeqHdrH + kMinGridH;
    static constexpr int kRowGap      = 0;   // rows are flush
    static constexpr int kOuterPad    = 8;   // horizontal padding left and right of row pairs
    static constexpr int kPairGutter  = 12;  // gap between the two half-modules in a row pair

    int m_gridH { 0 };  // computed dynamically in resized()

    // Derived positions — all depend on m_splitY
    int looperHeight() const noexcept { return m_showLooper ? kLooperH : 0; }
    int minBottomHeight() const noexcept { return kMinBottomBaseH + looperHeight(); }
    int rowsAreaTop()  const noexcept { return kStripeH; }
    int rowsAreaH()    const noexcept
    {
        if (m_splitY < 0)
            return juce::jmax (0, getHeight() - kStripeH - kHandleH - minBottomHeight());
        return juce::jmax (0, m_splitY - kStripeH);
    }
    int bottomStart()  const noexcept { return (m_splitY < 0 ? kStripeH + rowsAreaH() : m_splitY) + kHandleH; }
    int seqHeaderY()   const noexcept { return bottomStart(); }

    //==========================================================================
    // Initialisation

    void createDefaultGroups();

    //==========================================================================
    // Group / row management

    void addGroup           (const juce::String& name, juce::Colour color, bool addInitialEmptySlot = true);
    void addSlotToGroup     (int groupIndex, ModuleRow::ModuleType type = ModuleRow::ModuleType::Empty);
    void removeGroup        (int groupIndex);
    void connectGroupHeader (int groupIndex);
    int findGroupIndexForRow (const ModuleRow* row) const noexcept;

    //==========================================================================
    // Layout

    int  calculateRowsContentH () const noexcept;
    void layoutRowsContent     ();
    void updateSourceNames     ();
    void toggleSequencerFocusMode ();

    //==========================================================================
    // Focus

    void focusRow   (ModuleRow* row, int groupIndex);
    void defocusRow ();

    //==========================================================================
    // Row interactions

    void onRowClicked     (ModuleRow* row, int groupIndex);
    void onEmptyClicked   (ModuleRow* row, int groupIndex);
    void onRightClick     (ModuleRow* row, int groupIndex);
    void onRowTypeChosen  (ModuleRow* row, ModuleRow::ModuleType type);

    void showLoadDialog    (ModuleRow* row, juce::Rectangle<int> boundsInContent);
    void dismissLoadDialog ();
    ModuleRow* rowForFlatSlot (int flatSlotIndex, int* groupIndexOut = nullptr) noexcept;
    const ModuleRow* rowForFlatSlot (int flatSlotIndex, int* groupIndexOut = nullptr) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZoneBComponent)
};

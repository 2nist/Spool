#pragma once

#include "../../Theme.h"
#include "../../dsp/MidiRouter.h"
#include "ZoneAStyle.h"

/**
    MidiRoutingGrid — cross-point matrix for MIDI source → destination routing.

    Rows  = MIDI sources  (Keyboard In, Sequencer, Midi FX)
    Cols  = destinations  (Slot 1-8, Drum Rack)

    Each cell is a toggle.  Filled = route active.  Empty = no route.

    Activity indicators: destination columns 0-7 (Slot 1-8) flash when MidiRouter
    reports new events routed to that slot.  Source rows flash when any MIDI
    passes through the router and that row has at least one active cell.
    Timer runs at 30 Hz only while the component is visible.
*/
class MidiRoutingGrid : public juce::Component,
                        private juce::Timer
{
public:
    static constexpr int kSources = 3;
    static constexpr int kDests   = 9;

    MidiRoutingGrid();
    ~MidiRoutingGrid() override;

    void setCell (int row, int col, bool on);
    bool getCell (int row, int col) const noexcept;
    void clearAll();

    /** Wire to the processor's MidiRouter for live activity polling.
        Pass nullptr to disconnect.  Pointer must remain valid for the
        lifetime of this component (processor outlives editor). */
    void setMidiRouter (const MidiRouter* router);

    static juce::String sourceName  (int row);   // full name used in RouteEntry
    static juce::String destName    (int col);
    static juce::String sourceLabel (int row);   // short label for grid header
    static juce::String destLabel   (int col);
    static int sourceIndex (const juce::String& name);
    static int destIndex   (const juce::String& name);

    /** Fired when the user clicks a cell. */
    std::function<void (int row, int col, bool on)> onCellToggled;

    void paint              (juce::Graphics&) override;
    void mouseDown          (const juce::MouseEvent&) override;
    void mouseMove          (const juce::MouseEvent&) override;
    void mouseExit          (const juce::MouseEvent&) override;
    void visibilityChanged  () override;

private:
    // --- Cell state ---
    bool m_cells[kSources][kDests] {};
    int  m_hoverRow { -1 };
    int  m_hoverCol { -1 };

    // --- Activity ---
    const MidiRouter* m_router { nullptr };
    uint32_t m_lastInputTick { 0 };
    uint32_t m_lastDestTicks[MidiRouter::kNumSlots] {};
    float    m_sourceFlash[kSources] {};   // 0..1, decays each timer tick
    float    m_destFlash[kDests]     {};

    static constexpr float kFlashDecay = 0.16f;  // per 30 Hz tick ≈ 200 ms fade

    void timerCallback() override;

    // --- Layout ---
    static constexpr int kLabelW  = 34;
    static constexpr int kHeaderH = 18;
    static constexpr int kCellGap = 2;

    int  cellW() const noexcept;
    int  cellH() const noexcept;
    juce::Rectangle<int>  cellRect    (int row, int col) const noexcept;
    std::pair<int, int>   cellAtPoint (juce::Point<int> p) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiRoutingGrid)
};

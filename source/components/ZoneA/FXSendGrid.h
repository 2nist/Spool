#pragma once

#include "../../Theme.h"
#include "../../state/RoutingState.h"
#include "ZoneAStyle.h"

/**
    FXSendGrid -- 2-D parallel send matrix for the FX routing tab.

    Rows  = active slots   (up to kFXSlots, driven by setSlotNames)
    Cols  = Bus A, Bus B, Bus C, Bus D, Master  (fixed 5 columns)

    Each cell:
      - tap to toggle enabled/disabled
      - drag vertically to adjust send level 0.0-1.0 when enabled

    Column header tap fires onBusSelected("bus_a") etc.
    Activity pulse: cells flash when m_audioTick changes (same pattern as
    the existing RoutingPanel audio-tick flash).

    Timer runs at 30 Hz only while the component is visible and a tick
    pointer has been supplied.
*/
class FXSendGrid : public juce::Component,
                   private juce::Timer
{
public:
    static constexpr int kCols = kFXTargets;  // 5

    FXSendGrid();
    ~FXSendGrid() override;

    //--- State -----------------------------------------------------------------
    void setSlotNames  (const juce::StringArray& names);  // display names, index = slot row
    void setMatrix     (const FXSendMatrix& matrix);
    const FXSendMatrix& getMatrix() const noexcept { return m_matrix; }

    //--- Audio tick for activity pulse -----------------------------------------
    void setAudioTick (const std::atomic<uint32_t>* tick);

    //--- Callbacks -------------------------------------------------------------
    /** Fired when a cell changes.  Delivers full updated matrix. */
    std::function<void (const FXSendMatrix&)> onMatrixChanged;

    /** Fired when the user taps a column header.
        Argument is the stable bus ID: "bus_a", "bus_b", "bus_c", "bus_d", "master". */
    std::function<void (const juce::String&)> onBusSelected;

    //--- juce::Component -------------------------------------------------------
    void paint              (juce::Graphics&) override;
    void mouseDown          (const juce::MouseEvent&) override;
    void mouseDrag          (const juce::MouseEvent&) override;
    void mouseMove          (const juce::MouseEvent&) override;
    void mouseExit          (const juce::MouseEvent&) override;
    void visibilityChanged  () override;

private:
    //--- State -----------------------------------------------------------------
    FXSendMatrix     m_matrix;
    juce::StringArray m_slotNames;     // display names for rows
    int               m_numRows { 0 }; // active rows = min(m_slotNames.size(), kFXSlots)

    //--- Interaction -----------------------------------------------------------
    int m_hoverRow { -1 };
    int m_hoverCol { -1 };

    // For vertical drag to set level
    int   m_dragRow   { -1 };
    int   m_dragCol   { -1 };
    float m_dragStartLevel { 0.0f };
    int   m_dragStartY     { 0 };

    //--- Activity flash --------------------------------------------------------
    const std::atomic<uint32_t>* m_audioTickPtr { nullptr };
    uint32_t m_lastAudioTick { 0 };
    float    m_cellFlash[kFXSlots][kCols] {};   // per-cell 0..1 decay
    static constexpr float kFlashDecay = 0.16f; // per 30 Hz tick ~200 ms

    void timerCallback() override;

    //--- Layout ----------------------------------------------------------------
    // Left label column + cell columns
    static constexpr int kLabelW  = 48;
    static constexpr int kHeaderH = 18;
    static constexpr int kCellGap = 2;
    static constexpr int kMinCellW = 28;
    static constexpr int kMinCellH = 14;

    int  cellW()    const noexcept;
    int  cellH()    const noexcept;
    int  numRows()  const noexcept { return m_numRows; }

    juce::Rectangle<int> cellRect    (int row, int col) const noexcept;
    juce::Rectangle<int> headerRect  (int col)          const noexcept;

    // Returns {row, col}; row=-1 means header; both -1 means miss.
    std::pair<int,int> hitTest (juce::Point<int> p) const noexcept;

    // Column header display labels (short)
    static const char* colLabel (int col);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FXSendGrid)
};

#pragma once

#include "../../Theme.h"
#include "InlineEditor.h"
#include "SlotPattern.h"
#include "DrumMachineData.h"

//==============================================================================
/**
    ModuleSlot — one slot in Zone B's 2×4 module grid.

    Pixel layout (base 80px):
      A: Type stripe    3px  — signal type colour
      B: Slot header   18px  — slot number, type badge, name
      C: Module face   45px  — compact param display (interactive)
      D: Port row      14px  — I/O port dots
      E: InlineEditor  94px  — expands when selected (= 174px total)

    Interaction:
      Empty slot — left-click anywhere → onEmptyClicked
      Loaded slot — left-click (no drag) → onSlotClicked
      Loaded slot header — right-click → onRightClickHeader
      Face mini sliders — mouseDown/Drag to change param values
*/
class ModuleSlot : public juce::Component
{
public:
    enum class ModuleType { Empty, Synth, Sampler, Sequencer, Vst3, DrumMachine, Output, Reel };

    explicit ModuleSlot (int index);
    ~ModuleSlot() override = default;

    void setModuleType  (ModuleType type);
    void setSelected    (bool selected);
    void setFocused     (bool focused);   // highlights without showing InlineEditor
    void setMuted       (bool muted);
    void setGroupColor  (juce::Colour c);  // stripe uses group color instead of signal color

    bool       isSelected()     const noexcept { return m_selected;  }
    bool       isFocused()      const noexcept { return m_focused;   }
    bool       isMuted()        const noexcept { return m_muted;     }
    bool       isEmpty()        const noexcept { return m_type == ModuleType::Empty; }
    bool       isDrumMachine()  const noexcept { return m_type == ModuleType::DrumMachine; }
    bool       isSequencer()    const noexcept { return m_type == ModuleType::Sequencer;   }
    ModuleType getModuleType()  const noexcept { return m_type;      }
    int        getSlotIndex()   const noexcept { return m_index;     }

    SlotPattern&       pattern()       noexcept { return m_pattern; }
    const SlotPattern& pattern() const noexcept { return m_pattern; }

    DrumMachineData* drumData() noexcept { return m_drumData.get(); }

    juce::String getModuleTypeName() const;
    juce::Colour getSignalColour()   const;

    // Param/port accessors for ModuleDetailPanel
    const juce::Array<InlineEditor::Param>& getAllParams() const noexcept { return m_allParams; }
    const juce::StringArray&                getInPorts()   const noexcept { return m_inPorts;   }
    const juce::StringArray&                getOutPorts()  const noexcept { return m_outPorts;  }

    static constexpr int kBaseH     = 80;
    static constexpr int kExpandedH = 174;
    static constexpr int kEditorH   = 94;

    // Callbacks set by ZoneBComponent
    std::function<void (int index)> onEmptyClicked;
    std::function<void (int index)> onSlotClicked;
    std::function<void (int index)> onRightClickHeader;

    void paint    (juce::Graphics&) override;
    void resized  () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    juce::MouseCursor getMouseCursor() override;

    InlineEditor& getInlineEditor() noexcept { return m_inlineEditor; }

private:
    int        m_index;
    ModuleType m_type       { ModuleType::Empty };
    bool       m_selected   { false };
    bool       m_focused    { false };
    bool       m_muted      { false };
    juce::Colour m_groupColor { 0x00000000 };  // transparent = use signal color

    SlotPattern                      m_pattern;
    std::unique_ptr<DrumMachineData> m_drumData;

    // All params for inline editor; compact = first 3 (or fewer)
    juce::Array<InlineEditor::Param> m_allParams;
    // Ports
    juce::StringArray m_inPorts, m_outPorts;

    InlineEditor m_inlineEditor;

    // Mini-slider drag state (compact face only)
    int   m_draggingCompactParam { -1 };
    float m_dragStartValue       { 0.0f };
    int   m_dragStartX           { 0 };
    bool  m_mouseDragOccurred    { false };

    // Region constants
    static constexpr int kStripeH = 3;
    static constexpr int kHeaderH = 18;
    static constexpr int kFaceH   = 45;
    static constexpr int kPortH   = 14;
    // Compact face row spec
    static constexpr int kRowH    = 13;   // height of one param row
    static constexpr int kLabelW  = 36;   // label column width
    static constexpr int kValueW  = 28;   // value column width
    static constexpr int kPadV    = 3;    // top/bottom padding inside face
    static constexpr int kThumbD  = 8;    // slider thumb diameter (compact)

    juce::Rectangle<int> regionStripe() const noexcept;
    juce::Rectangle<int> regionHeader() const noexcept;
    juce::Rectangle<int> regionFace()   const noexcept;
    juce::Rectangle<int> regionPort()   const noexcept;

    void paintStripe  (juce::Graphics&) const;
    void paintHeader  (juce::Graphics&) const;
    void paintFaceEmpty       (juce::Graphics&) const;
    void paintFaceLoaded      (juce::Graphics&) const;
    void paintFaceDrumMachine (juce::Graphics&) const;
    void paintPortRow         (juce::Graphics&) const;

    void paintCompactRow (juce::Graphics& g, juce::Rectangle<int> rowR,
                          const InlineEditor::Param& p, int idx) const;

    juce::Rectangle<int> compactSliderTrack (juce::Rectangle<int> rowR) const noexcept;

    /** How many rows fit in compact face (capped at 3). */
    int compactRowCount() const noexcept
    {
        return juce::jmin (m_allParams.size(), 3);
    }

    /** Returns the compact param index hit by point (relative to component),
        or -1 if none. Only checks slider track region. */
    int hitTestCompactSlider (juce::Point<int> pos) const noexcept;

    void initParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleSlot)
};

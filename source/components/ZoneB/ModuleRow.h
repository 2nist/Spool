#pragma once

#include "../../Theme.h"
#include "SlotPattern.h"
#include "DrumMachineData.h"
#include "InlineEditor.h"
#include "VerticalFader.h"
#include "../../instruments/reel/ReelParams.h"

//==============================================================================
/**
    ModuleRow — one full-width row in Zone B's module list.

    Replaces the compact ModuleSlot card grid.  Each row spans the full
    Zone B viewport width so param sliders have room to breathe.

    Layout:
      Header    (kHeaderH = 20px, always visible)
        3px left colour stripe | row# | name | type badge | mute dot | collapse ▾
      Param area (kParamH = 36px, hidden when collapsed)
        Up to 8 params laid out as horizontal sliders with label + value.

    Heights:
      Expanded  : kExpandedH  = kHeaderH + kParamH = 56px
      Collapsed : kCollapsedH = kHeaderH            = 20px
*/
class ModuleRow : public juce::Component
{
public:
    enum class ModuleType { Empty, Synth, Sampler, Sequencer, Vst3, DrumMachine, Output, Reel };

    explicit ModuleRow (int index);
    ~ModuleRow() override = default;

    //==========================================================================
    // State setters

    void setModuleType (ModuleType type);
    void setSelected   (bool selected);
    void setMuted      (bool muted);
    void setGroupColor (juce::Colour c);
    void setCollapsed  (bool collapsed);

    //==========================================================================
    // Queries

    bool         isSelected()    const noexcept { return m_selected;   }
    bool         isMuted()       const noexcept { return m_muted;      }
    bool         isEmpty()       const noexcept { return m_type == ModuleType::Empty;       }
    bool         isDrumMachine() const noexcept { return m_type == ModuleType::DrumMachine; }
    bool         isCollapsed()   const noexcept { return m_collapsed;  }
    ModuleType   getModuleType() const noexcept { return m_type;       }
    int          getRowIndex()   const noexcept { return m_index;      }
    juce::Colour getGroupColor() const noexcept { return m_groupColor; }

    juce::String getModuleTypeName() const;
    juce::Colour getSignalColour()   const;

    //==========================================================================
    // Data

    SlotPattern&       pattern()       noexcept { return m_pattern; }
    const SlotPattern& pattern() const noexcept { return m_pattern; }
    DrumMachineData*   drumData()      noexcept { return m_drumData.get(); }

    const juce::Array<InlineEditor::Param>& getAllParams() const noexcept { return m_params;   }
    const juce::StringArray&                getInPorts()   const noexcept { return m_inPorts;  }
    const juce::StringArray&                getOutPorts()  const noexcept { return m_outPorts; }

    //==========================================================================
    // Height helpers

    static constexpr int kExpandedH  = 180;   // header(20) + two fader rows(160)
    static constexpr int kCollapsedH = 20;
    static constexpr int kHeaderH    = 20;
    static constexpr int kParamH     = kExpandedH - kHeaderH;  // 160
    static constexpr int kSectionH   = 10;   // height of each section label strip

    int currentHeight() const noexcept { return m_collapsed ? kCollapsedH : (m_expandedOverrideH > 0 ? m_expandedOverrideH : kExpandedH); }

    /** Allow caller to override the expanded height (0 = use default kExpandedH). */
    void setExpandedHeightOverride (int h) noexcept { m_expandedOverrideH = h; }

    //==========================================================================
    // Callbacks

    std::function<void (int index)> onRowClicked;     // left-click (non-button area)
    std::function<void (int index)> onEmptyClicked;   // left-click on empty row
    std::function<void (int index)> onRightClicked;   // right-click → context menu
    std::function<void (int index)> onEditorRequested; // [E] button — future VST3
    std::function<void (int index)> onCollapseToggled; // collapse arrow clicked

    /** Fired when a fader value changes. paramIdx = index into getAllParams(). norm = 0-1. */
    std::function<void (int paramIdx, float norm)> onParamChanged;

    //==========================================================================
    // Component overrides

    void paint     (juce::Graphics&) override;
    void resized   () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

private:
    int          m_index;
    ModuleType   m_type      { ModuleType::Empty };
    bool         m_selected  { false };
    bool         m_muted     { false };
    bool         m_collapsed { false };
    juce::Colour m_groupColor { 0xFF4B9EDB };

    SlotPattern                      m_pattern;
    std::unique_ptr<DrumMachineData> m_drumData;

    juce::Array<InlineEditor::Param> m_params;
    juce::StringArray                m_inPorts, m_outPorts;

    int          m_paramRow2Start { -1 };   // -1 = single row; else index of first row-2 fader
    juce::String m_row1Label;
    juce::String m_row2Label;

    //==========================================================================
    // Fader row — VerticalFaders as direct children, clipped to param area

    juce::OwnedArray<VerticalFader> m_faders;

    // Optional override for expanded height (0 = use static kExpandedH)
    int m_expandedOverrideH { 0 };

    //==========================================================================
    // Layout helpers (all coords relative to this component's top-left)

    static constexpr int kStripeW   = 3;   // left group-colour stripe
    static constexpr int kRowNumW   = 22;  // "01", "02" …
    static constexpr int kMuteW     = 14;
    static constexpr int kCollapseW = 18;
    static constexpr int kFaderGap  = 8;   // px between faders

    juce::Rectangle<int> headerRect()    const noexcept;
    juce::Rectangle<int> paramAreaRect() const noexcept;
    juce::Rectangle<int> collapseRect()  const noexcept;
    juce::Rectangle<int> muteRect()      const noexcept;

    void paintHeader    (juce::Graphics&) const;
    void paintParamArea (juce::Graphics&) const;
    void paintEmptyHint (juce::Graphics&) const;

    void initParams();
    void initFaders();

    static juce::Colour          paramColorForLabel (const juce::String& label);
    static VerticalFader::FormatFn makeFormatter    (const InlineEditor::Param& p);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleRow)
};

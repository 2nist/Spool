#pragma once

#include "../../Theme.h"
#include "ChainState.h"
#include "FXChainDisplay.h"
#include "EffectPickerOverlay.h"
#include "MacroKnobsStrip.h"

//==============================================================================
/**
    ZoneCComponent — FX chain editor panel (right column).

    Responsibilities:
      - Own all 8 per-slot ChainState objects (persistent across slot changes)
      - Display the chain for the current (or pinned) slot via FXChainDisplay
      - Show the MacroKnobsStrip at the bottom
      - Handle pin/unpin, collapse/expand, and tab switching
*/
class ZoneCComponent : public juce::Component
{
public:
    //==========================================================================
    // Layout constants
    static constexpr int kCollapsedW  = 20;
    static constexpr int kHeaderH     = 28;
    static constexpr int kPinBannerH  = 20;
    static constexpr int kTabsH       = 24;
    static constexpr int kMacroH      = 80;

    enum class Tab { Chain, Routing, Settings };

    //==========================================================================
    // Collapse callback — fired whenever the collapsed state changes.
    // PluginEditor should call resized() in response.
    std::function<void(bool isCollapsed)> onCollapseChanged;

    //==========================================================================
    // DSP wiring callbacks — fired on the MESSAGE thread.
    // PluginEditor wires these to PluginProcessor::setFXChainParam / rebuildFXChain.

    /** Fired when a parameter is dragged in the FX panel.
        Arguments: slotIndex, nodeIndex (chain position), paramIndex, new value. */
    std::function<void(int slotIdx, int nodeIdx, int paramIdx, float value)> onDspParamChanged;

    /** Fired when the chain structure changes (effect added, removed, reordered).
        Arguments: slotIndex, full ChainState snapshot. */
    std::function<void(int slotIdx, const ChainState& chain)> onChainRebuilt;

    //==========================================================================
    ZoneCComponent();
    ~ZoneCComponent() override = default;

    //==========================================================================
    // Called from PluginEditor / ZoneBComponent::Listener
    void showChainForSlot (int slotIndex);
    void showDefault();

    // Called from PluginEditor::slotSelected to update the module name label
    void setCurrentModuleTypeName (const juce::String& name);

    // Returns actual render width (collapsed = kCollapsedW, else m_expandedWidth)
    int  currentWidth() const;

    /** Called by PluginEditor when the Zone C resizer moves. */
    void setExpandedWidth (int w) noexcept { m_expandedWidth = juce::jlimit (kCollapsedW + 1, 600, w); }

    //==========================================================================
    void paint    (juce::Graphics&) override;
    void resized  () override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    //==========================================================================
    // Child components
    FXChainDisplay              m_chainDisplay;
    juce::Viewport              m_chainViewport;
    MacroKnobsStrip             m_macroStrip;
    std::unique_ptr<EffectPickerOverlay> m_picker;

    //==========================================================================
    // State
    ChainState  m_chains[8];
    int         m_currentSlotIndex  = -1;
    bool        m_isPinned          = false;
    int         m_pinnedSlotIndex   = -1;
    bool        m_isCollapsed       = false;
    int         m_expandedWidth     = static_cast<int> (Theme::Space::zoneCWidth);
    Tab         m_activeTab         = Tab::Chain;
    juce::String m_moduleTypeName;

    //==========================================================================
    // Hit-test rects (set in resized/paint)
    juce::Rectangle<int> m_pinButtonRect;
    juce::Rectangle<int> m_collapseButtonRect;
    juce::Rectangle<int> m_unpinLinkRect;
    juce::Rectangle<int> m_tabRects[3];

    //==========================================================================
    // Helpers
    int  pinBannerHeight()  const;     // kPinBannerH when pinned, 0 otherwise
    int  tabsHeight()       const;     // kTabsH always
    juce::Rectangle<int> chainRect()  const;   // viewport area

    ChainState*       currentChainPtr();
    void              applyDefaultChain  (int slotIndex);
    void              refreshDisplay();
    void              setPinned          (bool pin);
    void              showEffectPicker   ();
    void              dismissPicker      ();
    void              addEffect          (const juce::String& effectId);
    void              removeNode         (int nodeIndex);
    void              moveNode           (int fromIndex, int toIndex);
    void              updateNodeBypass   (int nodeIndex, bool bypassed);
    void              updateNodeParam    (int nodeIndex, int paramIndex, float value);

    void paintCollapsed  (juce::Graphics&);
    void paintHeader     (juce::Graphics&);
    void paintPinBanner  (juce::Graphics&);
    void paintTabs       (juce::Graphics&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZoneCComponent)
};

#pragma once

#include "../../Theme.h"
#include "ChainState.h"
#include "FXChainDisplay.h"
#include "EffectPickerOverlay.h"
#include "MacroKnobsStrip.h"
#include <array>

//==============================================================================
/**
    ZoneCComponent — FX chain editor panel (right column).

    Responsibilities:
      - Own insert chains (8 slots), shared bus chains (A-D), and master chain
      - Display the chain for the active context via FXChainDisplay
      - Show the MacroKnobsStrip at the bottom
      - Handle pin/unpin, collapse/expand, and tab switching
*/
class ZoneCComponent : public juce::Component
{
public:
    enum class ChainContext
    {
        insert = 0,
        busA,
        busB,
        busC,
        busD,
        master
    };

    static constexpr int kNumBusChains = 4;
    static constexpr int kNumContexts  = 6;

    struct ChainBank
    {
        std::array<ChainState, 8> inserts;
        std::array<ChainState, kNumBusChains> buses;
        ChainState master;
    };

    //==========================================================================
    // Layout constants
    static constexpr int kCollapsedW  = 20;
    static constexpr int kHeaderH     = 28;
    static constexpr int kPinBannerH  = 20;
    static constexpr int kTabsH       = 20;
    static constexpr int kMacroH      = 80;

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

    /** Context-aware DSP callback for parameter edits.
        INSERT: contextIndex = slot 0..7
        BUS A-D: contextIndex = bus 0..3
        MASTER: contextIndex = -1 */
    std::function<void(ChainContext context, int contextIndex, int nodeIdx, int paramIdx, float value)> onContextDspParamChanged;

    /** Context-aware callback when chain structure changes. */
    std::function<void(ChainContext context, int contextIndex, const ChainState& chain)> onContextChainRebuilt;

    /** Fired when the active chain context changes.
        contextIndex = slot index for INSERT, bus index (0..3) for BUS A..D, -1 for MASTER. */
    std::function<void(ChainContext context, int contextIndex)> onContextChanged;

    //==========================================================================
    ZoneCComponent();
    ~ZoneCComponent() override = default;

    //==========================================================================
    // Called from PluginEditor / ZoneBComponent::Listener
    void showChainForSlot (int slotIndex);
    void showDefault();
    void setChainContext (ChainContext context);
    ChainContext getChainContext() const noexcept { return m_chainContext; }
    void showInsertContext() { setChainContext (ChainContext::insert); }
    void showBusContext (int busIndex);
    void showMasterContext() { setChainContext (ChainContext::master); }

    // Called from PluginEditor::slotSelected to update the module name label
    void setCurrentModuleTypeName (const juce::String& name);

    // Returns actual render width (collapsed = kCollapsedW, else m_expandedWidth)
    int  currentWidth() const;
    void setMacroStripVisible (bool shouldShow);

    /** Called by PluginEditor when the Zone C resizer moves. */
    void setExpandedWidth (int w) noexcept { m_expandedWidth = juce::jlimit (kCollapsedW + 1, 600, w); }

    ChainBank getChainBank() const;
    void setChainBank (const ChainBank& bank, bool pushToDspCallbacks);

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
    ChainState  m_busChains[kNumBusChains];
    ChainState  m_masterChain;
    int         m_currentSlotIndex  = -1;
    bool        m_isPinned          = false;
    int         m_pinnedSlotIndex   = -1;
    ChainContext m_chainContext     = ChainContext::insert;
    bool        m_isCollapsed       = false;
    bool        m_showMacroStrip    = true;
    int         m_expandedWidth     = static_cast<int> (Theme::Space::zoneCWidth);
    juce::String m_moduleTypeName;
    int         m_focusedNodeIndex   = 0;

    //==========================================================================
    // Hit-test rects (set in resized/paint)
    juce::Rectangle<int> m_pinButtonRect;
    juce::Rectangle<int> m_collapseButtonRect;
    juce::Rectangle<int> m_unpinLinkRect;
    std::array<juce::Rectangle<int>, kNumContexts> m_contextTabRects;
    //==========================================================================
    // Helpers
    int  pinBannerHeight()  const;     // kPinBannerH when pinned, 0 otherwise
    int  tabsHeight()       const;     // kTabsH always
    int  macroStripHeight() const noexcept { return m_showMacroStrip ? kMacroH : 0; }
    juce::Rectangle<int> chainRect()  const;   // viewport area

    bool              isInsertContext() const noexcept;
    int               currentContextIndex() const noexcept;
    ChainState*       chainForContext (ChainContext context);
    ChainState*       currentChainPtr();
    void              ensureCurrentChainInitialised();
    void              applyDefaultChain  (int slotIndex);
    void              applyDefaultBusChain (int busIndex);
    void              applyDefaultMasterChain();
    void              refreshDisplay();
    void              notifyContextChanged();
    void              notifyChainRebuiltForCurrentContext();
    void              notifyParamChangedForCurrentContext (int nodeIndex, int paramIndex, float value);
    void              setPinned          (bool pin);
    void              showEffectPicker   ();
    void              dismissPicker      ();
    void              addEffect          (const juce::String& effectId);
    void              removeNode         (int nodeIndex);
    void              moveNode           (int fromIndex, int toIndex);
    void              updateNodeBypass   (int nodeIndex, bool bypassed);
    void              updateNodeParam    (int nodeIndex, int paramIndex, float value);
    void              setFocusedNode     (int nodeIndex);

    void paintCollapsed  (juce::Graphics&);
    void paintHeader     (juce::Graphics&);
    void paintPinBanner  (juce::Graphics&);
    void paintContextTabs (juce::Graphics&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZoneCComponent)
};

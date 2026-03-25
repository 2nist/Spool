#pragma once

#include "../../Theme.h"
#include "ChainState.h"
#include "FXNode.h"

//==============================================================================
/**
    FXChainDisplay — scrollable content component inside Zone C's Viewport.

    Paints IN node, connector lines, FXNode children, + button, OUT node.
    Handles drag-reorder of effect nodes.

    Total height = 144 + (N × 84)  where N = number of effect nodes.
    Sets its own height via setSize() in refresh().
*/
class FXChainDisplay : public juce::Component
{
public:
    FXChainDisplay();
    ~FXChainDisplay() override = default;

    /** Rebuild display from chain data. Called whenever chain changes. */
    void refresh (const ChainState* chain);

    /** Returns the Y position of the + button in FXChainDisplay-local coords. */
    int addButtonY() const noexcept { return m_addButtonY; }

    // Callbacks
    std::function<void()>                             onAddEffectClicked;
    std::function<void(int nodeIndex, bool bypassed)> onBypassChanged;
    std::function<void(int nodeIndex)>                onNodeRemoved;
    std::function<void(int nodeIndex, int paramIdx, float value)> onParamChanged;
    std::function<void(int nodeIndexFrom, int nodeIndexTo)>       onNodeMoved;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

private:
    const ChainState* m_chain         { nullptr };
    int               m_addButtonY    { 0 };
    bool              m_addHover      { false };

    // FXNode children — one per effect node in the chain
    juce::OwnedArray<FXNode>  m_nodes;

    // Drag reorder state
    int  m_dragNodeIndex   { -1 };
    int  m_dropTargetIndex { -1 };
    int  m_dragStartY      { 0 };
    bool m_isDragging      { false };

    // Confirm-last-node state
    int  m_confirmNodeIndex { -1 };

    static constexpr int kPad      = 16;   // top and bottom padding
    static constexpr int kConnH    = 12;   // connector gap height
    static constexpr int kNodeH    = 72;   // FXNode height
    static constexpr int kNodeStep = 84;   // kNodeH + kConnH
    static constexpr int kInOutH   = 32;   // IN / OUT node height
    static constexpr int kAddBtnH  = 24;   // + ADD EFFECT height

    // Returns Y for IN node top
    int inNodeY()     const noexcept { return kPad; }
    // Returns Y for first effect node top
    int firstNodeY()  const noexcept { return inNodeY() + kInOutH + kConnH; }
    // Returns Y for effect node [i] top
    int nodeY (int i) const noexcept { return firstNodeY() + i * kNodeStep; }

    void paintConnector   (juce::Graphics& g, int y) const;
    void paintInOutNode   (juce::Graphics& g, bool isOut, int y) const;
    void paintAddButton   (juce::Graphics& g) const;
    void paintDragOverlay (juce::Graphics& g) const;

    void startDrag (int nodeIndex, const juce::MouseEvent& e);

    void buildNodes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FXChainDisplay)
};

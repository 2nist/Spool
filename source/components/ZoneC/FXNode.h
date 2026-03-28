#pragma once

#include "../../Theme.h"
#include "ChainState.h"

//==============================================================================
/**
    FXNode — one effect node in Zone C's FX chain display.

    Total height: 72px
      Header:  20px   (colour dot, name, tag, bypass, drag handle, remove)
      Params:  45px   (3 rows × 15px)
      Padding:  7px   (3px top of params + 4px bottom of params)

    Callbacks set by FXChainDisplay:
      onBypassToggled (nodeIndex)
      onRemoveClicked (nodeIndex)
      onDragStarted   (nodeIndex, MouseEvent)
      onParamChanged  (nodeIndex, paramIndex, newValue)
*/
class FXNode : public juce::Component
{
public:
    static constexpr int kHeight     = 72;
    static constexpr int kHeaderH    = 20;
    static constexpr int kParamAreaH = 45;
    static constexpr int kPadTop     = 3;
    static constexpr int kPadBot     = 4;
    static constexpr int kRowH       = 15;
    static constexpr int kLabelW     = 44;
    static constexpr int kValueW     = 28;
    static constexpr int kArrowW     = 14;

    FXNode (int nodeIndex, const EffectNode& data);
    ~FXNode() override = default;

    void setData (int nodeIndex, const EffectNode& data);
    void setDragging (bool isDragging);
    void showRemoveConfirmation (bool show);
    void setFocused (bool isFocused);

    // Callbacks
    std::function<void(int nodeIndex)>                             onBypassToggled;
    std::function<void(int nodeIndex)>                             onRemoveClicked;
    std::function<void(int nodeIndex, const juce::MouseEvent&)>    onDragStarted;
    std::function<void(int nodeIndex, int paramIdx, float value)>  onParamChanged;
    std::function<void(int nodeIndex)>                             onFocused;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    juce::MouseCursor getMouseCursor() override;

private:
    int        m_nodeIndex  { 0 };
    EffectNode m_data;
    bool       m_isDragging { false };
    bool       m_isFocused { false };
    bool       m_showConfirm { false };
    bool       m_hoverRemove { false };

    // Param slider drag state
    int   m_draggingParam  { -1 };
    float m_dragStartVal   { 0.0f };
    int   m_dragStartX     { 0 };

    // Regions
    juce::Rectangle<int> headerRect()     const noexcept;
    juce::Rectangle<int> paramAreaRect()  const noexcept;
    juce::Rectangle<int> bypassRect()     const noexcept;
    juce::Rectangle<int> removeRect()     const noexcept;
    juce::Rectangle<int> dragHandleRect() const noexcept;
    juce::Rectangle<int> rowRect (int rowIdx) const noexcept;

    // Paint helpers
    void paintHeader  (juce::Graphics&) const;
    void paintParams  (juce::Graphics&) const;
    void paintConfirm (juce::Graphics&) const;
    void paintRow     (juce::Graphics&, int rowIdx) const;

    juce::Rectangle<int> trackRect (juce::Rectangle<int> rowR) const noexcept;

    int hitTestParam (juce::Point<int> pos) const noexcept;
    int hitTestArrow (juce::Point<int> pos, int rowIdx) const noexcept; // -1,0,+1

    const EffectTypeDef* def() const noexcept
    {
        return EffectRegistry::find (m_data.effectType);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FXNode)
};

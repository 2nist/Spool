#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    InlineEditor — expandable parameter panel shown inside a selected ModuleSlot.

    Total height: 94px.
      Top border:      1px  — signal type colour
      Editor header:  16px  — "PARAMETERS" label + module type
      Parameter area:  fills remaining (59px base)
      Bottom strip:   18px  — IN/OUT port type badges

    Mini sliders (15px rows) respond to mouse drag.
    All params for the module type are shown here.
*/
class InlineEditor : public juce::Component
{
public:
    struct Param
    {
        juce::String label;
        float        min { 0.0f }, max { 1.0f }, value { 0.5f };
    };

    InlineEditor();
    ~InlineEditor() override = default;

    void setParams (const juce::Array<Param>&       params,
                    const juce::Colour&              signalColour,
                    const juce::String&              moduleTypeName,
                    const juce::StringArray&         inPorts,
                    const juce::StringArray&         outPorts);

    std::function<void (int paramIdx, float newValue)> onParamChanged;

    void paint   (juce::Graphics&) override;
    void resized () override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

private:
    juce::Array<Param>  m_params;
    juce::Colour        m_signalColour { Theme::Colour::inkMid };
    juce::String        m_moduleType;
    juce::StringArray   m_inPorts, m_outPorts;

    int   m_draggingParamIdx { -1 };
    float m_dragStartValue   { 0.0f };
    int   m_dragStartX       { 0 };

    static constexpr int kTopBorderH = 1;
    static constexpr int kHeaderH    = 16;
    static constexpr int kBottomH    = 18;
    static constexpr int kRowH       = 15;
    static constexpr int kLabelW     = 52;
    static constexpr int kValueW     = 32;
    static constexpr int kThumbD     = 10;
    static constexpr int kPadV       = 4;  // top/bottom padding inside param area

    juce::Rectangle<int> paramAreaBounds() const noexcept;
    juce::Rectangle<int> rowBounds (int i) const noexcept;
    juce::Rectangle<int> trackBounds (juce::Rectangle<int> rowR) const noexcept;

    void paintRow (juce::Graphics& g, int i) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InlineEditor)
};

#pragma once

#include "../../Theme.h"
#include "../../module/ModuleProcessor.h"

//==============================================================================
/**
    PolySynthEditorComponent — real mounted editor for a PolySynthProcessor slot.

    All controls are live JUCE child components.  Writes directly to the
    processor's atomic setters; safe to call from the message thread while
    audio runs.

    Call setProcessor(ptr) to bind.  Call setProcessor(nullptr) to unbind
    (controls become non-functional but remain visible).

    Required / preferred height: kRequiredHeight px.
    InstrumentPanel wraps this in a Viewport so it scrolls if the panel is short.
*/
class PolySynthEditorComponent : public juce::Component
{
public:
    static constexpr int kRequiredHeight = 540;

    PolySynthEditorComponent();
    ~PolySynthEditorComponent() override;

    /** Bind to a running processor.  Reads back available initial values. */
    void setProcessor (ModuleProcessor* proc);

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    class EnvelopeGraphComponent;

    ModuleProcessor* m_proc = nullptr;

    //------------------------------------------------------------------
    // Oscillators
    juce::TextButton m_osc1On { "OSC 1" }, m_osc2On { "OSC 2" };
    juce::TextButton m_osc1ShapeBtn { "SAW" }, m_osc2ShapeBtn { "SAW" };
    juce::ComboBox   m_osc1Octave, m_osc2Octave;
    juce::Slider     m_osc1Detune, m_osc2Detune;
    juce::Slider     m_osc1PulseWidth, m_osc2PulseWidth;
    juce::Slider     m_osc2Level;

    //------------------------------------------------------------------
    // Filter
    juce::Slider   m_filterCutoff, m_filterRes, m_filterEnvAmt;

    //------------------------------------------------------------------
    // Amp envelope
    std::unique_ptr<EnvelopeGraphComponent> m_ampEnvGraph;
    float m_ampAttack  { 0.01f };
    float m_ampDecay   { 0.1f  };
    float m_ampSustain { 0.8f  };
    float m_ampRelease { 0.3f  };

    //------------------------------------------------------------------
    // Filter envelope
    std::unique_ptr<EnvelopeGraphComponent> m_filtEnvGraph;
    juce::Slider   m_filtA, m_filtD, m_filtS, m_filtR;
    float m_filtAttack  { 0.01f };
    float m_filtDecay   { 0.2f  };
    float m_filtSustain { 0.5f  };
    float m_filtRelease { 0.3f  };

    //------------------------------------------------------------------
    // LFO
    juce::ToggleButton m_lfoOn { "LFO" };
    juce::Slider   m_lfoRate, m_lfoDepth;
    juce::ComboBox m_lfoTarget;

    //------------------------------------------------------------------
    // Voice mode
    juce::TextButton m_polyBtn { "POLY" }, m_monoBtn { "MONO" };

    //------------------------------------------------------------------
    // Character
    juce::Slider   m_charAsym, m_charDrive, m_charDrift;

    //------------------------------------------------------------------
    // Output
    juce::Slider   m_outLevel, m_outPan;

    //------------------------------------------------------------------
    // Layout data built in resized(), consumed by paint()
    struct SectionInfo { juce::String name; juce::Colour colour; int y; };
    struct LabelInfo   { juce::String text; juce::Rectangle<int> bounds; };
    juce::Array<SectionInfo> m_sections;
    juce::Array<LabelInfo>   m_labels;

    //------------------------------------------------------------------
    void setupControls();
    void readFromProcessor();
    void syncAmpEnvelopeGraph();
    void syncFilterEnvelopeGraph();

    // Slider factory helper — configures style, range, default, and onChange
    void initSlider (juce::Slider& s,
                     double min, double max, double midPt, double defaultVal,
                     std::function<void(double)> onChange);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolySynthEditorComponent)
};

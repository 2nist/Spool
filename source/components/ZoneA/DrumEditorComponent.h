#pragma once

#include "../../Theme.h"
#include "../../instruments/drum/DrumModuleState.h"
#include "ZoneAControlStyle.h"

//==============================================================================
/**
    DrumEditorComponent — Zone A sound-design editor for a drum module state.

    Exposes per-voice Tone / Noise / Click / Output controls for the currently
    selected voice in the authoritative DrumModuleState.

    Voice selection is dynamic and scales with the number of voices present in
    the state object. Edits mutate the shared model and fire onStateChanged so
    the owner can apply the updated state to DSP in one explicit sync point.
*/
class DrumEditorComponent : public juce::Component
{
public:
    static constexpr int kRequiredHeight = 492;

    DrumEditorComponent();
    ~DrumEditorComponent() override;

    /** Bind to the authoritative drum state. Pass nullptr to unbind. */
    void setState (DrumModuleState* state);

    /** Fired after any shared-state edit performed in the editor. */
    std::function<void()> onStateChanged;

    /** Fired when the user previews a voice from the editor. */
    std::function<void (int voiceIndex, float velocity)> onPreviewVoice;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    DrumModuleState* m_state         = nullptr;
    int              m_selectedVoice = 0;

    /** Local mirror of the current voice's params — prevents stale-read when
        multiple controls change within one gesture. */
    DrumVoiceParams m_localParams {};

    //----------------------------------------------------------------------
    // Voice selector + trigger
    juce::OwnedArray<juce::TextButton> m_voiceTabs;
    juce::TextButton m_trigBtn  { "TRIG" };

    //----------------------------------------------------------------------
    // Tone layer
    juce::TextButton m_toneEnable  { "ON" };
    juce::Slider     m_tonePitch, m_toneSweep, m_tonePtDecay, m_toneAttack, m_toneDecay, m_toneLevel, m_toneBody;
    juce::TextButton m_toneWave    { "SIN" };

    //----------------------------------------------------------------------
    // Noise layer
    juce::TextButton m_noiseEnable { "ON" };
    juce::Slider     m_noiseDecay, m_noiseLevel, m_noiseHP, m_noiseLP;

    //----------------------------------------------------------------------
    // Click layer
    juce::TextButton m_clickEnable { "ON" };
    juce::Slider     m_clickLevel, m_clickDecay;

    //----------------------------------------------------------------------
    // Output
    juce::Slider m_outLevel, m_outPan, m_outTune;

    //----------------------------------------------------------------------
    // Character
    juce::Slider m_charDrive, m_charGrit, m_charSnap;

    //----------------------------------------------------------------------
    // Layout cache (rebuilt in resized(), consumed by paint())
    struct SectionInfo { juce::String name; juce::Colour colour; int y; };
    struct LabelInfo   { juce::String text; juce::Rectangle<int> bounds; };
    juce::Array<SectionInfo> m_sections;
    juce::Array<LabelInfo>   m_labels;

    //----------------------------------------------------------------------
    void setupControls();
    void rebuildVoiceTabs();
    void selectVoice    (int voiceIndex);
    void readFromState();
    DrumVoiceState* selectedVoice() noexcept;
    const DrumVoiceState* selectedVoice() const noexcept;

    /** Copy-modify-push: apply fn to m_localParams then push to processor. */
    void modifyVoice (std::function<void(DrumVoiceParams&)> fn);

    void initSlider (juce::Slider& s,
                     double min, double max, double mid, double def,
                     std::function<void(double)> fn);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumEditorComponent)
};

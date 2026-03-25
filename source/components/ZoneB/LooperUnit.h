#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    LooperUnit — visual stub for one looper track.

    Layout (inside allocated rect):
      LEFT CONTROLS (120px):
        SOURCE label + "MIX" dropdown stub
        MODE toggle "QUANTIZED" ↔ "FREE"
        Transport: ● REC  ▶ PLAY  ⊕ DUB  ⊗ CLEAR  (each 18×18px)

      CENTRE (fills remaining):
        Empty: "HOLD REC TO CAPTURE"
        Recorded: waveform RMS display + playhead  (TODO: DSP session)

      RIGHT DRAG HANDLE (16px):
        "···" drag indicator
        Drag to Zone D: creates audio clip (TODO: drag drop in DSP session)

    Audio is a visual stub — all DSP annotated TODO.
*/
class LooperUnit : public juce::Component
{
public:
    LooperUnit();
    ~LooperUnit() override = default;

    /** Populate the SOURCE selector with available group/slot names. */
    void setSourceNames (const juce::StringArray& names);

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::MouseCursor getMouseCursor() override;

    static constexpr int kCtrlW   = 120;
    static constexpr int kHandleW = 16;
    static constexpr int kBtnSz   = 18;

private:
    juce::StringArray m_sourceNames;
    int               m_selectedSource { 0 };
    bool              m_freeMode        { false };  // false = QUANTIZED

    // Transport stub booleans
    bool              m_recording  { false };
    bool              m_playing    { false };
    bool              m_dubbing    { false };

    // Drag handle hover
    bool              m_handleHovered { false };

    // Region helpers
    juce::Rectangle<int> ctrlRect()        const noexcept;
    juce::Rectangle<int> waveformRect()    const noexcept;
    juce::Rectangle<int> handleRect()      const noexcept;
    juce::Rectangle<int> sourceDropRect()  const noexcept;
    juce::Rectangle<int> modeToggleRect()  const noexcept;
    juce::Rectangle<int> recBtnRect()      const noexcept;
    juce::Rectangle<int> playBtnRect()     const noexcept;
    juce::Rectangle<int> dubBtnRect()      const noexcept;
    juce::Rectangle<int> clearBtnRect()    const noexcept;

    void paintCtrl     (juce::Graphics& g) const;
    void paintWaveform (juce::Graphics& g) const;
    void paintHandle   (juce::Graphics& g) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperUnit)
};

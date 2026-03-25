#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    TransportSettingsPanel — transport-related settings.

    Opened via the TPRT tab or the ⚙ gear button in Zone D's transport strip.

    Content:
      Metronome on/off toggle + volume slider
      Count-in (0..4 bars selector)
      MIDI clock out toggle
      Ableton Link toggle
      Tempo master toggle
*/
class TransportSettingsPanel : public juce::Component
{
public:
    TransportSettingsPanel();
    ~TransportSettingsPanel() override = default;

    // Callbacks
    std::function<void(bool enabled, float volume)> onMetronomeChanged;
    std::function<void(int bars)>                   onCountInChanged;
    std::function<void(bool enabled)>               onMidiClockChanged;
    std::function<void(bool enabled)>               onLinkToggled;
    std::function<void(bool master)>                onTempoMasterChanged;

    bool isLinkEnabled() const noexcept { return m_linkEnabled; }

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    bool  m_metronomeEnabled  { false };
    float m_metronomeVolume   { 0.7f  };
    int   m_countInBars       { 0 };
    bool  m_midiClockEnabled  { false };
    bool  m_linkEnabled       { false };
    bool  m_tempoMaster       { true  };

    juce::Slider     m_metronomeSlider;
    juce::ComboBox   m_countInCombo;

    static constexpr int kPad  = 8;
    static constexpr int kRowH = 28;
    static constexpr int kLblH = 14;

    struct Row
    {
        juce::String label;
        bool*        state;
    };

    void mouseDown (const juce::MouseEvent&) override;
    juce::Rectangle<int> toggleRowRect (int index) const noexcept;
    void paintToggleRow (juce::Graphics& g, int index, const juce::String& label, bool enabled) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportSettingsPanel)
};

#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    RoutingPanel — simple routing UI for the RTE tab.

    Minimal skeleton: shows main output summary and a small "Open routing" button.
*/
class RoutingPanel : public juce::Component
{
public:
    RoutingPanel();
    ~RoutingPanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

    // For tests / external control
    static constexpr int kSources = 4;
    static constexpr int kOutputs = 2;

private:
    juce::Rectangle<int> headerRect() const noexcept;

    std::vector<std::unique_ptr<juce::ToggleButton>> m_buttons;
    juce::StringArray m_sourceNames { "Input", "Synth", "Sampler", "FX" };
    juce::StringArray m_outputNames { "L", "R" };

public:
    /** Returns a compact summary string for display (e.g. "Stereo", "Mono L"). */
    juce::String getSummary() const;

    /** Called when the matrix changes. Args: flat row-major vector of kSources*kOutputs bytes (0/1). */
    std::function<void (const std::vector<uint8_t>&)> onMatrixChanged;

    /** Programmatically set the matrix. */
    void setMatrix (const std::vector<uint8_t>& vals);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoutingPanel)
};

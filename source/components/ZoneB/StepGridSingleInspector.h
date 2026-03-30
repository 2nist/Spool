#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "StepGridSingle.h"
#include "SlotPattern.h"

class StepGridSingleInspector  : public juce::Component
{
public:
    StepGridSingleInspector();
    ~StepGridSingleInspector() override;

    void setStepGridSingle (StepGridSingle* sg) noexcept;

    // Programmatic helpers for tests and automation
    void setVelocityAndCommit (int velocity);
    void setGateAndCommit (bool gateOn);
    void setLengthAndCommit (int lengthUnits);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    StepGridSingle* stepGrid = nullptr;

    juce::Slider velocitySlider;
    juce::ToggleButton gateToggle { "Gate" };
    juce::ComboBox lengthBox;

    void refresh();
    void attachCallbacks();

    void applyVelocityChange();
    void applyGateChange();
    void applyLengthChange();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepGridSingleInspector)
};

#include "StepGridSingleInspector.h"

StepGridSingleInspector::StepGridSingleInspector()
{
}

StepGridSingleInspector::~StepGridSingleInspector() = default;

void StepGridSingleInspector::setStepGridSingle (ZoneB::StepGridSingle* sg) noexcept
{
    stepGrid = sg;
    refresh();
}

void StepGridSingleInspector::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::transparentBlack);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.drawRect (getLocalBounds());
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (14.0f);
    g.drawText ("Step Inspector", getLocalBounds().reduced (6), juce::Justification::topLeft);
}

void StepGridSingleInspector::resized()
{
}

void StepGridSingleInspector::refresh()
{
    // TODO: read state from `stepGrid` and update child controls (velocity, gate, length)
}

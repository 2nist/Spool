#include "StepGridSingleInspector.h"

#include <iostream>

StepGridSingleInspector::StepGridSingleInspector()
{
    addAndMakeVisible (velocitySlider);
    velocitySlider.setRange (0.0, 127.0, 1.0);
    velocitySlider.setTextValueSuffix (" vel");
    velocitySlider.setValue (100.0);

    addAndMakeVisible (gateToggle);

    addAndMakeVisible (lengthBox);
    for (int i = 1; i <= 16; ++i)
        lengthBox.addItem (juce::String (i), i);
    lengthBox.setSelectedId (1);

    attachCallbacks();
}


StepGridSingleInspector::~StepGridSingleInspector() = default;

void StepGridSingleInspector::attachCallbacks()
{
    velocitySlider.onValueChange = [this]() { applyVelocityChange(); };
    gateToggle.onClick = [this]() { applyGateChange(); };
    lengthBox.onChange = [this]() { applyLengthChange(); };
}

void StepGridSingleInspector::setStepGridSingle (StepGridSingle* sg) noexcept
{
    stepGrid = sg;
    refresh();
}

void StepGridSingleInspector::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::transparentBlack);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.drawRect (getLocalBounds());
}

void StepGridSingleInspector::resized()
{
    auto r = getLocalBounds().reduced (6);
    auto left = r.removeFromLeft (r.getWidth() / 3);
    velocitySlider.setBounds (left.removeFromTop (32));
    gateToggle.setBounds (left.removeFromTop (22).withTrimmedLeft (4));
    lengthBox.setBounds (r.removeFromLeft (120));
}


void StepGridSingleInspector::refresh()
{
    if (stepGrid == nullptr)
        return;

    if (const auto* step = stepGrid->getSelectedStepData())
    {
        // Use first microEvent velocity as representative if present
        uint8_t vel = 100;
        if (step->microEventCount > 0)
            vel = step->microEvents[0].velocity;
        velocitySlider.setValue ((double) vel, juce::dontSendNotification);
        gateToggle.setToggleState (step->gateMode != SlotPattern::GateMode::rest, juce::dontSendNotification);
        lengthBox.setSelectedId (juce::jlimit (1, SlotPattern::MAX_STEP_DURATION, step->duration), juce::dontSendNotification);
    }
}

void StepGridSingleInspector::applyVelocityChange()
{
    if (stepGrid == nullptr) return;
    auto* step = stepGrid->getSelectedStepData();
    if (step == nullptr) return;

    if (stepGrid->onBeforeEdit) stepGrid->onBeforeEdit();

    const int newVel = (int) velocitySlider.getValue();
    if (step->microEventCount <= 0)
    {
        step->microEventCount = 1;
        step->microEvents[0] = {};
    }
    step->microEvents[0].velocity = (uint8_t) juce::jlimit (0, 127, newVel);

    if (stepGrid->onModified) stepGrid->onModified();
    if (stepGrid->onGestureEnd) stepGrid->onGestureEnd();
}

void StepGridSingleInspector::applyGateChange()
{
    if (stepGrid == nullptr) return;
    auto* step = stepGrid->getSelectedStepData();
    if (step == nullptr) return;

    if (stepGrid->onBeforeEdit) stepGrid->onBeforeEdit();

    const bool isOn = gateToggle.getToggleState();
    step->gateMode = isOn ? SlotPattern::GateMode::gate : SlotPattern::GateMode::rest;
    if (isOn && step->microEventCount <= 0)
    {
        step->microEventCount = 1;
        step->microEvents[0] = {};
    }
    if (! isOn)
        step->microEventCount = 0;

    if (stepGrid->onModified) stepGrid->onModified();
    if (stepGrid->onGestureEnd) stepGrid->onGestureEnd();
}

void StepGridSingleInspector::applyLengthChange()
{
    if (stepGrid == nullptr) return;
    auto* step = stepGrid->getSelectedStepData();
    if (step == nullptr) return;

    if (stepGrid->onBeforeEdit) stepGrid->onBeforeEdit();

    const int newLen = lengthBox.getSelectedId();
    step->duration = juce::jlimit (1, SlotPattern::MAX_STEP_DURATION, newLen);

    if (stepGrid->onModified) stepGrid->onModified();
    if (stepGrid->onGestureEnd) stepGrid->onGestureEnd();
}

void StepGridSingleInspector::setVelocityAndCommit (int velocity)
{
    velocitySlider.setValue ((double) velocity, juce::dontSendNotification);
    applyVelocityChange();
}

void StepGridSingleInspector::setGateAndCommit (bool gateOn)
{
    gateToggle.setToggleState (gateOn, juce::dontSendNotification);
    applyGateChange();
}

void StepGridSingleInspector::setLengthAndCommit (int lengthUnits)
{
    lengthBox.setSelectedId (lengthUnits, juce::dontSendNotification);
    applyLengthChange();
}



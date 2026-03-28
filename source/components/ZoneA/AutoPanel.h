#pragma once

#include "../../Theme.h"
#include "../../state/AutomationState.h"
#include "ZoneAStyle.h"
#include "ZoneAControlStyle.h"

class PluginProcessor;

class AutoPanel : public juce::Component,
                  private juce::ListBoxModel
{
public:
    explicit AutoPanel (PluginProcessor& processor);
    ~AutoPanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshFromModel();

private:
    PluginProcessor& processorRef;
    juce::ListBox laneList;
    juce::TextButton addLaneButton { "+ LANE" };
    juce::TextButton removeLaneButton { "DEL" };
    juce::ToggleButton enabledToggle { "ENABLED" };
    juce::ComboBox scopeBox;
    juce::ComboBox targetBox;
    juce::Label laneHeader;
    juce::Label inspectorHeader;
    juce::Label hintLabel;
    juce::Label scopeLabel;
    juce::Label targetLabel;
    juce::Label pointsSummary;
    int selectedIndex { -1 };
    bool suppressCallbacks { false };

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int row) override;

    AutomationState& automationForEdit();
    const AutomationState& automation() const;
    AutomationLane* selectedLane();
    void commitChanges (const AutomationState& beforeState, const juce::String& actionName);
    void refreshAll();
    void refreshInspector();
    void addLane();
    void removeLane();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoPanel)
};

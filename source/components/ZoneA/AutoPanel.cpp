#include "AutoPanel.h"
#include "../../PluginProcessor.h"
#include "../../PluginEditor.h"

namespace
{
struct AutomationUndoAction final : public juce::UndoableAction
{
    AutomationUndoAction (PluginProcessor& processorIn,
                          const AutomationState& beforeIn,
                          const AutomationState& afterIn)
        : processor (processorIn), beforeState (beforeIn), afterState (afterIn) {}

    bool perform() override
    {
        processor.getSongManager().replaceAutomationState (afterState);
        processor.syncAuthoredSongToRuntime();
        if (auto* editor = dynamic_cast<PluginEditor*> (processor.getActiveEditor()))
            editor->refreshFromAuthoredSong();
        return true;
    }

    bool undo() override
    {
        processor.getSongManager().replaceAutomationState (beforeState);
        processor.syncAuthoredSongToRuntime();
        if (auto* editor = dynamic_cast<PluginEditor*> (processor.getActiveEditor()))
            editor->refreshFromAuthoredSong();
        return true;
    }

    int getSizeInUnits() override { return 1; }

    PluginProcessor& processor;
    AutomationState beforeState;
    AutomationState afterState;
};
}

AutoPanel::AutoPanel (PluginProcessor& processor) : processorRef (processor)
{
    const auto accent = ZoneAStyle::accentForTabId ("automate");
    for (auto* c : { static_cast<juce::Component*> (&laneList), static_cast<juce::Component*> (&addLaneButton),
                     static_cast<juce::Component*> (&removeLaneButton), static_cast<juce::Component*> (&enabledToggle),
                     static_cast<juce::Component*> (&scopeBox), static_cast<juce::Component*> (&targetBox),
                     static_cast<juce::Component*> (&laneHeader), static_cast<juce::Component*> (&inspectorHeader),
                     static_cast<juce::Component*> (&hintLabel), static_cast<juce::Component*> (&scopeLabel),
                     static_cast<juce::Component*> (&targetLabel), static_cast<juce::Component*> (&pointsSummary) })
        addAndMakeVisible (*c);

    laneHeader.setText ("AUTOMATION LANES", juce::dontSendNotification);
    inspectorHeader.setText ("FOUNDATION", juce::dontSendNotification);
    hintLabel.setText ("Choose target + scope now; later timeline work can draw points against these same lanes.", juce::dontSendNotification);
    scopeLabel.setText ("SCOPE", juce::dontSendNotification);
    targetLabel.setText ("TARGET", juce::dontSendNotification);

    for (auto* label : { &laneHeader, &inspectorHeader })
    {
        label->setFont (Theme::Font::micro());
        label->setColour (juce::Label::textColourId, accent);
    }
    for (auto* label : { &hintLabel, &scopeLabel, &targetLabel, &pointsSummary })
    {
        label->setFont (Theme::Font::micro());
        label->setColour (juce::Label::textColourId, Theme::Colour::inkGhost);
    }

    laneList.setModel (this);
    laneList.setRowHeight (24);
    laneList.setColour (juce::ListBox::backgroundColourId, Theme::Colour::surface2);
    ZoneAControlStyle::styleTextButton (addLaneButton, accent);
    ZoneAControlStyle::styleTextButton (removeLaneButton, accent);
    ZoneAControlStyle::styleToggleButton (enabledToggle, accent);
    ZoneAControlStyle::styleComboBox (scopeBox, accent);
    ZoneAControlStyle::styleComboBox (targetBox, accent);

    scopeBox.addItem ("Mixer", 1);
    scopeBox.addItem ("Slot", 2);
    scopeBox.addItem ("FX", 3);
    scopeBox.addItem ("Route", 4);
    targetBox.addItem ("Master Gain", 1);
    targetBox.addItem ("Master Pan", 2);
    targetBox.addItem ("Slot Level", 3);
    targetBox.addItem ("FX Param", 4);
    targetBox.addItem ("Route Enable", 5);

    addLaneButton.onClick = [this] { addLane(); };
    removeLaneButton.onClick = [this] { removeLane(); };
    enabledToggle.onClick = [this]
    {
        if (suppressCallbacks)
            return;
        if (auto* lane = selectedLane())
        {
            const auto beforeState = automation();
            lane->enabled = enabledToggle.getToggleState();
            commitChanges (beforeState, "Toggle Automation Lane");
            refreshAll();
        }
    };
    scopeBox.onChange = [this]
    {
        if (suppressCallbacks)
            return;
        if (auto* lane = selectedLane())
        {
            const auto beforeState = automation();
            lane->scope = scopeBox.getText();
            commitChanges (beforeState, "Change Automation Scope");
            refreshAll();
        }
    };
    targetBox.onChange = [this]
    {
        if (suppressCallbacks)
            return;
        if (auto* lane = selectedLane())
        {
            const auto beforeState = automation();
            lane->displayName = targetBox.getText();
            lane->targetId = targetBox.getText().toLowerCase().replaceCharacter (' ', '_');
            commitChanges (beforeState, "Change Automation Target");
            refreshAll();
        }
    };

    refreshAll();
}

AutomationState& AutoPanel::automationForEdit()
{
    return processorRef.getSongManager().getAutomationStateForEdit();
}

const AutomationState& AutoPanel::automation() const
{
    return processorRef.getSongManager().getAutomationState();
}

AutomationLane* AutoPanel::selectedLane()
{
    auto& state = automationForEdit();
    if (selectedIndex < 0 || selectedIndex >= static_cast<int> (state.lanes.size()))
        return nullptr;
    return &state.lanes[(size_t) selectedIndex];
}

void AutoPanel::commitChanges (const AutomationState& beforeState, const juce::String& actionName)
{
    const auto afterState = automation();
    processorRef.getUndoManager().beginNewTransaction (actionName);
    processorRef.getUndoManager().perform (new AutomationUndoAction (processorRef, beforeState, afterState));
}

int AutoPanel::getNumRows()
{
    return static_cast<int> (automation().lanes.size());
}

void AutoPanel::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    const auto& state = automation();
    if (row < 0 || row >= static_cast<int> (state.lanes.size()))
        return;
    const auto& lane = state.lanes[(size_t) row];
    const auto accent = ZoneAStyle::accentForTabId ("automate");
    ZoneAStyle::drawRowBackground (g, { 0, 0, width, height }, false, selected, accent);
    g.setColour (lane.enabled ? Theme::Colour::inkLight : Theme::Colour::inkGhost);
    g.setFont (Theme::Font::label());
    g.drawText (lane.displayName.isNotEmpty() ? lane.displayName : "Untitled Lane", 8, 0, width - 90, height, juce::Justification::centredLeft, true);
    g.setColour (Theme::Colour::inkGhost);
    g.setFont (Theme::Font::micro());
    g.drawText (lane.scope, width - 88, 0, 80, height, juce::Justification::centredRight, false);
}

void AutoPanel::selectedRowsChanged (int row)
{
    selectedIndex = row;
    refreshInspector();
}

void AutoPanel::refreshAll()
{
    laneList.updateContent();
    if (selectedIndex >= 0)
        laneList.selectRow (selectedIndex, false, false);
    refreshInspector();
}

void AutoPanel::refreshInspector()
{
    suppressCallbacks = true;
    const auto* lane = selectedLane();
    const bool has = lane != nullptr;
    removeLaneButton.setEnabled (has);
    enabledToggle.setEnabled (has);
    scopeBox.setEnabled (has);
    targetBox.setEnabled (has);

    if (! has)
    {
        enabledToggle.setToggleState (false, juce::dontSendNotification);
        scopeBox.setSelectedId (0, juce::dontSendNotification);
        targetBox.setSelectedId (0, juce::dontSendNotification);
        pointsSummary.setText ("No lane selected. Add a lane to define target ownership before timeline point editing arrives.", juce::dontSendNotification);
        suppressCallbacks = false;
        return;
    }

    enabledToggle.setToggleState (lane->enabled, juce::dontSendNotification);
    scopeBox.setText (lane->scope, juce::dontSendNotification);
    targetBox.setText (lane->displayName, juce::dontSendNotification);
    pointsSummary.setText ("Points: " + juce::String ((int) lane->points.size())
                               + "  |  target id: " + lane->targetId
                               + "  |  foundation only for this pass",
                           juce::dontSendNotification);
    suppressCallbacks = false;
}

void AutoPanel::addLane()
{
    const auto beforeState = automation();
    auto& state = automationForEdit();
    AutomationLane lane;
    lane.id = state.nextId++;
    lane.displayName = "Master Gain";
    lane.targetId = "master_gain";
    lane.scope = "Mixer";
    lane.points.push_back ({ 0.0, 0.8f });
    lane.points.push_back ({ 16.0, 0.8f });
    state.lanes.push_back (lane);
    selectedIndex = static_cast<int> (state.lanes.size() - 1);
    commitChanges (beforeState, "Add Automation Lane");
    refreshAll();
}

void AutoPanel::removeLane()
{
    const auto beforeState = automation();
    auto& state = automationForEdit();
    if (selectedIndex < 0 || selectedIndex >= static_cast<int> (state.lanes.size()))
        return;
    state.lanes.erase (state.lanes.begin() + selectedIndex);
    selectedIndex = juce::jmin (selectedIndex, static_cast<int> (state.lanes.size()) - 1);
    commitChanges (beforeState, "Delete Automation Lane");
    refreshAll();
}

void AutoPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);
    auto accent = ZoneAStyle::accentForTabId ("automate");
    ZoneAStyle::drawHeader (g, { 0, 0, getWidth(), 24 }, "AUTOMATION", accent);
    ZoneAStyle::drawCard (g, laneList.getBounds().expanded (6, 28), accent);
    ZoneAStyle::drawCard (g, inspectorHeader.getBounds().getUnion (pointsSummary.getBounds()).expanded (6, 24), accent);
}

void AutoPanel::resized()
{
    auto area = getLocalBounds().reduced (8);
    area.removeFromTop (24);
    laneHeader.setBounds (area.removeFromTop (16));
    auto btnRow = area.removeFromTop (18);
    addLaneButton.setBounds (btnRow.removeFromLeft (54));
    btnRow.removeFromLeft (4);
    removeLaneButton.setBounds (btnRow.removeFromLeft (42));
    area.removeFromTop (4);
    laneList.setBounds (area.removeFromTop (126));
    area.removeFromTop (8);
    inspectorHeader.setBounds (area.removeFromTop (16));
    hintLabel.setBounds (area.removeFromTop (16));
    area.removeFromTop (6);
    enabledToggle.setBounds (area.removeFromTop (18));
    area.removeFromTop (4);
    scopeLabel.setBounds (area.removeFromTop (12));
    scopeBox.setBounds (area.removeFromTop (20));
    area.removeFromTop (4);
    targetLabel.setBounds (area.removeFromTop (12));
    targetBox.setBounds (area.removeFromTop (20));
    area.removeFromTop (6);
    pointsSummary.setBounds (area.removeFromTop (32));
}
void AutoPanel::refreshFromModel()
{
    selectedIndex = juce::jmin (selectedIndex, static_cast<int> (automation().lanes.size()) - 1);
    refreshAll();
    repaint();
}

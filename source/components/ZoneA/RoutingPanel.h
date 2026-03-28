#pragma once

#include "../../Theme.h"
#include "../../state/RoutingState.h"
#include "ZoneAStyle.h"
#include "ZoneAControlStyle.h"

class RoutingPanel : public juce::Component,
                     private juce::ListBoxModel
{
public:
    RoutingPanel();
    ~RoutingPanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setRoutingState (const RoutingState& state);
    const RoutingState& getRoutingState() const noexcept { return routingState; }

    std::function<void (const RoutingState&)> onRoutingStateChanged;
    std::function<void (const std::vector<uint8_t>&)> onMatrixChanged;
    juce::String getSummary() const;
    void setMatrix (const std::vector<uint8_t>& vals);

private:
    enum class Tab
    {
        midi = 0,
        audio,
        fx
    };

    RoutingState routingState;
    Tab activeTab { Tab::midi };
    juce::ListBox routeList;
    juce::TextButton addRouteButton { "+ ROUTE" };
    juce::TextButton deleteRouteButton { "DEL" };
    juce::TextButton toggleRouteButton { "TOGGLE" };
    juce::Label headerLabel;
    juce::Label hintLabel;
    juce::Label sourceLabel;
    juce::Label destinationLabel;
    juce::Label busLabel;
    juce::ComboBox sourceBox;
    juce::ComboBox destinationBox;
    juce::ComboBox busBox;
    juce::TextButton midiTabButton { "MIDI" };
    juce::TextButton audioTabButton { "AUDIO" };
    juce::TextButton fxTabButton { "FX" };
    int selectedIndex { -1 };
    bool suppressCallbacks { false };

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int row) override;

    std::vector<RouteEntry>& activeRoutes();
    const std::vector<RouteEntry>& activeRoutes() const;
    RouteEntry* selectedRoute();
    void refreshAll();
    void refreshInspector();
    void commitChanges();
    void addRoute();
    void deleteRoute();
    void toggleSelectedRoute();
    void rebuildChoiceLists();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoutingPanel)
};

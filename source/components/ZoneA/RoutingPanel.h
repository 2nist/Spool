#pragma once

#include "../../Theme.h"
#include "../../state/RoutingState.h"
#include "ZoneAStyle.h"
#include "ZoneAControlStyle.h"
#include "MidiRoutingGrid.h"
#include "FXSendGrid.h"

class RoutingPanel : public juce::Component,
                     private juce::ListBoxModel,
                     private juce::Timer
{
public:
    RoutingPanel();
    ~RoutingPanel() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

    void setRoutingState (const RoutingState& state);
    const RoutingState& getRoutingState() const noexcept { return routingState; }
    void setMidiRouter (const MidiRouter* router) { midiGrid.setMidiRouter (router); }

    /** Supply dynamic module names ("GroupName:ModuleType" format, indexed by slot).
        Used to build audio route combo options and the FX send matrix row labels. */
    void setModuleNames (const juce::StringArray& names);

    /** Wire audio activity counter from PluginProcessor for route pulse indicators. */
    void setAudioTick (const std::atomic<uint32_t>* tick);

    std::function<void (const RoutingState&)> onRoutingStateChanged;
    std::function<void (const std::vector<uint8_t>&)> onMatrixChanged;

    /** Fired when the user taps a bus column header in the FX send matrix.
        Argument is the stable bus ID: "bus_a", "bus_b", "bus_c", "bus_d", "master".
        Phase 2 (ZoneCComponent) wires this to jump the Zone C context bar. */
    std::function<void (const juce::String&)> onBusSelected;

    juce::String getSummary() const;
    void setMatrix (const std::vector<uint8_t>& vals);

private:
    enum class Tab { midi = 0, audio, fx };

    RoutingState routingState;
    Tab activeTab { Tab::midi };

    // --- Audio tab: List + inspector ---
    juce::ListBox routeList;
    juce::TextButton addRouteButton   { "+ ROUTE" };
    juce::TextButton deleteRouteButton { "DEL" };
    juce::TextButton toggleRouteButton { "TOGGLE" };
    juce::Label headerLabel;
    juce::Label hintLabel;
    juce::Label sourceLabel;
    juce::Label destinationLabel;
    juce::Label busLabel;
    juce::Label levelLabel;
    juce::ComboBox sourceBox;
    juce::ComboBox destinationBox;
    juce::ComboBox busBox;
    juce::Slider levelSlider;

    // --- Tab buttons ---
    juce::TextButton midiTabButton  { "MIDI"  };
    juce::TextButton audioTabButton { "AUDIO" };
    juce::TextButton fxTabButton    { "FX"    };

    // --- MIDI grid (midi tab) ---
    MidiRoutingGrid midiGrid;
    juce::Label gridLegendLabel;

    // --- FX send matrix (fx tab) ---
    FXSendGrid fxSendGrid;
    juce::Label fxLegendLabel;

    // --- Dynamic module names for audio/FX combo lists ---
    juce::StringArray m_moduleNames;

    // --- Audio activity telemetry ---
    const std::atomic<uint32_t>* m_audioTickPtr { nullptr };
    uint32_t m_lastAudioTick { 0 };
    float    m_audioFlash    { 0.0f };
    static constexpr float kFlashDecay = 0.16f;   // per 30 Hz tick ~200 ms fade

    // --- Layout rects computed in resized() and reused in paint() ---
    juce::Rectangle<int> m_flowArea;

    int selectedIndex { -1 };
    bool suppressCallbacks { false };

    // --- ListBoxModel ---
    int  getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int row) override;

    // --- Timer ---
    void timerCallback() override;

    // --- Helpers ---
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
    void syncStateToGrid();
    void syncGridToState();
    void updateTimerState();
    void paintFlowDiagram (juce::Graphics& g, const RouteEntry& route, juce::Rectangle<int> r) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoutingPanel)
};

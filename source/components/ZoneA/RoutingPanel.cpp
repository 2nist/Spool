#include "RoutingPanel.h"

namespace
{
juce::String signalLabel (RouteSignalType type)
{
    switch (type)
    {
        case RouteSignalType::midi: return "MIDI";
        case RouteSignalType::audio: return "AUDIO";
        case RouteSignalType::fx: return "FX";
    }

    return "AUDIO";
}
}

RoutingPanel::RoutingPanel()
{
    routingState = RoutingState::makeDefault();
    auto accent = ZoneAStyle::accentForTabId ("routing");
    for (auto* c : { static_cast<juce::Component*> (&routeList), static_cast<juce::Component*> (&addRouteButton),
                     static_cast<juce::Component*> (&deleteRouteButton), static_cast<juce::Component*> (&toggleRouteButton),
                     static_cast<juce::Component*> (&headerLabel), static_cast<juce::Component*> (&hintLabel),
                     static_cast<juce::Component*> (&sourceLabel), static_cast<juce::Component*> (&destinationLabel),
                     static_cast<juce::Component*> (&busLabel), static_cast<juce::Component*> (&sourceBox),
                     static_cast<juce::Component*> (&destinationBox), static_cast<juce::Component*> (&busBox),
                     static_cast<juce::Component*> (&midiTabButton), static_cast<juce::Component*> (&audioTabButton),
                     static_cast<juce::Component*> (&fxTabButton) })
        addAndMakeVisible (*c);

    headerLabel.setText ("ROUTING AUTHORITY", juce::dontSendNotification);
    hintLabel.setText ("Inspect one signal family at a time. This panel owns the editable route table for the shell.", juce::dontSendNotification);
    sourceLabel.setText ("SOURCE", juce::dontSendNotification);
    destinationLabel.setText ("DEST", juce::dontSendNotification);
    busLabel.setText ("BUS", juce::dontSendNotification);

    for (auto* label : { &headerLabel })
    {
        label->setFont (Theme::Font::micro());
        label->setColour (juce::Label::textColourId, accent);
    }
    for (auto* label : { &hintLabel, &sourceLabel, &destinationLabel, &busLabel })
    {
        label->setFont (Theme::Font::micro());
        label->setColour (juce::Label::textColourId, Theme::Colour::inkGhost);
    }

    routeList.setModel (this);
    routeList.setRowHeight (24);
    routeList.setColour (juce::ListBox::backgroundColourId, Theme::Colour::surface2);
    for (auto* b : { &addRouteButton, &deleteRouteButton, &toggleRouteButton, &midiTabButton, &audioTabButton, &fxTabButton })
        ZoneAControlStyle::styleTextButton (*b, accent);
    for (auto* box : { &sourceBox, &destinationBox, &busBox })
        ZoneAControlStyle::styleComboBox (*box, accent);

    midiTabButton.onClick = [this] { activeTab = Tab::midi; selectedIndex = -1; rebuildChoiceLists(); refreshAll(); };
    audioTabButton.onClick = [this] { activeTab = Tab::audio; selectedIndex = -1; rebuildChoiceLists(); refreshAll(); };
    fxTabButton.onClick = [this] { activeTab = Tab::fx; selectedIndex = -1; rebuildChoiceLists(); refreshAll(); };
    addRouteButton.onClick = [this] { addRoute(); };
    deleteRouteButton.onClick = [this] { deleteRoute(); };
    toggleRouteButton.onClick = [this] { toggleSelectedRoute(); };

    sourceBox.onChange = [this]
    {
        if (suppressCallbacks)
            return;
        if (auto* route = selectedRoute())
        {
            route->sourceId = sourceBox.getText();
            commitChanges();
            refreshAll();
        }
    };
    destinationBox.onChange = [this]
    {
        if (suppressCallbacks)
            return;
        if (auto* route = selectedRoute())
        {
            route->destinationId = destinationBox.getText();
            commitChanges();
            refreshAll();
        }
    };
    busBox.onChange = [this]
    {
        if (suppressCallbacks)
            return;
        if (auto* route = selectedRoute())
        {
            route->busContext = busBox.getText();
            commitChanges();
            refreshAll();
        }
    };

    rebuildChoiceLists();
    refreshAll();
}

void RoutingPanel::setRoutingState (const RoutingState& state)
{
    routingState = state;
    selectedIndex = -1;
    rebuildChoiceLists();
    refreshAll();
}

std::vector<RouteEntry>& RoutingPanel::activeRoutes()
{
    if (activeTab == Tab::midi) return routingState.midiRoutes;
    if (activeTab == Tab::fx) return routingState.fxRoutes;
    return routingState.audioRoutes;
}

const std::vector<RouteEntry>& RoutingPanel::activeRoutes() const
{
    return const_cast<RoutingPanel*> (this)->activeRoutes();
}

RouteEntry* RoutingPanel::selectedRoute()
{
    auto& routes = activeRoutes();
    if (selectedIndex < 0 || selectedIndex >= static_cast<int> (routes.size()))
        return nullptr;
    return &routes[(size_t) selectedIndex];
}

int RoutingPanel::getNumRows()
{
    return static_cast<int> (activeRoutes().size());
}

void RoutingPanel::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    const auto& routes = activeRoutes();
    if (row < 0 || row >= static_cast<int> (routes.size()))
        return;

    const auto& route = routes[(size_t) row];
    const auto accent = ZoneAStyle::accentForTabId ("routing");
    ZoneAStyle::drawRowBackground (g, { 0, 0, width, height }, false, selected, accent);
    g.setFont (Theme::Font::label());
    g.setColour (route.enabled ? Theme::Colour::inkLight : Theme::Colour::inkGhost);
    g.drawText (route.sourceId + " -> " + route.destinationId, 8, 0, width - 90, height, juce::Justification::centredLeft, true);
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText (route.busContext, width - 86, 0, 78, height, juce::Justification::centredRight, false);
}

void RoutingPanel::selectedRowsChanged (int row)
{
    selectedIndex = row;
    refreshInspector();
}

void RoutingPanel::rebuildChoiceLists()
{
    suppressCallbacks = true;
    sourceBox.clear();
    destinationBox.clear();
    busBox.clear();

    if (activeTab == Tab::midi)
    {
        sourceBox.addItem ("Keyboard In", 1);
        sourceBox.addItem ("Sequencer", 2);
        sourceBox.addItem ("Midi FX", 3);
        destinationBox.addItem ("Focused Slot", 1);
        destinationBox.addItem ("Instrument Rack", 2);
        destinationBox.addItem ("Drum Rack", 3);
        busBox.addItem ("Ch 1", 1);
        busBox.addItem ("Ch 2", 2);
        busBox.addItem ("Phrase", 3);
    }
    else if (activeTab == Tab::audio)
    {
        sourceBox.addItem ("Input", 1);
        sourceBox.addItem ("Looper", 2);
        sourceBox.addItem ("Tape History", 3);
        destinationBox.addItem ("Tape History", 1);
        destinationBox.addItem ("Timeline", 2);
        destinationBox.addItem ("Master Out", 3);
        busBox.addItem ("Mono", 1);
        busBox.addItem ("Stereo", 2);
        busBox.addItem ("Post-Fader", 3);
    }
    else
    {
        sourceBox.addItem ("Midi FX", 1);
        sourceBox.addItem ("Audio FX", 2);
        sourceBox.addItem ("Slot FX", 3);
        destinationBox.addItem ("Instruments", 1);
        destinationBox.addItem ("Master Out", 2);
        destinationBox.addItem ("Looper", 3);
        busBox.addItem ("Pre-Instrument", 1);
        busBox.addItem ("Insert", 2);
        busBox.addItem ("Send", 3);
    }

    suppressCallbacks = false;
}

void RoutingPanel::refreshAll()
{
    routeList.updateContent();
    if (selectedIndex >= 0)
        routeList.selectRow (selectedIndex, false, false);
    refreshInspector();
}

void RoutingPanel::refreshInspector()
{
    suppressCallbacks = true;
    const auto* route = selectedRoute();
    const bool has = route != nullptr;
    deleteRouteButton.setEnabled (has);
    toggleRouteButton.setEnabled (has);
    sourceBox.setEnabled (has);
    destinationBox.setEnabled (has);
    busBox.setEnabled (has);
    if (! has)
    {
        sourceBox.setSelectedId (0, juce::dontSendNotification);
        destinationBox.setSelectedId (0, juce::dontSendNotification);
        busBox.setSelectedId (0, juce::dontSendNotification);
        suppressCallbacks = false;
        return;
    }

    sourceBox.setText (route->sourceId, juce::dontSendNotification);
    destinationBox.setText (route->destinationId, juce::dontSendNotification);
    busBox.setText (route->busContext, juce::dontSendNotification);
    toggleRouteButton.setButtonText (route->enabled ? "DISABLE" : "ENABLE");
    suppressCallbacks = false;
}

void RoutingPanel::commitChanges()
{
    if (onRoutingStateChanged)
        onRoutingStateChanged (routingState);

    if (onMatrixChanged)
    {
        std::vector<uint8_t> legacy (8, 0);
        for (int i = 0; i < juce::jmin (8, (int) routingState.audioRoutes.size()); ++i)
            legacy[(size_t) i] = routingState.audioRoutes[(size_t) i].enabled ? 1 : 0;
        onMatrixChanged (legacy);
    }
}

void RoutingPanel::addRoute()
{
    auto& routes = activeRoutes();
    RouteEntry route;
    route.id = routingState.nextId++;
    route.signalType = activeTab == Tab::midi ? RouteSignalType::midi : activeTab == Tab::fx ? RouteSignalType::fx : RouteSignalType::audio;
    route.orderIndex = static_cast<int> (routes.size());
    route.sourceId = sourceBox.getNumItems() > 0 ? sourceBox.getItemText (0) : "Source";
    route.destinationId = destinationBox.getNumItems() > 0 ? destinationBox.getItemText (0) : "Destination";
    route.busContext = busBox.getNumItems() > 0 ? busBox.getItemText (0) : "Main";
    routes.push_back (route);
    selectedIndex = static_cast<int> (routes.size() - 1);
    commitChanges();
    refreshAll();
}

void RoutingPanel::deleteRoute()
{
    auto& routes = activeRoutes();
    if (selectedIndex < 0 || selectedIndex >= static_cast<int> (routes.size()))
        return;
    routes.erase (routes.begin() + selectedIndex);
    selectedIndex = juce::jmin (selectedIndex, static_cast<int> (routes.size()) - 1);
    commitChanges();
    refreshAll();
}

void RoutingPanel::toggleSelectedRoute()
{
    if (auto* route = selectedRoute())
    {
        route->enabled = ! route->enabled;
        commitChanges();
        refreshAll();
    }
}

void RoutingPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);
    const auto accent = ZoneAStyle::accentForTabId ("routing");
    const int headerH = ZoneAStyle::compactHeaderHeight();
    const int cardPad = ZoneAControlStyle::compactGap() + 2;
    ZoneAStyle::drawHeader (g, { 0, 0, getWidth(), headerH }, "ROUTING", accent);
    ZoneAStyle::drawCard (g, routeList.getBounds().expanded (cardPad, headerH), accent);
    ZoneAStyle::drawCard (g, sourceLabel.getBounds().getUnion (busBox.getBounds()).expanded (cardPad, headerH), accent);
}

void RoutingPanel::resized()
{
    const int gap = ZoneAControlStyle::compactGap();
    const int labelH = ZoneAControlStyle::sectionHeaderHeight() - 2;
    const int rowH = ZoneAControlStyle::controlHeight();
    const int headerH = ZoneAStyle::compactHeaderHeight();
    const int groupHeaderH = ZoneAStyle::compactGroupHeaderHeight();
    routeList.setRowHeight (ZoneAStyle::compactRowHeight());

    auto area = getLocalBounds().reduced (gap + 4);
    area.removeFromTop (headerH);
    headerLabel.setBounds (area.removeFromTop (groupHeaderH));
    hintLabel.setBounds (area.removeFromTop (groupHeaderH));
    area.removeFromTop (gap);
    auto tabs = area.removeFromTop (rowH);
    midiTabButton.setBounds (tabs.removeFromLeft (46));
    tabs.removeFromLeft (gap);
    audioTabButton.setBounds (tabs.removeFromLeft (52));
    tabs.removeFromLeft (gap);
    fxTabButton.setBounds (tabs.removeFromLeft (36));
    area.removeFromTop (gap);
    auto row = area.removeFromTop (rowH);
    addRouteButton.setBounds (row.removeFromLeft (58));
    row.removeFromLeft (gap);
    deleteRouteButton.setBounds (row.removeFromLeft (42));
    row.removeFromLeft (gap);
    toggleRouteButton.setBounds (row.removeFromLeft (60));
    area.removeFromTop (gap);
    routeList.setBounds (area.removeFromTop (126));
    area.removeFromTop (gap + 2);
    sourceLabel.setBounds (area.removeFromTop (labelH));
    sourceBox.setBounds (area.removeFromTop (rowH));
    area.removeFromTop (gap);
    destinationLabel.setBounds (area.removeFromTop (labelH));
    destinationBox.setBounds (area.removeFromTop (rowH));
    area.removeFromTop (gap);
    busLabel.setBounds (area.removeFromTop (labelH));
    busBox.setBounds (area.removeFromTop (rowH));
}

juce::String RoutingPanel::getSummary() const
{
    const auto& routes = activeRoutes();
    int enabled = 0;
    for (const auto& route : routes)
        enabled += route.enabled ? 1 : 0;
    return signalLabel (activeTab == Tab::midi ? RouteSignalType::midi : activeTab == Tab::fx ? RouteSignalType::fx : RouteSignalType::audio)
         + " " + juce::String (enabled) + " on";
}

void RoutingPanel::setMatrix (const std::vector<uint8_t>& vals)
{
    auto& routes = routingState.audioRoutes;
    for (int i = 0; i < juce::jmin ((int) routes.size(), (int) vals.size()); ++i)
        routes[(size_t) i].enabled = vals[(size_t) i] != 0;
    refreshAll();
}

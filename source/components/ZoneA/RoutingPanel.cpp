#include "RoutingPanel.h"

namespace
{
juce::String signalLabel (RouteSignalType type)
{
    switch (type)
    {
        case RouteSignalType::midi:  return "MIDI";
        case RouteSignalType::audio: return "AUDIO";
        case RouteSignalType::fx:    return "FX";
    }
    return "AUDIO";
}

// Parse "GroupName:ModuleType" and return a short display name like "SYNTH 01"
juce::StringArray buildSlotNames (const juce::StringArray& moduleNames)
{
    juce::StringArray out;
    for (int i = 0; i < moduleNames.size(); ++i)
    {
        const auto& item = moduleNames[i];
        const int colon = item.indexOfChar (':');
        auto type = (colon >= 0 ? item.substring (colon + 1) : item).trim().toUpperCase();
        if (type == "DRUM MACHINE") type = "DRUMS";
        if (type.isNotEmpty())
            out.add (type + " " + juce::String (i + 1).paddedLeft ('0', 2));
    }
    return out;
}
} // namespace

//==============================================================================
RoutingPanel::RoutingPanel()
{
    routingState = RoutingState::makeDefault();
    const auto accent = ZoneAStyle::accentForTabId ("routing");

    // Tab buttons -- always visible
    for (auto* b : { &midiTabButton, &audioTabButton, &fxTabButton })
    {
        addAndMakeVisible (*b);
        ZoneAControlStyle::styleTextButton (*b, accent);
    }

    // Header / hint labels
    addAndMakeVisible (headerLabel);
    addAndMakeVisible (hintLabel);
    headerLabel.setText ("ROUTING AUTHORITY", juce::dontSendNotification);
    headerLabel.setFont (Theme::Font::micro());
    headerLabel.setColour (juce::Label::textColourId, accent);
    hintLabel.setText ("Inspect one signal family at a time.", juce::dontSendNotification);
    hintLabel.setFont (Theme::Font::micro());
    hintLabel.setColour (juce::Label::textColourId, Theme::Colour::inkGhost);

    // MIDI grid
    addAndMakeVisible (midiGrid);
    gridLegendLabel.setText ("Tap a cell to connect source to destination.", juce::dontSendNotification);
    gridLegendLabel.setFont (Theme::Font::micro());
    gridLegendLabel.setColour (juce::Label::textColourId, Theme::Colour::inkGhost);
    addAndMakeVisible (gridLegendLabel);
    midiGrid.onCellToggled = [this] (int, int, bool) { syncGridToState(); };

    // FX send grid
    addAndMakeVisible (fxSendGrid);
    fxLegendLabel.setText ("Tap to toggle send. Drag cell up/down to set level.", juce::dontSendNotification);
    fxLegendLabel.setFont (Theme::Font::micro());
    fxLegendLabel.setColour (juce::Label::textColourId, Theme::Colour::inkGhost);
    addAndMakeVisible (fxLegendLabel);

    fxSendGrid.onMatrixChanged = [this] (const FXSendMatrix& m)
    {
        routingState.fxSendMatrix = m;
        commitChanges();
    };
    fxSendGrid.onBusSelected = [this] (const juce::String& busId)
    {
        if (onBusSelected) onBusSelected (busId);
    };

    // Audio tab: List + inspector components
    for (auto* c : { static_cast<juce::Component*> (&routeList),
                     static_cast<juce::Component*> (&addRouteButton),
                     static_cast<juce::Component*> (&deleteRouteButton),
                     static_cast<juce::Component*> (&toggleRouteButton),
                     static_cast<juce::Component*> (&sourceLabel),
                     static_cast<juce::Component*> (&destinationLabel),
                     static_cast<juce::Component*> (&busLabel),
                     static_cast<juce::Component*> (&levelLabel),
                     static_cast<juce::Component*> (&sourceBox),
                     static_cast<juce::Component*> (&destinationBox),
                     static_cast<juce::Component*> (&busBox),
                     static_cast<juce::Component*> (&levelSlider) })
        addAndMakeVisible (*c);

    for (auto* label : { &sourceLabel, &destinationLabel, &busLabel, &levelLabel })
    {
        label->setFont (Theme::Font::micro());
        label->setColour (juce::Label::textColourId, Theme::Colour::inkGhost);
    }
    sourceLabel.setText ("SOURCE", juce::dontSendNotification);
    destinationLabel.setText ("DEST", juce::dontSendNotification);
    busLabel.setText ("BUS", juce::dontSendNotification);
    levelLabel.setText ("LEVEL", juce::dontSendNotification);

    routeList.setModel (this);
    routeList.setRowHeight (ZoneAStyle::compactRowHeight());
    routeList.setColour (juce::ListBox::backgroundColourId, Theme::Colour::surface2);

    for (auto* b : { &addRouteButton, &deleteRouteButton, &toggleRouteButton })
        ZoneAControlStyle::styleTextButton (*b, accent);
    for (auto* box : { &sourceBox, &destinationBox, &busBox })
        ZoneAControlStyle::styleComboBox (*box, accent);

    ZoneAControlStyle::initBarSlider (levelSlider, "LEVEL");
    levelSlider.setRange (0.0, 1.0, 0.0);
    levelSlider.setValue (1.0, juce::dontSendNotification);
    ZoneAControlStyle::tintBarSlider (levelSlider, accent);

    // Tab callbacks
    midiTabButton.onClick  = [this] { activeTab = Tab::midi;  selectedIndex = -1; rebuildChoiceLists(); refreshAll(); };
    audioTabButton.onClick = [this] { activeTab = Tab::audio; selectedIndex = -1; rebuildChoiceLists(); refreshAll(); };
    fxTabButton.onClick    = [this] { activeTab = Tab::fx;    selectedIndex = -1; rebuildChoiceLists(); refreshAll(); };

    addRouteButton.onClick    = [this] { addRoute(); };
    deleteRouteButton.onClick = [this] { deleteRoute(); };
    toggleRouteButton.onClick = [this] { toggleSelectedRoute(); };

    sourceBox.onChange = [this]
    {
        if (suppressCallbacks) return;
        if (auto* route = selectedRoute())
        { route->sourceId = sourceBox.getText(); commitChanges(); refreshAll(); }
    };
    destinationBox.onChange = [this]
    {
        if (suppressCallbacks) return;
        if (auto* route = selectedRoute())
        { route->destinationId = destinationBox.getText(); commitChanges(); refreshAll(); }
    };
    busBox.onChange = [this]
    {
        if (suppressCallbacks) return;
        if (auto* route = selectedRoute())
        { route->busContext = busBox.getText(); commitChanges(); refreshAll(); }
    };
    levelSlider.onValueChange = [this]
    {
        if (suppressCallbacks) return;
        if (auto* route = selectedRoute())
        { route->level = (float) levelSlider.getValue(); commitChanges(); }
    };

    rebuildChoiceLists();
    refreshAll();
}

//==============================================================================
void RoutingPanel::setRoutingState (const RoutingState& state)
{
    routingState = state;
    selectedIndex = -1;
    fxSendGrid.setMatrix (routingState.fxSendMatrix);
    rebuildChoiceLists();
    refreshAll();
}

void RoutingPanel::setModuleNames (const juce::StringArray& names)
{
    m_moduleNames = names;
    const auto slotNames = buildSlotNames (names);
    fxSendGrid.setSlotNames (slotNames);
    if (activeTab != Tab::midi)
    {
        rebuildChoiceLists();
        refreshInspector();
    }
}

void RoutingPanel::setAudioTick (const std::atomic<uint32_t>* tick)
{
    m_audioTickPtr = tick;
    if (tick)
        m_lastAudioTick = tick->load (std::memory_order_relaxed);
    fxSendGrid.setAudioTick (tick);
    updateTimerState();
}

void RoutingPanel::visibilityChanged()
{
    updateTimerState();
}

void RoutingPanel::updateTimerState()
{
    if (isVisible() && activeTab == Tab::audio && m_audioTickPtr)
        startTimerHz (30);
    else
        stopTimer();
}

void RoutingPanel::timerCallback()
{
    const uint32_t tick = m_audioTickPtr->load (std::memory_order_relaxed);
    if (tick != m_lastAudioTick)
    {
        m_lastAudioTick = tick;
        m_audioFlash = 1.0f;
        repaint();
    }
    else if (m_audioFlash > 0.0f)
    {
        m_audioFlash = juce::jmax (0.0f, m_audioFlash - kFlashDecay);
        repaint();
    }
}

//==============================================================================
std::vector<RouteEntry>& RoutingPanel::activeRoutes()
{
    if (activeTab == Tab::midi) return routingState.midiRoutes;
    if (activeTab == Tab::fx)   return routingState.fxRoutes;
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

//==============================================================================
int RoutingPanel::getNumRows()
{
    if (activeTab == Tab::midi || activeTab == Tab::fx) return 0;
    return static_cast<int> (activeRoutes().size());
}

void RoutingPanel::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    const auto& routes = activeRoutes();
    if (row < 0 || row >= static_cast<int> (routes.size())) return;

    const auto& route  = routes[(size_t) row];
    const auto  accent = ZoneAStyle::accentForTabId ("routing");
    ZoneAStyle::drawRowBackground (g, { 0, 0, width, height }, false, selected, accent);

    // Activity dot on the left
    const float flash = route.enabled ? m_audioFlash : 0.0f;
    const int dotR  = 3;
    const int dotCY = height / 2;
    if (flash > 0.0f)
        g.setColour (accent.withAlpha (flash));
    else
        g.setColour (route.enabled ? accent.withAlpha (0.35f) : juce::Colour (Theme::Colour::inkGhost).withAlpha (0.2f));
    g.fillEllipse (5.0f, (float) (dotCY - dotR), (float) (dotR * 2), (float) (dotR * 2));

    // Route text
    g.setFont (Theme::Font::label());
    g.setColour (route.enabled ? Theme::Colour::inkLight : Theme::Colour::inkGhost);
    const int textLeft = 5 + dotR * 2 + 4;
    g.drawText (route.sourceId + " ->" + route.destinationId,
                textLeft, 0, width - textLeft - 60, height,
                juce::Justification::centredLeft, true);

    // Level + bus on the right
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    const juce::String detail = "L:" + juce::String (route.level, 2) + "  " + route.busContext;
    g.drawText (detail, width - 58, 0, 56, height, juce::Justification::centredRight, false);
}

void RoutingPanel::selectedRowsChanged (int row)
{
    selectedIndex = row;
    refreshInspector();
}

//==============================================================================
void RoutingPanel::syncStateToGrid()
{
    midiGrid.clearAll();
    for (const auto& route : routingState.midiRoutes)
    {
        if (!route.enabled) continue;
        const int r = MidiRoutingGrid::sourceIndex (route.sourceId);
        const int c = MidiRoutingGrid::destIndex (route.destinationId);
        if (r >= 0 && c >= 0)
            midiGrid.setCell (r, c, true);
    }
}

void RoutingPanel::syncGridToState()
{
    routingState.midiRoutes.clear();
    for (int r = 0; r < MidiRoutingGrid::kSources; ++r)
    {
        for (int c = 0; c < MidiRoutingGrid::kDests; ++c)
        {
            if (midiGrid.getCell (r, c))
            {
                RouteEntry entry;
                entry.id            = routingState.nextId++;
                entry.signalType    = RouteSignalType::midi;
                entry.sourceId      = MidiRoutingGrid::sourceName (r);
                entry.destinationId = MidiRoutingGrid::destName (c);
                entry.busContext    = "Ch " + juce::String (c + 1);
                entry.orderIndex    = r * MidiRoutingGrid::kDests + c;
                entry.enabled       = true;
                entry.level         = 1.0f;
                routingState.midiRoutes.push_back (entry);
            }
        }
    }
    commitChanges();
}

//==============================================================================
void RoutingPanel::rebuildChoiceLists()
{
    suppressCallbacks = true;
    sourceBox.clear();
    destinationBox.clear();
    busBox.clear();

    if (activeTab == Tab::midi || activeTab == Tab::fx)
    {
        suppressCallbacks = false;
        return;
    }

    const auto slotNames = buildSlotNames (m_moduleNames);

    // Audio tab
    int id = 1;
    for (auto& s : { "Input", "Looper", "Tape History" }) sourceBox.addItem (s, id++);
    for (const auto& n : slotNames)                        sourceBox.addItem (n, id++);

    id = 1;
    for (auto& s : { "Tape History", "Timeline", "Master Out" }) destinationBox.addItem (s, id++);
    for (const auto& n : slotNames)                              destinationBox.addItem (n, id++);

    id = 1;
    for (auto& s : { "Mono", "Stereo", "Post-Fader", "Pre-Fader" }) busBox.addItem (s, id++);

    suppressCallbacks = false;
}

void RoutingPanel::refreshAll()
{
    const bool isMidi = (activeTab == Tab::midi);
    const bool isFX   = (activeTab == Tab::fx);
    const bool isAudio = (activeTab == Tab::audio);

    midiGrid.setVisible (isMidi);
    gridLegendLabel.setVisible (isMidi);

    fxSendGrid.setVisible (isFX);
    fxLegendLabel.setVisible (isFX);

    routeList.setVisible (isAudio);
    addRouteButton.setVisible (isAudio);
    deleteRouteButton.setVisible (isAudio);
    toggleRouteButton.setVisible (isAudio);
    sourceLabel.setVisible (isAudio);
    destinationLabel.setVisible (isAudio);
    busLabel.setVisible (isAudio);
    levelLabel.setVisible (isAudio);
    sourceBox.setVisible (isAudio);
    destinationBox.setVisible (isAudio);
    busBox.setVisible (isAudio);
    levelSlider.setVisible (isAudio);

    if (isMidi)
    {
        syncStateToGrid();
    }
    else if (isAudio)
    {
        routeList.updateContent();
        if (selectedIndex >= 0)
            routeList.selectRow (selectedIndex, false, false);
        refreshInspector();
    }
    else // FX
    {
        fxSendGrid.setMatrix (routingState.fxSendMatrix);
    }

    updateTimerState();
    resized();
    repaint();
}

void RoutingPanel::refreshInspector()
{
    suppressCallbacks = true;
    const auto* route = selectedRoute();
    const bool  has   = route != nullptr;

    deleteRouteButton.setEnabled (has);
    toggleRouteButton.setEnabled (has);
    sourceBox.setEnabled (has);
    destinationBox.setEnabled (has);
    busBox.setEnabled (has);
    levelSlider.setEnabled (has);

    if (!has)
    {
        sourceBox.setSelectedId (0, juce::dontSendNotification);
        destinationBox.setSelectedId (0, juce::dontSendNotification);
        busBox.setSelectedId (0, juce::dontSendNotification);
        levelSlider.setValue (1.0, juce::dontSendNotification);
        suppressCallbacks = false;
        return;
    }

    sourceBox.setText (route->sourceId,      juce::dontSendNotification);
    destinationBox.setText (route->destinationId, juce::dontSendNotification);
    busBox.setText (route->busContext,        juce::dontSendNotification);
    levelSlider.setValue (static_cast<double> (route->level), juce::dontSendNotification);
    toggleRouteButton.setButtonText (route->enabled ? "DISABLE" : "ENABLE");
    suppressCallbacks = false;
}

//==============================================================================
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
    route.id          = routingState.nextId++;
    route.signalType  = RouteSignalType::audio;
    route.orderIndex  = static_cast<int> (routes.size());
    route.sourceId    = sourceBox.getNumItems() > 0 ? sourceBox.getItemText (0) : "Source";
    route.destinationId = destinationBox.getNumItems() > 0 ? destinationBox.getItemText (0) : "Destination";
    route.busContext  = busBox.getNumItems() > 0 ? busBox.getItemText (0) : "Main";
    route.level       = 1.0f;
    routes.push_back (route);
    selectedIndex = static_cast<int> (routes.size() - 1);
    commitChanges();
    refreshAll();
}

void RoutingPanel::deleteRoute()
{
    auto& routes = activeRoutes();
    if (selectedIndex < 0 || selectedIndex >= static_cast<int> (routes.size())) return;
    routes.erase (routes.begin() + selectedIndex);
    selectedIndex = juce::jmin (selectedIndex, static_cast<int> (routes.size()) - 1);
    commitChanges();
    refreshAll();
}

void RoutingPanel::toggleSelectedRoute()
{
    if (auto* route = selectedRoute())
    {
        route->enabled = !route->enabled;
        commitChanges();
        refreshAll();
    }
}

//==============================================================================
void RoutingPanel::paintFlowDiagram (juce::Graphics& g,
                                     const RouteEntry& route,
                                     juce::Rectangle<int> r) const
{
    const auto& theme  = ThemeManager::get().theme();
    const auto  accent = ZoneAStyle::accentForTabId ("routing");
    const auto  font   = Theme::Font::micro();

    // Card background
    g.setColour (theme.controlBg.withAlpha (0.35f));
    g.fillRoundedRectangle (r.toFloat(), 3.0f);

    const int pad = 5;

    // Enabled dot (left)
    const int dotR  = 3;
    const int dotCY = r.getCentreY();
    const int dotCX = r.getX() + pad + dotR;
    auto dotCol = route.enabled ? accent : juce::Colour (Theme::Colour::inkGhost).withAlpha (0.4f);
    if (m_audioFlash > 0.0f && route.enabled)
        dotCol = dotCol.brighter (m_audioFlash * 0.7f);
    g.setColour (dotCol);
    g.fillEllipse ((float) (dotCX - dotR), (float) (dotCY - dotR),
                   (float) (dotR * 2),     (float) (dotR * 2));

    // Flow text: source -> dest
    g.setFont (font);
    const int textX = dotCX + dotR + 4;
    const int levelW = 36;
    const int textW  = r.getRight() - textX - levelW - pad;
    const juce::String flowText = route.sourceId + " ->" + route.destinationId;
    g.setColour (route.enabled ? juce::Colour (Theme::Colour::inkLight)
                               : juce::Colour (Theme::Colour::inkGhost));
    g.drawText (flowText, textX, r.getY(), textW, r.getHeight(),
                juce::Justification::centredLeft, true);

    // Level badge (right)
    const juce::String lvlText = "L:" + juce::String (route.level, 2);
    g.setColour (juce::Colour (Theme::Colour::inkGhost));
    g.drawText (lvlText, r.getRight() - levelW - pad, r.getY(), levelW, r.getHeight(),
                juce::Justification::centredRight, false);
}

void RoutingPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);
    const auto accent  = ZoneAStyle::accentForTabId ("routing");
    const int  headerH = ZoneAStyle::compactHeaderHeight();
    const int  cardPad = ZoneAControlStyle::compactGap() + 2;
    ZoneAStyle::drawHeader (g, { 0, 0, getWidth(), headerH }, "ROUTING", accent);

    if (activeTab == Tab::midi)
    {
        ZoneAStyle::drawCard (g, midiGrid.getBounds().expanded (cardPad, cardPad), accent);
    }
    else if (activeTab == Tab::fx)
    {
        ZoneAStyle::drawCard (g, fxSendGrid.getBounds().expanded (cardPad, cardPad), accent);
    }
    else
    {
        ZoneAStyle::drawCard (g, routeList.getBounds().expanded (cardPad, cardPad), accent);
        ZoneAStyle::drawCard (g, sourceLabel.getBounds().getUnion (levelSlider.getBounds()).expanded (cardPad, cardPad), accent);

        // Flow diagram for selected route
        if (selectedIndex >= 0)
        {
            if (const auto* route = selectedRoute())
                paintFlowDiagram (g, *route, m_flowArea);
        }
        else if (!m_flowArea.isEmpty())
        {
            const auto& theme = ThemeManager::get().theme();
            g.setColour (theme.controlBg.withAlpha (0.2f));
            g.fillRoundedRectangle (m_flowArea.toFloat(), 3.0f);
            g.setFont (Theme::Font::micro());
            g.setColour (Theme::Colour::inkGhost);
            g.drawText ("Select a route to inspect", m_flowArea, juce::Justification::centred, false);
        }
    }
}

void RoutingPanel::resized()
{
    const int gap       = ZoneAControlStyle::compactGap();
    const int rowH      = ZoneAControlStyle::controlHeight();
    const int labelH    = ZoneAControlStyle::sectionHeaderHeight() - 2;
    const int headerH   = ZoneAStyle::compactHeaderHeight();
    const int groupHdrH = ZoneAStyle::compactGroupHeaderHeight();

    auto area = getLocalBounds().reduced (gap + 4);
    area.removeFromTop (headerH);
    headerLabel.setBounds (area.removeFromTop (groupHdrH));
    hintLabel.setBounds (area.removeFromTop (groupHdrH));
    area.removeFromTop (gap);

    // Tab bar
    auto tabs = area.removeFromTop (rowH);
    midiTabButton.setBounds (tabs.removeFromLeft (46));
    tabs.removeFromLeft (gap);
    audioTabButton.setBounds (tabs.removeFromLeft (52));
    tabs.removeFromLeft (gap);
    fxTabButton.setBounds (tabs.removeFromLeft (36));
    area.removeFromTop (gap);

    if (activeTab == Tab::midi)
    {
        gridLegendLabel.setBounds (area.removeFromTop (groupHdrH));
        area.removeFromTop (gap);
        midiGrid.setBounds (area);
    }
    else if (activeTab == Tab::fx)
    {
        fxLegendLabel.setBounds (area.removeFromTop (groupHdrH));
        area.removeFromTop (gap);
        fxSendGrid.setBounds (area);
    }
    else
    {
        // Audio: Action buttons
        auto row = area.removeFromTop (rowH);
        addRouteButton.setBounds (row.removeFromLeft (58));
        row.removeFromLeft (gap);
        deleteRouteButton.setBounds (row.removeFromLeft (42));
        row.removeFromLeft (gap);
        toggleRouteButton.setBounds (row.removeFromLeft (60));
        area.removeFromTop (gap);

        // Route list (compact)
        routeList.setRowHeight (ZoneAStyle::compactRowHeight());
        routeList.setBounds (area.removeFromTop (96));
        area.removeFromTop (gap);

        // Flow diagram
        m_flowArea = area.removeFromTop (24);
        area.removeFromTop (gap);

        // Inspector
        sourceLabel.setBounds (area.removeFromTop (labelH));
        sourceBox.setBounds (area.removeFromTop (rowH));
        area.removeFromTop (gap);
        destinationLabel.setBounds (area.removeFromTop (labelH));
        destinationBox.setBounds (area.removeFromTop (rowH));
        area.removeFromTop (gap);
        busLabel.setBounds (area.removeFromTop (labelH));
        busBox.setBounds (area.removeFromTop (rowH));
        area.removeFromTop (gap);
        levelLabel.setBounds (area.removeFromTop (labelH));
        levelSlider.setBounds (area.removeFromTop (rowH));
    }
}

//==============================================================================
juce::String RoutingPanel::getSummary() const
{
    if (activeTab == Tab::midi)
    {
        int count = 0;
        for (const auto& r : routingState.midiRoutes) count += r.enabled ? 1 : 0;
        return "MIDI " + juce::String (count) + " on";
    }
    if (activeTab == Tab::fx)
    {
        int count = 0;
        for (int s = 0; s < kFXSlots; ++s)
            for (int t = 0; t < kFXTargets; ++t)
                if (routingState.fxSendMatrix.enabled[s][t]) ++count;
        return "FX " + juce::String (count) + " sends";
    }
    const auto& routes = activeRoutes();
    int enabled = 0;
    for (const auto& route : routes) enabled += route.enabled ? 1 : 0;
    return signalLabel (RouteSignalType::audio) + " " + juce::String (enabled) + " on";
}

void RoutingPanel::setMatrix (const std::vector<uint8_t>& vals)
{
    auto& routes = routingState.audioRoutes;
    for (int i = 0; i < juce::jmin ((int) routes.size(), (int) vals.size()); ++i)
        routes[(size_t) i].enabled = vals[(size_t) i] != 0;
    refreshAll();
}

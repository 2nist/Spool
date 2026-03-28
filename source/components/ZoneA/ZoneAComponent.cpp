#include "ZoneAComponent.h"
#include "../../PluginProcessor.h"
#include "ZoneAStyle.h"

//==============================================================================
ZoneAComponent::ZoneAComponent (PluginProcessor& p) : processorRef (p)
{
}

//==============================================================================
// Panel switching
//==============================================================================

void ZoneAComponent::switchToModulePanel (const juce::String& moduleType, int slotIndex)
{
    if (moduleType == "REEL")
    {
        const juce::String tabId = "reel_" + juce::String (slotIndex);
        ensurePanelCreated (tabId);
        setActivePanel (tabId);
        return;
    }

    // "DRUM MACHINE" (Zone B display string) normalises to "DRUMS" here
    const juce::String normType = (moduleType == "DRUM MACHINE") ? "DRUMS" : moduleType;

    // SYNTH, DRUMS, and other instrument types → INSTRUMENT tab
    if (normType == "SYNTH" || normType == "DRUMS"
        || normType == "SAMPLER" || normType == "VST3")
    {
        ensurePanelCreated ("instrument");
        if (m_instrumentPanel)
            m_instrumentPanel->showSlot (slotIndex);
        setActivePanel ("instrument");
        return;
    }
    // Other types: no auto-switch
}

void ZoneAComponent::setInstrumentSlots (const juce::StringArray& moduleList)
{
    ensurePanelCreated ("instrument");
    if (!m_instrumentPanel) return;

    // Parse "GroupName:ModuleType" format
    juce::Array<InstrumentPanel::SlotEntry> entries;

    int slotIndex = 0;
    for (const auto& item : moduleList)
    {
        InstrumentPanel::SlotEntry entry;
        entry.slotIndex = slotIndex++;

        const int colon = item.indexOfChar (':');
        if (colon >= 0)
        {
            entry.groupName  = item.substring (0, colon).trim();
            const auto raw   = item.substring (colon + 1).trim();
            // "DRUM MACHINE" (Zone B display) → canonical "DRUMS" for Zone A
            entry.moduleType = (raw == "DRUM MACHINE") ? "DRUMS" : raw;
        }
        else
        {
            entry.groupName  = "DEFAULT";
            const auto raw   = item.trim();
            entry.moduleType = (raw == "DRUM MACHINE") ? "DRUMS" : raw;
        }

        entry.groupColor = ZoneAStyle::accentForGroupName (entry.groupName);

        // Build display name
        if (entry.moduleType.isNotEmpty())
            entry.displayName = entry.moduleType + " "
                                + juce::String (entry.slotIndex + 1).paddedLeft ('0', 2);
        else
            entry.displayName = "EMPTY " + juce::String (entry.slotIndex + 1).paddedLeft ('0', 2);

        entries.add (entry);
    }

    m_instrumentPanel->setSlots (entries);
}

void ZoneAComponent::setMixerSlots (const juce::StringArray& moduleList)
{
    ensurePanelCreated ("mixer");
    if (!m_mixerPanel) return;

    juce::Array<MixerPanel::SlotInfo> entries;
    int slotIndex = 0;

    for (const auto& item : moduleList)
    {
        MixerPanel::SlotInfo entry;
        entry.slotIndex = slotIndex++;

        const int colon = item.indexOfChar (':');
        if (colon >= 0)
        {
            entry.groupName  = item.substring (0, colon).trim();
            const auto type  = item.substring (colon + 1).trim();
            entry.isEmpty    = type.isEmpty();
            entry.name = type.isEmpty()
                ? "EMPTY " + juce::String (entry.slotIndex + 1).paddedLeft ('0', 2)
                : type + " " + juce::String (entry.slotIndex + 1).paddedLeft ('0', 2);
        }
        else
        {
            entry.groupName = "DEFAULT";
            entry.isEmpty   = item.trim().isEmpty();
            entry.name = item.trim().isEmpty()
                ? "EMPTY " + juce::String (entry.slotIndex + 1).paddedLeft ('0', 2)
                : item.trim();
        }

        entry.groupColor = ZoneAStyle::accentForGroupName (entry.groupName);
        entry.level      = processorRef.getSlotLevel (entry.slotIndex);
        entry.muted      = processorRef.isSlotMuted  (entry.slotIndex);
        entry.soloed     = processorRef.isSlotSoloed (entry.slotIndex);

        entries.add (entry);
    }

    m_mixerPanel->setSlots (entries);
}

void ZoneAComponent::setTrackLanes (const juce::StringArray& moduleList)
{
    ensurePanelCreated ("tracks");
    if (!m_tracksPanel) return;

    juce::Array<TracksPanel::LaneInfo> lanes;
    int slotIndex = 0;

    for (const auto& item : moduleList)
    {
        TracksPanel::LaneInfo lane;

        const int colon = item.indexOfChar (':');
        juce::String groupName, type;
        if (colon >= 0)
        {
            groupName = item.substring (0, colon).trim();
            type      = item.substring (colon + 1).trim();
        }
        else
        {
            groupName = "DEFAULT";
            type      = item.trim();
        }

        lane.name   = type.isEmpty()
            ? "EMPTY " + juce::String (slotIndex + 1).paddedLeft ('0', 2)
            : type + " " + juce::String (slotIndex + 1).paddedLeft ('0', 2);
        lane.muted  = processorRef.isSlotMuted  (slotIndex);
        lane.soloed = processorRef.isSlotSoloed (slotIndex);
        lane.armed  = false;

        lane.colour = ZoneAStyle::accentForGroupName (groupName);

        lanes.add (lane);
        ++slotIndex;
    }

    m_tracksPanel->setLanes (lanes);
}

ReelEditorPanel* ZoneAComponent::getReelPanel (int slotIndex) noexcept
{
    if (slotIndex < 0 || slotIndex >= 8)
        return nullptr;
    return m_reelPanels[slotIndex].get();
}

void ZoneAComponent::ensurePanelCreated (const juce::String& tabId)
{
    // REEL per-slot panels
    if (tabId.startsWith ("reel_"))
    {
        const int slot = tabId.getTrailingIntValue();
        if (slot >= 0 && slot < 8 && m_reelPanels[slot] == nullptr)
        {
            m_reelPanels[slot] = std::make_unique<ReelEditorPanel> (slot);
            addChildComponent (*m_reelPanels[slot]);
        }
        return;
    }

    if (tabId == "theme" && m_themePanel == nullptr)
    {
        m_themePanel = std::make_unique<ThemeEditorPanel>();
        addChildComponent (*m_themePanel);
    }
    else if (tabId == "routing" && m_routingPanel == nullptr)
    {
        m_routingPanel = std::make_unique<RoutingPanel>();
        if (m_onRoutingChanged)
        {
            m_routingPanel->onRoutingStateChanged = [this] (const RoutingState& state) { m_onRoutingChanged (state); };
        }
        addChildComponent (*m_routingPanel);
    }
    else if (tabId == "tracks" && m_tracksPanel == nullptr)
    {
        m_tracksPanel = std::make_unique<TracksPanel>();
        addChildComponent (*m_tracksPanel);
    }
    else if (tabId == "feed" && m_feedPanel == nullptr)
    {
        m_feedPanel = std::make_unique<FeedSettingsPanel>();
        addChildComponent (*m_feedPanel);
    }
    else if (tabId == "song" && m_songPanel == nullptr)
    {
        m_songPanel = std::make_unique<SongInfoPanel>();
        addChildComponent (*m_songPanel);
    }
    else if (tabId == "transport" && m_transportPanel == nullptr)
    {
        m_transportPanel = std::make_unique<TransportSettingsPanel>();
        addChildComponent (*m_transportPanel);
    }
    else if (tabId == "patch" && m_patchPanel == nullptr)
    {
        m_patchPanel = std::make_unique<PatchBayPanel>();
        addChildComponent (*m_patchPanel);
    }
    // New panels
    else if (tabId == "instrument" && m_instrumentPanel == nullptr)
    {
        m_instrumentPanel = std::make_unique<InstrumentPanel> (processorRef);
        m_instrumentPanel->onSlotSelected = [this] (int slotIdx, const juce::String& type)
        {
            if (onInstrumentSlotSelected)
                onInstrumentSlotSelected (slotIdx, type);
        };
        addChildComponent (*m_instrumentPanel);
    }
    else if (tabId == "mixer" && m_mixerPanel == nullptr)
    {
        m_mixerPanel = std::make_unique<MixerPanel> (processorRef);
        m_mixerPanel->onLevelChanged = [this] (int slot, float level)
        {
            processorRef.setSlotLevel (slot, level);
        };
        m_mixerPanel->onMuteChanged = [this] (int slot, bool muted)
        {
            processorRef.setSlotMuted (slot, muted);
        };
        m_mixerPanel->onSoloChanged = [this] (int slot, bool soloed)
        {
            processorRef.setSlotSoloed (slot, soloed);
        };
        addChildComponent (*m_mixerPanel);
    }
    else if (tabId == "structure" && m_structurePanel == nullptr)
    {
        m_structurePanel = std::make_unique<StructurePanel>(processorRef);
        addChildComponent (*m_structurePanel);
    }
    else if (tabId == "lyrics" && m_lyricsPanel == nullptr)
    {
        m_lyricsPanel = std::make_unique<LyricsPanel> (processorRef);
        addChildComponent (*m_lyricsPanel);
    }
    else if (tabId == "macro" && m_macroPanel == nullptr)
    {
        m_macroPanel = std::make_unique<MacroPanel>();
        addChildComponent (*m_macroPanel);
    }
    else if (tabId == "tape" && m_tapePanel == nullptr)
    {
        m_tapePanel = std::make_unique<TapePanel>();
        addChildComponent (*m_tapePanel);
    }
    else if (tabId == "automate" && m_autoPanel == nullptr)
    {
        m_autoPanel = std::make_unique<AutoPanel> (processorRef);
        addChildComponent (*m_autoPanel);
    }
}

void ZoneAComponent::hideAllPanels()
{
    if (m_patchPanel)      m_patchPanel->setVisible (false);
    if (m_themePanel)      m_themePanel->setVisible (false);
    if (m_routingPanel)    m_routingPanel->setVisible (false);
    if (m_tracksPanel)     m_tracksPanel->setVisible (false);
    if (m_feedPanel)       m_feedPanel->setVisible (false);
    if (m_songPanel)       m_songPanel->setVisible (false);
    if (m_transportPanel)  m_transportPanel->setVisible (false);
    if (m_instrumentPanel) m_instrumentPanel->setVisible (false);
    if (m_mixerPanel)      m_mixerPanel->setVisible (false);
    if (m_structurePanel)  m_structurePanel->setVisible (false);
    if (m_lyricsPanel)     m_lyricsPanel->setVisible (false);
    if (m_macroPanel)      m_macroPanel->setVisible (false);
    if (m_tapePanel)       m_tapePanel->setVisible (false);
    if (m_autoPanel)       m_autoPanel->setVisible (false);
    for (auto& p : m_reelPanels)
        if (p) p->setVisible (false);
}

juce::Component* ZoneAComponent::activePanel() const noexcept
{
    if (m_activePanelId == "patch"      && m_patchPanel)      return m_patchPanel.get();
    if (m_activePanelId == "theme"      && m_themePanel)      return m_themePanel.get();
    if (m_activePanelId == "routing"    && m_routingPanel)    return m_routingPanel.get();
    if (m_activePanelId == "tracks"     && m_tracksPanel)     return m_tracksPanel.get();
    if (m_activePanelId == "feed"       && m_feedPanel)       return m_feedPanel.get();
    if (m_activePanelId == "song"       && m_songPanel)       return m_songPanel.get();
    if (m_activePanelId == "transport"  && m_transportPanel)  return m_transportPanel.get();
    if (m_activePanelId == "instrument" && m_instrumentPanel) return m_instrumentPanel.get();
    if (m_activePanelId == "mixer"      && m_mixerPanel)      return m_mixerPanel.get();
    if (m_activePanelId == "structure"  && m_structurePanel)  return m_structurePanel.get();
    if (m_activePanelId == "lyrics"     && m_lyricsPanel)     return m_lyricsPanel.get();
    if (m_activePanelId == "macro"      && m_macroPanel)      return m_macroPanel.get();
    if (m_activePanelId == "tape"       && m_tapePanel)       return m_tapePanel.get();
    if (m_activePanelId == "automate"   && m_autoPanel)       return m_autoPanel.get();

    if (m_activePanelId.startsWith ("reel_"))
    {
        const int slot = m_activePanelId.getTrailingIntValue();
        if (slot >= 0 && slot < 8 && m_reelPanels[slot])
            return m_reelPanels[slot].get();
    }

    return nullptr;
}

void ZoneAComponent::setActivePanel (const juce::String& tabId)
{
    m_activePanelId = tabId;

    if (tabId.isNotEmpty())
        ensurePanelCreated (tabId);

    hideAllPanels();

    if (auto* p = activePanel())
        p->setVisible (true);

    resized();
    repaint();
}

void ZoneAComponent::setOnRoutingChanged (std::function<void (const RoutingState&)> cb)
{
    m_onRoutingChanged = std::move (cb);
    if (m_routingPanel && m_onRoutingChanged)
        m_routingPanel->onRoutingStateChanged = [this] (const RoutingState& state) { m_onRoutingChanged (state); };
}

void ZoneAComponent::setRoutingState (const RoutingState& state)
{
    ensurePanelCreated ("routing");
    if (m_routingPanel)
        m_routingPanel->setRoutingState (state);
}

void ZoneAComponent::setPatchModuleNames (const juce::StringArray& names)
{
    ensurePanelCreated ("patch");
    if (m_patchPanel)
        m_patchPanel->setModuleNames (names);
}

//==============================================================================
// Layout helpers
//==============================================================================

juce::Rectangle<int> ZoneAComponent::panelArea() const noexcept
{
    return getLocalBounds();
}

juce::Rectangle<int> ZoneAComponent::outputArea() const noexcept
{
    return {};
}

//==============================================================================
// Paint & resize
//==============================================================================

void ZoneAComponent::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);

    // Right-edge border
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine (static_cast<float> (getWidth() - 1), 0.0f,
                static_cast<float> (getWidth() - 1), static_cast<float> (getHeight()),
                Theme::Stroke::subtle);
}

void ZoneAComponent::resized()
{
    if (auto* p = activePanel())
        p->setBounds (panelArea());
}

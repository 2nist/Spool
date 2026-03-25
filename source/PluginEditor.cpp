#include "PluginEditor.h"
#include "instruments/drum/DrumVoiceParams.h"
#include "instruments/drum/DrumKitPatch.h"

//==============================================================================
// Helpers
namespace
{
TransportStrip::StructureFollowState computeFollowState (bool followEnabled, bool hasStructure)
{
    if (! followEnabled)
        return TransportStrip::StructureFollowState::legacy;
    return hasStructure ? TransportStrip::StructureFollowState::active
                        : TransportStrip::StructureFollowState::ready;
}
}

static DrumVoiceParams drumParamsForNote (int note)
{
    switch (note)
    {
        case 36: return DrumVoiceParams::kick();
        case 38: case 40: return DrumVoiceParams::snare();
        case 42: return DrumVoiceParams::closedHat();
        case 46: return DrumVoiceParams::openHat();
        case 37: case 39: return DrumVoiceParams::clap();
        case 45: case 47: case 48: case 50: return DrumVoiceParams::tom();
        case 51: case 59: return DrumVoiceParams::cymbal();
        default: return DrumVoiceParams::kick();
    }
}

static uint64_t slotPatternToBits (const SlotPattern& p)
{
    uint64_t bits = 0;
    for (int s = 0; s < p.activeStepCount(); ++s)
        if (p.stepActive (s))
            bits |= (1ULL << s);
    return bits;
}

static void syncDrumDataToProcessor (int slotIdx,
                                     DrumMachineData* data,
                                     PluginProcessor& proc)
{
    if (data == nullptr) return;

    // Build and load kit into DSP voice engine
    DrumKitPatch kit;
    kit.name = "UI Kit";
    for (auto* v : data->voices)
    {
        DrumKitEntry entry;
        entry.name   = v->name.toStdString();
        entry.params = drumParamsForNote (v->midiNote);
        entry.params.midiNote = v->midiNote;
        kit.voices.push_back (entry);
    }
    proc.getAudioGraph().getDrumMachine (slotIdx).loadKit (kit);

    // Sync sequencer step data
    const int numVoices = data->voices.size();
    proc.setDrumVoiceCount (slotIdx, numVoices);
    for (int vi = 0; vi < numVoices; ++vi)
    {
        const auto* v = data->voices[vi];
        proc.setDrumVoiceMidiNote  (slotIdx, vi, v->midiNote);
        proc.setDrumVoiceStepBits  (slotIdx, vi, slotPatternToBits (v->pattern));
        proc.setDrumVoiceStepCount (slotIdx, vi, v->pattern.activeStepCount());
    }
}

//==============================================================================

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), zoneA (p)
{
    juce::ignoreUnused (processorRef);
    addAndMakeVisible (menuBar);
    addAndMakeVisible (songHeader);
    addAndMakeVisible (systemFeed);
    addAndMakeVisible (m_tabStrip);
    addAndMakeVisible (zoneA);
    

    addAndMakeVisible (zoneB);
    addAndMakeVisible (zoneC);
    addAndMakeVisible (zoneD);
    addAndMakeVisible (historyStrip);

    // Wire Zone A's output strip to processor master controls
    zoneA.setOnLevelChanged ([this] (float v) { processorRef.setMasterGain (v); });
    zoneA.setOnPanChanged   ([this] (float p) { processorRef.setMasterPan  (p); });

    // Wire routing changes from Zone A to the processor, and push initial state
    zoneA.setOnRoutingChanged ([this] (const std::array<uint8_t,8>& m)
    {
        processorRef.setRoutingMatrix (m);
    });
    zoneA.setRoutingMatrix (processorRef.getRoutingMatrix());

    // Wire Zone B module list changes to patch bay, instrument browser, and mixer
    zoneB.onModuleListChanged = [this] (const juce::StringArray& names)
    {
        m_lastModuleNames = names;
        zoneA.setPatchModuleNames   (names);
        zoneA.setInstrumentSlots    (names);
        zoneA.setMixerSlots         (names);
        zoneA.setTrackLanes         (names);
    };

    // Wire tab strip → Zone A panel switching
    m_tabStrip.onTabSelected = [this] (const juce::String& tabId)
    {
        if (tabId.isEmpty())
        {
            m_contentPanelOpen = false;
            zoneA.setActivePanel ({});
        }
        else
        {
            m_contentPanelOpen = true;
            zoneA.setActivePanel (tabId);
        }
        resized();
    };

    // Wire BPM drag in song header to the processor
    songHeader.onBpmChanged = [this] (float bpm)
    {
        auto& appState = processorRef.getAppState();
        appState.transport.bpm = bpm;
        appState.song.bpm = static_cast<int> (bpm);
        processorRef.syncTransportFromAppState();
        if (auto* tp = zoneA.getTracksPanel())
            tp->setBpm (bpm);
        if (auto* sp = zoneA.getSongPanel())
            sp->setBpm (bpm);
    };

    // Wire TracksPanel transport callbacks to processor and Zone D
    // (Panel is lazily created; wiring happens when TracksPanel is first opened)
    // We wire via a lambda that checks for the panel each time
    auto wireTracksPanel = [this]
    {
        if (auto* tp = zoneA.getTracksPanel())
        {
            tp->onPlay   = [this]
            {
                auto& appState = processorRef.getAppState();
                appState.transport.isPlaying = true;
                processorRef.syncTransportFromAppState();
                zoneD.setPlaying (true);
            };
            tp->onStop   = [this]
            {
                auto& appState = processorRef.getAppState();
                appState.transport.isPlaying = false;
                processorRef.syncTransportFromAppState();
                processorRef.seekTransport (0.0);
                zoneD.setPlaying (false);
                zoneD.setStructureBeat (0.0);
            };
            tp->onRecord = [this] { zoneD.setRecording (true);  };
            tp->onBpmChanged = [this] (float bpm)
            {
                auto& appState = processorRef.getAppState();
                appState.transport.bpm = bpm;
                appState.song.bpm = static_cast<int> (bpm);
                processorRef.syncTransportFromAppState();
                songHeader.setBpm   (bpm);
            };

            tp->onMuteChanged = [this] (int lane, bool muted)
            {
                processorRef.setSlotMuted (lane, muted);
            };
            tp->onSoloChanged = [this] (int lane, bool soloed)
            {
                processorRef.setSlotSoloed (lane, soloed);
            };
            tp->onArmChanged = [this] (int lane, bool armed)
            {
                juce::ignoreUnused (lane, armed);
            };
        }
    };

    // Wire SongInfoPanel BPM callback
    auto wireSongPanel = [this]
    {
        if (auto* sp = zoneA.getSongPanel())
        {
            sp->onBpmChanged = [this] (float bpm)
            {
                auto& appState = processorRef.getAppState();
                appState.transport.bpm = bpm;
                appState.song.bpm = static_cast<int> (bpm);
                processorRef.syncTransportFromAppState();
                songHeader.setBpm   (bpm);
            };

            sp->onKeyChanged = [this] (const juce::String& key)
            {
                auto& appState = processorRef.getAppState();
                appState.song.keyRoot = key.trim();
                if (auto* structure = zoneA.getStructurePanel())
                    structure->setIntentKeyMode (appState.song.keyRoot, appState.song.keyScale);
            };

            sp->onScaleChanged = [this] (const juce::String& scale)
            {
                auto& appState = processorRef.getAppState();
                appState.song.keyScale = scale.trim();
                if (auto* structure = zoneA.getStructurePanel())
                    structure->setIntentKeyMode (appState.song.keyRoot, appState.song.keyScale);
            };
        }
    };

    // Wire TransportSettingsPanel Link callback
    auto wireTransportPanel = [this]
    {
        if (auto* tsp = zoneA.getTransportPanel())
        {
            juce::ignoreUnused (tsp);
            // Link/metronome callbacks stubbed — wire to audio engine in DSP session
        }
    };

    auto wireStructurePanel = [this]
    {
        if (auto* sp = zoneA.getStructurePanel())
        {
            sp->onRailsSeeded = [this] (const juce::Array<int>& slots)
            {
                for (const auto slot : slots)
                    zoneB.seedPatternForSlot (slot, 16, { 0, 4, 8, 12 });
                zoneD.seedStructureRails (processorRef.getAppState().structure);
            };

            sp->onStructureFollowModeChanged = [this] (bool enabled)
            {
                const bool hasStructure = ! processorRef.getAppState().structure.sections.empty();
                zoneD.setStructureFollowState (computeFollowState (enabled, hasStructure));
                systemFeed.addMessage (enabled ? "Structure follow enabled" : "Structure follow disabled");
            };

            sp->onStructureApplied = [this]
            {
                zoneD.setStructureView (&processorRef.getAppState().structure, &processorRef.getStructureEngine());
                zoneD.seedStructureRails (processorRef.getAppState().structure);
                zoneD.setStructureBeat (processorRef.getAppState().transport.playheadBeats);
                zoneD.setStructureFollowState (computeFollowState (processorRef.isStructureFollowForTracksEnabled(),
                                                                   ! processorRef.getAppState().structure.sections.empty()));
                if (auto* song = zoneA.getSongPanel())
                {
                    song->setKey (processorRef.getAppState().song.keyRoot);
                    song->setScale (processorRef.getAppState().song.keyScale);
                }
                systemFeed.addMessage ("Structure applied: "
                                       + juce::String (processorRef.getAppState().structure.sections.size())
                                       + " section(s)");
            };
        }
    };

    // Wrap tab selection to also wire panels on first open
    auto originalOnTabSelected = m_tabStrip.onTabSelected;
    m_tabStrip.onTabSelected = [this, wireTracksPanel, wireSongPanel, wireTransportPanel, wireStructurePanel,
                                  originalOnTabSelected] (const juce::String& tabId)
    {
        if (originalOnTabSelected) originalOnTabSelected (tabId);
        wireTracksPanel();
        wireSongPanel();
        wireTransportPanel();
        wireStructurePanel();

        // Populate mixer on first open (or any re-open) using the cached module list
        if (tabId == "mixer" && m_lastModuleNames.size() > 0)
            zoneA.setMixerSlots (m_lastModuleNames);

        // Populate tracks lanes on first open using the cached module list
        if (tabId == "tracks" && m_lastModuleNames.size() > 0)
            zoneA.setTrackLanes (m_lastModuleNames);
    };

    // Wire Zone C param changes to the DSP layer
    zoneC.onDspParamChanged = [this] (int slotIdx, int nodeIdx, int paramIdx, float value)
    {
        processorRef.setFXChainParam (slotIdx, nodeIdx, paramIdx, value);
    };
    zoneC.onChainRebuilt = [this] (int slotIdx, const ChainState& chain)
    {
        processorRef.rebuildFXChain (slotIdx, chain);
    };

    // Wire Zone B step-pattern edits to the processor
    zoneB.onPatternModified = [this] (int slotIdx, const SlotPattern& pattern)
    {
        const int stepCount = pattern.activeStepCount();
        processorRef.setStepCount (slotIdx, stepCount);
        for (int s = 0; s < SlotPattern::MAX_STEPS; ++s)
            processorRef.setStepActive (slotIdx, s, s < stepCount && pattern.stepActive (s));
    };

    // Wire Zone B DnD drop → load the pending clip into the target slot
    zoneB.onClipDropped = [this] (int flatSlotIndex)
    {
        if (!m_dragClip.has_value()) return;
        loadClipToReelSlot (*m_dragClip, flatSlotIndex);
        m_dragClip.reset();
    };

    // Set up Audio History Strip
    historyStrip.setCircularBuffer (&processorRef.getCircularBuffer());
    historyStrip.setSourceInfo ("MIX", Theme::Colour::accent);

    historyStrip.onHeightChanged = [this] (int newH)
    {
        m_historyStripHeight = newH;
        resized();
    };

    historyStrip.onGrabLastBars = [this] (int numBars)
    {
        processorRef.grabLastBars (numBars);

        // Compute display region: same math as CircularAudioBuffer::getLastBars
        const double bpm        = static_cast<double> (processorRef.getBpm());
        const double bpb        = processorRef.getBeatsPerBar();
        const double secPerBar  = (bpm > 0.0) ? (bpb / (bpm / 60.0)) : 2.0;
        const double len        = numBars * secPerBar;
        const double secIntoBeat = std::fmod (processorRef.getCurrentBeat(), bpb)
                                   / (bpm / 60.0);
        historyStrip.setGrabbedRegion (len + secIntoBeat, len);
        systemFeed.addMessage ("Grabbed " + juce::String (numBars) + " bars");
    };

    historyStrip.onGrabRegion = [this] (double start, double len)
    {
        processorRef.grabRegion (start, len);
        historyStrip.setGrabbedRegion (start, len);
        systemFeed.addMessage ("Grabbed " + juce::String (len, 1) + "s region");
    };

    historyStrip.onSendToReel = [this]
    {
        const int targetSlot = (m_focusedSlotIndex >= 0) ? m_focusedSlotIndex : 0;
        loadGrabbedClipToReelSlot (targetSlot);
    };

    historyStrip.onSendToNewReelSlot = [this]
    {
        // Load into the next slot (cycling), giving a quick way to fill successive REEL slots.
        const int base = (m_focusedSlotIndex >= 0) ? m_focusedSlotIndex : 0;
        loadGrabbedClipToReelSlot ((base + 1) % 8);
    };

    historyStrip.onSendToTimeline = [this]
    {
        systemFeed.addMessage ("Clip \xe2\x86\x92 Timeline \xc2\xb7 bar stub");
    };

    historyStrip.onActiveToggled = [this] (bool active)
    {
        systemFeed.addMessage (juce::String ("History buffer: ") + (active ? "LIVE" : "PAUSED"));
    };

    historyStrip.onDragStarted = [this]
    {
        if (!processorRef.hasGrabbedClip()) { m_dragClip.reset(); return; }
        CapturedAudioClip clip;
        clip.buffer.makeCopyOf (processorRef.getGrabbedClip());
        clip.sampleRate = static_cast<double> (processorRef.getSampleRate());
        clip.sourceName = (m_focusedSlotIndex >= 0)
            ? ("SLOT " + juce::String (m_focusedSlotIndex + 1).paddedLeft ('0', 2))
            : "MIX";
        clip.sourceSlot = m_focusedSlotIndex;
        m_dragClip = std::move (clip);
    };

    // Wire LooperStrip callbacks
    {
        auto& ls = zoneB.getLooperStrip();

        ls.onGrabLastBars = [this] (int numBars)
        {
            processorRef.grabLastBars (numBars);

            const double bpm       = static_cast<double> (processorRef.getBpm());
            const double bpb       = processorRef.getBeatsPerBar();
            const double secPerBar = (bpm > 0.0) ? (bpb / (bpm / 60.0)) : 2.0;
            const double len       = numBars * secPerBar;
            const double secIntoBeat = std::fmod (processorRef.getCurrentBeat(), bpb)
                                       / (bpm / 60.0);
            historyStrip.setGrabbedRegion (len + secIntoBeat, len);
            zoneB.getLooperStrip().setHasGrabbedClip (true);
            systemFeed.addMessage ("Grabbed " + juce::String (numBars) + " bars");
        };

        ls.onGrabFree = [this]
        {
            systemFeed.addMessage ("Free grab — select a region in the history strip");
        };

        ls.onSendToReel = [this]
        {
            const int targetSlot = (m_focusedSlotIndex >= 0) ? m_focusedSlotIndex : 0;
            loadGrabbedClipToReelSlot (targetSlot);
        };

        ls.onSendToTimeline = [this]
        {
            systemFeed.addMessage ("Clip \xe2\x86\x92 Timeline \xc2\xb7 bar stub");
        };

        ls.onLiveToggled = [this] (bool active)
        {
            systemFeed.addMessage (juce::String ("Looper: ") + (active ? "LIVE" : "PAUSED"));
        };

        ls.onSourceChanged = [this] (int sourceIndex)
        {
            // 0 = MIX (maps to -1 in circular buffer), 1+ = slot index (0-based)
            processorRef.getCircularBuffer().setSource (sourceIndex - 1);
        };
    }

    // Wire drum machine focus → load kit + sync DSP
    zoneB.onDrumSlotFocused = [this] (int slotIdx, DrumMachineData* data)
    {
        processorRef.getAudioGraph().setSlotSynthType (slotIdx, 2);
        syncDrumDataToProcessor (slotIdx, data, processorRef);
    };

    // Wire drum step edits → sync DSP
    zoneB.onDrumPatternModified = [this] (int slotIdx, DrumMachineData* data)
    {
        syncDrumDataToProcessor (slotIdx, data, processorRef);
    };

    // Wire step advance notifications from the processor to Zone B playhead
    processorRef.onStepAdvanced = [this] (int step) { onStepAdvanced (step); };

    // Wire gear button in Zone D transport strip → TPRT panel
    zoneD.onSettingsClicked = [this] { openTransportSettings(); };

    zoneB.addListener (this);
    zoneD.addTransportListener (this);
    ThemeManager::get().addListener (this);

    zoneC.onCollapseChanged = [this] (bool) { resized(); };
    zoneD.onHeightChanged   = [this] (int)  { resized(); };

    // Zone resizer setup — added AFTER zones so they render on top
    m_resizerAB.setConstraints (80, 400, 192);   // min/max/dragStart for content panel
    m_resizerAB.onResize = [this] (int s) { m_zoneAWidth = s + TabStrip::kWidth; resized(); };
    addAndMakeVisible (m_resizerAB);

    m_resizerBC.setConstraints (80, 600, 200);
    m_resizerBC.onResize = [this] (int s) { zoneC.setExpandedWidth (s); resized(); };
    m_resizerBC.setInvertDrag (true);
    addAndMakeVisible (m_resizerBC);

    m_resizerBD.setConstraints (80, 400, 180);
    m_resizerBD.onResize = [this] (int s) { zoneD.setTotalHeight (s); resized(); };
    m_resizerBD.setInvertDrag (true);
    addAndMakeVisible (m_resizerBD);

    m_resizerSysB.setConstraints (20, 80, 32);
    m_resizerSysB.onResize = [this] (int s) { m_sysFeedHeight = s; resized(); };
    addAndMakeVisible (m_resizerSysB);

    setResizeLimits (800, 500, 3840, 2160);

    if (auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        auto area = display->userBounds;
        // Start with a reasonable default window size (half the screen),
        // then override layout members to match the requested default state.
        setSize (juce::roundToInt (area.getWidth()  / 2.0f),
                 juce::roundToInt (area.getHeight() / 2.0f));
    }
    else
    {
        setSize (1280, 800);
    }

    // ---- Default layout overrides to match requested initial state ----
    // Show history strip at default height (not collapsed) so it remains accessible.
    m_historyStripHeight = AudioHistoryStrip::kDefaultH;

    // Zone A: make content width comfortable; overall width includes 28px tab strip
    // Content width ~ 240px so zoneA height will be visibly taller than width
    m_zoneAWidth = TabStrip::kWidth + 240; // total = 28 + 240 = 268
    m_contentPanelOpen = true; // open Zone A by default

    // Zone C default width (right panel)
    m_zoneCWidth = 200;

    // Zone D (timeline): set to show roughly four lanes (cylinder band ≈ 4 * laneH)
    // CylinderBand::kLaneH = 27 -> 4*27 = 108; plus header (20) + transport (28) = 156
    m_zoneDHeight = 156;
    zoneD.setTotalHeight (m_zoneDHeight);

    // Ensure there are four visible tape lanes by initializing lane info (empty names)
    for (int i = 0; i < 4; ++i)
        zoneD.setLaneInfo (i, false, {});

    // Open the SONG panel in Zone A as the default active panel
    m_tabStrip.setActiveTab ("song");
    zoneA.setActivePanel ("song");

    // Read flow: initialize controls from AppState.
    songHeader.setBpm (static_cast<float> (processorRef.getAppState().transport.bpm));
    if (auto* song = zoneA.getSongPanel())
    {
        song->setKey (processorRef.getAppState().song.keyRoot);
        song->setScale (processorRef.getAppState().song.keyScale);
    }
    if (auto* structure = zoneA.getStructurePanel())
        structure->setIntentKeyMode (processorRef.getAppState().song.keyRoot, processorRef.getAppState().song.keyScale);
    zoneD.setStructureView (&processorRef.getAppState().structure, &processorRef.getStructureEngine());
    zoneD.seedStructureRails (processorRef.getAppState().structure);
    zoneD.setStructureBeat (processorRef.getAppState().transport.playheadBeats);
    zoneD.setStructureFollowState (computeFollowState (processorRef.isStructureFollowForTracksEnabled(),
                                                       ! processorRef.getAppState().structure.sections.empty()));
}

PluginEditor::~PluginEditor()
{
    zoneB.removeListener (this);
    zoneD.removeTransportListener (this);
    ThemeManager::get().removeListener (this);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);
}

void PluginEditor::resized()
{
    const int w      = getWidth();
    const int h      = getHeight();
    const int menuH  = m_menuBarHeight;
    const int hdrH   = m_songHeaderH;
    const int feedH  = m_sysFeedHeight;
    const int topOff = topOffset();

    const int zoneCW  = zoneC.currentWidth();
    const int zoneDH  = zoneD.currentHeight();
    const int zoneH   = h - topOff - zoneDH - m_historyStripHeight;

    // Tab strip and content panel widths
    const int kTabW      = TabStrip::kWidth;         // 28
    const int contentW   = contentPanelW();           // 0 when collapsed
    const int effZoneAW  = effectiveZoneAW();         // kTabW + contentW
    const int zoneBW     = zoneBWidth();

    static constexpr int kResizerW = 6;
    static constexpr int kResizerH = 6;

    // Fixed strips at top
    menuBar.setBounds    (0, 0,            w, menuH);
    songHeader.setBounds (0, menuH,        w, hdrH);
    systemFeed.setBounds (0, menuH + hdrH, w, feedH);

    // Vertical resizer between system feed and zones
    m_resizerSysB.setBounds (0, topOff - kResizerH / 2, w, kResizerH);

    // Tab strip — full height from topOffset to window bottom (alongside Zone D)
    m_tabStrip.setBounds (0, topOff, kTabW, h - topOff);

    // Zone A content panel — same height as Zones B/C (does not overlap Zone D)
    zoneA.setBounds (kTabW, topOff, contentW, zoneH);

    // Zone B
    zoneB.setBounds (effZoneAW + kResizerW, topOff, zoneBW, zoneH);

    // Zone C (overlays from right edge)
    zoneC.setBounds (w - zoneCW, topOff, zoneCW, zoneH);

    // Audio History Strip — sits between Zone B/C and Zone D, full width
    const int contentBottom = topOff + zoneH;
    historyStrip.setBounds (0, contentBottom, w, m_historyStripHeight);

    // Zone D
    zoneD.setBounds (0, h - zoneDH, w, zoneDH);

    // Horizontal resizer A|B (only visible when content panel is open)
    m_resizerAB.setVisible (m_contentPanelOpen);
    m_resizerAB.setBounds (effZoneAW, topOff, kResizerW, zoneH);

    // Horizontal resizer B|C
    m_resizerBC.setBounds (w - zoneCW, topOff, kResizerW, zoneH);

    // Vertical resizer B/C | D
    m_resizerBD.setBounds (0, h - zoneDH - kResizerH / 2, w, kResizerH);
}

void PluginEditor::slotSelected (int slotIndex, const juce::String& moduleType)
{
    m_focusedSlotIndex = slotIndex;
    zoneC.setCurrentModuleTypeName (moduleType);
    zoneC.showChainForSlot (slotIndex);
    zoneD.setLaneInfo   (slotIndex, true, moduleType);
    zoneD.highlightLane (slotIndex);

    processorRef.setFocusedSlot (slotIndex);

    // Update history strip source to follow selection
    processorRef.getCircularBuffer().setSource (slotIndex);
    historyStrip.setSourceInfo (moduleType + " · SLOT " +
                                juce::String (slotIndex + 1).paddedLeft ('0', 2),
                                Theme::Colour::accent);

    // Route slot to correct DSP node
    if (moduleType == "REEL")
        processorRef.getAudioGraph().setSlotSynthType (slotIndex, 3);
    else if (moduleType == "SYNTH")
        processorRef.getAudioGraph().setSlotSynthType (slotIndex, 1);
    else if (moduleType != "DRUM MACHINE")  // drum type is set by onDrumSlotFocused
        processorRef.getAudioGraph().setSlotSynthType (slotIndex, 0);

    const bool isAudioProducer = (moduleType == "SYNTH" || moduleType == "SAMPLER"
                                  || moduleType == "DRUM MACHINE" || moduleType == "REEL");
    processorRef.setSlotActive (slotIndex, isAudioProducer);

    // Auto-switch Zone A to the module's editor panel
    zoneA.switchToModulePanel (moduleType, slotIndex);

    if (moduleType == "REEL")
    {
        // Wire the ReelEditorPanel to its processor and connect param callbacks
        if (auto* panel = zoneA.getReelPanel (slotIndex))
        {
            auto& reelProc = processorRef.getAudioGraph().getReel (slotIndex);
            panel->setProcessor (&reelProc);
            panel->onParamChanged = [this, slotIndex] (const juce::String& paramId, float value)
            {
                processorRef.getAudioGraph().getReel (slotIndex).setParam (paramId, value);
            };
        }
        m_contentPanelOpen = true;
        resized();
    }
    else if (moduleType == "SYNTH" || moduleType == "DRUMS"
             || moduleType == "SAMPLER" || moduleType == "VST3")
    {
        // Open Zone A to INSTRUMENT tab and show the correct editor
        m_contentPanelOpen = true;
        m_tabStrip.setActiveTab ("instrument");
        resized();
    }
}

void PluginEditor::slotDeselected()
{
    zoneC.showDefault();
    zoneD.clearHighlight();
}

void PluginEditor::playStateChanged (bool playing)
{
    auto& appState = processorRef.getAppState();
    appState.transport.isPlaying = playing;
    processorRef.syncTransportFromAppState();
    zoneD.setPlaying (playing);
    if (auto* tp = zoneA.getTracksPanel())
        tp->setPlaying (playing);
    if (playing)
        systemFeed.addMessage ("Transport: playing  "
                               + juce::String (static_cast<int> (processorRef.getBpm()))
                               + " BPM");
    else
        systemFeed.addMessage ("Transport: stopped");
}

void PluginEditor::onStepAdvanced (int step)
{
    zoneB.setPlayheadStep (step);
}

void PluginEditor::positionChanged (float beat)
{
    if (! processorRef.isPlaying())
        processorRef.seekTransport (static_cast<double> (beat));

    const double processorBeat = processorRef.getCurrentSongBeat();
    double structureBeat = processorBeat;
    {
        const juce::ScopedReadLock structureRead (processorRef.getStructureLock());
        const int totalBars = juce::jmax (0, processorRef.getAppState().structure.totalBars);
        const double totalBeats = static_cast<double> (totalBars) * 4.0;
        if (totalBeats > 0.0)
            structureBeat = std::fmod (juce::jmax (0.0, processorBeat), totalBeats);
    }

    if (auto* tp = zoneA.getTracksPanel())
        tp->setPosition (static_cast<float> (processorBeat));
    zoneD.setStructureBeat (structureBeat);
}

void PluginEditor::recordStateChanged (bool recording)
{
    processorRef.getAppState().transport.isRecording = recording;
    if (auto* tp = zoneA.getTracksPanel())
        tp->setRecording (recording);
}

void PluginEditor::loadClipToReelSlot (const CapturedAudioClip& clip, int slotIndex)
{
    // 1. Load audio data into the REEL processor
    auto& reelProc = processorRef.getAudioGraph().getReel (slotIndex);
    reelProc.loadFromBuffer (clip.buffer,
                             clip.sampleRate);

    // 2. Ensure DSP graph routes this slot as REEL (type 3) and marks it active
    processorRef.getAudioGraph().setSlotSynthType (slotIndex, 3);
    processorRef.setSlotActive (slotIndex, true);

    // 3. Open the REEL editor in Zone A (creates panel if needed)
    zoneA.switchToModulePanel ("REEL", slotIndex);

    // 4. Bind editor panel to the live processor and wire param callbacks
    if (auto* panel = zoneA.getReelPanel (slotIndex))
    {
        panel->setProcessor (&reelProc);
        panel->onParamChanged = [this, slotIndex] (const juce::String& paramId, float value)
        {
            processorRef.getAudioGraph().getReel (slotIndex).setParam (paramId, value);
        };
        panel->repaint();
    }

    // 5. Focus the destination slot
    m_focusedSlotIndex = slotIndex;
    processorRef.setFocusedSlot (slotIndex);

    // 6. Redirect circular buffer and history strip label to follow the slot
    processorRef.getCircularBuffer().setSource (slotIndex);
    historyStrip.setSourceInfo ("REEL \xc2\xb7 SLOT "
                                    + juce::String (slotIndex + 1).paddedLeft ('0', 2),
                                Theme::Colour::accent);

    // 7. Ensure Zone A content panel is visible
    m_contentPanelOpen = true;
    resized();

    // 8. Status
    const float durSecs = clip.buffer.getNumSamples() / static_cast<float> (clip.sampleRate);
    systemFeed.addMessage ("REEL loaded \xc2\xb7 "
                           + juce::String (durSecs, 1) + "s \xe2\x86\x92 slot "
                           + juce::String (slotIndex + 1));
}

void PluginEditor::loadGrabbedClipToReelSlot (int slotIndex)
{
    if (!processorRef.hasGrabbedClip()) return;

    CapturedAudioClip clip;
    clip.buffer.makeCopyOf (processorRef.getGrabbedClip());
    clip.sampleRate = static_cast<double> (processorRef.getSampleRate());
    loadClipToReelSlot (clip, slotIndex);
}

void PluginEditor::openTransportSettings()
{
    m_contentPanelOpen = true;
    m_tabStrip.setActiveTab ("tracks");
    zoneA.setActivePanel ("transport");
    resized();
}

void PluginEditor::themeChanged()
{
    repaintAll (*this);
}

void PluginEditor::repaintAll (juce::Component& root)
{
    root.repaint();
    for (int i = 0; i < root.getNumChildComponents(); ++i)
        repaintAll (*root.getChildComponent (i));
}

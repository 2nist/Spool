#include "PluginEditor.h"
#include "instruments/drum/DrumModuleState.h"

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

juce::String chordText (const Chord& chord)
{
    return chord.root.isNotEmpty() ? chord.root + chord.type : juce::String();
}
}

//==============================================================================
// Zone B quick-control → DSP translation
//
// Label-based routing: matches param labels in ModuleRow::initParams() so the
// mapping is stable regardless of future changes to the Zone B param order.
// All PolySynthProcessor setters are covered so any label can be added to the
// Zone B surface without touching this function.
//
static void applySynthQuickParam (PolySynthProcessor& synth, const juce::String& label, float value)
{
    if      (label == "OSC1")  synth.setOsc1Shape        (juce::roundToInt (value));
    else if (label == "DET1")  synth.setOsc1Detune       (value);
    else if (label == "OSC2")  synth.setOsc2Shape        (juce::roundToInt (value));
    else if (label == "DET2")  synth.setOsc2Detune       (value);
    else if (label == "OCT2")  synth.setOsc2Octave       (juce::roundToInt (value));
    else if (label == "MIX2")  synth.setOsc2Level        (value);
    else if (label == "CUT")   synth.setFilterCutoff     (value);
    else if (label == "RES")   synth.setFilterResonance  (value);
    else if (label == "ENVA")  synth.setFilterEnvAmt     (value);
    else if (label == "ATK")   synth.setAmpAttack        (value);
    else if (label == "DEC")   synth.setAmpDecay         (value);
    else if (label == "SUS")   synth.setAmpSustain       (value);
    else if (label == "REL")   synth.setAmpRelease       (value);
    else if (label == "FATK")  synth.setFiltAttack       (value);
    else if (label == "FDEC")  synth.setFiltDecay        (value);
    else if (label == "FSUS")  synth.setFiltSustain      (value);
    else if (label == "FREL")  synth.setFiltRelease      (value);
    else if (label == "LFRT")  synth.setLfoRate          (value);
    else if (label == "LFDP")  synth.setLfoDepth         (value);
}

// ----------------------------------------------------------------------------
// Zone B quick-control → authoritative DrumModuleState translation
//
// paramId format for per-voice params: "vN:field"
//   N      = voice index in the current kit
//   field  = tpitch, tdecay, body, clicklvl, noiselv, noisehp, metaldec,
//            drive, grit, snap, outl, outp, trig
//
// Special (kit-level) IDs: "voicepg", "muteall", "initkit" — placeholders for now.
// ----------------------------------------------------------------------------
static bool applyDrumQuickParam (DrumMachineData& drumState,
                                 const juce::String& paramId,
                                 float value)
{
    // Per-voice params: "vN:field"
    if (paramId.length() >= 3 && paramId[0] == 'v'
        && paramId.substring(1).containsChar (':'))
    {
        const int    colonIdx  = paramId.indexOfChar (':');
        const int    voiceIdx  = paramId.substring (1, colonIdx).getIntValue();
        const auto   field     = paramId.substring (colonIdx + 1);

        if (voiceIdx < 0 || voiceIdx >= static_cast<int> (drumState.voices.size()))
            return false;

        auto& voice = drumState.voices[static_cast<size_t> (voiceIdx)];

        if      (field == "tpitch")   voice.params.tone.pitch        = value;
        else if (field == "tdecay")   voice.params.tone.decay        = value;
        else if (field == "body")     voice.params.tone.body         = value;
        else if (field == "clicklvl") voice.params.click.level       = value;
        else if (field == "noiselv")  voice.params.noise.level       = value;
        else if (field == "noisehp")  voice.params.noise.hpfreq      = value;
        else if (field == "metaldec") voice.params.metal.decay       = value;
        else if (field == "drive")    voice.params.character.drive   = value;
        else if (field == "grit")     voice.params.character.grit    = value;
        else if (field == "snap")     voice.params.character.snap    = value;
        else if (field == "outl")     voice.params.out.level         = value;
        else if (field == "outp")     voice.params.out.pan           = value;
        else return false;

        voice.params.midiNote = voice.midiNote;
        drumState.clamp();
        return true;
    }

    if (paramId == "muteall")
    {
        const bool muted = value >= 0.5f;
        for (auto& voice : drumState.voices)
            voice.muted = muted;
        return true;
    }

    if (paramId == "initkit" && value >= 0.5f)
    {
        const auto focusedVoiceIndex = drumState.focusedVoiceIndex;
        drumState = DrumMachineData::makeDefault();
        drumState.focusedVoiceIndex = focusedVoiceIndex;
        drumState.clamp();
        return true;
    }

    juce::ignoreUnused (value); // "voicepg" or unsupported IDs remain no-op for this pass
    return false;
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
        syncAllSlotRuntimeFromModuleList();
    };

    // Wire Zone A InstrumentPanel browser slot selection to full slot-selected flow
    zoneA.onInstrumentSlotSelected = [this] (int slotIndex, const juce::String& moduleType)
    {
        slotSelected (slotIndex, moduleType);
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
        appState.structure.songTempo = static_cast<int> (bpm);
        processorRef.getStructureEngine().rebuild();
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
                appState.structure.songTempo = static_cast<int> (bpm);
                processorRef.getStructureEngine().rebuild();
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
            sp->setStructureSummary (processorRef.getAppState().structure);
            sp->onBpmChanged = [this] (float bpm)
            {
                auto& appState = processorRef.getAppState();
                appState.transport.bpm = bpm;
                appState.song.bpm = static_cast<int> (bpm);
                appState.structure.songTempo = static_cast<int> (bpm);
                processorRef.getStructureEngine().rebuild();
                processorRef.syncTransportFromAppState();
                songHeader.setBpm   (bpm);
                if (auto* song = zoneA.getSongPanel())
                    song->setStructureSummary (processorRef.getAppState().structure);
            };

            sp->onKeyChanged = [this] (const juce::String& key)
            {
                auto& appState = processorRef.getAppState();
                appState.song.keyRoot = key.trim();
                appState.structure.songKey = appState.song.keyRoot;
                if (auto* song = zoneA.getSongPanel())
                    song->setStructureSummary (processorRef.getAppState().structure);
                if (auto* structure = zoneA.getStructurePanel())
                    structure->setIntentKeyMode (appState.song.keyRoot, appState.song.keyScale);
            };

            sp->onScaleChanged = [this] (const juce::String& scale)
            {
                auto& appState = processorRef.getAppState();
                appState.song.keyScale = scale.trim();
                appState.structure.songMode = appState.song.keyScale;
                if (auto* song = zoneA.getSongPanel())
                    song->setStructureSummary (processorRef.getAppState().structure);
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
                const bool hasStructure = structureHasScaffold (processorRef.getAppState().structure);
                zoneD.setStructureFollowState (computeFollowState (enabled, hasStructure));
                updateSequencerStructureContext (processorRef.getCurrentSongBeat());
                systemFeed.addMessage (enabled ? "Structure follow enabled" : "Structure follow disabled");
            };

            sp->onStructureApplied = [this]
            {
                zoneD.setStructureView (&processorRef.getAppState().structure, &processorRef.getStructureEngine());
                zoneD.seedStructureRails (processorRef.getAppState().structure);
                zoneD.setStructureBeat (processorRef.getAppState().transport.playheadBeats);
                zoneD.setStructureFollowState (computeFollowState (processorRef.isStructureFollowForTracksEnabled(),
                                                                   structureHasScaffold (processorRef.getAppState().structure)));
                if (auto* song = zoneA.getSongPanel())
                {
                    song->setKey (processorRef.getAppState().song.keyRoot);
                    song->setScale (processorRef.getAppState().song.keyScale);
                    song->setStructureSummary (processorRef.getAppState().structure);
                }
                updateSequencerStructureContext (processorRef.getCurrentSongBeat());
                systemFeed.addMessage ("Structure applied: "
                                       + juce::String (processorRef.getAppState().structure.arrangement.size())
                                       + " block(s)");
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
        processorRef.setSlotPattern (slotIdx, pattern);
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
        if (!processorRef.hasGrabbedClip()) return;
        routeClipToTimeline (buildGrabClip());
    };

    historyStrip.onSendToLooper = [this]
    {
        if (!processorRef.hasGrabbedClip()) return;
        routeClipToLooper (buildGrabClip());
    };

    historyStrip.onActiveToggled = [this] (bool active)
    {
        systemFeed.addMessage (juce::String ("History buffer: ") + (active ? "LIVE" : "PAUSED"));
    };

    historyStrip.onDragStarted = [this]
    {
        if (!processorRef.hasGrabbedClip()) { m_dragClip.reset(); return; }
        m_dragClip = buildGrabClip();
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

        ls.onSendToLooper = [this]
        {
            if (!processorRef.hasGrabbedClip()) return;
            routeClipToLooper (buildGrabClip());
        };

        ls.onSendToTimeline = [this]
        {
            if (!processorRef.hasGrabbedClip()) return;
            routeClipToTimeline (buildGrabClip());
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
        juce::ignoreUnused (data);
        applyDrumStateToRuntime (slotIdx);
    };

    // Wire drum step edits → sync DSP
    zoneB.onDrumPatternModified = [this] (int slotIdx, DrumMachineData* data)
    {
        juce::ignoreUnused (data);
        applyDrumStateToRuntime (slotIdx);
    };

    // Wire Zone B fader changes → DSP quick controls.
    // m_lastModuleNames[slotIdx] is "GROUPNAME:TYPENAME"; strip the group prefix.
    zoneB.onQuickParamChanged = [this] (int slotIdx, const juce::String& paramId, float value)
    {
        if (slotIdx < 0 || slotIdx >= m_lastModuleNames.size()) return;

        const juce::String moduleType =
            m_lastModuleNames[slotIdx].fromLastOccurrenceOf (":", false, false).toUpperCase();

        if (moduleType == "SYNTH")
        {
            applySynthQuickParam (processorRef.getAudioGraph().getPolySynth (slotIdx),
                                  paramId, value);
        }
        else if (moduleType == "DRUMS")
        {
            auto* state = getDrumStateForSlot (slotIdx);
            if (state == nullptr)
                return;

            if (paramId.endsWith (":trig"))
            {
                const int colonIdx = paramId.indexOfChar (':');
                const int voiceIdx = paramId.substring (1, colonIdx).getIntValue();
                auto& drum = processorRef.getAudioGraph().getDrumMachine (slotIdx);
                if (voiceIdx >= 0 && voiceIdx < drum.getNumVoices())
                    drum.triggerVoice (voiceIdx, value);
                return;
            }

            if (applyDrumQuickParam (*state, paramId, value))
                replaceDrumStateForSlot (slotIdx, *state);
        }
        // REEL and other types: seam left for next pass
    };

    // Wire step advance notifications from the processor to Zone B playhead
    processorRef.onStepAdvanced = [this] (int step) { onStepAdvanced (step); };

    // Wire gear button in Zone D transport strip → TPRT panel
    zoneD.onSettingsClicked = [this] { openTransportSettings(); };

    zoneB.addListener (this);
    zoneD.addTransportListener (this);
    ThemeManager::get().addListener (this);

    // Re-broadcast the current module list now that all callbacks are wired.
    // ZoneBComponent fires onModuleListChanged during its own construction
    // (before PluginEditor wires the callback), so this initial state would
    // otherwise be missed and InstrumentPanel / MixerPanel would start empty.
    zoneB.broadcastModuleList();

    if (auto* instrumentPanel = zoneA.getInstrumentPanel())
    {
        instrumentPanel->onRequestDrumState = [this] (int slotIndex) -> DrumMachineData*
        {
            return getDrumStateForSlot (slotIndex);
        };
        instrumentPanel->onDrumStateChanged = [this] (int slotIndex, const DrumMachineData& state)
        {
            replaceDrumStateForSlot (slotIndex, state);
        };
        instrumentPanel->onPreviewDrumVoice = [this] (int slotIndex, int voiceIndex, float velocity)
        {
            auto& drum = processorRef.getAudioGraph().getDrumMachine (slotIndex);
            if (voiceIndex >= 0 && voiceIndex < drum.getNumVoices())
                drum.triggerVoice (voiceIndex, velocity);
        };
    }

    // Seed the default standalone synth slots into an actually audible init.
    // This keeps the groovebox startup state from coming up visually loaded but silent.
    auto applyAudibleSynthInit = [this] (int slotIndex, int midiNote, bool mono,
                                         std::initializer_list<int> activeSteps)
    {
        zoneB.seedPatternForSlot (slotIndex, 16, activeSteps);
        processorRef.setStepCount (slotIndex, 16);
        for (int step = 0; step < SlotPattern::MAX_STEPS; ++step)
            processorRef.setStepActive (slotIndex, step, false);
        for (const auto step : activeSteps)
            if (step >= 0 && step < 16)
                processorRef.setStepActive (slotIndex, step, true);

        processorRef.setSlotNote (slotIndex, midiNote);

        auto& synth = processorRef.getAudioGraph().getPolySynth (slotIndex);
        synth.setParam ("osc1.shape",   0.0f);
        synth.setParam ("osc2.shape",   0.0f);
        synth.setParam ("osc1.detune",  0.0f);
        synth.setParam ("osc2.detune",  7.0f);
        synth.setParam ("osc2.octave",  mono ? -1.0f : 0.0f);
        synth.setParam ("osc2.level",   mono ? 0.38f : 0.62f);
        synth.setParam ("filter.cutoff", mono ? 1400.0f : 5200.0f);
        synth.setParam ("filter.res",   mono ? 0.22f : 0.18f);
        synth.setParam ("filter.env_amt", mono ? 0.58f : 0.36f);
        synth.setParam ("amp.attack",   0.005f);
        synth.setParam ("amp.decay",    mono ? 0.18f : 0.28f);
        synth.setParam ("amp.sustain",  mono ? 0.58f : 0.82f);
        synth.setParam ("amp.release",  mono ? 0.10f : 0.22f);
        synth.setParam ("filt.attack",  0.005f);
        synth.setParam ("filt.decay",   mono ? 0.20f : 0.24f);
        synth.setParam ("filt.sustain", mono ? 0.16f : 0.28f);
        synth.setParam ("filt.release", 0.12f);
        synth.setParam ("lfo.rate",     0.35f);
        synth.setParam ("lfo.depth",    mono ? 0.00f : 0.03f);
        synth.setParam ("lfo.target",   1.0f);
        synth.setParam ("voice.mono",   mono ? 1.0f : 0.0f);
        synth.setParam ("char.asym",    mono ? 0.10f : 0.16f);
        synth.setParam ("char.drive",   mono ? 0.28f : 0.18f);
        synth.setParam ("char.drift",   0.08f);
    };

    applyAudibleSynthInit (0, 48, true,  { 0, 4, 8, 12 });
    applyAudibleSynthInit (1, 55, false, { 0, 5, 10, 13 });
    applyAudibleSynthInit (2, 60, false, { 2, 6, 10, 14 });

    // Standalone default: start in an audible groovebox state.
    processorRef.setFocusedSlot (0);
    processorRef.setPlaying (true);
    zoneD.setPlaying (true);
    if (auto* tp = zoneA.getTracksPanel())
        tp->setPlaying (true);
    systemFeed.addMessage ("Startup: transport running on synth slot 01");

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
        song->setStructureSummary (processorRef.getAppState().structure);
    }
    if (auto* structure = zoneA.getStructurePanel())
        structure->setIntentKeyMode (processorRef.getAppState().song.keyRoot, processorRef.getAppState().song.keyScale);
    zoneD.setStructureView (&processorRef.getAppState().structure, &processorRef.getStructureEngine());
    zoneD.seedStructureRails (processorRef.getAppState().structure);
    zoneD.setStructureBeat (processorRef.getAppState().transport.playheadBeats);
    zoneD.setStructureFollowState (computeFollowState (processorRef.isStructureFollowForTracksEnabled(),
                                                       structureHasScaffold (processorRef.getAppState().structure)));
    updateSequencerStructureContext (processorRef.getCurrentSongBeat());
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
    syncSlotRuntimeFromModuleType (slotIndex, moduleType);

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
    else if (moduleType == "SYNTH" || moduleType == "DRUMS" || moduleType == "DRUM MACHINE"
             || moduleType == "SAMPLER" || moduleType == "VST3")
    {
        // Open Zone A to INSTRUMENT tab and show the correct editor
        m_contentPanelOpen = true;
        m_tabStrip.setActiveTab ("instrument");
        resized();
    }

    updateSequencerStructureContext (processorRef.getCurrentSongBeat());
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
        const double totalBeats = static_cast<double> (juce::jmax (0, processorRef.getAppState().structure.totalBeats));
        if (totalBeats > 0.0)
            structureBeat = std::fmod (juce::jmax (0.0, processorBeat), totalBeats);
    }

    if (auto* tp = zoneA.getTracksPanel())
        tp->setPosition (static_cast<float> (processorBeat));
    zoneD.setStructureBeat (structureBeat);
    updateSequencerStructureContext (structureBeat);
}

void PluginEditor::updateSequencerStructureContext (double structureBeat)
{
    juce::String sectionName, positionLabel, currentChord, nextChord, transitionIntent;
    bool followingStructure = processorRef.isStructureFollowForTracksEnabled();
    bool locallyOverriding = ! followingStructure;

    {
        const juce::ScopedReadLock structureRead (processorRef.getStructureLock());
        if (const auto instance = processorRef.getStructureEngine().getSectionAtBeat (structureBeat))
        {
            if (instance->section != nullptr)
                sectionName = instance->section->name;
            const int localBarBeat = juce::jmax (0, juce::roundToInt (structureBeat - static_cast<double> (instance->startBeat)));
            const int sectionBarIndex = juce::jlimit (0, juce::jmax (0, instance->totalBars - 1), localBarBeat / juce::jmax (1, instance->beatsPerBar));
            positionLabel = "Bar " + juce::String (sectionBarIndex + 1) + "/" + juce::String (instance->totalBars);
            transitionIntent = instance->transitionIntent.isNotEmpty() ? "Transition: " + instance->transitionIntent : juce::String();

            currentChord = chordText (processorRef.getStructureEngine().getChordAtBeat (structureBeat));

            if (instance->section != nullptr && ! instance->section->progression.empty())
            {
                const int beatsPerBar = juce::jmax (1, instance->beatsPerBar);
                const int barsPerRepeat = juce::jmax (1, instance->barsPerRepeat);
                const int perRepeatBeats = juce::jmax (1, beatsPerBar * barsPerRepeat);
                const double beatInsideSection = juce::jmax (0.0, structureBeat - static_cast<double> (instance->startBeat));
                const int currentBar = static_cast<int> (std::floor (std::fmod (beatInsideSection, static_cast<double> (perRepeatBeats))
                                                                     / static_cast<double> (beatsPerBar)));
                const int nextBar = (currentBar + 1) % barsPerRepeat;
                const int nextIndex = nextBar % static_cast<int> (instance->section->progression.size());
                nextChord = chordText (instance->section->progression[(size_t) nextIndex]);
            }
        }
    }

    zoneB.setStructureContext (sectionName, positionLabel, currentChord, nextChord, transitionIntent, followingStructure, locallyOverriding);
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
    loadClipToReelSlot (buildGrabClip(), slotIndex);
}

CapturedAudioClip PluginEditor::buildGrabClip() const
{
    CapturedAudioClip clip;
    clip.buffer.makeCopyOf (processorRef.getGrabbedClip());
    clip.sampleRate = static_cast<double>  (processorRef.getSampleRate());
    clip.sourceName = (m_focusedSlotIndex >= 0)
        ? ("SLOT " + juce::String (m_focusedSlotIndex + 1).paddedLeft ('0', 2))
        : "MIX";
    clip.sourceSlot = m_focusedSlotIndex;
    clip.tempo      = static_cast<double>  (processorRef.getBpm());

    if (clip.tempo > 0.0 && processorRef.getBeatsPerBar() > 0.0)
    {
        const double barsExact = clip.durationSeconds() * clip.tempo / 60.0
                                 / processorRef.getBeatsPerBar();
        clip.bars = juce::roundToInt (barsExact);
    }

    return clip;
}

void PluginEditor::routeClipToLooper (const CapturedAudioClip& clip)
{
    // LOOPER destination — clip handed off to the looper workspace.
    // m_looperClip persists the clip until a loop DSP engine consumes it.
    // Visual: LooperStrip shows "clip loaded" state for all row-2 buttons.
    m_looperClip = clip;
    zoneB.getLooperStrip().setHasGrabbedClip (true);

    juce::String msg = "LOOPER \xc2\xb7 " + juce::String (clip.durationSeconds(), 1)
                       + "s from " + clip.sourceName;
    if (clip.bars > 0)
        msg += " (" + juce::String (clip.bars) + " bars)";
    systemFeed.addMessage (msg);
}

void PluginEditor::routeClipToTimeline (const CapturedAudioClip& clip)
{
    // TIMELINE destination stub — no arrangement DSP engine in this build.
    // Opens the Tracks panel to show placement intent.
    // Future: TracksPanel::placeClip (clip, barPosition) goes here.
    juce::String msg = "TIMELINE \xe2\x86\x92 " + juce::String (clip.durationSeconds(), 1)
                       + "s from " + clip.sourceName;
    if (clip.bars > 0)
        msg += " (" + juce::String (clip.bars) + " bars)";
    msg += " \xc2\xb7 stub";
    systemFeed.addMessage (msg);

    m_contentPanelOpen = true;
    m_tabStrip.setActiveTab ("tracks");
    zoneA.setActivePanel ("tracks");
    resized();
}

DrumMachineData* PluginEditor::getDrumStateForSlot (int slotIndex) noexcept
{
    return zoneB.getDrumDataForSlot (slotIndex);
}

const DrumMachineData* PluginEditor::getDrumStateForSlot (int slotIndex) const noexcept
{
    return zoneB.getDrumDataForSlot (slotIndex);
}

void PluginEditor::syncSlotRuntimeFromModuleType (int slotIndex, const juce::String& moduleType)
{
    if (slotIndex < 0 || slotIndex >= SpoolAudioGraph::kNumSlots)
        return;

    const auto normalized = moduleType.toUpperCase().trim();

    if (normalized == "DRUM MACHINE" || normalized == "DRUMS")
    {
        applyDrumStateToRuntime (slotIndex);
        return;
    }

    int synthType = 0;
    if (normalized == "SYNTH")
        synthType = 1;
    else if (normalized == "REEL")
        synthType = 3;

    const bool isAudioProducer = (normalized == "SYNTH"
                                  || normalized == "SAMPLER"
                                  || normalized == "REEL");

    processorRef.getAudioGraph().setSlotSynthType (slotIndex, synthType);
    processorRef.setSlotActive (slotIndex, isAudioProducer);
}

void PluginEditor::syncAllSlotRuntimeFromModuleList()
{
    for (int slotIndex = 0; slotIndex < m_lastModuleNames.size(); ++slotIndex)
    {
        const auto moduleType =
            m_lastModuleNames[slotIndex].fromLastOccurrenceOf (":", false, false);
        syncSlotRuntimeFromModuleType (slotIndex, moduleType);
    }
}

void PluginEditor::applyDrumStateToRuntime (int slotIndex)
{
    auto* state = getDrumStateForSlot (slotIndex);
    if (state == nullptr)
        return;

    state->clamp();

    processorRef.getAudioGraph().setSlotSynthType (slotIndex, 2);
    processorRef.setSlotActive (slotIndex, true);
    processorRef.getAudioGraph().getDrumMachine (slotIndex).applyState (*state);

    const int numVoices = static_cast<int> (state->voices.size());
    processorRef.setDrumVoiceCount (slotIndex, numVoices);
    for (int vi = 0; vi < numVoices; ++vi)
    {
        const auto& voice = state->voices[static_cast<size_t> (vi)];
        processorRef.setDrumVoiceMidiNote  (slotIndex, vi, voice.midiNote);
        processorRef.setDrumVoiceStepBits  (slotIndex, vi, voice.stepBits);
        processorRef.setDrumVoiceStepCount (slotIndex, vi, voice.stepCount);
        for (int step = 0; step < DrumVoiceState::kMaxSteps; ++step)
        {
            processorRef.setDrumStepVelocity (slotIndex, vi, step, voice.stepVelocity (step));
            processorRef.setDrumStepRatchet  (slotIndex, vi, step, voice.stepRatchetCount (step));
            processorRef.setDrumStepDivision (slotIndex, vi, step, voice.stepRepeatDivision (step));
        }
    }
}

void PluginEditor::replaceDrumStateForSlot (int slotIndex, const DrumMachineData& state)
{
    zoneB.setDrumDataForSlot (slotIndex, state);
    applyDrumStateToRuntime (slotIndex);
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

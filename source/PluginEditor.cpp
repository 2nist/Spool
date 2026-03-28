#include "PluginEditor.h"
#include "instruments/drum/DrumModuleState.h"
#include "theme/ThemeEditorPanel.h"

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
    return abbreviatedChordLabel (chord);
}

constexpr int kFileMenuNew = 1;
constexpr int kFileMenuOpen = 2;
constexpr int kFileMenuSave = 3;
constexpr int kFileMenuSaveAs = 4;
constexpr int kFileMenuImportLyrics = 5;
constexpr int kEditMenuUndo = 11;
constexpr int kEditMenuRedo = 12;
constexpr int kViewMenuSystemFeed = 21;
constexpr int kViewMenuSongHeader = 22;
constexpr int kViewMenuHistoryTape = 23;
constexpr int kViewMenuLooper = 24;
constexpr int kViewMenuZoneCMacros = 25;
constexpr int kViewMenuCompactHeader = 26;
constexpr int kSettingsMenuPreferences = 31;
constexpr int kSettingsMenuThemeDesigner = 32;
constexpr int kSettingsMenuImportTheme = 33;
constexpr int kSettingsMenuExportTheme = 34;

std::vector<juce::String> parseLyricImportLines (const juce::String& text)
{
    std::vector<juce::String> lines;
    auto sourceLines = juce::StringArray::fromLines (text);
    for (const auto& rawLine : sourceLines)
    {
        const auto trimmed = rawLine.trim();
        if (trimmed.isNotEmpty())
            lines.push_back (trimmed);
    }

    if (lines.empty())
    {
        const auto trimmed = text.trim();
        if (trimmed.isNotEmpty())
            lines.push_back (trimmed);
    }

    return lines;
}

class ThemeEditorModalContent final : public juce::Component
{
public:
    ThemeEditorModalContent()
        : m_snapshot (ThemeManager::get().theme())
    {
        addAndMakeVisible (m_editor);

        styleButton (m_doneButton, Theme::Zone::a);
        m_doneButton.onClick = [this]
        {
            m_cancelled = false;
            if (const auto presetName = ThemeManager::get().theme().presetName.trim();
                presetName.isNotEmpty())
            {
                AppPreferences::get().setThemePresetName (presetName);
            }
            closeSelf();
        };
        addAndMakeVisible (m_doneButton);

        styleButton (m_cancelButton, Theme::Colour::accentWarm);
        m_cancelButton.onClick = [this]
        {
            restoreSnapshot();
            closeSelf();
        };
        addAndMakeVisible (m_cancelButton);
    }

    ~ThemeEditorModalContent() override
    {
        if (m_cancelled)
            restoreSnapshot();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Theme::Colour::surface1);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRect (getLocalBounds(), 1);
        g.drawHorizontalLine (getHeight() - kFooterH, 0.0f, (float) getWidth());
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto footer = bounds.removeFromBottom (kFooterH).reduced (8, 6);
        m_doneButton.setBounds (footer.removeFromRight (72));
        footer.removeFromRight (6);
        m_cancelButton.setBounds (footer.removeFromRight (72));
        m_editor.setBounds (bounds);
    }

private:
    static constexpr int kFooterH = 34;

    ThemeEditorPanel m_editor;
    juce::TextButton m_doneButton { "DONE" };
    juce::TextButton m_cancelButton { "CANCEL" };
    ThemeData m_snapshot;
    bool m_cancelled { true };

    static void styleButton (juce::TextButton& button, juce::Colour accent)
    {
        button.setColour (juce::TextButton::buttonColourId, ThemeManager::get().theme().controlBg);
        button.setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.28f));
        button.setColour (juce::TextButton::textColourOffId, ThemeManager::get().theme().controlTextOn);
        button.setColour (juce::TextButton::textColourOnId, ThemeManager::get().theme().controlTextOn);
    }

    void restoreSnapshot()
    {
        ThemeManager::get().applyTheme (m_snapshot);
        if (m_snapshot.presetName.isNotEmpty())
            AppPreferences::get().setThemePresetName (m_snapshot.presetName);
    }

    void closeSelf()
    {
        if (auto* dialog = findParentComponentOfClass<juce::DialogWindow>())
        {
            dialog->exitModalState (0);
            dialog->setVisible (false);
        }
    }
};
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
    setWantsKeyboardFocus (true);

    ThemeManager::get().applyTheme (ThemePresets::presetByName (AppPreferences::get().getThemePresetName()));

    addAndMakeVisible (menuBar);
    addAndMakeVisible (songHeader);
    addAndMakeVisible (systemFeed);
    addAndMakeVisible (m_tabStrip);
    addAndMakeVisible (zoneA);
    

    addAndMakeVisible (zoneB);
    addAndMakeVisible (zoneC);
    addAndMakeVisible (zoneD);
    addAndMakeVisible (historyStrip);
    zoneB.setLooperVisible (AppPreferences::get().getShowLooper());
    zoneC.setMacroStripVisible (AppPreferences::get().getShowZoneCMacros());

    // Wire compact shell master controls to processor master output
    menuBar.onLevelChanged = [this] (float v) { processorRef.setMasterGain (v); };
    menuBar.onPanChanged   = [this] (float p) { processorRef.setMasterPan  (p); };
    menuBar.setLevel (processorRef.getMasterGain());
    menuBar.setPan   (processorRef.getMasterPan());

    // Wire routing changes from Zone A to the processor, and push initial state
    zoneA.setOnRoutingChanged ([this] (const RoutingState& state)
    {
        processorRef.setRoutingState (state);
    });
    zoneA.setRoutingState (processorRef.getRoutingState());

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

    menuBar.onMenuClicked = [this] (const juce::String& menuId, juce::Rectangle<int> anchorBounds)
    {
        showMenuPopup (menuId, anchorBounds);
    };

    // Wire BPM drag in song header to the processor
    songHeader.onBpmChanged = [this] (float bpm)
    {
        processorRef.getSongManager().setTempo (juce::roundToInt (bpm));
        processorRef.syncAuthoredSongToRuntime();
        if (auto* tp = zoneA.getTracksPanel())
            tp->setBpm (bpm);
        if (auto* sp = zoneA.getSongPanel())
            sp->setBpm (bpm);
    };
    songHeader.onTitleChanged = [this] (const juce::String& title)
    {
        processorRef.getSongManager().setSongTitle (title);
        processorRef.syncAuthoredSongToRuntime();

        if (auto* song = zoneA.getSongPanel())
            song->setTitle (processorRef.getSongManager().getSongTitle());
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
                processorRef.getSongManager().setTempo (juce::roundToInt (bpm));
                processorRef.syncAuthoredSongToRuntime();
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
            sp->setTitle (processorRef.getSongManager().getSongTitle());
            sp->setStructureSummary (processorRef.getSongManager().getStructureState());
            sp->onTitleChanged = [this] (const juce::String& title)
            {
                processorRef.getSongManager().setSongTitle (title);
                processorRef.syncAuthoredSongToRuntime();
                songHeader.setTitle (processorRef.getSongManager().getSongTitle());
            };
            sp->onBpmChanged = [this] (float bpm)
            {
                processorRef.getSongManager().setTempo (juce::roundToInt (bpm));
                processorRef.syncAuthoredSongToRuntime();
                songHeader.setBpm   (bpm);
                if (auto* song = zoneA.getSongPanel())
                    song->setStructureSummary (processorRef.getSongManager().getStructureState());
            };

            sp->onKeyChanged = [this] (const juce::String& key)
            {
                processorRef.getSongManager().setKeyRoot (key);
                processorRef.syncAuthoredSongToRuntime();
                if (auto* song = zoneA.getSongPanel())
                    song->setStructureSummary (processorRef.getSongManager().getStructureState());
                if (auto* structure = zoneA.getStructurePanel())
                    structure->setIntentKeyMode (processorRef.getSongManager().getKeyRoot(),
                                                 processorRef.getSongManager().getKeyScale());
            };

            sp->onScaleChanged = [this] (const juce::String& scale)
            {
                processorRef.getSongManager().setKeyScale (scale);
                processorRef.syncAuthoredSongToRuntime();
                if (auto* song = zoneA.getSongPanel())
                    song->setStructureSummary (processorRef.getSongManager().getStructureState());
                if (auto* structure = zoneA.getStructurePanel())
                    structure->setIntentKeyMode (processorRef.getSongManager().getKeyRoot(),
                                                 processorRef.getSongManager().getKeyScale());
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
                zoneD.seedStructureRails (processorRef.getSongManager().getStructureState());
            };

            sp->onStructureFollowModeChanged = [this] (bool enabled)
            {
                const bool hasStructure = structureHasScaffold (processorRef.getSongManager().getStructureState());
                zoneD.setStructureFollowState (computeFollowState (enabled, hasStructure));
                updateSequencerStructureContext (processorRef.getCurrentSongBeat());
                systemFeed.addMessage (enabled ? "Structure follow enabled" : "Structure follow disabled");
            };

            sp->onStructureApplied = [this]
            {
                const auto& structure = processorRef.getSongManager().getStructureState();
                zoneD.setStructureView (&structure, &processorRef.getStructureEngine());
                zoneD.seedStructureRails (structure);
                zoneD.setStructureBeat (processorRef.getAppState().transport.playheadBeats);
                zoneD.setStructureFollowState (computeFollowState (processorRef.isStructureFollowForTracksEnabled(),
                                                                   structureHasScaffold (structure)));
                if (auto* song = zoneA.getSongPanel())
                {
                    song->setKey (processorRef.getSongManager().getKeyRoot());
                    song->setScale (processorRef.getSongManager().getKeyScale());
                    song->setStructureSummary (structure);
                }
                updateSequencerStructureContext (processorRef.getCurrentSongBeat());
                systemFeed.addMessage ("Structure applied: "
                                       + juce::String (structure.arrangement.size())
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

        ls.onEditedClipChanged = [this] (const CapturedAudioClip& clip)
        {
            m_looperClip = clip;
            processorRef.setLooperPreviewClip (clip.buffer, clip.sampleRate);
        };

        ls.onPreviewStateChanged = [this] (bool playing, bool looping)
        {
            processorRef.setLooperPreviewState (playing, looping);
        };

        ls.onEditedClipSendToReel = [this] (const CapturedAudioClip& clip)
        {
            const int targetSlot = (m_focusedSlotIndex >= 0) ? m_focusedSlotIndex : 0;
            loadClipToReelSlot (clip, targetSlot);
            systemFeed.addMessage ("Looper -> REEL");
        };

        ls.onEditedClipSendToTimeline = [this] (const CapturedAudioClip& clip)
        {
            routeClipToTimeline (clip);
        };

        ls.onEditedClipDragStarted = [this] (const CapturedAudioClip& clip)
        {
            m_dragClip = clip;
        };

        ls.onStatus = [this] (const juce::String& message)
        {
            systemFeed.addMessage (message);
        };

        ls.onQueryPreviewProgress = [this]()
        {
            return processorRef.getLooperPreviewProgress();
        };

        ls.onQueryPreviewPlaying = [this]()
        {
            return processorRef.isLooperPreviewPlaying();
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
    zoneD.onSettingsClicked = [this] { openSettingsPanel(); };

    zoneB.addListener (this);
    zoneD.addTransportListener (this);
    ThemeManager::get().addListener (this);
    AppPreferences::get().addListener (this);

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
    processorRef.syncAuthoredSongToRuntime();
    applyAuthoredSongToUi();
}

PluginEditor::~PluginEditor()
{
    zoneB.removeListener (this);
    zoneD.removeTransportListener (this);
    ThemeManager::get().removeListener (this);
    AppPreferences::get().removeListener (this);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);
}

bool PluginEditor::keyPressed (const juce::KeyPress& key)
{
    const bool commandDown = key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown();
    if (! commandDown)
        return false;

    if (key.getTextCharacter() == 'z' || key.getKeyCode() == 'z')
    {
        if (key.getModifiers().isShiftDown())
        {
            if (processorRef.getUndoManager().canRedo())
            {
                processorRef.getUndoManager().redo();
                refreshFromAuthoredSong();
                return true;
            }
        }
        else if (processorRef.getUndoManager().canUndo())
        {
            processorRef.getUndoManager().undo();
            refreshFromAuthoredSong();
            return true;
        }
    }

    if ((key.getTextCharacter() == 'y' || key.getKeyCode() == 'y') && processorRef.getUndoManager().canRedo())
    {
        processorRef.getUndoManager().redo();
        refreshFromAuthoredSong();
        return true;
    }

    return false;
}

void PluginEditor::refreshFromAuthoredSong()
{
    applyAuthoredSongToUi();
}

void PluginEditor::resized()
{
    const int w      = getWidth();
    const int h      = getHeight();
    const int menuH  = m_menuBarHeight;
    const int hdrH   = songHeaderHeight();
    const int feedH  = systemFeedHeight();
    const int topOff = topOffset();

    const int zoneCW    = zoneC.currentWidth();
    const int zoneDH    = zoneD.currentHeight();
    const int historyH  = historyTapeHeight();
    const int zoneH     = h - topOff - zoneDH - historyH;

    // Tab strip and content panel widths
    const int kTabW      = TabStrip::kWidth;         // 28
    const int contentW   = contentPanelW();           // 0 when collapsed
    const int effZoneAW  = effectiveZoneAW();         // kTabW + contentW
    const int zoneBW     = zoneBWidth();

    static constexpr int kResizerW = 6;
    static constexpr int kResizerH = 6;

    // Fixed strips at top
    menuBar.setBounds (0, 0, w, menuH);

    songHeader.setVisible (hdrH > 0);
    if (hdrH > 0)
        songHeader.setBounds (0, menuH, w, hdrH);
    else
        songHeader.setBounds (0, menuH, 0, 0);

    systemFeed.setVisible (feedH > 0);
    if (feedH > 0)
        systemFeed.setBounds (0, menuH + hdrH, w, feedH);
    else
        systemFeed.setBounds (0, menuH + hdrH, 0, 0);

    // Vertical resizer between system feed and zones
    m_resizerSysB.setVisible (feedH > 0);
    if (feedH > 0)
        m_resizerSysB.setBounds (0, topOff - kResizerH / 2, w, kResizerH);
    else
        m_resizerSysB.setBounds (0, topOff, 0, 0);

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
    historyStrip.setVisible (historyH > 0);
    if (historyH > 0)
        historyStrip.setBounds (0, contentBottom, w, historyH);
    else
        historyStrip.setBounds (0, contentBottom, 0, 0);

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
        const double totalBeats = static_cast<double> (juce::jmax (0, processorRef.getSongManager().getStructureState().totalBeats));
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
    const auto keyRoot = processorRef.getSongManager().getKeyRoot();
    const auto keyScale = processorRef.getSongManager().getKeyScale();
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
                const double beatInRepeat = std::fmod (beatInsideSection, static_cast<double> (perRepeatBeats));
                const int currentIndex = chordIndexForLoopBeat (*instance->section, beatInRepeat);
                const int nextIndex = currentIndex >= 0
                                    ? (currentIndex + 1) % static_cast<int> (instance->section->progression.size())
                                    : 0;
                nextChord = chordText (instance->section->progression[(size_t) nextIndex]);
            }
        }
    }

    zoneB.setStructureContext (sectionName, positionLabel, keyRoot, keyScale, currentChord, nextChord, transitionIntent, followingStructure, locallyOverriding);
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
    auto& looper = zoneB.getLooperStrip();
    looper.setHasGrabbedClip (true);

    if (looper.hasEditedClip() && looper.isOverdubEnabled())
        looper.overdubClip (clip);
    else
        looper.loadClip (clip);

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

int PluginEditor::songHeaderHeight() const
{
    const auto& prefs = AppPreferences::get();
    if (! prefs.getShowSongHeader())
        return 0;
    return prefs.getCompactSongHeader() ? m_songHeaderCompactH : m_songHeaderH;
}

int PluginEditor::systemFeedHeight() const
{
    return AppPreferences::get().getShowSystemFeed() ? m_sysFeedHeight : 0;
}

int PluginEditor::historyTapeHeight() const
{
    return AppPreferences::get().getShowHistoryTape() ? m_historyStripHeight : 0;
}

int PluginEditor::topOffset() const
{
    return m_menuBarHeight + songHeaderHeight() + systemFeedHeight();
}

void PluginEditor::showMenuPopup (const juce::String& menuId, juce::Rectangle<int> anchorBounds)
{
    if (menuId == "file")      { showFileMenu (anchorBounds); return; }
    if (menuId == "edit")      { showEditMenu (anchorBounds); return; }
    if (menuId == "view")      { showViewMenu (anchorBounds); return; }
    if (menuId == "settings")  { showSettingsMenu (anchorBounds); return; }
}

void PluginEditor::showFileMenu (juce::Rectangle<int> anchorBounds)
{
    juce::PopupMenu menu;
    menu.addItem (kFileMenuNew, "New Song");
    menu.addItem (kFileMenuOpen, "Open Song...");
    menu.addSeparator();
    menu.addItem (kFileMenuSave, "Save");
    menu.addItem (kFileMenuSaveAs, "Save As...");
    menu.addSeparator();
    menu.addItem (kFileMenuImportLyrics, "Import Lyrics Text...");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (anchorBounds),
                        juce::ModalCallbackFunction::create ([this] (int result)
                        {
                            if (result == kFileMenuNew) newSong();
                            else if (result == kFileMenuOpen) openSong();
                            else if (result == kFileMenuSave) saveSong();
                            else if (result == kFileMenuSaveAs) saveSongAs();
                            else if (result == kFileMenuImportLyrics) importLyricsText();
                        }));
}

void PluginEditor::showEditMenu (juce::Rectangle<int> anchorBounds)
{
    auto& undo = processorRef.getUndoManager();
    juce::PopupMenu menu;
    menu.addItem (kEditMenuUndo,
                  undo.getUndoDescription().isNotEmpty() ? "Undo " + undo.getUndoDescription() : "Undo",
                  undo.canUndo());
    menu.addItem (kEditMenuRedo,
                  undo.getRedoDescription().isNotEmpty() ? "Redo " + undo.getRedoDescription() : "Redo",
                  undo.canRedo());

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (anchorBounds),
                        juce::ModalCallbackFunction::create ([this] (int result)
                        {
                            if (result == kEditMenuUndo && processorRef.getUndoManager().canUndo())
                            {
                                processorRef.getUndoManager().undo();
                                refreshFromAuthoredSong();
                            }
                            else if (result == kEditMenuRedo && processorRef.getUndoManager().canRedo())
                            {
                                processorRef.getUndoManager().redo();
                                refreshFromAuthoredSong();
                            }
                        }));
}

void PluginEditor::showViewMenu (juce::Rectangle<int> anchorBounds)
{
    auto& prefs = AppPreferences::get();
    juce::PopupMenu menu;
    menu.addItem (kViewMenuSystemFeed, "Show System Feed", true, prefs.getShowSystemFeed());
    menu.addItem (kViewMenuSongHeader, "Show Song Header", true, prefs.getShowSongHeader());
    menu.addItem (kViewMenuHistoryTape, "Show History Tape", true, prefs.getShowHistoryTape());
    menu.addItem (kViewMenuLooper, "Show Looper", true, prefs.getShowLooper());
    menu.addItem (kViewMenuZoneCMacros, "Show Zone C Macros", true, prefs.getShowZoneCMacros());
    menu.addItem (kViewMenuCompactHeader, "Compact Song Header", prefs.getShowSongHeader(), prefs.getCompactSongHeader());
    menu.addSeparator();
    menu.addItem (kSettingsMenuPreferences, "Open Settings...");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (anchorBounds),
                        juce::ModalCallbackFunction::create ([this, anchorBounds] (int result)
                        {
                            auto& prefs = AppPreferences::get();
                            if (result == kViewMenuSystemFeed) prefs.setShowSystemFeed (! prefs.getShowSystemFeed());
                            else if (result == kViewMenuSongHeader) prefs.setShowSongHeader (! prefs.getShowSongHeader());
                            else if (result == kViewMenuHistoryTape) prefs.setShowHistoryTape (! prefs.getShowHistoryTape());
                            else if (result == kViewMenuLooper) prefs.setShowLooper (! prefs.getShowLooper());
                            else if (result == kViewMenuZoneCMacros) prefs.setShowZoneCMacros (! prefs.getShowZoneCMacros());
                            else if (result == kViewMenuCompactHeader) prefs.setCompactSongHeader (! prefs.getCompactSongHeader());
                            else if (result == kSettingsMenuPreferences) openSettingsPanel (anchorBounds);
                        }));
}

void PluginEditor::showSettingsMenu (juce::Rectangle<int> anchorBounds)
{
    juce::PopupMenu menu;
    menu.addItem (kSettingsMenuPreferences, "Preferences...");
    menu.addItem (kSettingsMenuThemeDesigner, "Theme Designer...");
    menu.addSeparator();
    menu.addItem (kSettingsMenuImportTheme, "Import Theme...");
    menu.addItem (kSettingsMenuExportTheme, "Export Current Theme...");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (anchorBounds),
                        juce::ModalCallbackFunction::create ([this, anchorBounds] (int result)
                        {
                            if (result == kSettingsMenuPreferences) openSettingsPanel (anchorBounds);
                            else if (result == kSettingsMenuThemeDesigner) openThemeDesigner (anchorBounds);
                            else if (result == kSettingsMenuImportTheme) importThemeFile();
                            else if (result == kSettingsMenuExportTheme) exportThemeFile();
                        }));
}

void PluginEditor::newSong()
{
    processorRef.getSongManager().resetToDefault();
    processorRef.getStructureEngine().rebuild();
    processorRef.syncAuthoredSongToRuntime();
    m_currentSongFile = {};
    applyAuthoredSongToUi();
    systemFeed.addMessage ("New song");
}

void PluginEditor::openSong()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Open Spool Song",
                                                         m_currentSongFile.existsAsFile() ? m_currentSongFile : juce::File(),
                                                         "*.spool-song;*.json");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file == juce::File())
                                  return;

                              if (! processorRef.getSongManager().loadFromFile (file))
                              {
                                  systemFeed.addMessage ("Open failed: " + file.getFileName());
                                  return;
                              }

                              processorRef.getStructureEngine().rebuild();
                              processorRef.syncAuthoredSongToRuntime();
                              m_currentSongFile = file;
                              applyAuthoredSongToUi();
                              systemFeed.addMessage ("Opened song: " + file.getFileName());
                          });
}

void PluginEditor::saveSong()
{
    if (! m_currentSongFile.existsAsFile())
    {
        saveSongAs();
        return;
    }

    if (processorRef.getSongManager().saveToFile (m_currentSongFile))
        systemFeed.addMessage ("Saved song: " + m_currentSongFile.getFileName());
    else
        systemFeed.addMessage ("Save failed: " + m_currentSongFile.getFileName());
}

void PluginEditor::saveSongAs()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Save Spool Song",
                                                         m_currentSongFile.existsAsFile() ? m_currentSongFile
                                                                                          : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                                                                .getChildFile ("Untitled.spool-song"),
                                                         "*.spool-song");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                          | juce::FileBrowserComponent::canSelectFiles
                          | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (file == juce::File())
                                  return;

                              if (! file.hasFileExtension ("spool-song"))
                                  file = file.withFileExtension ("spool-song");

                              if (processorRef.getSongManager().saveToFile (file))
                              {
                                  m_currentSongFile = file;
                                  systemFeed.addMessage ("Saved song: " + file.getFileName());
                              }
                              else
                              {
                                  systemFeed.addMessage ("Save failed: " + file.getFileName());
                              }
                          });
}

void PluginEditor::importLyricsText()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Import Lyrics Text",
                                                         juce::File(),
                                                         "*.txt;*.md");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file == juce::File())
                                  return;

                              const auto importedLines = parseLyricImportLines (file.loadFileAsString());
                              if (importedLines.empty())
                              {
                                  systemFeed.addMessage ("No lyric text found: " + file.getFileName());
                                  return;
                              }

                              auto& lyrics = processorRef.getSongManager().getLyricsStateForEdit();
                              for (const auto& line : importedLines)
                              {
                                  LyricItem item;
                                  item.id = lyrics.nextId++;
                                  item.text = line;
                                  lyrics.items.push_back (item);
                              }

                              processorRef.getSongManager().commitAuthoredState();
                              processorRef.syncAuthoredSongToRuntime();
                              applyAuthoredSongToUi();
                              m_tabStrip.setActiveTab ("lyrics");
                              zoneA.setActivePanel ("lyrics");
                              m_contentPanelOpen = true;
                              resized();
                              systemFeed.addMessage ("Imported lyrics: " + juce::String (static_cast<int> (importedLines.size())) + " line(s)");
                          });
}

void PluginEditor::importThemeFile()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Import Theme",
                                                         ThemeManager::get().getUserThemeDir(),
                                                         "*.spool-theme;*.xml");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file == juce::File())
                                  return;

                              if (ThemeManager::get().loadFromFile (file))
                              {
                                  const auto presetName = ThemeManager::get().theme().presetName.trim();
                                  if (presetName.isNotEmpty())
                                      AppPreferences::get().setThemePresetName (presetName);
                                  systemFeed.addMessage ("Imported theme: " + file.getFileNameWithoutExtension());
                              }
                              else
                              {
                                  systemFeed.addMessage ("Theme import failed: " + file.getFileName());
                              }
                          });
}

void PluginEditor::exportThemeFile()
{
    auto chooser = std::make_shared<juce::FileChooser> ("Export Current Theme",
                                                         ThemeManager::get().getUserThemeDir().getChildFile ("Spool Theme.spool-theme"),
                                                         "*.spool-theme");
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                          | juce::FileBrowserComponent::canSelectFiles
                          | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (file == juce::File())
                                  return;

                              if (! file.hasFileExtension ("spool-theme"))
                                  file = file.withFileExtension ("spool-theme");

                              ThemeManager::get().saveToFile (file);
                              systemFeed.addMessage ("Exported theme: " + file.getFileName());
                          });
}

void PluginEditor::applyAuthoredSongToUi()
{
    auto& appState = processorRef.getAppState();
    const auto& authoredStructure = processorRef.getSongManager().getStructureState();

    songHeader.setCompactMode (AppPreferences::get().getCompactSongHeader());
    songHeader.setTitle (processorRef.getSongManager().getSongTitle());
    songHeader.setBpm (static_cast<float> (processorRef.getSongManager().getTempo()));

    if (auto* song = zoneA.getSongPanel())
    {
        song->setTitle (processorRef.getSongManager().getSongTitle());
        song->setBpm (static_cast<float> (processorRef.getSongManager().getTempo()));
        song->setKey (processorRef.getSongManager().getKeyRoot());
        song->setScale (processorRef.getSongManager().getKeyScale());
        song->setStructureSummary (authoredStructure);
    }

    if (auto* tracks = zoneA.getTracksPanel())
        tracks->setBpm (static_cast<float> (processorRef.getSongManager().getTempo()));

    if (auto* structurePanel = zoneA.getStructurePanel())
        structurePanel->setIntentKeyMode (processorRef.getSongManager().getKeyRoot(),
                                          processorRef.getSongManager().getKeyScale());

    if (auto* lyricsPanel = zoneA.getLyricsPanel())
        lyricsPanel->refreshFromModel();

    if (auto* autoPanel = zoneA.getAutoPanel())
        autoPanel->refreshFromModel();

    if (auto* structurePanel = zoneA.getStructurePanel())
        structurePanel->refreshFromModel();

    zoneD.setStructureView (&authoredStructure, &processorRef.getStructureEngine());
    zoneD.setAuthoredTimelineData (&processorRef.getSongManager().getLyricsState(),
                                   &processorRef.getSongManager().getAutomationState());
    zoneD.seedStructureRails (authoredStructure);
    zoneD.setStructureBeat (appState.transport.playheadBeats);
    zoneD.setStructureFollowState (computeFollowState (processorRef.isStructureFollowForTracksEnabled(),
                                                       structureHasScaffold (authoredStructure)));
    updateSequencerStructureContext (processorRef.getCurrentSongBeat());
    resized();
    repaint();
}

void PluginEditor::openSettingsPanel (juce::Rectangle<int> anchorBounds)
{
    if (anchorBounds.isEmpty())
        anchorBounds = menuBar.getAnchorBoundsForItem ("settings");

    if (m_settingsCallout != nullptr)
    {
        m_settingsCallout->dismiss();
        m_settingsCallout = nullptr;
    }

    auto settingsPanel = std::make_unique<SettingsPanel>();
    settingsPanel->onOpenThemeDesigner = [this] (juce::Rectangle<int> themeAnchor)
    {
        openThemeDesigner (themeAnchor);
    };
    settingsPanel->onImportTheme = [this]
    {
        importThemeFile();
    };
    settingsPanel->onExportTheme = [this]
    {
        exportThemeFile();
    };

    auto& box = juce::CallOutBox::launchAsynchronously (std::move (settingsPanel),
                                                        anchorBounds,
                                                        this);
    box.setDismissalMouseClicksAreAlwaysConsumed (true);
    m_settingsCallout = &box;
}

void PluginEditor::openThemeDesigner (juce::Rectangle<int> anchorBounds)
{
    if (m_settingsCallout != nullptr)
    {
        m_settingsCallout->dismiss();
        m_settingsCallout = nullptr;
    }

    if (m_themeDesignerWindow != nullptr)
    {
        m_themeDesignerWindow->exitModalState (0);
        m_themeDesignerWindow->setVisible (false);
        m_themeDesignerWindow = nullptr;
    }

    juce::ignoreUnused (anchorBounds);

    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned (new ThemeEditorModalContent());
    options.dialogTitle = "Theme Utility";
    options.dialogBackgroundColour = Theme::Colour::surface1;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = false;
    options.resizable = true;
    options.useBottomRightCornerResizer = true;
    options.componentToCentreAround = this;

    auto* dialog = options.launchAsync();
    if (dialog != nullptr)
    {
        dialog->setName ("Theme Utility");
        dialog->setResizable (true, true);
        dialog->centreWithSize (520, 760);
        m_themeDesignerWindow = dialog;
    }

    systemFeed.addMessage ("Theme utility opened");
}

void PluginEditor::themeChanged()
{
    const auto presetName = ThemeManager::get().theme().presetName.trim();
    if (presetName.isNotEmpty())
        AppPreferences::get().setThemePresetName (presetName);

    repaintAll (*this);
}

void PluginEditor::appPreferencesChanged()
{
    songHeader.setCompactMode (AppPreferences::get().getCompactSongHeader());
    zoneB.setLooperVisible (AppPreferences::get().getShowLooper());
    zoneC.setMacroStripVisible (AppPreferences::get().getShowZoneCMacros());
    resized();
    repaint();
}

void PluginEditor::repaintAll (juce::Component& root)
{
    root.repaint();
    for (int i = 0; i < root.getNumChildComponents(); ++i)
        repaintAll (*root.getChildComponent (i));
}

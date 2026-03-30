#include "PluginEditor.h"
#include "components/ZoneD/TimelineEditMath.h"
#include "instruments/drum/DrumModuleState.h"
#include "theme/ThemeEditorPanel.h"
#include <cmath>
#include <limits>

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
constexpr int kViewMenuTimelineDebug = 27;
constexpr int kSettingsMenuPreferences = 31;
constexpr int kSettingsMenuThemeDesigner = 32;
constexpr int kSettingsMenuImportTheme = 33;
constexpr int kSettingsMenuExportTheme = 34;
constexpr const char* kTimelineAssetsRootDir = "spool-media";
constexpr const char* kTimelineAssetsLeafDir = "timeline-clips";

juce::String makeTimelineClipId()
{
    auto id = juce::Uuid().toString()
                  .retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_");
    if (id.isEmpty())
        id = juce::String (juce::Time::currentTimeMillis());
    return id;
}

juce::Array<float> buildWaveformPreview (const juce::AudioBuffer<float>& buffer, int bins = 80)
{
    juce::Array<float> peaks;
    bins = juce::jlimit (8, 512, bins);
    if (buffer.getNumChannels() <= 0 || buffer.getNumSamples() <= 0)
        return peaks;

    peaks.ensureStorageAllocated (bins);
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    for (int b = 0; b < bins; ++b)
    {
        const int start = (b * numSamples) / bins;
        const int end = juce::jmax (start + 1, ((b + 1) * numSamples) / bins);
        float peak = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* src = buffer.getReadPointer (ch);
            for (int s = start; s < end; ++s)
                peak = juce::jmax (peak, std::abs (src[s]));
        }

        peaks.add (juce::jlimit (0.0f, 1.0f, peak));
    }

    return peaks;
}

CapturedAudioClip trimmedTimelineClip (const CapturedAudioClip& clip)
{
    constexpr float kSilenceThreshold = 0.0025f;
    if (clip.buffer.getNumChannels() <= 0 || clip.buffer.getNumSamples() <= 0)
        return clip;

    const int numChannels = clip.buffer.getNumChannels();
    const int numSamples = clip.buffer.getNumSamples();
    int first = numSamples;
    int last = -1;

    for (int s = 0; s < numSamples; ++s)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            peak = juce::jmax (peak, std::abs (clip.buffer.getSample (ch, s)));

        if (peak >= kSilenceThreshold)
        {
            first = juce::jmin (first, s);
            last = s;
        }
    }

    if (last < first)
        return clip;

    if (first == 0 && last == numSamples - 1)
        return clip;

    CapturedAudioClip trimmed = clip;
    const int trimmedSamples = juce::jmax (1, last - first + 1);
    trimmed.buffer.setSize (numChannels, trimmedSamples);
    for (int ch = 0; ch < numChannels; ++ch)
        trimmed.buffer.copyFrom (ch, 0, clip.buffer, ch, first, trimmedSamples);

    // Recompute inferred bars from trimmed duration.
    trimmed.bars = 0;
    if (trimmed.tempo > 0.0)
    {
        const double beatsPerBar = 4.0;
        const double barsExact = static_cast<double> (trimmed.durationSeconds()) * trimmed.tempo / 60.0 / beatsPerBar;
        trimmed.bars = juce::jmax (0, juce::roundToInt (barsExact));
    }

    return trimmed;
}

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

juce::var effectNodeToVar (const EffectNode& node)
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("effectType", node.effectType);
    obj->setProperty ("effectDomain", node.effectDomain);
    obj->setProperty ("bypassed", node.bypassed);

    juce::Array<juce::var> params;
    for (const auto p : node.params)
        params.add (p);
    obj->setProperty ("params", juce::var (params));
    return juce::var (obj.get());
}

bool effectNodeFromVar (const juce::var& value, EffectNode& out)
{
    auto* obj = value.getDynamicObject();
    if (obj == nullptr)
        return false;

    out.effectType = obj->getProperty ("effectType").toString();
    out.effectDomain = obj->getProperty ("effectDomain").toString();
    out.bypassed = static_cast<bool> (obj->getProperty ("bypassed"));
    out.params.clearQuick();

    if (auto* params = obj->getProperty ("params").getArray())
        for (const auto& param : *params)
            out.params.add (static_cast<float> (static_cast<double> (param)));

    if (out.effectDomain.isEmpty())
        out.effectDomain = "audio";
    return out.effectType.isNotEmpty();
}

juce::var chainStateToVar (const ChainState& chain)
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("slotIndex", chain.slotIndex);
    obj->setProperty ("initialised", chain.initialised);

    juce::Array<juce::var> nodes;
    for (const auto& node : chain.nodes)
        nodes.add (effectNodeToVar (node));
    obj->setProperty ("nodes", juce::var (nodes));
    return juce::var (obj.get());
}

bool chainStateFromVar (const juce::var& value, ChainState& out)
{
    auto* obj = value.getDynamicObject();
    if (obj == nullptr)
        return false;

    out.slotIndex = static_cast<int> (obj->getProperty ("slotIndex"));
    out.initialised = static_cast<bool> (obj->getProperty ("initialised"));
    out.nodes.clearQuick();

    if (auto* nodes = obj->getProperty ("nodes").getArray())
    {
        for (const auto& nodeVar : *nodes)
        {
            EffectNode node;
            if (effectNodeFromVar (nodeVar, node))
                out.nodes.add (node);
        }
    }

    if (! out.initialised && ! out.nodes.isEmpty())
        out.initialised = true;
    return true;
}

juce::String zoneCFxStateToJson (const ZoneCComponent::ChainBank& bank)
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    juce::Array<juce::var> insertChains;
    juce::Array<juce::var> busChains;

    for (const auto& chain : bank.inserts)
        insertChains.add (chainStateToVar (chain));
    for (const auto& chain : bank.buses)
        busChains.add (chainStateToVar (chain));

    root->setProperty ("insertChains", juce::var (insertChains));
    root->setProperty ("busChains", juce::var (busChains));
    root->setProperty ("masterChain", chainStateToVar (bank.master));
    return juce::JSON::toString (juce::var (root.get()), false);
}

bool zoneCFxStateFromJson (const juce::String& json, ZoneCComponent::ChainBank& out)
{
    const auto parsed = juce::JSON::parse (json);
    auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return false;

    out = {};

    if (auto* inserts = root->getProperty ("insertChains").getArray())
    {
        for (int i = 0; i < juce::jmin (8, inserts->size()); ++i)
            chainStateFromVar (inserts->getReference (i), out.inserts[(size_t) i]);
    }

    if (auto* buses = root->getProperty ("busChains").getArray())
    {
        for (int i = 0; i < juce::jmin (ZoneCComponent::kNumBusChains, buses->size()); ++i)
            chainStateFromVar (buses->getReference (i), out.buses[(size_t) i]);
    }

    chainStateFromVar (root->getProperty ("masterChain"), out.master);
    return true;
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
    setLookAndFeel (&m_lookAndFeel);
    setWantsKeyboardFocus (true);
    m_audioFormatManager.registerBasicFormats();

#if defined (VERSION)
    const juce::String buildVersion = juce::String (VERSION);
#else
    const juce::String buildVersion = "dev";
#endif
#if defined (CMAKE_BUILD_TYPE)
    const juce::String buildType = juce::String (CMAKE_BUILD_TYPE);
#else
    const juce::String buildType = "Release";
#endif
    menuBar.setBuildStamp ("v" + buildVersion + "  " + buildType + "  " + juce::String (__DATE__) + " " + juce::String (__TIME__));

    // Load last in-editor state if it exists, otherwise fall back to the named preset.
    if (!ThemeManager::get().loadFromFile (ThemeManager::get().getCurrentThemeFile()))
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
    zoneA.setOnRoutingBusSelected ([this] (const juce::String& busId)
    {
        if (busId == FXBusID::busA)   { zoneC.showBusContext (0); return; }
        if (busId == FXBusID::busB)   { zoneC.showBusContext (1); return; }
        if (busId == FXBusID::busC)   { zoneC.showBusContext (2); return; }
        if (busId == FXBusID::busD)   { zoneC.showBusContext (3); return; }
        zoneC.showMasterContext();
    });
    zoneA.setRoutingState (processorRef.getRoutingState());
    zoneA.setMidiRouter (&processorRef.getMidiRouter());
    zoneA.setRoutingAudioTick (&processorRef.getAudioTick());

    // Wire Zone B module list changes to patch bay, instrument browser, and mixer
    zoneB.onModuleListChanged = [this] (const juce::StringArray& names)
    {
        m_lastModuleNames = names;
        zoneA.setPatchModuleNames     (names);
        zoneA.setInstrumentSlots      (names);
        zoneA.setMixerSlots           (names);
        zoneA.setRoutingModuleNames   (names);
        zoneA.setTrackLanes         (names);
        applyTimelineLaneArmStateUi();
        syncAllSlotRuntimeFromModuleList();
        refreshTimelinePlacementsUi();
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
            tp->onRecord = [this]
            {
                const bool newState = ! processorRef.getAppState().transport.isRecording;
                zoneD.setRecording (newState);
            };
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
                if (lane < 0 || lane >= SpoolAudioGraph::kNumSlots)
                    return;

                m_timelineLaneArmed[static_cast<size_t> (lane)] = armed;
                processorRef.getSongManager().setTimelineLaneArmed (lane, armed);
                zoneD.armLane (lane, armed);
                if (auto* tracks = zoneA.getTracksPanel())
                    tracks->setLaneArmed (lane, armed);

                systemFeed.addMessage (juce::String ("Lane ")
                                       + juce::String (lane + 1).paddedLeft ('0', 2)
                                       + (armed ? " armed" : " disarmed"));
            };

            applyTimelineLaneArmStateUi();
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
                if (enabled)
                    zoneB.setFocusedPatternFollowStructure (true, true);
                updateSequencerStructureContext (processorRef.getCurrentSongBeat());
                if (enabled && ! hasStructure)
                    systemFeed.addMessage ("Structure follow armed (add sections to drive transposition)");
                else
                    systemFeed.addMessage (enabled ? "Structure follow enabled" : "Structure follow disabled");
            };

            sp->onStructureApplied = [this]
            {
                const auto& structure = processorRef.getSongManager().getStructureState();
                zoneD.setStructureView (&structure, &processorRef.getStructureEngine());
                zoneD.seedStructureRails (structure);
                zoneD.setStructureBeat (processorRef.getCurrentSongBeat());
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

    // Wire Zone C context-aware DSP callbacks (insert, bus, and master chains).
    zoneC.onContextDspParamChanged = [this] (ZoneCComponent::ChainContext context,
                                             int contextIndex,
                                             int nodeIdx,
                                             int paramIdx,
                                             float value)
    {
        switch (context)
        {
            case ZoneCComponent::ChainContext::insert:
                processorRef.setFXChainParam (contextIndex, nodeIdx, paramIdx, value);
                break;
            case ZoneCComponent::ChainContext::busA:
            case ZoneCComponent::ChainContext::busB:
            case ZoneCComponent::ChainContext::busC:
            case ZoneCComponent::ChainContext::busD:
                processorRef.setFXBusChainParam (contextIndex, nodeIdx, paramIdx, value);
                break;
            case ZoneCComponent::ChainContext::master:
                processorRef.setMasterFXChainParam (nodeIdx, paramIdx, value);
                break;
        }
        persistZoneCFxStateToSong();
    };
    zoneC.onContextChainRebuilt = [this] (ZoneCComponent::ChainContext context,
                                          int contextIndex,
                                          const ChainState& chain)
    {
        switch (context)
        {
            case ZoneCComponent::ChainContext::insert:
                processorRef.rebuildFXChain (contextIndex, chain);
                break;
            case ZoneCComponent::ChainContext::busA:
            case ZoneCComponent::ChainContext::busB:
            case ZoneCComponent::ChainContext::busC:
            case ZoneCComponent::ChainContext::busD:
                processorRef.rebuildFXBusChain (contextIndex, chain);
                break;
            case ZoneCComponent::ChainContext::master:
                processorRef.rebuildMasterFXChain (chain);
                break;
        }
        persistZoneCFxStateToSong();
    };

    // Wire Zone B step-pattern edits to the processor via a 40ms debounce.
    // During drag-paint gestures this can fire 10-20× per second; coalescing
    // prevents a full compileRuntimePattern() on every mouse-move event.
    zoneB.onPatternModified = [this] (int slotIdx, const SlotPattern& pattern)
    {
        if (slotIdx < 0 || slotIdx >= static_cast<int> (m_pendingPatterns.size()))
            return;
        m_pendingPatterns[(size_t) slotIdx] = pattern;
        m_patternDirty[(size_t) slotIdx] = true;
        if (! isTimerRunning())
            startTimer (40);
    };

    // Push a pattern undo action whenever a step-editing gesture completes.
    zoneB.onUndoableEdit = [this] (int slotIdx, SlotPattern before, SlotPattern after)
    {
        struct PatternEditAction : public juce::UndoableAction
        {
            ZoneBComponent& zb;
            int             slot;
            SlotPattern     bef, aft;

            PatternEditAction (ZoneBComponent& z, int s, SlotPattern b, SlotPattern a)
                : zb (z), slot (s), bef (std::move (b)), aft (std::move (a)) {}

            bool perform() override { zb.applyPatternForUndo (slot, aft); return true; }
            bool undo()    override { zb.applyPatternForUndo (slot, bef); return true; }
            int  getSizeInUnits() override { return 1; }
        };

        processorRef.getUndoManager().perform (
            new PatternEditAction (zoneB, slotIdx, std::move (before), std::move (after)));
    };

    // Push a drum-machine undo action whenever a drum edit gesture completes.
    zoneB.onDrumUndoableEdit = [this] (int slotIdx, const DrumMachineData& before, const DrumMachineData& after)
    {
        struct DrumEditAction : public juce::UndoableAction
        {
            ZoneBComponent& zb;
            int             slot;
            DrumMachineData bef, aft;

            DrumEditAction (ZoneBComponent& z, int s, DrumMachineData b, DrumMachineData a)
                : zb (z), slot (s), bef (std::move (b)), aft (std::move (a)) {}

            bool perform() override { zb.applyDrumStateForUndo (slot, aft); return true; }
            bool undo()    override { zb.applyDrumStateForUndo (slot, bef); return true; }
            int  getSizeInUnits() override { return 1; }
        };

        processorRef.getUndoManager().perform (
            new DrumEditAction (zoneB, slotIdx, before, after));
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
        if (!processorRef.hasGrabbedClip())
        {
            systemFeed.addMessage ("Timeline send needs a grabbed clip (use 4/8/16 or FREE first)");
            return;
        }
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
            if (!processorRef.hasGrabbedClip())
            {
                systemFeed.addMessage ("Timeline send needs a grabbed clip (use 4/8/16 or FREE first)");
                return;
            }
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
    zoneD.onClipDropped = [this] (int laneIndex)
    {
        if (! m_dragClip.has_value())
        {
            systemFeed.addMessage ("Timeline drop ignored: no clip payload");
            return;
        }

        const int targetSlot = (laneIndex >= 0) ? laneIndex
                                                : ((m_focusedSlotIndex >= 0) ? m_focusedSlotIndex : 0);
        routeClipToTimeline (*m_dragClip, targetSlot);
        m_dragClip.reset();
    };
    zoneD.onTimelineClipEdited = [this] (int laneIndex,
                                         const juce::String& clipId,
                                         float startBeat,
                                         float lengthBeats)
    {
        auto placements = processorRef.getSongManager().getTimelinePlacements();
        bool changed = false;
        const auto loopBeats = static_cast<float> (juce::jmax (1.0,
                                                                zoneD.loopLengthBars()
                                                                * juce::jmax (1.0, processorRef.getBeatsPerBar())));
        static constexpr float kMinLen = 0.25f;

        if (clipId.isNotEmpty())
        {
            for (auto& placement : placements)
            {
                if (placement.clipId == clipId)
                {
                    const auto oldStart = placement.startBeat;
                    const auto oldLen = placement.lengthBeats;
                    placement.laneIndex = juce::jlimit (0, SpoolAudioGraph::kNumSlots - 1, laneIndex);
                    placement.startBeat = TimelineEditMath::wrapBeat (startBeat, loopBeats);
                    placement.lengthBeats = juce::jmax (kMinLen, lengthBeats);
                    placement.sourceTotalBeats = placement.sourceTotalBeats > 0.0f
                                                     ? placement.sourceTotalBeats
                                                     : juce::jmax (kMinLen, oldLen);
                    placement.sourceTotalBeats = juce::jmax (placement.sourceTotalBeats, placement.lengthBeats);
                    placement.sourceStartBeat = juce::jmax (0.0f, placement.sourceStartBeat);

                    const bool lengthChanged = std::abs (placement.lengthBeats - oldLen) > 0.0001f;
                    const bool startChanged = std::abs (placement.startBeat - oldStart) > 0.0001f;
                    if (lengthChanged && startChanged)
                    {
                        float delta = placement.startBeat - oldStart;
                        if (delta < -loopBeats * 0.5f)
                            delta += loopBeats;
                        else if (delta > loopBeats * 0.5f)
                            delta -= loopBeats;
                        placement.sourceStartBeat = juce::jmax (0.0f, placement.sourceStartBeat + delta);
                    }

                    placement.sourceStartBeat = juce::jlimit (0.0f,
                                                              juce::jmax (0.0f, placement.sourceTotalBeats - placement.lengthBeats),
                                                              placement.sourceStartBeat);
                    changed = true;
                    break;
                }
            }
        }
        else if (laneIndex >= 0 && laneIndex < SpoolAudioGraph::kNumSlots)
        {
            int unresolvedLanePlacements = 0;
            int unresolvedPlacementIndex = -1;
            for (int i = 0; i < placements.size(); ++i)
            {
                const auto& placement = placements.getReference (i);
                if (placement.laneIndex == laneIndex && placement.clipId.trim().isEmpty())
                {
                    ++unresolvedLanePlacements;
                    unresolvedPlacementIndex = i;
                }
            }

            if (unresolvedLanePlacements == 1 && unresolvedPlacementIndex >= 0)
            {
                auto& placement = placements.getReference (unresolvedPlacementIndex);
                const auto oldStart = placement.startBeat;
                const auto oldLen = placement.lengthBeats;
                placement.startBeat = TimelineEditMath::wrapBeat (startBeat, loopBeats);
                placement.lengthBeats = juce::jmax (kMinLen, lengthBeats);
                placement.sourceTotalBeats = placement.sourceTotalBeats > 0.0f
                                                 ? placement.sourceTotalBeats
                                                 : juce::jmax (kMinLen, oldLen);
                placement.sourceTotalBeats = juce::jmax (placement.sourceTotalBeats, placement.lengthBeats);
                placement.sourceStartBeat = juce::jmax (0.0f, placement.sourceStartBeat);

                const bool lengthChanged = std::abs (placement.lengthBeats - oldLen) > 0.0001f;
                const bool startChanged = std::abs (placement.startBeat - oldStart) > 0.0001f;
                if (lengthChanged && startChanged)
                {
                    float delta = placement.startBeat - oldStart;
                    if (delta < -loopBeats * 0.5f)
                        delta += loopBeats;
                    else if (delta > loopBeats * 0.5f)
                        delta -= loopBeats;
                    placement.sourceStartBeat = juce::jmax (0.0f, placement.sourceStartBeat + delta);
                }

                placement.sourceStartBeat = juce::jlimit (0.0f,
                                                          juce::jmax (0.0f, placement.sourceTotalBeats - placement.lengthBeats),
                                                          placement.sourceStartBeat);
                changed = true;
            }
        }

        if (! changed)
        {
            systemFeed.addMessage ("Timeline edit skipped: clip id not resolved.");
            return;
        }

        processorRef.getSongManager().replaceTimelinePlacements (placements);
        refreshTimelinePlacementsUi();
        applyTimelineLaneArmStateUi();
    };
    zoneD.onTimelineClipSplitRequested = [this] (int laneIndex,
                                                 const juce::String& clipId,
                                                 float splitBeat)
    {
        auto placements = processorRef.getSongManager().getTimelinePlacements();
        const auto loopBeats = static_cast<float> (juce::jmax (1.0,
                                                                zoneD.loopLengthBars()
                                                                * juce::jmax (1.0, processorRef.getBeatsPerBar())));
        static constexpr float kMinLen = 0.25f;
        int splitIndex = -1;

        if (clipId.isNotEmpty())
        {
            for (int i = 0; i < placements.size(); ++i)
            {
                if (placements.getReference (i).clipId == clipId)
                {
                    splitIndex = i;
                    break;
                }
            }
        }
        else if (laneIndex >= 0 && laneIndex < SpoolAudioGraph::kNumSlots)
        {
            int unresolvedOnLane = 0;
            for (int i = 0; i < placements.size(); ++i)
            {
                const auto& placement = placements.getReference (i);
                if (placement.laneIndex == laneIndex && placement.clipId.trim().isEmpty())
                {
                    ++unresolvedOnLane;
                    splitIndex = i;
                }
            }
            if (unresolvedOnLane != 1)
                splitIndex = -1;
        }

        if (splitIndex < 0 || splitIndex >= placements.size())
        {
            systemFeed.addMessage ("Split skipped: clip id not resolved.");
            return;
        }

        auto original = placements.getReference (splitIndex);
        float splitOffset = 0.0f;
        if (! TimelineEditMath::splitOffsetWithinClip (original.startBeat,
                                                       original.lengthBeats,
                                                       splitBeat,
                                                       loopBeats,
                                                       kMinLen,
                                                       splitOffset))
        {
            systemFeed.addMessage ("Split skipped: playhead not inside clip body.");
            return;
        }

        auto first = original;
        auto second = original;
        first.lengthBeats = splitOffset;
        second.startBeat = TimelineEditMath::wrapBeat (original.startBeat + splitOffset, loopBeats);
        second.lengthBeats = juce::jmax (kMinLen, original.lengthBeats - splitOffset);
        second.clipId = makeTimelineClipId();

        const float sourceTotal = original.sourceTotalBeats > 0.0f
                                      ? original.sourceTotalBeats
                                      : juce::jmax (kMinLen, original.lengthBeats);
        const float sourceStart = juce::jmax (0.0f, original.sourceStartBeat);
        first.sourceTotalBeats = sourceTotal;
        second.sourceTotalBeats = sourceTotal;
        first.sourceStartBeat = juce::jlimit (0.0f,
                                              juce::jmax (0.0f, sourceTotal - first.lengthBeats),
                                              sourceStart);
        second.sourceStartBeat = juce::jlimit (0.0f,
                                               juce::jmax (0.0f, sourceTotal - second.lengthBeats),
                                               sourceStart + splitOffset);

        placements.set (splitIndex, first);
        placements.insert (splitIndex + 1, second);

        if (clipId.isNotEmpty())
        {
            if (const auto* clipAudio = findTimelineClipAudio (clipId))
                upsertTimelineClipAudio (second.clipId, *clipAudio);
        }

        processorRef.getSongManager().replaceTimelinePlacements (placements);
        refreshTimelinePlacementsUi();
        applyTimelineLaneArmStateUi();
        systemFeed.addMessage ("Split clip at beat " + juce::String (splitBeat, 2));
    };
    zoneD.onScrubBeatChanged = [this] (float beat)
    {
        positionChanged (beat);
    };
    zoneD.onLoopLengthChanged = [this] (float bars)
    {
        const auto beatsPerBar = juce::jmax (1.0, processorRef.getBeatsPerBar());
        processorRef.setTimelineLoopLengthBeats (static_cast<double> (bars) * beatsPerBar);
    };
    processorRef.setTimelineLoopLengthBeats (static_cast<double> (zoneD.loopLengthBars())
                                             * juce::jmax (1.0, processorRef.getBeatsPerBar()));

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

    // Startup baseline: begin empty (no seeded module patterns, transport stopped).
    processorRef.setPlaying (false);
    zoneD.setPlaying (false);
    if (auto* tp = zoneA.getTracksPanel())
        tp->setPlaying (false);
    systemFeed.addMessage ("Startup: empty project");

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

    if (juce::JUCEApplicationBase::isStandaloneApp())
    {
        m_splash = std::make_unique<SplashComponent>();
        m_splash->onFinished = [this]
        {
            if (m_splash == nullptr)
                return;

            removeChildComponent (m_splash.get());
            m_splash.reset();
        };
        addAndMakeVisible (*m_splash);
        m_splash->setBounds (getLocalBounds());
        m_splash->toFront (false);
    }

    m_inspector = std::make_unique<melatonin::Inspector> (*this, false);
}

PluginEditor::~PluginEditor()
{
    // Stop debounce timer and flush any pending pattern compile before teardown.
    stopTimer();
    flushPendingPatterns();

    // Clear processor callback before any member teardown to prevent a queued
    // AsyncUpdater from firing into a partially-destroyed editor.
    processorRef.onStepAdvanced = nullptr;

    if (m_splash != nullptr)
        m_splash->onFinished = {};

    setLookAndFeel (nullptr);
    zoneB.removeListener (this);
    zoneD.removeTransportListener (this);
    ThemeManager::get().removeListener (this);
    AppPreferences::get().removeListener (this);
}

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);
}

void PluginEditor::paintOverChildren (juce::Graphics& g)
{
    if (m_timelineDebugOverlayEnabled)
        paintTimelineDebugOverlay (g);
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

    if (key.getTextCharacter() == 'i' || key.getKeyCode() == 'I')
    {
        m_inspector->toggle();
        return true;
    }

    if (key.getModifiers().isShiftDown()
        && (key.getTextCharacter() == 'd' || key.getTextCharacter() == 'D' || key.getKeyCode() == 'D'))
    {
        toggleTimelineDebugOverlay();
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

    if (m_splash != nullptr)
    {
        m_splash->setBounds (getLocalBounds());
        m_splash->toFront (false);
    }
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
    if (! playing)
        m_hasObservedProcessorBeat = false;
    if (playing)
        systemFeed.addMessage ("Transport: playing  "
                               + juce::String (static_cast<int> (processorRef.getBpm()))
                               + " BPM");
    else
        systemFeed.addMessage ("Transport: stopped");

    if (m_timelineDebugOverlayEnabled)
        repaint();
}

void PluginEditor::onStepAdvanced (int step)
{
    zoneB.setPlayheadStep (step);
}

void PluginEditor::timerCallback()
{
    stopTimer();
    flushPendingPatterns();
}

void PluginEditor::flushPendingPatterns()
{
    for (int slot = 0; slot < static_cast<int> (m_patternDirty.size()); ++slot)
    {
        if (! m_patternDirty[(size_t) slot])
            continue;

        m_patternDirty[(size_t) slot] = false;
        const auto& pattern = m_pendingPatterns[(size_t) slot];
        const int stepCount = pattern.activeStepCount();
        processorRef.setStepCount (slot, stepCount);
        for (int s = 0; s < SlotPattern::MAX_STEPS; ++s)
            processorRef.setStepActive (slot, s, s < stepCount && pattern.stepActive (s));
        processorRef.setSlotPattern (slot, pattern);
    }
}

void PluginEditor::positionChanged (float beat)
{
    if (! processorRef.isPlaying())
        processorRef.seekTransport (static_cast<double> (beat));

    const double processorBeat = processorRef.getCurrentSongBeat();
    double wrappedStructureBeat = processorBeat;
    {
        const juce::ScopedReadLock structureRead (processorRef.getStructureLock());
        const double totalBeats = static_cast<double> (juce::jmax (0, processorRef.getSongManager().getStructureState().totalBeats));
        if (totalBeats > 0.0)
            wrappedStructureBeat = std::fmod (juce::jmax (0.0, processorBeat), totalBeats);
    }

    if (processorRef.isPlaying())
    {
        if (m_hasObservedProcessorBeat && (processorBeat + 1.0e-6) < m_lastObservedProcessorBeat)
        {
            const auto nowMs = juce::Time::currentTimeMillis();
            if ((nowMs - m_lastBackstepWarningMs) > 1000)
            {
                systemFeed.addMessage ("Playhead warning: beat moved backwards from "
                                       + juce::String (m_lastObservedProcessorBeat, 3)
                                       + " to "
                                       + juce::String (processorBeat, 3));
                m_lastBackstepWarningMs = nowMs;
            }
            DBG ("[SPOOL] playhead backstep while playing: "
                 << m_lastObservedProcessorBeat << " -> " << processorBeat);
        }

        m_lastObservedProcessorBeat = processorBeat;
        m_hasObservedProcessorBeat = true;
    }

    if (auto* tp = zoneA.getTracksPanel())
        tp->setPosition (static_cast<float> (processorBeat));
    // Feed Zone D absolute beat; sub-components can wrap for loop visuals as needed.
    zoneD.setStructureBeat (processorBeat);
    updateSequencerStructureContext (wrappedStructureBeat);

    if (m_timelineDebugOverlayEnabled)
        repaint();
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

    systemFeed.addMessage (recording
                               ? "Record armed (use History grab controls to capture clips)"
                               : "Record disarmed");
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

void PluginEditor::routeClipToTimeline (const CapturedAudioClip& clip, std::optional<int> targetSlotOverride)
{
    const auto routedClip = trimmedTimelineClip (clip);
    const auto beatsPerBar = juce::jmax (1.0, processorRef.getBeatsPerBar());
    const auto loopLengthBeats = juce::jmax (1.0, static_cast<double> (zoneD.loopLengthBars()) * beatsPerBar);
    const auto timelineNowBeat = juce::jmax (0.0, processorRef.getCurrentSongBeat());
    const auto startBeat = processorRef.isPlaying()
                               ? std::fmod (timelineNowBeat, loopLengthBeats)
                               : 0.0;
    const int preferredSlot = targetSlotOverride.has_value()
                                  ? *targetSlotOverride
                                  : preferredArmedTimelineLane (routedClip);
    const auto targetSlot = juce::jlimit (0, SpoolAudioGraph::kNumSlots - 1, preferredSlot);

    auto lengthBeats = 0.0;
    const auto tempo = routedClip.tempo > 0.0 ? routedClip.tempo : static_cast<double> (processorRef.getBpm());
    if (tempo > 0.0 && routedClip.durationSeconds() > 0.0f)
        lengthBeats = static_cast<double> (routedClip.durationSeconds()) * tempo / 60.0;
    else if (routedClip.bars > 0)
        lengthBeats = static_cast<double> (routedClip.bars) * beatsPerBar;
    lengthBeats = juce::jmax (0.25, lengthBeats);
    lengthBeats = juce::jmin (lengthBeats, loopLengthBeats);

    Clip timelineClip;
    timelineClip.startBeat = static_cast<float> (startBeat);
    timelineClip.lengthBeats = static_cast<float> (lengthBeats);
    timelineClip.name = routedClip.sourceName.isNotEmpty() ? routedClip.sourceName : "CAPTURE";
    timelineClip.type = ClipType::audio;
    timelineClip.tint = Theme::Signal::audio.withAlpha (0.92f);
    timelineClip.waveformPreview = buildWaveformPreview (routedClip.buffer);

    const auto moduleType = moduleTypeForSlot (targetSlot);
    zoneD.setLaneInfo (targetSlot, true, moduleType);
    zoneD.addTimelineClip (targetSlot, timelineClip, moduleType);

    TimelineClipPlacement placementModel;
    placementModel.laneIndex = targetSlot;
    placementModel.moduleType = moduleType;
    placementModel.clipName = timelineClip.name;
    placementModel.clipId = makeTimelineClipId();
    timelineClip.clipId = placementModel.clipId;
    if (m_currentSongFile.existsAsFile())
    {
        const auto assetFile = timelineAssetFileFor (m_currentSongFile, placementModel.clipId);
        placementModel.audioAssetPath = assetFile.getRelativePathFrom (m_currentSongFile.getParentDirectory());
    }
    placementModel.startBeat = timelineClip.startBeat;
    placementModel.lengthBeats = timelineClip.lengthBeats;
    placementModel.sourceStartBeat = 0.0f;
    placementModel.sourceTotalBeats = placementModel.lengthBeats;
    processorRef.getSongManager().addTimelinePlacement (placementModel);
    upsertTimelineClipAudio (placementModel.clipId, routedClip);
    processorRef.addTimelineAudioClip (routedClip,
                                       startBeat,
                                       lengthBeats,
                                       targetSlot,
                                       moduleType,
                                       timelineClip.name,
                                       placementModel.sourceStartBeat,
                                       placementModel.sourceTotalBeats);

    m_contentPanelOpen = true;
    m_tabStrip.setActiveTab ("tracks");
    zoneA.setActivePanel ("tracks");
    resized();

    if (auto* tracks = zoneA.getTracksPanel())
    {
        TracksPanel::TimelinePlacement placementUi;
        placementUi.laneIndex = targetSlot;
        placementUi.laneName = moduleType + " " + juce::String (targetSlot + 1).paddedLeft ('0', 2);
        placementUi.clipName = timelineClip.name;
        placementUi.startBeat = timelineClip.startBeat;
        placementUi.lengthBeats = timelineClip.lengthBeats;
        tracks->addTimelinePlacement (placementUi);
    }

    const auto startBar = juce::jmax (1, juce::roundToInt (std::floor (startBeat / beatsPerBar)) + 1);
    const auto barLen = static_cast<float> (lengthBeats / beatsPerBar);
    juce::String msg = "TIMELINE placed " + timelineClip.name
                       + " -> lane " + juce::String (targetSlot + 1).paddedLeft ('0', 2)
                       + " @ bar " + juce::String (startBar)
                       + " (" + juce::String (barLen, 1) + " bars)";
    systemFeed.addMessage (msg);

    updateSequencerStructureContext (processorRef.getCurrentSongBeat());
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
    for (int slotIndex = 0; slotIndex < SpoolAudioGraph::kNumSlots; ++slotIndex)
    {
        juce::String moduleType;
        if (slotIndex < m_lastModuleNames.size())
            moduleType = m_lastModuleNames[slotIndex].fromLastOccurrenceOf (":", false, false);
        syncSlotRuntimeFromModuleType (slotIndex, moduleType);
    }
}

juce::String PluginEditor::moduleTypeForSlot (int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= m_lastModuleNames.size())
        return "AUDIO";

    const auto type = m_lastModuleNames[slotIndex].fromLastOccurrenceOf (":", false, false).trim();
    return type.isNotEmpty() ? type : juce::String ("AUDIO");
}

void PluginEditor::refreshTimelinePlacementsUi()
{
    processorRef.clearTimelineAudioClips();
    zoneD.clearTimelineClips();

    if (auto* tracks = zoneA.getTracksPanel())
        tracks->clearTimelinePlacements();

    const auto& placements = processorRef.getSongManager().getTimelinePlacements();
    for (const auto& placement : placements)
    {
        const int slotIndex = juce::jlimit (0, SpoolAudioGraph::kNumSlots - 1, placement.laneIndex);
        const auto moduleType = placement.moduleType.isNotEmpty()
                                    ? placement.moduleType
                                    : moduleTypeForSlot (slotIndex);

        Clip clip;
        clip.startBeat = juce::jmax (0.0f, placement.startBeat);
        clip.lengthBeats = juce::jmax (0.25f, placement.lengthBeats);
        clip.clipId = placement.clipId;
        clip.name = placement.clipName.isNotEmpty() ? placement.clipName : "CAPTURE";
        clip.type = ClipType::audio;
        clip.tint = Theme::Signal::audio.withAlpha (0.92f);

        const CapturedAudioClip* cachedClip = placement.clipId.isNotEmpty()
                                                  ? findTimelineClipAudio (placement.clipId)
                                                  : nullptr;
        CapturedAudioClip loadedClip;
        if (cachedClip == nullptr
            && placement.audioAssetPath.isNotEmpty()
            && loadTimelineClipAudioFromFile (placement, loadedClip))
        {
            const auto normalizedLoadedClip = trimmedTimelineClip (loadedClip);
            if (placement.clipId.isNotEmpty())
            {
                upsertTimelineClipAudio (placement.clipId, normalizedLoadedClip);
                cachedClip = findTimelineClipAudio (placement.clipId);
            }
            else
            {
                loadedClip = normalizedLoadedClip;
                cachedClip = &loadedClip;
            }
        }

        if (cachedClip != nullptr)
        {
            clip.waveformPreview = buildWaveformPreview (cachedClip->buffer);
            processorRef.addTimelineAudioClip (*cachedClip,
                                               placement.startBeat,
                                               placement.lengthBeats,
                                               slotIndex,
                                               moduleType,
                                               clip.name,
                                               placement.sourceStartBeat,
                                               placement.sourceTotalBeats);
        }

        zoneD.setLaneInfo (slotIndex, true, moduleType);
        zoneD.addTimelineClip (slotIndex, clip, moduleType);

        if (auto* tracks = zoneA.getTracksPanel())
        {
            TracksPanel::TimelinePlacement placementUi;
            placementUi.laneIndex = slotIndex;
            placementUi.laneName = moduleType + " " + juce::String (slotIndex + 1).paddedLeft ('0', 2);
            placementUi.clipName = clip.name;
            placementUi.startBeat = clip.startBeat;
            placementUi.lengthBeats = clip.lengthBeats;
            tracks->addTimelinePlacement (placementUi);
        }
    }
}

void PluginEditor::applyTimelineLaneArmStateUi()
{
    for (int lane = 0; lane < SpoolAudioGraph::kNumSlots; ++lane)
    {
        const bool armed = m_timelineLaneArmed[static_cast<size_t> (lane)];
        zoneD.armLane (lane, armed);
        if (auto* tracks = zoneA.getTracksPanel())
            tracks->setLaneArmed (lane, armed);
    }
}

int PluginEditor::preferredArmedTimelineLane (const CapturedAudioClip& clip) const
{
    auto isArmed = [this] (int lane) -> bool
    {
        return lane >= 0
               && lane < SpoolAudioGraph::kNumSlots
               && m_timelineLaneArmed[static_cast<size_t> (lane)];
    };

    if (isArmed (clip.sourceSlot))
        return clip.sourceSlot;

    if (isArmed (m_focusedSlotIndex))
        return m_focusedSlotIndex;

    for (int lane = 0; lane < SpoolAudioGraph::kNumSlots; ++lane)
        if (isArmed (lane))
            return lane;

    if (clip.sourceSlot >= 0)
        return clip.sourceSlot;
    if (m_focusedSlotIndex >= 0)
        return m_focusedSlotIndex;
    return 0;
}

juce::String PluginEditor::ensureTimelineClipId (TimelineClipPlacement& placement)
{
    if (placement.clipId.trim().isEmpty())
        placement.clipId = makeTimelineClipId();
    return placement.clipId;
}

juce::File PluginEditor::timelineAssetFileFor (const juce::File& songFile, const juce::String& clipId) const
{
    auto safeClipId = clipId.retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_");
    if (safeClipId.isEmpty())
        safeClipId = makeTimelineClipId();

    return songFile.getParentDirectory()
        .getChildFile (kTimelineAssetsRootDir)
        .getChildFile (songFile.getFileNameWithoutExtension())
        .getChildFile (kTimelineAssetsLeafDir)
        .getChildFile (safeClipId + ".wav");
}

const CapturedAudioClip* PluginEditor::findTimelineClipAudio (const juce::String& clipId) const
{
    for (const auto& entry : m_timelineClipAudioCache)
    {
        if (entry.clipId == clipId)
            return &entry.clip;
    }

    return nullptr;
}

void PluginEditor::upsertTimelineClipAudio (const juce::String& clipId, const CapturedAudioClip& clip)
{
    if (clipId.trim().isEmpty())
        return;

    for (auto& entry : m_timelineClipAudioCache)
    {
        if (entry.clipId == clipId)
        {
            entry.clip = clip;
            return;
        }
    }

    TimelineClipAudioCacheEntry entry;
    entry.clipId = clipId;
    entry.clip = clip;
    m_timelineClipAudioCache.add (std::move (entry));
}

bool PluginEditor::loadTimelineClipAudioFromFile (const TimelineClipPlacement& placement, CapturedAudioClip& outClip)
{
    if (! m_currentSongFile.existsAsFile() || placement.audioAssetPath.trim().isEmpty())
        return false;

    const auto audioFile = m_currentSongFile.getParentDirectory().getChildFile (placement.audioAssetPath);
    if (! audioFile.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader (m_audioFormatManager.createReaderFor (audioFile));
    if (reader == nullptr || reader->numChannels <= 0 || reader->lengthInSamples <= 0)
        return false;

    outClip.buffer.setSize (static_cast<int> (reader->numChannels),
                            static_cast<int> (reader->lengthInSamples));
    if (! reader->read (&outClip.buffer,
                        0,
                        static_cast<int> (reader->lengthInSamples),
                        0,
                        true,
                        true))
        return false;

    outClip.sampleRate = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
    outClip.sourceName = placement.clipName.isNotEmpty() ? placement.clipName : juce::String ("CAPTURE");
    outClip.sourceSlot = placement.laneIndex;
    outClip.tempo = static_cast<double> (processorRef.getSongManager().getTempo());
    const auto beatsPerBar = juce::jmax (1.0, processorRef.getBeatsPerBar());
    if (outClip.tempo > 0.0 && beatsPerBar > 0.0)
    {
        const double barsExact = outClip.durationSeconds() * outClip.tempo / 60.0 / beatsPerBar;
        outClip.bars = juce::jmax (1, juce::roundToInt (barsExact));
    }

    return true;
}

bool PluginEditor::saveTimelineClipAudioToFile (const CapturedAudioClip& clip, const juce::File& file) const
{
    if (clip.buffer.getNumChannels() <= 0 || clip.buffer.getNumSamples() <= 0)
        return false;

    const auto parent = file.getParentDirectory();
    if (! parent.exists())
    {
        auto createResult = parent.createDirectory();
        if (createResult.failed())
            return false;
    }

    juce::WavAudioFormat format;
    std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
        return false;

    const auto options = juce::AudioFormatWriterOptions {}
                             .withSampleRate (clip.sampleRate > 0.0 ? clip.sampleRate : 44100.0)
                             .withNumChannels (clip.buffer.getNumChannels())
                             .withBitsPerSample (24);
    auto writer = format.createWriterFor (stream, options);
    if (writer == nullptr)
        return false;

    return writer->writeFromAudioSampleBuffer (clip.buffer, 0, clip.buffer.getNumSamples());
}

bool PluginEditor::writeTimelineAudioAssetsForSong (const juce::File& songFile)
{
    auto placements = processorRef.getSongManager().getTimelinePlacements();
    if (placements.isEmpty())
        return true;

    bool placementsChanged = false;
    int missingClips = 0;
    int failedWrites = 0;

    for (auto& placement : placements)
    {
        const bool hadClipId = placement.clipId.isNotEmpty();
        auto clipId = ensureTimelineClipId (placement);
        if (! hadClipId)
            placementsChanged = true;

        const auto previousAssetPath = placement.audioAssetPath;
        const auto targetAssetFile = timelineAssetFileFor (songFile, clipId);
        const auto targetRelativePath = targetAssetFile.getRelativePathFrom (songFile.getParentDirectory());
        if (placement.audioAssetPath != targetRelativePath)
        {
            placement.audioAssetPath = targetRelativePath;
            placementsChanged = true;
        }

        const CapturedAudioClip* cachedClip = findTimelineClipAudio (clipId);
        CapturedAudioClip loadedClip;
        if (cachedClip == nullptr && previousAssetPath.isNotEmpty() && m_currentSongFile.existsAsFile())
        {
            TimelineClipPlacement lookupPlacement = placement;
            lookupPlacement.audioAssetPath = previousAssetPath;
            if (loadTimelineClipAudioFromFile (lookupPlacement, loadedClip))
            {
                upsertTimelineClipAudio (clipId, trimmedTimelineClip (loadedClip));
                cachedClip = findTimelineClipAudio (clipId);
            }
        }

        if (cachedClip == nullptr)
        {
            ++missingClips;
            continue;
        }

        if (! saveTimelineClipAudioToFile (*cachedClip, targetAssetFile))
            ++failedWrites;
    }

    if (placementsChanged)
        processorRef.getSongManager().replaceTimelinePlacements (placements);

    return missingClips == 0 && failedWrites == 0;
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
    menu.addItem (kViewMenuTimelineDebug, "Timeline Debug Overlay", true, m_timelineDebugOverlayEnabled);
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
                            else if (result == kViewMenuTimelineDebug) toggleTimelineDebugOverlay();
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

void PluginEditor::toggleTimelineDebugOverlay()
{
    m_timelineDebugOverlayEnabled = ! m_timelineDebugOverlayEnabled;
    systemFeed.addMessage (juce::String ("Timeline Debug overlay ")
                           + (m_timelineDebugOverlayEnabled ? "enabled" : "disabled")
                           + " (Cmd/Ctrl+Shift+D)");
    repaint();
}

void PluginEditor::paintTimelineDebugOverlay (juce::Graphics& g) const
{
    const auto& placements = processorRef.getSongManager().getTimelinePlacements();
    const double beatsPerBar = juce::jmax (1.0, processorRef.getBeatsPerBar());
    const double loopBeats = juce::jmax (1.0, static_cast<double> (zoneD.loopLengthBars()) * beatsPerBar);
    const double processorBeat = juce::jmax (0.0, processorRef.getCurrentSongBeat());

    auto wrapPositive = [] (double value, double modulus)
    {
        if (modulus <= 0.0)
            return value;

        auto wrapped = std::fmod (value, modulus);
        if (wrapped < 0.0)
            wrapped += modulus;
        return wrapped;
    };

    const double beatInLoop = wrapPositive (processorBeat, loopBeats);

    struct Match
    {
        int index = -1;
        bool active = false;
        double score = std::numeric_limits<double>::max();
        double startBeat = 0.0;
        double lengthBeats = 0.0;
        double relativeBeat = 0.0;
        juce::String name;
    };

    Match chosen;

    auto containsBeat = [loopBeats] (double beat, double startBeat, double lengthBeats) noexcept
    {
        if (lengthBeats >= loopBeats - 1.0e-6)
            return true;

        const double endBeat = startBeat + lengthBeats;
        if (endBeat <= loopBeats)
            return beat >= startBeat && beat < endBeat;

        const double wrappedEnd = endBeat - loopBeats;
        return beat >= startBeat || beat < wrappedEnd;
    };

    for (int i = 0; i < placements.size(); ++i)
    {
        const auto& placement = placements.getReference (i);
        const double startBeat = wrapPositive (placement.startBeat, loopBeats);
        const double lengthBeats = juce::jlimit (0.25, loopBeats,
                                                 static_cast<double> (placement.lengthBeats));
        const double relativeBeat = wrapPositive (beatInLoop - startBeat, loopBeats);
        const double centerBeat = wrapPositive (startBeat + lengthBeats * 0.5, loopBeats);
        double score = std::abs (beatInLoop - centerBeat);
        score = juce::jmin (score, loopBeats - score);
        const bool isActive = containsBeat (beatInLoop, startBeat, lengthBeats);

        if (isActive)
        {
            if (! chosen.active || score < chosen.score)
            {
                chosen.index = i;
                chosen.active = true;
                chosen.score = score;
                chosen.startBeat = startBeat;
                chosen.lengthBeats = lengthBeats;
                chosen.relativeBeat = relativeBeat;
                chosen.name = placement.clipName.isNotEmpty() ? placement.clipName : "CAPTURE";
            }
        }
        else if (! chosen.active && score < chosen.score)
        {
            chosen.index = i;
            chosen.score = score;
            chosen.startBeat = startBeat;
            chosen.lengthBeats = lengthBeats;
            chosen.relativeBeat = relativeBeat;
            chosen.name = placement.clipName.isNotEmpty() ? placement.clipName : "CAPTURE";
        }
    }

    const int panelW = 300;
    const int panelH = 102;
    const int x = juce::jmax (10, getWidth() - panelW - 10);
    const int y = juce::jmax (topOffset() + 8, 8);
    const juce::Rectangle<float> panel ((float) x, (float) y, (float) panelW, (float) panelH);
    const auto inner = panel.reduced (10.0f);

    g.setColour (Theme::Colour::surface1.withAlpha (0.88f));
    g.fillRoundedRectangle (panel, 6.0f);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.62f));
    g.drawRoundedRectangle (panel, 6.0f, 1.0f);

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkLight.withAlpha (0.95f));

    auto drawLine = [&] (int lineIndex, const juce::String& text)
    {
        const float lineH = 13.0f;
        const juce::Rectangle<int> r (juce::roundToInt (inner.getX()),
                                      juce::roundToInt (inner.getY() + lineIndex * lineH),
                                      juce::roundToInt (inner.getWidth()),
                                      juce::roundToInt (lineH));
        g.drawText (text, r, juce::Justification::centredLeft, false);
    };

    drawLine (0, "Timeline Debug");
    drawLine (1, "beat " + juce::String (processorBeat, 2)
                 + " | loop " + juce::String (beatInLoop, 2)
                 + " / " + juce::String (loopBeats, 2));
    drawLine (2, "clips " + juce::String (placements.size()));

    if (chosen.index >= 0)
    {
        drawLine (3, juce::String (chosen.active ? "active " : "nearest ")
                     + chosen.name);
        drawLine (4, "start " + juce::String (chosen.startBeat, 2)
                     + " len " + juce::String (chosen.lengthBeats, 2)
                     + " rel " + juce::String (chosen.relativeBeat, 2));
    }
    else
    {
        drawLine (3, "no timeline clips");
        drawLine (4, "drop a clip to inspect timing");
    }
}

void PluginEditor::newSong()
{
    processorRef.getSongManager().resetToDefault();
    processorRef.getStructureEngine().rebuild();
    processorRef.syncAuthoredSongToRuntime();
    m_timelineLaneArmed = processorRef.getSongManager().getTimelineLaneArmed();
    m_timelineClipAudioCache.clearQuick();
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
                              m_timelineLaneArmed = processorRef.getSongManager().getTimelineLaneArmed();
                              m_timelineClipAudioCache.clearQuick();
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

    const bool timelineAssetsOk = writeTimelineAudioAssetsForSong (m_currentSongFile);
    if (processorRef.getSongManager().saveToFile (m_currentSongFile))
    {
        systemFeed.addMessage ("Saved song: " + m_currentSongFile.getFileName());
        if (! timelineAssetsOk)
            systemFeed.addMessage ("Timeline audio warning: some clip audio assets were missing.");
    }
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

                              const bool timelineAssetsOk = writeTimelineAudioAssetsForSong (file);
                              if (processorRef.getSongManager().saveToFile (file))
                              {
                                  m_currentSongFile = file;
                                  systemFeed.addMessage ("Saved song: " + file.getFileName());
                                  if (! timelineAssetsOk)
                                      systemFeed.addMessage ("Timeline audio warning: some clip audio assets were missing.");
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
    m_timelineLaneArmed = processorRef.getSongManager().getTimelineLaneArmed();
    const auto zoneCFxStateJson = processorRef.getSongManager().getZoneCFxStateJson();

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
    refreshTimelinePlacementsUi();
    applyTimelineLaneArmStateUi();
    zoneD.setStructureBeat (processorRef.getCurrentSongBeat());
    zoneD.setStructureFollowState (computeFollowState (processorRef.isStructureFollowForTracksEnabled(),
                                                       structureHasScaffold (authoredStructure)));
    processorRef.setTimelineLoopLengthBeats (static_cast<double> (zoneD.loopLengthBars())
                                             * juce::jmax (1.0, processorRef.getBeatsPerBar()));
    if (zoneCFxStateJson.isNotEmpty())
        applyZoneCFxStateJson (zoneCFxStateJson, true);
    else
        persistZoneCFxStateToSong();

    updateSequencerStructureContext (processorRef.getCurrentSongBeat());
    resized();
    repaint();
}

juce::String PluginEditor::serialiseZoneCFxStateJson() const
{
    return zoneCFxStateToJson (zoneC.getChainBank());
}

void PluginEditor::applyZoneCFxStateJson (const juce::String& json, bool pushToDsp)
{
    if (json.trim().isEmpty())
        return;

    ZoneCComponent::ChainBank bank;
    if (! zoneCFxStateFromJson (json, bank))
        return;

    zoneC.setChainBank (bank, pushToDsp);
}

void PluginEditor::persistZoneCFxStateToSong()
{
    processorRef.getSongManager().setZoneCFxStateJson (serialiseZoneCFxStateJson());
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

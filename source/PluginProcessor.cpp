#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <atomic>
#include <cmath>
#include <limits>

namespace
{
double wrapBeatToStructure (double beat, const StructureState& structure)
{
    const auto totalBars = juce::jmax (0, structure.totalBars);
    const double totalBeats = static_cast<double> (totalBars) * 4.0;
    if (totalBeats <= 0.0)
        return beat;
    return std::fmod (juce::jmax (0.0, beat), totalBeats);
}

int snapToNearestPitchClass (const std::vector<int>& allowedPitchClasses, int inputNote)
{
    if (allowedPitchClasses.empty())
        return juce::jlimit (0, 127, inputNote);

    int best = juce::jlimit (0, 127, inputNote);
    int bestDistance = std::numeric_limits<int>::max();
    const int inputOct = inputNote / 12;

    for (const auto pc : allowedPitchClasses)
    {
        for (int oct = inputOct - 1; oct <= inputOct + 1; ++oct)
        {
            const int candidate = oct * 12 + pc;
            if (candidate < 0 || candidate > 127)
                continue;

            const int distance = std::abs (candidate - inputNote);
            if (distance < bestDistance || (distance == bestDistance && candidate > best))
            {
                bestDistance = distance;
                best = candidate;
            }
        }
    }

    return best;
}
} // namespace

//==============================================================================
PluginProcessor::PluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       m_structureEngine (m_appState.structure)
{
    m_midiConstraintEngine.setStructure (&m_appState.structure);
    m_midiConstraintEngine.setStructureEngine (&m_structureEngine);
    m_midiConstraintEngine.setScaleLock (true);
    m_midiConstraintEngine.setChordLock (false);
    m_midiConstraintEngine.setNotePolicy (MidiConstraintEngine::NotePolicy::Guide);

    m_appState.song.bpm = static_cast<int> (m_bpm.load());
    m_appState.transport.bpm = static_cast<double> (m_bpm.load());
    m_appState.transport.isPlaying = m_playing.load();

    m_appState.slots.resize (SpoolAudioGraph::kNumSlots);
    for (int s = 0; s < SpoolAudioGraph::kNumSlots; ++s)
    {
        m_stepBits [s].store (0ULL);
        m_stepCount[s].store (16);
        m_slotNote [s].store (60);  // Default: middle C
        m_drumVoiceCount[s].store (0);
        m_lastNotePlayed[s] = 60;

        auto& slotState = m_appState.slots[(size_t) s];
        slotState.slotIndex = s;
        slotState.name = "Slot " + juce::String (s + 1);
    }

    for (auto& mapped : m_midiNoteMap)
        mapped = -1;
}

PluginProcessor::~PluginProcessor() {}

//==============================================================================
const juce::String PluginProcessor::getName() const  { return JucePlugin_Name; }

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const { return 2.0; }

int  PluginProcessor::getNumPrograms()                             { return 1;  }
int  PluginProcessor::getCurrentProgram()                          { return 0;  }
void PluginProcessor::setCurrentProgram (int)                      {}
const juce::String PluginProcessor::getProgramName (int)           { return {}; }
void PluginProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    m_sampleRate = sampleRate;
    m_samplePos  = 0.0;

    m_audioGraph.prepare (sampleRate, samplesPerBlock);

    m_circularBuffer.prepare (sampleRate, 120.0);
    m_circularBuffer.setSource (-1);   // master mix by default
    m_currentBeat = 0.0;

    for (auto& mb : m_slotMidi)
        mb.clear();

    DBG ("PluginProcessor: prepared at " << sampleRate
         << " Hz, " << samplesPerBlock << " buf");
}

void PluginProcessor::releaseResources()
{
    m_audioGraph.reset();
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
  #endif
}

//==============================================================================
void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer&          midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    // Constraint pass: incoming MIDI -> structure-aware constrained notes
    juce::MidiBuffer constrainedIncoming;
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        const auto samplePos = metadata.samplePosition;

        if (msg.isNoteOn())
        {
            const int inputNote = msg.getNoteNumber();
            const double rawBeat = m_appState.transport.playheadBeats;
            Chord activeChord { "C", "maj" };
            int constrained = inputNote;
            {
                const juce::ScopedReadLock structureRead (m_structureLock);
                const double structureBeat = wrapBeatToStructure (rawBeat, m_appState.structure);
                activeChord = m_structureEngine.getChordAtBeat (structureBeat);
                constrained = m_midiConstraintEngine.processNote (inputNote, structureBeat);
            }
            m_midiNoteMap[inputNote] = constrained;
            msg = juce::MidiMessage::noteOn (msg.getChannel(), constrained, msg.getFloatVelocity());

            DBG ("Constraint beat=" << rawBeat
                 << " chord=" << activeChord.root << activeChord.type
                 << " in=" << inputNote
                 << " out=" << constrained);
        }
        else if (msg.isNoteOff())
        {
            const int inputNote = msg.getNoteNumber();
            const int mapped = (inputNote >= 0 && inputNote < 128 && m_midiNoteMap[inputNote] >= 0)
                                 ? m_midiNoteMap[inputNote]
                                 : inputNote;
            msg = juce::MidiMessage::noteOff (msg.getChannel(), mapped, msg.getFloatVelocity());
            if (inputNote >= 0 && inputNote < 128)
                m_midiNoteMap[inputNote] = -1;
        }

        constrainedIncoming.addEvent (msg, samplePos);
    }
    midiMessages.swapWith (constrainedIncoming);

    // Clear per-slot MIDI buffers (pre-allocated — no allocation)
    for (auto& mb : m_slotMidi)
        mb.clear();

    // Flush any held notes when transport is paused/stopped
    if (m_flushAllNotes.exchange (false))
    {
        for (int slot = 0; slot < SpoolAudioGraph::kNumSlots; ++slot)
        {
            if (m_noteHeld[slot])
            {
                m_slotMidi[slot].addEvent (
                    juce::MidiMessage::noteOff (1, m_lastNotePlayed[slot]),
                    0);
                m_noteHeld[slot] = false;
            }
        }
    }

    // Route incoming MIDI to the correct slot
    m_midiRouter.route (midiMessages, m_slotMidi, SpoolAudioGraph::kNumSlots);

    // ── Song clock ───────────────────────────────────────────────────────────
    if (m_playing.load())
    {
        const double bpm            = static_cast<double> (m_bpm.load());
        const double bps            = bpm / 60.0;
        const double samplesPerBeat = m_sampleRate / bps;
        const double beatDelta      = static_cast<double> (numSamples) / samplesPerBeat;
        
        m_currentBeat += beatDelta;
        m_currentSongBeat += beatDelta;
        m_appState.transport.playheadBeats = m_currentSongBeat;

        // Minimal integration point: transport can query structure context.
        const Section* activeSection = nullptr;
        Chord activeChord { "C", "maj" };
        {
            const juce::ScopedReadLock structureRead (m_structureLock);
            const double structureBeat = wrapBeatToStructure (m_currentSongBeat, m_appState.structure);
            activeSection = m_structureEngine.getSectionAtBeat (structureBeat);
            activeChord = m_structureEngine.getChordAtBeat (structureBeat);
        }
        juce::ignoreUnused (activeSection, activeChord);

        // Per-sample step advance (16th note resolution)
        const double samplesPerStep16th = samplesPerBeat / 4.0;
        for (int i = 0; i < numSamples; ++i)
        {
            m_samplePos += 1.0;
            if (m_samplePos >= samplesPerStep16th)
            {
                m_samplePos -= samplesPerStep16th;
                advanceSongBeat(i);  // new song-aware advance
            }
        }
    }

    // ── Audio graph ──────────────────────────────────────────────────────────
    m_audioGraph.process (buffer, m_slotMidi);

    // ── Master gain + pan ────────────────────────────────────────────────────
    const float gain = m_masterGain.load();
    const float pan  = m_masterPan.load();
    constexpr float pi = 3.14159265358979323846f;
    const float angle      = ((pan + 1.0f) * 0.5f) * (pi * 0.5f);
    const float leftGain   = gain * std::cos (angle);
    const float rightGain  = gain * std::sin (angle);

    const int numCh = juce::jmin (getTotalNumOutputChannels(), buffer.getNumChannels());
    if (numCh > 0) buffer.applyGain (0, 0, numSamples, leftGain);
    if (numCh > 1) buffer.applyGain (1, 0, numSamples, rightGain);

    // ── Circular buffer — LAST operation, after all gain/pan applied ─────────
    if (m_circularBuffer.isActive())
    {
        const int srcSlot = m_circularBuffer.getSource();
        if (srcSlot == -1)
        {
            // Master mix
            m_circularBuffer.writeBlock (buffer, numSamples);
        }
        else if (srcSlot >= 0 && srcSlot < SpoolAudioGraph::kNumSlots
                 && m_audioGraph.isSlotActive (srcSlot))
        {
            m_circularBuffer.writeBlock (m_audioGraph.getSlotBuffer (srcSlot), numSamples);
        }
    }
}

//==============================================================================
void PluginProcessor::advanceSongBeat (int sampleOffset)
{
    bool structureActive = false;
    double structureBeat = 0.0;
    std::vector<int> structureChordPcs;
    {
        const juce::ScopedReadLock structureRead (m_structureLock);
        structureActive = ! m_appState.structure.sections.empty();
        if (structureActive)
        {
            structureBeat = wrapBeatToStructure (m_currentSongBeat, m_appState.structure);
            structureChordPcs = m_structureEngine.resolveChordAtBeat (structureBeat);
        }
    }
    
    // Note-off previous notes
    for (int slot = 0; slot < SpoolAudioGraph::kNumSlots; ++slot)
    {
        if (m_noteHeld[slot])
        {
            m_slotMidi[slot].addEvent (
                juce::MidiMessage::noteOff (1, m_lastNotePlayed[slot]),
                sampleOffset);
            m_noteHeld[slot] = false;
        }
    }

    // Structure-driven thin-slice sequencing path: generate quarter-note pulses
    // across the full structure loop so users hear harmonic movement immediately.
    if (m_useStructureForTracks.load() && structureActive && ! structureChordPcs.empty())
    {
        const int step16 = juce::jmax (0, static_cast<int> (std::floor (structureBeat * 4.0 + 1.0e-6)));
        const int chordRootPc = structureChordPcs.front();

        for (int slot = 0; slot < SpoolAudioGraph::kNumSlots; ++slot)
        {
            if (! m_audioGraph.isSlotActive (slot))
                continue;

            const int stepCount = juce::jmax (1, m_stepCount[slot].load (std::memory_order_relaxed));
            const int slotStep = step16 % stepCount;
            const uint64_t bit = 1ULL << juce::jlimit (0, 63, slotStep);
            const bool slotStepActive = (m_stepBits[slot].load (std::memory_order_relaxed) & bit) != 0;

            if (! slotStepActive)
                continue;

            int note = m_slotNote[slot].load();
            note = snapToNearestPitchClass ({ chordRootPc }, note);
            m_slotMidi[slot].addEvent (
                juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (100)),
                sampleOffset);
            m_noteHeld[slot] = true;
            m_lastNotePlayed[slot] = note;
        }

        m_currentStep.store(step16 % 64);
        triggerAsyncUpdate();
        return;
    }

    auto state = m_songMgr.query (m_currentSongBeat);
    if (state.sectionId < 0) return;  // no active section

    const auto& sec = m_songMgr.getSections()[state.sectionId];
    const auto& pat = m_songMgr.getPatterns()[sec.patternId];

    const int stepsPerBeat = 4;  // 16th notes
    const int totalPatternSteps = juce::jmax (1, static_cast<int> (pat.lengthBeats * stepsPerBeat));
    const int step = static_cast<int> (state.localBeat * stepsPerBeat) % totalPatternSteps;

    // Dispatch to active slots
    for (int i = 0; i < state.numActiveSlots; ++i)
    {
        const int slot = state.activeSlots[i];
        if (! m_audioGraph.isSlotActive (slot)) continue;

        // Check pattern step active
        if (pat.baseSteps.contains(step))
        {
            int note = m_slotNote[slot].load();
            note += state.transposeTotal;  // apply transposes

            if (state.hasActiveChord)
                note = state.activeChord.root;

            m_slotMidi[slot].addEvent (
                juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (100)),
                sampleOffset);
            m_noteHeld[slot] = true;
            m_lastNotePlayed[slot] = note;
        }
    }

    m_currentStep.store(step);  // legacy UI
    triggerAsyncUpdate();
}


void PluginProcessor::handleAsyncUpdate()
{
    if (onStepAdvanced)
        onStepAdvanced (m_currentStep.load());
}

//==============================================================================
// Transport
//==============================================================================
void PluginProcessor::setPlaying (bool playing) noexcept
{
    m_playing.store (playing);
    m_appState.transport.isPlaying = playing;
    if (! playing)
        m_flushAllNotes.store (true);
}

void PluginProcessor::setBpm (float bpm) noexcept
{
    const auto clamped = juce::jlimit (20.0f, 999.0f, bpm);
    m_bpm.store (clamped);
    m_appState.song.bpm = static_cast<int> (clamped);
    m_appState.transport.bpm = static_cast<double> (clamped);
}

void PluginProcessor::syncTransportFromAppState() noexcept
{
    const auto desiredBpm = juce::jlimit (20.0, 999.0, m_appState.transport.bpm);
    const auto desiredPlaying = m_appState.transport.isPlaying;

    const auto oldBpm = static_cast<double> (m_bpm.load());
    const auto oldPlaying = m_playing.load();

    if (std::abs (oldBpm - desiredBpm) > 0.0001)
    {
        m_bpm.store (static_cast<float> (desiredBpm));
        m_appState.song.bpm = static_cast<int> (desiredBpm);
        DBG ("BPM updated via AppState: " << desiredBpm);
    }

    if (oldPlaying != desiredPlaying)
    {
        m_playing.store (desiredPlaying);
        DBG ("Play state updated via AppState: " << (desiredPlaying ? "playing" : "stopped"));
    }
}

void PluginProcessor::seekTransport (double beat) noexcept
{
    const double clamped = juce::jmax (0.0, beat);
    m_samplePos = 0.0;
    m_currentSongBeat = clamped;
    m_currentBeat = clamped;
    m_appState.transport.playheadBeats = clamped;
    m_currentStep.store (static_cast<int> (std::floor (clamped * 4.0)) % 64);
}

void PluginProcessor::setConstraintNotePolicy (MidiConstraintEngine::NotePolicy policy) noexcept
{
    m_midiConstraintEngine.setNotePolicy (policy);
}

MidiConstraintEngine::NotePolicy PluginProcessor::getConstraintNotePolicy() const noexcept
{
    return m_midiConstraintEngine.getNotePolicy();
}

void PluginProcessor::setConstraintScaleLock (bool enabled) noexcept
{
    m_midiConstraintEngine.setScaleLock (enabled);
}

void PluginProcessor::setConstraintChordLock (bool enabled) noexcept
{
    m_midiConstraintEngine.setChordLock (enabled);
}

void PluginProcessor::setStructureFollowForTracks (bool enabled) noexcept
{
    m_useStructureForTracks.store (enabled);
}

bool PluginProcessor::isStructureFollowForTracksEnabled() const noexcept
{
    return m_useStructureForTracks.load();
}

//==============================================================================
void PluginProcessor::setFocusedSlot (int slot) noexcept
{
    m_midiRouter.setFocusedSlot (slot);
    m_focusedSlotIndex.store (slot);
}

//==============================================================================
void PluginProcessor::setStepActive (int slot, int step, bool active) noexcept
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots) return;
    if (step < 0 || step >= 64) return;
    const uint64_t mask = 1ULL << step;
    if (active)
        m_stepBits[slot].fetch_or  (mask, std::memory_order_relaxed);
    else
        m_stepBits[slot].fetch_and (~mask, std::memory_order_relaxed);
}

void PluginProcessor::setStepCount (int slot, int count) noexcept
{
    if (slot >= 0 && slot < SpoolAudioGraph::kNumSlots)
        m_stepCount[slot].store (juce::jlimit (1, 64, count));
}

void PluginProcessor::setSlotNote (int slot, int note) noexcept
{
    if (slot >= 0 && slot < SpoolAudioGraph::kNumSlots)
        m_slotNote[slot].store (juce::jlimit (0, 127, note));
}

int PluginProcessor::getSlotNote (int slot) const noexcept
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots) return 60;
    return m_slotNote[slot].load();
}

void  PluginProcessor::setSlotLevel  (int slot, float level) noexcept { m_audioGraph.setSlotLevel  (slot, level);  }
float PluginProcessor::getSlotLevel  (int slot) const noexcept        { return m_audioGraph.getSlotLevel  (slot);  }
void  PluginProcessor::setSlotMuted  (int slot, bool muted)  noexcept { m_audioGraph.setSlotMuted  (slot, muted);  }
bool  PluginProcessor::isSlotMuted   (int slot) const noexcept        { return m_audioGraph.isSlotMuted   (slot);  }
void  PluginProcessor::setSlotSoloed (int slot, bool soloed) noexcept { m_audioGraph.setSlotSoloed (slot, soloed); }
bool  PluginProcessor::isSlotSoloed  (int slot) const noexcept        { return m_audioGraph.isSlotSoloed  (slot);  }
float PluginProcessor::getSlotMeter  (int slot) const noexcept        { return m_audioGraph.getSlotMeter  (slot);  }
void  PluginProcessor::setSlotActive (int slot, bool active) noexcept { m_audioGraph.setSlotActive (slot, active); }

//==============================================================================
void PluginProcessor::setFXChainParam (int slotIndex, int nodeIndex,
                                        int paramIndex, float value) noexcept
{
    if (slotIndex < 0 || slotIndex >= SpoolAudioGraph::kNumSlots) return;
    m_audioGraph.getFXChain (slotIndex).setParam (nodeIndex, paramIndex, value);
}

void PluginProcessor::rebuildFXChain (int slotIndex, const ChainState& chain)
{
    if (slotIndex < 0 || slotIndex >= SpoolAudioGraph::kNumSlots) return;
    m_audioGraph.getFXChain (slotIndex).rebuildFromChain (chain);
}

//==============================================================================
void  PluginProcessor::setMasterGain (float g) noexcept { m_masterGain.store (juce::jlimit (0.0f, 2.0f, g)); }
void  PluginProcessor::setMasterPan  (float p) noexcept { m_masterPan.store  (juce::jlimit (-1.0f, 1.0f, p)); }
float PluginProcessor::getMasterGain() const noexcept   { return m_masterGain.load(); }
float PluginProcessor::getMasterPan()  const noexcept   { return m_masterPan.load();  }

//==============================================================================
// Circular buffer grab
//==============================================================================
void PluginProcessor::grabRegion (double startSecondsAgo, double lengthSeconds)
{
    m_grabbedClip    = m_circularBuffer.getRegion (startSecondsAgo, lengthSeconds);
    m_hasGrabbedClip = true;
    triggerAsyncUpdate();
}

void PluginProcessor::grabLastBars (int numBars)
{
    m_grabbedClip    = m_circularBuffer.getLastBars (numBars,
                                                      static_cast<double> (m_bpm.load()),
                                                      m_beatsPerBar,
                                                      m_currentBeat);
    m_hasGrabbedClip = true;
    triggerAsyncUpdate();
}

//==============================================================================
void PluginProcessor::logRuntimeAction (const juce::String& msg) const
{
    juce::ignoreUnused (msg);
    DBG ("[SPOOL] " << msg);
}

//==============================================================================
// Drum machine step data
//==============================================================================
void PluginProcessor::setDrumVoiceStepBits (int slot, int voice, uint64_t bits) noexcept
{
    if (slot  < 0 || slot  >= SpoolAudioGraph::kNumSlots) return;
    if (voice < 0 || voice >= kMaxDrumVoices)             return;
    m_drumVoices[slot][voice].stepBits.store (bits);
}

void PluginProcessor::setDrumVoiceStepCount (int slot, int voice, int count) noexcept
{
    if (slot  < 0 || slot  >= SpoolAudioGraph::kNumSlots) return;
    if (voice < 0 || voice >= kMaxDrumVoices)             return;
    m_drumVoices[slot][voice].stepCount.store (juce::jlimit (1, 64, count));
}

void PluginProcessor::setDrumVoiceMidiNote (int slot, int voice, int note) noexcept
{
    if (slot  < 0 || slot  >= SpoolAudioGraph::kNumSlots) return;
    if (voice < 0 || voice >= kMaxDrumVoices)             return;
    m_drumVoices[slot][voice].midiNote.store (juce::jlimit (0, 127, note));
}

void PluginProcessor::setDrumVoiceCount (int slot, int count) noexcept
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots) return;
    m_drumVoiceCount[slot].store (juce::jlimit (0, kMaxDrumVoices, count));
}

uint64_t PluginProcessor::getDrumVoiceStepBits (int slot, int voice) const noexcept
{
    if (slot  < 0 || slot  >= SpoolAudioGraph::kNumSlots) return 0ULL;
    if (voice < 0 || voice >= kMaxDrumVoices)             return 0ULL;
    return m_drumVoices[slot][voice].stepBits.load();
}

int PluginProcessor::getDrumVoiceStepCount (int slot, int voice) const noexcept
{
    if (slot  < 0 || slot  >= SpoolAudioGraph::kNumSlots) return 16;
    if (voice < 0 || voice >= kMaxDrumVoices)             return 16;
    return m_drumVoices[slot][voice].stepCount.load();
}

void PluginProcessor::setRoutingMatrix (const std::array<uint8_t, 8>& m)
{
    std::lock_guard<std::mutex> lock (m_routingLock);
    m_routingState = m;
}

std::array<uint8_t, 8> PluginProcessor::getRoutingMatrix() const
{
    std::lock_guard<std::mutex> lock (m_routingLock);
    return m_routingState;
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement state ("SPOOLState");
    state.setAttribute ("masterGain", static_cast<double> (m_masterGain.load()));
    state.setAttribute ("masterPan",  static_cast<double> (m_masterPan.load()));
    state.setAttribute ("bpm",        static_cast<double> (m_bpm.load()));
    {
        std::lock_guard<std::mutex> lock (m_routingLock);
        juce::String bits;
        for (auto b : m_routingState)
            bits += (b ? '1' : '0');
        state.setAttribute ("routing", bits);
    }
    const juce::String xmlText = state.toString();
    destData.replaceAll (xmlText.toRawUTF8(),
                         static_cast<size_t> (xmlText.getNumBytesAsUTF8()));
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0) return;
    const juce::String s (juce::CharPointer_UTF8 (static_cast<const char*> (data)));
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (s));
    if (xml == nullptr) return;

    m_masterGain.store (static_cast<float> (xml->getDoubleAttribute ("masterGain", 1.0)));
    m_masterPan.store  (static_cast<float> (xml->getDoubleAttribute ("masterPan",  0.0)));
    m_bpm.store        (static_cast<float> (xml->getDoubleAttribute ("bpm",       120.0)));

    const juce::String bits = xml->getStringAttribute ("routing", {});
    if (bits.isNotEmpty())
    {
        std::lock_guard<std::mutex> lock (m_routingLock);
        for (int i = 0; i < (int) m_routingState.size() && i < bits.length(); ++i)
            m_routingState[i] = (bits[i] == '1') ? 1 : 0;
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

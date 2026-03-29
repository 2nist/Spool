#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <atomic>
#include "dsp/SpoolAudioGraph.h"
#include "dsp/MidiRouter.h"
#include "dsp/CircularAudioBuffer.h"
#include "dsp/CapturedAudioClip.h"
#include "dsp/SongManager.h"
#include "midi/MidiConstraintEngine.h"
#include "state/AppState.h"
#include "state/RoutingState.h"
#include "structure/StructureEngine.h"
#include "theory/TheoryAdapter.h"
#include "components/ZoneB/SlotPattern.h"

#if (MSVC)
#include "ipps.h"
#endif

class PluginProcessor : public juce::AudioProcessor,
                        public juce::AsyncUpdater
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    //--- AudioProcessor -------------------------------------------------------
    void prepareToPlay   (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock    (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool  acceptsMidi()   const override;
    bool  producesMidi()  const override;
    bool  isMidiEffect()  const override;
    double getTailLengthSeconds() const override;

    int  getNumPrograms() override;
    int  getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation  (const void* data, int sizeInBytes) override;

    //--- DSP graph -----------------------------------------------------------
    SpoolAudioGraph& getAudioGraph() noexcept { return m_audioGraph; }

    //--- Transport (thread-safe via atomics) ---------------------------------
    void  setPlaying (bool playing)  noexcept;
    void  setBpm     (float bpm)     noexcept;
    bool  isPlaying() const noexcept { return m_playing.load(); }
    float getBpm()    const noexcept { return m_bpm.load();    }
    void  seekTransport (double beat) noexcept;

    /** Current song beat (double). */
    double getCurrentSongBeat() const noexcept { return m_currentSongBeat.load (std::memory_order_relaxed); }
    SongManager& getSongManager() noexcept { return m_songMgr; }
    const SongManager& getSongManager() const noexcept { return m_songMgr; }
    juce::UndoManager& getUndoManager() noexcept { return m_undoManager; }
    AppState& getAppState() noexcept { return m_appState; }
    StructureEngine& getStructureEngine() noexcept { return m_structureEngine; }
    juce::ReadWriteLock& getStructureLock() noexcept { return m_structureLock; }
    void syncAuthoredSongToRuntime() noexcept;
    void syncTransportFromAppState() noexcept;
    void setConstraintNotePolicy (MidiConstraintEngine::NotePolicy policy) noexcept;
    MidiConstraintEngine::NotePolicy getConstraintNotePolicy() const noexcept;
    void setConstraintScaleLock (bool enabled) noexcept;
    void setConstraintChordLock (bool enabled) noexcept;
    void setStructureFollowForTracks (bool enabled) noexcept;
    bool isStructureFollowForTracksEnabled() const noexcept;

    /** Legacy step for UI compatibility */
    int getCurrentStep() const noexcept { return m_currentStep.load(); }

    //--- MIDI routing --------------------------------------------------------
    void setFocusedSlot (int slot) noexcept;
    int  getFocusedSlot() const noexcept { return m_focusedSlotIndex.load(); }

    //--- Per-slot step data (UI thread writes, audio thread reads) -----------
    void setStepActive (int slot, int step, bool active) noexcept;
    void setStepCount  (int slot, int count) noexcept;
    void setSlotPattern (int slot, const SlotPattern& pattern) noexcept;

    /** MIDI note each slot plays when a step fires.  Default: 60 (C4). */
    void setSlotNote  (int slot, int note)  noexcept;
    int  getSlotNote  (int slot)      const noexcept;

    /** Per-slot output gain (0..2). */
    void  setSlotLevel (int slot, float level) noexcept;
    float getSlotLevel (int slot) const noexcept;

    /** Per-slot mute — silences a slot's output without stopping its DSP. */
    void setSlotMuted (int slot, bool muted) noexcept;
    bool isSlotMuted  (int slot) const noexcept;

    /** Per-slot solo.  When any slot is soloed, un-soloed slots are silenced.
        Exclusive by default; call with soloed=false to clear a solo. */
    void setSlotSoloed (int slot, bool soloed) noexcept;
    bool isSlotSoloed  (int slot) const noexcept;

    /** Post-FX RMS meter level for UI metering (0..1 approx). */
    float getSlotMeter (int slot) const noexcept;

    /** Activate / silence a slot in the audio graph. */
    void setSlotActive (int slot, bool active) noexcept;

    //--- Drum machine step data (thread-safe, written from UI thread) ---------

    static constexpr int kMaxDrumVoices = 16;

    /** Set the step pattern bits for one voice in a drum slot. */
    void setDrumVoiceStepBits  (int slot, int voice, uint64_t bits)  noexcept;
    void setDrumVoiceStepCount (int slot, int voice, int count)      noexcept;
    void setDrumVoiceMidiNote  (int slot, int voice, int note)       noexcept;
    void setDrumVoiceCount     (int slot, int count)                 noexcept;
    void setDrumStepVelocity   (int slot, int voice, int step, float velocity) noexcept;
    void setDrumStepRatchet    (int slot, int voice, int step, int count) noexcept;
    void setDrumStepDivision   (int slot, int voice, int step, int division) noexcept;

    uint64_t getDrumVoiceStepBits  (int slot, int voice) const noexcept;
    int      getDrumVoiceStepCount (int slot, int voice) const noexcept;

    //--- FX chain (called from UI thread) ------------------------------------
    void setFXChainParam (int slotIndex, int nodeIndex, int paramIndex, float value) noexcept;
    void rebuildFXChain  (int slotIndex, const ChainState& chain);

    //--- Master output (thread-safe via atomics) -----------------------------
    void  setMasterGain (float g) noexcept;
    void  setMasterPan  (float p) noexcept;
    float getMasterGain() const noexcept;
    float getMasterPan()  const noexcept;

    //--- Circular audio buffer ------------------------------------------------
    CircularAudioBuffer& getCircularBuffer() noexcept { return m_circularBuffer; }

    /** Grab a free region from the buffer (message thread — allocates). */
    void grabRegion   (double startSecondsAgo, double lengthSeconds);
    /** Grab the last N complete bars, snapped to bar boundary. */
    void grabLastBars (int numBars);
    void setLooperPreviewClip (const juce::AudioBuffer<float>& buffer, double sampleRate);
    void clearLooperPreview();
    void setLooperPreviewState (bool playing, bool looping);
    void setTimelineLoopLengthBeats (double beats) noexcept;
    float getLooperPreviewProgress() const noexcept { return m_looperPreviewProgress.load(); }
    bool isLooperPreviewPlaying() const noexcept { return m_looperPreviewPlaying.load(); }

    /** Timeline clip playback (audio-thread mixed in processBlock). */
    void addTimelineAudioClip (const CapturedAudioClip& clip,
                               double startBeat,
                               double lengthBeats,
                               int laneIndex,
                               const juce::String& moduleType,
                               const juce::String& clipName);
    void clearTimelineAudioClips();

    bool hasGrabbedClip() const noexcept { return m_hasGrabbedClip; }
    const juce::AudioBuffer<float>& getGrabbedClip() const noexcept { return m_grabbedClip; }
    void clearGrabbedClip() noexcept { m_hasGrabbedClip = false; }

    //--- Beat position (used by getLastBars) ---
    double getCurrentBeat()  const noexcept { return m_currentBeat.load (std::memory_order_relaxed); }
    double getBeatsPerBar()  const noexcept { return m_beatsPerBar; }
    void   setBeatsPerBar (double bpb) noexcept { m_beatsPerBar = bpb; }

    /** Append a UI action to the runtime log (message thread only). */
    void logRuntimeAction (const juce::String& msg) const;

    /** Routing matrix (4 sources × 2 outputs = 8 bits). */
    void setRoutingMatrix (const std::array<uint8_t, 8>& m);
    std::array<uint8_t, 8> getRoutingMatrix() const;
    void setRoutingState (const RoutingState& state);
    RoutingState getRoutingState() const;

    //--- Step-advance notification (message thread) --------------------------
    /** Fired on the message thread after each sequencer step advance.
        Wire this in PluginEditor to update Zone B's playhead highlight. */
    std::function<void(int step)> onStepAdvanced;

    //--- AsyncUpdater --------------------------------------------------------
    void handleAsyncUpdate() override;

private:
    //--- DSP -----------------------------------------------------------------
    SongManager         m_songMgr;
    juce::UndoManager   m_undoManager;
    AppState            m_appState;
    StructureEngine     m_structureEngine;
    MidiConstraintEngine m_midiConstraintEngine;
    mutable juce::ReadWriteLock m_structureLock;
    SpoolAudioGraph     m_audioGraph;
    MidiRouter          m_midiRouter;
    CircularAudioBuffer m_circularBuffer;
    juce::SpinLock m_looperPreviewLock;
    juce::AudioBuffer<float> m_looperPreviewBuffer;
    double m_looperPreviewSourceRate { 44100.0 };
    double m_looperPreviewReadPos { 0.0 };
    bool m_looperPreviewLoopingFlag { true };
    std::atomic<bool> m_looperPreviewPlaying { false };
    std::atomic<float> m_looperPreviewProgress { 0.0f };
    struct TimelineAudioClip
    {
        juce::AudioBuffer<float> buffer;
        double startBeat { 0.0 };
        double lengthBeats { 4.0 };
    };
    juce::SpinLock m_timelineAudioLock;
    juce::Array<TimelineAudioClip> m_timelineAudioClips;
    std::atomic<double> m_timelineLoopLengthBeats { 32.0 };

    // Per-slot MIDI scratch buffers (pre-allocated, cleared each block)
    juce::MidiBuffer m_slotMidi[SpoolAudioGraph::kNumSlots];

    //--- Transport -----------------------------------------------------------
    std::atomic<bool>  m_playing     { false };
    std::atomic<bool>  m_useStructureForTracks { true };
    std::atomic<float> m_bpm         { 120.0f };
    std::atomic<int>   m_currentStep { 0 };
    std::atomic<int>   m_focusedSlotIndex { 0 };

    double m_sampleRate = 44100.0;
    double m_samplePos  = 0.0;   // samples elapsed within the current step

    // Per-slot step data (atomic for lock-free audio reads)
    std::atomic<uint64_t> m_stepBits [SpoolAudioGraph::kNumSlots];
    std::atomic<int>      m_stepCount[SpoolAudioGraph::kNumSlots];
    std::atomic<int>      m_slotNote [SpoolAudioGraph::kNumSlots];

    static constexpr int kSequencerTicksPerBeat = 96;
    static constexpr int kSequencerTicksPerStep = kSequencerTicksPerBeat / 4;
    static constexpr int kMaxSequencerEventsPerSlot = SlotPattern::MAX_STEPS * SlotPattern::MAX_MICRO_EVENTS;
    static constexpr int kMaxHeldNotesPerSlot = 16;
    static constexpr int kMaxDrumStepsPerVoice = 64;

    struct RuntimeMicroEvent
    {
        int startTick  { 0 };
        int lengthTicks { 1 };
        int pitchValue { SlotPattern::kUseSlotBasePitch };
        int anchorValue { SlotPattern::kUseSlotBasePitch };
        int stepIndex { 0 };
        int eventOrdinalInStep { 0 };
        int eventsInStep { 1 };
        uint8_t velocity { 100 };
        SlotPattern::NoteMode noteMode { SlotPattern::NoteMode::absolute };
        SlotPattern::StepRole role { SlotPattern::StepRole::lead };
        SlotPattern::HarmonicSource harmonicSource { SlotPattern::HarmonicSource::key };
        bool lookAheadToNextChord { false };
    };

    struct RuntimeSlotPattern
    {
        int patternLengthTicks { kSequencerTicksPerStep * SlotPattern::DEFAULT_PATTERN_UNITS };
        int eventCount { 0 };
        bool hasEvents { false };
        SlotPattern::LaneContext laneContext { SlotPattern::LaneContext::melodic };
        std::array<RuntimeMicroEvent, kMaxSequencerEventsPerSlot> events {};
    };

    struct HeldSlotNote
    {
        int note { -1 };
        int ticksRemaining { 0 };
        bool active { false };
    };

    struct PhraseMemory
    {
        bool hasPreviousNote { false };
        int previousNote { 60 };
        int previousDirection { 0 };
        int recentLow { 60 };
        int recentHigh { 60 };
        int previousChordRootPc { 0 };
        int notesSinceChordChange { 0 };
        SlotPattern::StepRole previousRole { SlotPattern::StepRole::lead };
        int previousChordVoiceIndex { 0 };
        int previousChordTopNote { 60 };
        bool preferOpenChordVoicing { false };
        int cadenceTemplate { 0 };
        int cadenceEventCounter { 0 };
    };

    RuntimeSlotPattern m_runtimePatterns[SpoolAudioGraph::kNumSlots];
    juce::SpinLock     m_runtimePatternLocks[SpoolAudioGraph::kNumSlots];
    HeldSlotNote       m_heldNotes[SpoolAudioGraph::kNumSlots][kMaxHeldNotesPerSlot];
    PhraseMemory       m_phraseMemory[SpoolAudioGraph::kNumSlots];
    TheoryAdapter      m_theoryAdapter;

    // Note-hold state (audio thread only — no atomic needed)
    bool m_noteHeld[SpoolAudioGraph::kNumSlots] = {};
    int  m_midiNoteMap[128] = {};
    std::atomic<bool> m_flushAllNotes { false };
    int  m_lastNotePlayed[SpoolAudioGraph::kNumSlots] = {};

    //--- Master --------------------------------------------------------------
    std::atomic<float> m_masterGain { 1.0f };
    std::atomic<float> m_masterPan  { 0.0f };

    //--- Drum machine per-voice step state -----------------------------------
    struct DrumVoiceStepState
    {
        std::atomic<uint64_t> stepBits  { 0  };
        std::atomic<int>      stepCount { 16 };
        std::atomic<int>      midiNote  { 36 };
        std::array<std::atomic<int>, kMaxDrumStepsPerVoice> stepVelocitiesNorm {};
        std::array<std::atomic<int>, kMaxDrumStepsPerVoice> stepRatchets {};
        std::array<std::atomic<int>, kMaxDrumStepsPerVoice> stepDivisions {};
        DrumVoiceStepState() = default;
    };
    DrumVoiceStepState   m_drumVoices [SpoolAudioGraph::kNumSlots][kMaxDrumVoices];
    std::atomic<int>     m_drumVoiceCount [SpoolAudioGraph::kNumSlots];

    //--- Grabbed clip --------------------------------------------------------
    juce::AudioBuffer<float> m_grabbedClip;
    bool                     m_hasGrabbedClip { false };

    //--- Beat tracking -------------------------------------------------------
    std::atomic<double> m_currentBeat { 0.0 };
    double m_beatsPerBar { 4.0 };

    //--- Legacy routing matrix -----------------------------------------------
    mutable std::mutex      m_routingLock;
    std::array<uint8_t, 8>  m_routingState { 0 };
    RoutingState            m_routeModel { RoutingState::makeDefault() };
    static constexpr int    kRoutingSources = 4;
    static constexpr int    kRoutingOutputs = 2;

    //--- Song helpers
    void advanceSequencerTick (int sampleOffset);
    static RuntimeSlotPattern compileRuntimePattern (const SlotPattern& pattern);
    int realizeRuntimePitch (int slot,
                             const RuntimeMicroEvent& event,
                             int baseNote,
                             const juce::String& keyRoot,
                             const juce::String& keyScale,
                             const Chord& currentChord,
                             const Chord& nextChord,
                             bool structureFollow,
                             const std::vector<int>& structureChordPcs);
    void triggerRuntimePatternEvents (int slot,
                                      int patternTick,
                                      int sampleOffset,
                                      bool structureFollow,
                                      const juce::String& keyRoot,
                                      const juce::String& keyScale,
                                      const Chord& currentChord,
                                      const Chord& nextChord,
                                      const std::vector<int>& structureChordPcs);
    void resetPhraseMemory (int slot);
    void updatePhraseMemory (int slot, int realizedNote, SlotPattern::StepRole role, const Chord& currentChord);
    void releaseFinishedSlotNotes (int slot, int sampleOffset);
    void pushHeldNote (int slot, int note, int lengthTicks);
    void mixTimelineAudio (juce::AudioBuffer<float>& buffer,
                           int numSamples,
                           double blockStartBeat,
                           double beatsPerSample) noexcept;
    
    std::atomic<double> m_currentSongBeat { 0.0 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};

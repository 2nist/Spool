#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "DrumVoiceDSP.h"
#include "DrumKitPatch.h"
#include "DrumModuleState.h"
#include "../../module/ModuleProcessor.h"

//==============================================================================
/**
    DrumMachineProcessor — multi-voice drum synthesizer, one per Zone B slot.

    Owns up to MAX_VOICES DrumVoiceDSP instances.
    Routes incoming MIDI note-on events to matching voices by MIDI note number.
    Can also be triggered directly from the step sequencer via triggerVoice().

    Real-time safe: no allocations in process() or triggerVoice().
    All voice setup (addVoice, setVoiceParams) must be called off the audio thread
    (e.g. from the message thread when the UI changes a preset).
*/
class DrumMachineProcessor : public ModuleProcessor
{
public:
    static constexpr int MAX_VOICES = 16;

    DrumMachineProcessor() = default;

    //==========================================================================
    // ModuleProcessor identity + stubs

    const char* getModuleId()   const noexcept override { return "com.spool.drummachine"; }
    const char* getModuleName() const noexcept override { return "DrumMachine"; }

    // No manifest yet — drum voices are managed imperatively
    const ParamDef* getParamDefs()  const noexcept override { return nullptr; }
    int             getNumParams()  const noexcept override { return 0; }

    void  setParam (juce::StringRef, float)       noexcept override {}
    float getParam (juce::StringRef) const noexcept override { return 0.f; }

    void getState (juce::MemoryBlock&) const override;
    bool setState (const void*, int) override;

    //==========================================================================
    // Lifecycle

    void prepare (double sampleRate, int blockSize);

    /** Render all active voices into \p audio (adds to existing content).
        Processes MIDI note-on events to trigger voices. */
    void process (juce::AudioBuffer<float>& audio,
                  const juce::MidiBuffer&   midi) noexcept;

    //==========================================================================
    // Voice management — call from message thread only

    /** Add a voice.  Returns its index (0-based), or -1 if at capacity. */
    int  addVoice (const DrumVoiceParams& params,
                   const juce::String&   name,
                   int                   midiNote);

    /** Replace params for an existing voice.  Thread-safe via a pending-flag scheme. */
    void setVoiceParams (int voiceIndex, const DrumVoiceParams& params);

    void removeVoice (int voiceIndex);
    int  getNumVoices() const noexcept { return m_numVoices; }

    /** Load a full kit (replaces all voices). */
    void loadKit (const DrumKitPatch& kit);

    /** Apply a full drum module state snapshot to the DSP engine. */
    void applyState (const DrumModuleState& state);

    /** Capture the current DSP-facing drum state. */
    DrumModuleState captureState() const;

    //==========================================================================
    // Direct trigger — safe from audio thread (used by step sequencer)

    void triggerVoice (int voiceIndex, float velocity) noexcept;

    //==========================================================================
    // Per-voice accessors (message thread)

    DrumVoiceParams& getVoiceParams (int index) noexcept { return m_params[index]; }
    const juce::String& getVoiceName (int index) const noexcept { return m_names[index]; }
    int  getVoiceMidiNote (int index) const noexcept { return m_midiNotes[index]; }
    void setVoiceMuted (int index, bool muted) noexcept;
    bool isVoiceMuted  (int index) const noexcept;

private:
    //==========================================================================
    DrumVoiceDSP    m_voices   [MAX_VOICES];
    DrumVoiceParams m_params   [MAX_VOICES];
    juce::String    m_names    [MAX_VOICES];
    int             m_midiNotes[MAX_VOICES] = {};
    bool            m_muted    [MAX_VOICES] = {};

    int     m_numVoices  = 0;
    double  m_sampleRate = 44100.0;

    //==========================================================================
    // Pending params — written from message thread, applied at block start

    struct PendingParams
    {
        DrumVoiceParams params;
        bool            dirty = false;
    };
    PendingParams m_pending[MAX_VOICES];

    void applyPendingParams() noexcept;
    void processMidi (const juce::MidiBuffer& midi) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumMachineProcessor)
};

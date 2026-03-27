#pragma once

#include <juce_core/juce_core.h>
#include "../instruments/drum/DrumKitPatch.h"
#include "../instruments/drum/DrumModuleState.h"

class ModuleProcessor;

//==============================================================================
/**
    PresetManager — factory + user preset bank for Synth and Drum modules.

    Synth presets store PolySynthProcessor param IDs as JSON floats.
    Drum presets store per-voice DSP params as JSON.  Both formats include a
    "version" field for forward-compatibility.

    Factory presets live in memory.  User presets are saved as JSON files under
    ~/Documents/Spool/Presets/Synth/  and  ~/Documents/Spool/Presets/Drum/.

    Preset loading is always on the message thread; no RT-safety concern here
    since setParam / loadKit are themselves message-thread entry points.
*/
class PresetManager
{
public:
    static PresetManager& getInstance();

    enum class ModuleType { Synth, Drum };

    struct PresetEntry
    {
        juce::String name;
        bool         isFactory = false;
        juce::File   file;  // empty for factory presets
        juce::var    factoryData; // populated for factory presets
    };

    //==========================================================================
    const juce::Array<PresetEntry>& getSynthPresets()  const noexcept { return m_synthPresets; }
    const juce::Array<PresetEntry>& getDrumPresets()   const noexcept { return m_drumPresets;  }

    /** Load preset data.  For factory: returns pre-built var.  For user: reads file. */
    static juce::var loadPresetData (const PresetEntry& entry);

    //==========================================================================
    // Apply preset to a live processor (message thread)

    static void applySynthPreset (const juce::var& preset, ModuleProcessor& proc);
    static void applyDrumPreset  (const juce::var& preset, DrumModuleState& state);

    //==========================================================================
    // Capture current processor state as a JSON var

    static juce::var captureSynthPreset (ModuleProcessor& proc, const juce::String& name);
    static juce::var captureDrumPreset  (const DrumModuleState& state, const juce::String& name);

    //==========================================================================
    // Save user preset to disk; appends / replaces entry in the list

    bool saveUserSynthPreset (const juce::String& name, const juce::var& data);
    bool saveUserDrumPreset  (const juce::String& name, const juce::var& data);

    /** Rescans user-preset folder for changes (call after external save). */
    void rescanUserPresets();

    juce::File getUserSynthPresetsDir() const;
    juce::File getUserDrumPresetsDir()  const;

private:
    PresetManager();

    juce::Array<PresetEntry> m_synthPresets;
    juce::Array<PresetEntry> m_drumPresets;

    void buildFactoryPresets();
    void scanUserPresets();

    void addFactorySynth (const juce::String& name, juce::var data);
    void addFactoryDrum  (const juce::String& name, juce::var data);

    static juce::var makeObj();
    static void      setProp (juce::var& obj, const juce::String& key, juce::var val);
    static float     getF    (const juce::var& obj, const juce::String& key, float def = 0.f);
    static int       getI    (const juce::var& obj, const juce::String& key, int   def = 0);
    static int64_t   getInt64 (const juce::var& obj, const juce::String& key, int64_t def = 0);

    static juce::var buildSynthPreset (const juce::String& name,
                                       int o1sh, int o2sh,
                                       float o1det, float o2det, int o2oct, float o2lv,
                                       float fcut, float fres, float fenv,
                                       float ampA, float ampD, float ampS, float ampR,
                                       float filtA, float filtD, float filtS, float filtR,
                                       float lfoR, float lfoD, int lfoT,
                                       int mono,
                                       float charA, float charD, float charDr);

    static juce::var stateToVar (const DrumModuleState& state, const juce::String& name);
    static juce::var voiceToVar (const DrumVoiceState& voice);

    static DrumModuleState varToDrumState (const juce::var& v);
    static DrumVoiceState  varToVoice     (const juce::var& v, int fallbackIndex);

    bool savePreset (const juce::File& dir, const juce::String& name, const juce::var& data);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};

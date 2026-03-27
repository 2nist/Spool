#pragma once

#include <juce_core/juce_core.h>

//==============================================================================
/**
    ModuleProcessor — abstract base for all Spool DSP instrument cores.

    Each instrument family (PolySynth, DrumMachine, Reel) implements this
    interface.  The base provides:
      - A parameter manifest (ParamDef array) describing every automatable param.
      - Generic setParam / getParam routing (safe from the message thread).
      - Binary state serialisation for presets and VST3 getStateInformation.
      - Module identity strings (for VST3 plugin descriptors, future IPC, etc.).

    DSP lifecycle (prepare / process / reset) is NOT in this base because the
    process() signature differs between families (MIDI const vs. non-const;
    noexcept guarantees).  Concrete processors carry their own process() method.
    A standalone VST3 wrapper calls the concrete type directly alongside the
    ModuleProcessor* for parameter access.

    Thread safety
    =============
    - setParam / getParam — safe to call from the message thread while audio runs
      (implementations must use atomics or equivalent).
    - getState / setState — call from the message thread only (may allocate).
    - getParamDefs / getNumParams — safe from any thread (returns static data).
*/
class ModuleProcessor
{
public:
    virtual ~ModuleProcessor() = default;

    //==========================================================================
    // Identity

    /** Reverse-DNS style identifier, e.g. "com.spool.polysynth". */
    virtual const char* getModuleId()   const noexcept = 0;

    /** Human-readable display name, e.g. "PolySynth". */
    virtual const char* getModuleName() const noexcept = 0;

    //==========================================================================
    // Parameter manifest

    struct ParamDef
    {
        const char* id;          ///< Canonical string key, e.g. "filter.cutoff"
        const char* name;        ///< Short display name, e.g. "Cutoff"
        const char* group;       ///< Section grouping for UI, e.g. "filter"
        float       minVal;
        float       maxVal;
        float       defaultVal;
        bool        isDiscrete;  ///< true → integer values (shapes, modes)
    };

    /** Returns a pointer to a static, null-terminated array of ParamDef.
        The array length equals getNumParams().  Never null. */
    virtual const ParamDef* getParamDefs()  const noexcept = 0;
    virtual int              getNumParams() const noexcept = 0;

    //==========================================================================
    // Parameter access  (message-thread safe via atomics in implementations)

    /** Write a parameter by its canonical ID.  Unknown IDs are silently ignored. */
    virtual void  setParam (juce::StringRef id, float value) noexcept = 0;

    /** Read the current value of a parameter.  Returns 0.0f for unknown IDs. */
    virtual float getParam (juce::StringRef id) const noexcept = 0;

    //==========================================================================
    // State serialisation  (message thread only)

    /** Serialise all parameter values into dest.
        Format is implementation-defined; must round-trip through setState(). */
    virtual void getState (juce::MemoryBlock& dest) const = 0;

    /** Restore state previously saved by getState().
        Returns false and leaves state unchanged if data is unrecognised. */
    virtual bool setState (const void* data, int sizeBytes) = 0;

    //==========================================================================
    // Helpers

    /** Reset all parameters to their default values (from getParamDefs()). */
    void resetToDefaults() noexcept
    {
        const auto* defs = getParamDefs();
        const int   n    = getNumParams();
        for (int i = 0; i < n; ++i)
            setParam (defs[i].id, defs[i].defaultVal);
    }
};

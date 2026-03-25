#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "ReelBuffer.h"
#include "ReelParams.h"
#include "ReelLoopPlayer.h"
#include "ReelGranular.h"
#include "ReelSampler.h"
#include "ReelSlicer.h"

//==============================================================================
/**
    ReelProcessor — top-level instrument processor for one REEL slot.

    Owns the buffer, params, and all mode sub-processors.
    Dispatches process() to the active mode's engine.

    Thread safety:
      - loadFromBuffer / loadFromFile: message thread only
      - setParam / setMode:            message thread only (uses simple copy)
      - process:                       audio thread only
      - getBuffer / getParams:         read-only, safe after load
*/
class ReelProcessor
{
public:
    ReelProcessor();

    //==========================================================================
    // Lifecycle

    void prepare (double sampleRate, int blockSize);
    void reset();

    //==========================================================================
    // Audio processing

    void process (juce::AudioBuffer<float>& audio,
                  juce::MidiBuffer&         midi);

    //==========================================================================
    // Buffer loading (message thread)

    void loadFromBuffer (const juce::AudioBuffer<float>& src,
                         double sampleRate = 44100.0);
    bool loadFromFile   (const juce::File& file);
    void clearBuffer();

    bool isLoaded() const noexcept { return m_buffer.isLoaded(); }

    //==========================================================================
    // Mode

    void     setMode (ReelMode mode);
    ReelMode getMode() const noexcept { return m_params.mode; }

    //==========================================================================
    // Params

    void  setParam (const juce::String& paramId, float value);
    float getParam (const juce::String& paramId) const noexcept;

    //==========================================================================
    // Direct slice trigger (from step sequencer — audio thread safe)

    void triggerSlice (int sliceIndex, float velocity) noexcept;

    //==========================================================================
    // Transport sync

    void setTransportPosition (double beat, double bpm) noexcept;

    //==========================================================================
    // Read-only access for display

    const ReelBuffer& getBuffer() const noexcept { return m_buffer; }
    const ReelParams& getParams() const noexcept { return m_params; }

    /** Grain positions snapshot for Zone A grain cloud display. */
    int getGrainPositions (float* positions, int maxPositions) const noexcept;

    //==========================================================================
    // State persistence

    void getState (juce::MemoryBlock& destData) const;
    void setState (const void* data, int sizeInBytes);

private:
    ReelBuffer     m_buffer;
    ReelParams     m_params;
    ReelLoopPlayer m_loopPlayer;
    ReelGranular   m_granular;
    ReelSampler    m_sampler;
    ReelSlicer     m_slicer;

    double m_sampleRate { 44100.0 };
    int    m_blockSize  { 256 };

    /** Push current params to all sub-processors. */
    void syncParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReelProcessor)
};

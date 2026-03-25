#pragma once

#include <juce_dsp/juce_dsp.h>
#include "../components/ZoneC/ChainState.h"

//==============================================================================
/**
    FXChainProcessor — applies the Zone C FX chain to an audio buffer.

    Each slot in SpoolAudioGraph owns one FXChainProcessor.

    Design:
      - Pre-allocates one DSP node per effect type (no allocation in process()).
      - Chain order + bypass stored as atomic ints (lock-free reads from audio thread).
      - Per-effect-type params stored as atomic floats — updated from UI thread,
        read from audio thread without locks.
      - rebuildFromChain() is called from the MESSAGE thread when Zone C changes.
      - setParam() is called from the MESSAGE thread when a knob is dragged.

    Real-time safe: process() does not allocate, lock, or call UI functions.
*/
class FXChainProcessor
{
public:
    static constexpr int kMaxChain  = 8;
    static constexpr int kMaxParams = 8;

    //--- Effect type enum (must match EffectRegistry IDs) ---------------------
    enum class FXType : int
    {
        None        = -1,
        Filter      =  0,
        Reverb      =  1,
        EQ          =  2,
        Compressor  =  3,
        Delay       =  4,
        Distortion  =  5,
        Chorus      =  6,
        Count       =  7
    };

    static FXType typeFromString (const juce::String& id) noexcept;

    //--------------------------------------------------------------------------
    FXChainProcessor();
    ~FXChainProcessor() = default;

    /** Called once (message thread) before any audio is produced. */
    void prepare (double sampleRate, int blockSize);

    /** Called from MESSAGE thread when Zone C chain changes.
        Atomically updates which effects are active and in what order.
        Does NOT allocate — DSP nodes are pre-allocated in prepare(). */
    void rebuildFromChain (const ChainState& chain);

    /** Called from MESSAGE thread when a parameter knob is dragged.
        nodeIndex = position in current chain (0-based).
        paramIndex = param slot within that effect type. */
    void setParam (int nodeIndex, int paramIndex, float value) noexcept;

    /** Called from AUDIO thread — applies the current chain to buffer in-place. */
    void process (juce::AudioBuffer<float>& buffer);

    /** Reset DSP state (e.g. reverb tail, delay feedback). */
    void reset();

private:
    //--- Chain config (atomic — written on message thread, read on audio thread)
    std::atomic<int>  m_chainType  [kMaxChain];
    std::atomic<bool> m_chainBypass[kMaxChain];
    std::atomic<int>  m_chainLength { 0 };

    //--- Per-effect-type atomic params ----------------------------------------
    // Index: [FXType][paramIndex]
    std::atomic<float> m_params[static_cast<int> (FXType::Count)][kMaxParams];

    //--- DSP nodes (one instance per effect type, prepared once) --------------
    juce::dsp::StateVariableTPTFilter<float>  m_filter;
    juce::dsp::Reverb                          m_reverb;
    juce::dsp::IIR::Filter<float>              m_eqLow, m_eqMid, m_eqHigh;
    juce::dsp::Compressor<float>               m_compressor;
    juce::dsp::Chorus<float>                   m_chorus;

    // Delay: manual implementation using DelayLine (stereo)
    static constexpr int kMaxDelaySamples = 192001; // ~2s at 96kHz
    juce::dsp::DelayLine<float>  m_delayL { kMaxDelaySamples };
    juce::dsp::DelayLine<float>  m_delayR { kMaxDelaySamples };

    double m_sampleRate = 44100.0;
    int    m_blockSize  = 256;
    bool   m_prepared   = false;

    //--- Per-effect apply helpers (audio thread) ------------------------------
    void applyFilter      (juce::AudioBuffer<float>& buf, const float* p);
    void applyReverb      (juce::AudioBuffer<float>& buf, const float* p);
    void applyEQ          (juce::AudioBuffer<float>& buf, const float* p);
    void applyCompressor  (juce::AudioBuffer<float>& buf, const float* p);
    void applyDelay       (juce::AudioBuffer<float>& buf, const float* p);
    void applyDistortion  (juce::AudioBuffer<float>& buf, const float* p);
    void applyChorus      (juce::AudioBuffer<float>& buf, const float* p);

    void updateFilterParams   (const float* p);
    void updateReverbParams   (const float* p);
    void updateEQParams       (const float* p);
    void updateCompressorParams (const float* p);
    void updateChorusParams   (const float* p);
};

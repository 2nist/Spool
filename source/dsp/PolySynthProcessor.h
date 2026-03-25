#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "PolyVoice.h"

//==============================================================================
/**
    PolySynthProcessor — polyphonic / monophonic subtractive synthesizer.

    8-voice polyphony with:
      - 2 phase-accumulator oscillators per voice (saw / square / sine / tri / noise)
      - Moog-style ladder filter (LPF24) with cutoff, resonance, and drive
      - Amplitude ADSR envelope
      - Filter ADSR envelope with depth control
      - Global sine LFO routable to pitch or filter cutoff
      - Poly / mono mode (mono retriggers pitch, holds legato)

    All parameter setters are atomic — safe to call from the UI thread while
    audio is running.  DSP state is entirely audio-thread-local; no locks.

    Usage:
        synth.prepare (sampleRate, maxBlock);   // once, off audio thread
        synth.process (outputBuffer, midiBuf);  // each block, audio thread
*/
class PolySynthProcessor
{
public:
    static constexpr int kNumVoices = 8;

    /** Waveform index passed to setOsc1Shape / setOsc2Shape. */
    enum Waveform { Saw = 0, Square, Sine, Triangle, Noise };

    /** LFO modulation target. */
    enum LfoTarget { LfoToPitch = 0, LfoToFilter };

    //==========================================================================
    PolySynthProcessor() = default;

    void prepare (double sampleRate, int maxBlock);
    void process (juce::AudioBuffer<float>& output, const juce::MidiBuffer& midi);
    void reset();

    //==========================================================================
    // Parameter setters — safe to call from the UI thread

    // --- Oscillators ---
    void setOsc1Shape  (int shape)    noexcept { m_osc1Shape.store (shape);   }
    void setOsc2Shape  (int shape)    noexcept { m_osc2Shape.store (shape);   }
    void setOsc1Detune (float cents)  noexcept { m_osc1Detune.store (cents);  }
    void setOsc2Detune (float cents)  noexcept { m_osc2Detune.store (cents);  }
    void setOsc2Octave (int octave)   noexcept { m_osc2Octave.store (octave); }
    void setOsc2Level  (float level)  noexcept { m_osc2Level.store (level);   }

    // --- Filter ---
    void setFilterCutoff    (float hz)  noexcept { m_filterCutoff.store (hz);    }
    void setFilterResonance (float res) noexcept { m_filterRes.store (res);      }
    void setFilterEnvAmt    (float amt) noexcept { m_filterEnvAmt.store (amt);   }

    // --- Amp envelope ---
    void setAmpAttack  (float s) noexcept { m_ampA.store (s); }
    void setAmpDecay   (float s) noexcept { m_ampD.store (s); }
    void setAmpSustain (float v) noexcept { m_ampS.store (v); }
    void setAmpRelease (float s) noexcept { m_ampR.store (s); }

    // --- Filter envelope ---
    void setFiltAttack  (float s) noexcept { m_filtA.store (s); }
    void setFiltDecay   (float s) noexcept { m_filtD.store (s); }
    void setFiltSustain (float v) noexcept { m_filtS.store (v); }
    void setFiltRelease (float s) noexcept { m_filtR.store (s); }

    // --- LFO ---
    void setLfoRate   (float hz)     noexcept { m_lfoRate.store (hz);     }
    void setLfoDepth  (float depth)  noexcept { m_lfoDepth.store (depth); }
    void setLfoTarget (int  target)  noexcept { m_lfoTarget.store (target); }

    // --- Mode ---
    void setMonoMode (bool mono) noexcept { m_monoMode.store (mono); }

    //==========================================================================
    // Getters (for UI readback)

    float getFilterCutoff()    const noexcept { return m_filterCutoff.load(); }
    float getFilterResonance() const noexcept { return m_filterRes.load();    }
    int   getOsc1Shape()       const noexcept { return m_osc1Shape.load();    }
    int   getOsc2Shape()       const noexcept { return m_osc2Shape.load();    }
    bool  isMonoMode()         const noexcept { return m_monoMode.load();     }

private:
    //==========================================================================
    // DSP state — audio thread only

    PolyVoice m_voices[kNumVoices];
    double    m_sampleRate { 44100.0 };
    int       m_maxBlock   { 512     };
    double    m_lfoPhase   { 0.0     };

    //==========================================================================
    // Parameters — atomics so UI thread can write freely

    std::atomic<int>   m_osc1Shape  { Saw   };
    std::atomic<int>   m_osc2Shape  { Saw   };
    std::atomic<float> m_osc1Detune { 0.0f  };  // cents
    std::atomic<float> m_osc2Detune { 7.0f  };  // cents (slight detune by default)
    std::atomic<int>   m_osc2Octave { 0     };   // -2..+2 octaves
    std::atomic<float> m_osc2Level  { 0.7f  };

    std::atomic<float> m_filterCutoff { 4000.0f };  // Hz
    std::atomic<float> m_filterRes    { 0.3f    };  // 0..1
    std::atomic<float> m_filterEnvAmt { 0.5f    };  // 0..1

    std::atomic<float> m_ampA { 0.01f };  // seconds
    std::atomic<float> m_ampD { 0.1f  };
    std::atomic<float> m_ampS { 0.8f  };  // 0..1
    std::atomic<float> m_ampR { 0.3f  };

    std::atomic<float> m_filtA { 0.01f };
    std::atomic<float> m_filtD { 0.2f  };
    std::atomic<float> m_filtS { 0.5f  };
    std::atomic<float> m_filtR { 0.3f  };

    std::atomic<float> m_lfoRate   { 5.0f         };  // Hz
    std::atomic<float> m_lfoDepth  { 0.0f         };  // 0..1
    std::atomic<int>   m_lfoTarget { LfoToPitch    };

    std::atomic<bool>  m_monoMode  { false };

    //==========================================================================
    // Private helpers — audio thread only

    void handleMidi (const juce::MidiBuffer& midi);
    void noteOn     (int note, float velocity,
                     const juce::ADSR::Parameters& ampP,
                     const juce::ADSR::Parameters& filtP);
    void noteOff    (int note);

    void renderVoice (PolyVoice& v, int numSamples,
                      int osc1Shape, int osc2Shape,
                      double osc1Freq, double osc2Freq, float osc2Level,
                      float baseCutoff, float filterRes, float filterEnvAmt,
                      float lfoValue, int lfoTarget) noexcept;

    static float renderOsc (double& phase, double phaseInc, int shape) noexcept;

    PolyVoice* findFreeVoice    ()        noexcept;
    PolyVoice* findVoiceForNote (int note) noexcept;
    PolyVoice* stealVoice       ()        noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolySynthProcessor)
};

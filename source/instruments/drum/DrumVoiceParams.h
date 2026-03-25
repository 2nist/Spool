#pragma once

#include <cmath>

//==============================================================================
/**
    DrumVoiceParams — authoritative parameter set for one unified drum voice.

    All four synthesis layers (tone, noise, click, metal) are always present;
    voice TYPE presets just set intelligent defaults and enable/disable layers.

    These are plain data — no JUCE types — so they can be copied freely
    across threads via atomic double-buffer or snapshot pattern.
*/
struct DrumVoiceParams
{
    //==========================================================================
    // TONE LAYER — pitched oscillator with pitch envelope
    struct ToneParams
    {
        bool  enable      = true;
        float pitch       = 55.0f;   // Hz — base (target) frequency
        float pitchstart  = 3.5f;    // multiplier: start pitch = pitch * pitchstart
        float pitchdecay  = 0.06f;   // seconds — pitch fall time
        float decay       = 0.6f;    // seconds — amplitude decay
        float level       = 1.0f;    // 0..1
        bool  triangleWave= false;   // false=sine, true=triangle
    } tone;

    //==========================================================================
    // NOISE LAYER — filtered broadband noise
    struct NoiseParams
    {
        bool  enable  = true;
        float decay   = 0.02f;    // seconds
        float level   = 0.15f;   // 0..1
        float hpfreq  = 80.0f;   // Hz — high-pass cutoff
        float lpfreq  = 800.0f;  // Hz — low-pass cutoff
        float color   = 0.0f;    // 0=white, 1=pink (1-pole tint)
    } noise;

    //==========================================================================
    // CLICK LAYER — short transient burst
    struct ClickParams
    {
        bool  enable       = true;
        float level        = 0.6f;    // 0..1
        float decay        = 0.003f;  // seconds (1–20 ms)
        float tone         = 0.7f;    // 0=noise transient, 1=tonal transient
        int   bursts       = 1;       // 1–5
        float burstspacing = 0.010f;  // seconds between bursts (clap)
    } click;

    //==========================================================================
    // METALLIC LAYER — inharmonic oscillator cluster (hat/cymbal)
    struct MetalParams
    {
        bool  enable    = false;
        int   voices    = 6;          // 2–8 oscillators
        float basefreq  = 400.0f;     // Hz — fundamental of the cluster

        // Frequency ratios (osc freq = basefreq * ratio[i])
        // 808-style: 2.0, 3.16, 4.16, 5.43, 6.79, 8.21
        float ratios[8] = { 2.0f, 3.16f, 4.16f, 5.43f, 6.79f, 8.21f, 9.0f, 10.2f };

        float decay   = 0.04f;    // seconds
        float level   = 1.0f;
        float hpfreq  = 2000.0f;  // Hz — keeps only the bright metallic content
    } metal;

    //==========================================================================
    // OUTPUT
    struct OutParams
    {
        float level    = 1.0f;   // 0..2
        float pan      = 0.0f;   // -1..+1
        float tune     = 0.0f;   // semitones, -24..+24
        float velocity = 0.8f;   // velocity sensitivity 0..1
    } out;

    //==========================================================================
    // MIDI
    int midiNote = 36;   // GM drum note

    //==========================================================================
    // Voice-type factory methods (GM defaults)

    static DrumVoiceParams kick()
    {
        DrumVoiceParams p;
        p.tone   = { true,  55.0f,  3.5f,  0.06f, 0.6f,  1.0f, false };
        p.noise  = { true,  0.02f, 0.15f,  80.0f, 800.0f, 0.0f };
        p.click  = { true,  0.6f,  0.003f, 0.7f,  1, 0.010f };
        p.metal.enable = false;
        p.out    = { 1.0f, 0.0f, 0.0f, 0.8f };
        p.midiNote = 36;
        return p;
    }

    static DrumVoiceParams snare()
    {
        DrumVoiceParams p;
        p.tone   = { true,  180.0f, 1.2f,  0.015f, 0.12f, 0.6f, false };
        p.noise  = { true,  0.15f,  0.8f,  300.0f, 8000.0f, 0.0f };
        p.click  = { true,  0.9f,   0.004f, 0.3f,  1, 0.010f };
        p.metal.enable = false;
        p.out    = { 1.0f, 0.0f, 0.0f, 0.9f };
        p.midiNote = 38;
        return p;
    }

    static DrumVoiceParams closedHat()
    {
        DrumVoiceParams p;
        p.tone.enable  = false;
        p.noise  = { true,  0.04f, 0.3f,  6000.0f, 18000.0f, 0.0f };
        p.click  = { true,  0.5f,  0.002f, 0.2f,   1, 0.010f };
        p.metal  = { true,  6, 400.0f,
                     { 2.0f, 3.16f, 4.16f, 5.43f, 6.79f, 8.21f, 9.0f, 10.2f },
                     0.04f, 1.0f, 2000.0f };
        p.out    = { 1.0f, 0.0f, 0.0f, 0.7f };
        p.midiNote = 42;
        return p;
    }

    static DrumVoiceParams openHat()
    {
        DrumVoiceParams p = closedHat();
        p.metal.decay = 0.4f;
        p.noise.decay = 0.35f;
        p.midiNote    = 46;
        return p;
    }

    static DrumVoiceParams clap()
    {
        DrumVoiceParams p;
        p.tone.enable  = false;
        p.noise  = { true,  0.2f,  1.0f,  800.0f, 12000.0f, 0.0f };
        p.click  = { true,  1.0f,  0.008f, 0.0f,  4, 0.010f };
        p.metal.enable = false;
        p.out    = { 1.0f, 0.0f, 0.0f, 0.8f };
        p.midiNote = 39;
        return p;
    }

    static DrumVoiceParams tom (float pitchHz = 100.0f, int gmNote = 45)
    {
        DrumVoiceParams p;
        p.tone   = { true,  pitchHz, 1.8f,  0.04f, 0.3f,  1.0f, false };
        p.noise  = { true,  0.05f,   0.2f,  200.0f, 3000.0f, 0.0f };
        p.click  = { true,  0.4f,   0.003f, 0.5f,  1, 0.010f };
        p.metal.enable = false;
        p.out    = { 1.0f, 0.0f, 0.0f, 0.8f };
        p.midiNote = gmNote;
        return p;
    }

    static DrumVoiceParams perc()
    {
        DrumVoiceParams p = tom (300.0f, 60);
        p.tone.decay   = 0.08f;
        p.tone.pitchstart = 1.5f;
        p.midiNote     = 60;
        return p;
    }

    static DrumVoiceParams cymbal()
    {
        DrumVoiceParams p = openHat();
        p.metal.decay   = 1.2f;
        p.noise.decay   = 0.8f;
        p.metal.hpfreq  = 3000.0f;
        p.midiNote      = 51;
        return p;
    }
};

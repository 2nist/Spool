#pragma once

#include <array>
#include <cmath>

enum class DrumVoiceRole
{
    kick,
    subKick,
    snare,
    hat,
    perc1,
    perc2,
    clap,
    tom,
    cymbal,
    custom
};

inline const char* toString (DrumVoiceRole role) noexcept
{
    switch (role)
    {
        case DrumVoiceRole::kick:    return "kick";
        case DrumVoiceRole::subKick: return "subkick";
        case DrumVoiceRole::snare:   return "snare";
        case DrumVoiceRole::hat:     return "hat";
        case DrumVoiceRole::perc1:   return "perc1";
        case DrumVoiceRole::perc2:   return "perc2";
        case DrumVoiceRole::clap:    return "clap";
        case DrumVoiceRole::tom:     return "tom";
        case DrumVoiceRole::cymbal:  return "cymbal";
        case DrumVoiceRole::custom:  return "custom";
    }

    return "custom";
}

//==============================================================================
/**
    DrumVoiceParams — authoritative parameter set for one unified drum voice.

    All synthesis layers remain available for every voice. Role factories provide
    strong defaults, while tone/noise/click/metal plus light character controls
    allow the same engine to cover kick, sub, snare, hats, and machine percussion.
*/
struct DrumVoiceParams
{
    //==========================================================================
    // Tone — pitched oscillator layer (kick body, sub, tonal percussion)
    struct ToneParams
    {
        bool  enable       = true;
        float pitch        = 80.0f;    // Hz base frequency
        float pitchstart   = 2.0f;     // pitch multiplier at attack (1.0 = no sweep)
        float pitchdecay   = 0.05f;    // seconds for pitch envelope to settle
        float decay        = 0.30f;    // seconds for amplitude decay
        float attack       = 0.003f;   // seconds of attack
        float level        = 0.8f;     // 0..1
        float body         = 0.5f;     // blend between sine and harmonic content
        bool  triangleWave = false;    // false = sine, true = triangle
    } tone;

    //==========================================================================
    // FM (NEW Phase 1)
    struct FmParams
    {
        bool  enable        = false;
        float level         = 0.0f;      // 0..1
        float decay         = 0.3f;      // seconds
        float carrierFreq   = 220.0f;    // Hz base
        float modRatio      = 2.0f;      // 0.5..8.0
        float modIndex      = 4.0f;      // 0..16 semitones deviation
    } fm;

    struct NoiseParams
    {
        bool  enable = true;
        float decay  = 0.02f;     // seconds
        float level  = 0.15f;     // 0..1
        float hpfreq = 80.0f;     // Hz
        float lpfreq = 800.0f;    // Hz
        float color  = 0.0f;      // 0=white, 1=pink
    } noise;

    struct ClickParams
    {
        bool  enable       = true;
        float level        = 0.6f;    // 0..1
        float decay        = 0.003f;  // seconds
        float tone         = 0.7f;    // 0=noise transient, 1=tonal transient
        float pitch        = 2000.0f; // Hz for tonal transient
        int   bursts       = 1;       // 1–5
        float burstspacing = 0.010f;  // seconds between bursts
    } click;

    struct MetalParams
    {
        bool  enable   = false;
        int   voices   = 6;
        float basefreq = 400.0f;
        std::array<float, 8> ratios { 2.0f, 3.16f, 4.16f, 5.43f, 6.79f, 8.21f, 9.0f, 10.2f };
        float decay    = 0.04f;
        float level    = 1.0f;
        float hpfreq   = 2000.0f;
    } metal;

    struct CharacterParams
    {
        float drive = 0.2f;  // density / saturation / punch
        float grit  = 0.1f;  // roughness / paper / age
        float snap  = 0.2f;  // transient emphasis / contour
    } character;

    struct OutParams
    {
        float level    = 1.0f;   // 0..2
        float pan      = 0.0f;   // -1..+1
        float tune     = 0.0f;   // semitones, -24..+24
        float velocity = 0.8f;   // velocity sensitivity 0..1
    } out;

    DrumVoiceRole role = DrumVoiceRole::kick;
    int midiNote = 36;

    // FM defaults for roles
    static DrumVoiceParams withFmDefaults (DrumVoiceRole role);

    static int defaultMidiNote (DrumVoiceRole role) noexcept
    {
        switch (role)
        {
            case DrumVoiceRole::kick:    return 36;
            case DrumVoiceRole::subKick: return 35;
            case DrumVoiceRole::snare:   return 38;
            case DrumVoiceRole::hat:     return 42;
            case DrumVoiceRole::perc1:   return 39;
            case DrumVoiceRole::perc2:   return 56;
            case DrumVoiceRole::clap:    return 39;
            case DrumVoiceRole::tom:     return 45;
            case DrumVoiceRole::cymbal:  return 51;
            case DrumVoiceRole::custom:  return 36;
        }

        return 36;
    }

    static const char* defaultName (DrumVoiceRole role) noexcept
    {
        switch (role)
        {
            case DrumVoiceRole::kick:    return "KICK";
            case DrumVoiceRole::subKick: return "SUB";
            case DrumVoiceRole::snare:   return "SNARE";
            case DrumVoiceRole::hat:     return "HAT";
            case DrumVoiceRole::perc1:   return "PERC 1";
            case DrumVoiceRole::perc2:   return "PERC 2";
            case DrumVoiceRole::clap:    return "CLAP";
            case DrumVoiceRole::tom:     return "TOM";
            case DrumVoiceRole::cymbal:  return "CYM";
            case DrumVoiceRole::custom:  return "VOICE";
        }

        return "VOICE";
    }

    static DrumVoiceParams kick()
    {
        DrumVoiceParams p;
        p.role   = DrumVoiceRole::kick;
        p.tone   = { true, 72.0f, 1.85f, 0.040f, 0.0035f, 0.280f, 1.00f, 0.88f, false };
        p.noise  = { true, 0.012f, 0.025f, 240.0f, 1800.0f, 0.05f };
        p.click  = { true, 0.080f, 0.0016f, 0.22f, 1450.0f, 1, 0.010f };
        p.fm     = { false, 0.0f, 0.3f, 60.0f, 1.0f, 2.0f };  // Tuned sub pulse
        p.character = { 0.28f, 0.08f, 0.16f };
        p.out    = { 1.10f, 0.0f, 0.0f, 0.75f };
        p.midiNote = defaultMidiNote (p.role);
        return p;
    }

    static DrumVoiceParams subKick()
    {
        DrumVoiceParams p;
        p.role   = DrumVoiceRole::subKick;
        p.tone   = { true, 48.0f, 1.35f, 0.055f, 0.0060f, 0.380f, 0.95f, 1.00f, false };
        p.noise  = { true, 0.010f, 0.010f, 120.0f, 900.0f, 0.10f };
        p.click  = { false, 0.0f, 0.0020f, 0.1f, 1000.0f, 1, 0.010f };
        p.character = { 0.22f, 0.03f, 0.05f };
        p.out    = { 0.95f, 0.0f, -2.0f, 0.60f };
        p.midiNote = defaultMidiNote (p.role);
        return p;
    }

    static DrumVoiceParams snare()
    {
        DrumVoiceParams p;
        p.role   = DrumVoiceRole::snare;
        p.tone   = { true, 208.0f, 1.18f, 0.020f, 0.0015f, 0.160f, 0.75f, 0.58f, false };
        p.noise  = { true, 0.185f, 0.78f, 900.0f, 10500.0f, 0.08f };
        p.click  = { true, 0.62f, 0.0055f, 0.18f, 2800.0f, 1, 0.010f };
        p.fm     = { true, 0.45f, 0.12f, 440.0f, 4.0f, 8.0f };  // Metallic ring tail
        p.character = { 0.26f, 0.13f, 0.82f };
        p.out    = { 1.00f, 0.0f, 0.0f, 0.85f };
        p.midiNote = defaultMidiNote (p.role);
        return p;
    }

    static DrumVoiceParams closedHat()
    {
        DrumVoiceParams p;
        p.role = DrumVoiceRole::hat;
        p.tone.enable = false;
        p.noise = { true, 0.026f, 0.20f, 6800.0f, 18000.0f, 0.02f };
        p.click = { true, 0.30f, 0.0012f, 0.06f, 6800.0f, 1, 0.010f };
        p.metal = { true, 6, 540.0f,
                    { 2.0f, 3.17f, 4.21f, 5.42f, 6.83f, 8.18f, 9.31f, 10.47f },
                    0.030f, 0.95f, 4600.0f };
        p.fm     = { true, 0.65f, 0.018f, 1200.0f, 6.4f, 12.0f };  // Hi-shimmer FM
        p.character = { 0.18f, 0.10f, 0.92f };
        p.out = { 0.95f, 0.0f, 0.0f, 0.72f };
        p.midiNote = defaultMidiNote (p.role);
        return p;
    }

    static DrumVoiceParams openHat()
    {
        DrumVoiceParams p = closedHat();
        p.noise.decay = 0.180f;
        p.metal.decay = 0.260f;
        p.click.level = 0.22f;
        p.midiNote    = 46;
        return p;
    }

    static DrumVoiceParams clap()
    {
        DrumVoiceParams p;
        p.role = DrumVoiceRole::clap;
        p.tone.enable = false;
        p.noise = { true, 0.16f, 0.82f, 1100.0f, 12000.0f, 0.12f };
        p.click = { true, 0.88f, 0.0065f, 0.02f, 2600.0f, 4, 0.009f };
        p.character = { 0.18f, 0.22f, 0.78f };
        p.out = { 1.0f, 0.0f, 0.0f, 0.78f };
        p.midiNote = defaultMidiNote (p.role);
        return p;
    }

    static DrumVoiceParams tom (float pitchHz = 100.0f, int gmNote = 45)
    {
        DrumVoiceParams p;
        p.role   = DrumVoiceRole::tom;
        p.tone   = { true, pitchHz, 1.6f, 0.035f, 0.0015f, 0.230f, 0.92f, 0.55f, false };
        p.noise  = { true, 0.045f, 0.12f, 250.0f, 3500.0f, 0.04f };
        p.click  = { true, 0.24f, 0.0028f, 0.38f, 1700.0f, 1, 0.010f };
        p.character = { 0.18f, 0.08f, 0.36f };
        p.out    = { 1.0f, 0.0f, 0.0f, 0.8f };
        p.midiNote = gmNote;
        return p;
    }

    static DrumVoiceParams perc()
    {
        DrumVoiceParams p;
        p.role   = DrumVoiceRole::perc1;
        p.tone   = { true, 320.0f, 1.95f, 0.026f, 0.0012f, 0.070f, 0.72f, 0.34f, false };
        p.noise  = { true, 0.017f, 0.18f, 1800.0f, 9000.0f, 0.06f };
        p.click  = { true, 0.56f, 0.0025f, 0.45f, 2300.0f, 1, 0.010f };
        p.character = { 0.20f, 0.18f, 0.62f };
        p.out    = { 0.95f, 0.0f, 0.0f, 0.8f };
        p.midiNote = defaultMidiNote (p.role);
        return p;
    }

    static DrumVoiceParams perc2()
    {
        DrumVoiceParams p;
        p.role   = DrumVoiceRole::perc2;
        p.tone   = { true, 540.0f, 2.60f, 0.012f, 0.0008f, 0.040f, 0.48f, 0.18f, false };
        p.noise  = { true, 0.012f, 0.24f, 2600.0f, 12500.0f, 0.02f };
        p.click  = { true, 0.78f, 0.0018f, 0.12f, 4200.0f, 1, 0.010f };
        p.character = { 0.26f, 0.22f, 0.74f };
        p.out    = { 0.90f, 0.0f, 0.0f, 0.75f };
        p.midiNote = defaultMidiNote (p.role);
        return p;
    }

    static DrumVoiceParams cymbal()
    {
        DrumVoiceParams p = openHat();
        p.role         = DrumVoiceRole::cymbal;
        p.metal.decay  = 0.90f;
        p.noise.decay  = 0.55f;
        p.metal.hpfreq = 3200.0f;
        p.character    = { 0.14f, 0.18f, 0.72f };
        p.midiNote     = defaultMidiNote (p.role);
        return p;
    }

    static DrumVoiceParams makeForRole (DrumVoiceRole role)
    {
        switch (role)
        {
            case DrumVoiceRole::kick:    return kick();
            case DrumVoiceRole::subKick: return subKick();
            case DrumVoiceRole::snare:   return snare();
            case DrumVoiceRole::hat:     return closedHat();
            case DrumVoiceRole::perc1:   return perc();
            case DrumVoiceRole::perc2:   return perc2();
            case DrumVoiceRole::clap:    return clap();
            case DrumVoiceRole::tom:     return tom();
            case DrumVoiceRole::cymbal:  return cymbal();
            case DrumVoiceRole::custom:  break;
        }

        DrumVoiceParams p = kick();
        p.role = DrumVoiceRole::custom;
        return p;
    }
};

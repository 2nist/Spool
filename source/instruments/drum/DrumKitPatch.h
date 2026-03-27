#pragma once

#include "DrumVoiceParams.h"
#include <vector>
#include <string>

//==============================================================================
/**
    DrumKitEntry — one voice slot in a kit.
*/
struct DrumKitEntry
{
    std::string     name;
    DrumVoiceParams params;
};

//==============================================================================
/**
    DrumKitPatch — a complete drum kit: named collection of voice presets.

    Provides four built-in factory kits covering the most common styles.
*/
struct DrumKitPatch
{
    std::string              name;
    std::vector<DrumKitEntry> voices;

    //==========================================================================
    // Built-in factory kits

    static DrumKitPatch makeSpoolKit()
    {
        DrumKitPatch kit;
        kit.name = "Spool Core";
        kit.voices = {
            { "KICK",   DrumVoiceParams::kick()    },
            { "SUB",    DrumVoiceParams::subKick() },
            { "SNARE",  DrumVoiceParams::snare()   },
            { "HAT",    DrumVoiceParams::closedHat() },
            { "PERC 1", DrumVoiceParams::perc()    },
            { "PERC 2", DrumVoiceParams::perc2()   },
        };
        return kit;
    }

    static DrumKitPatch make808Kit()
    {
        DrumKitPatch kit;
        kit.name = "808 Kit";
        kit.voices = {
            { "KICK",  DrumVoiceParams::kick()       },
            { "SUB",   DrumVoiceParams::subKick()    },
            { "SNARE", DrumVoiceParams::snare()      },
            { "CLAP",  DrumVoiceParams::clap()       },
            { "CH",    DrumVoiceParams::closedHat()  },
            { "OH",    DrumVoiceParams::openHat()    },
            { "PERC 1",DrumVoiceParams::perc()       },
            { "PERC 2",DrumVoiceParams::perc2()      },
        };
        return kit;
    }

    static DrumKitPatch make909Kit()
    {
        DrumKitPatch kit;
        kit.name = "909 Kit";

        // 909 kick: punchier, higher pitch, faster decay
        DrumVoiceParams k909 = DrumVoiceParams::kick();
        k909.tone.pitch       = 75.0f;
        k909.tone.pitchstart  = 2.2f;
        k909.tone.pitchdecay  = 0.04f;
        k909.tone.decay       = 0.24f;
        k909.click.level      = 0.22f;
        k909.character.drive  = 0.36f;

        // 909 snare: brighter, more crack
        DrumVoiceParams s909 = DrumVoiceParams::snare();
        s909.noise.hpfreq     = 1200.0f;
        s909.noise.level      = 0.9f;
        s909.click.level      = 0.78f;

        // 909 hat: shorter, crisper
        DrumVoiceParams ch909 = DrumVoiceParams::closedHat();
        ch909.metal.decay     = 0.025f;
        ch909.noise.decay     = 0.025f;
        ch909.character.snap  = 1.0f;

        DrumVoiceParams oh909 = DrumVoiceParams::openHat();
        oh909.metal.decay     = 0.25f;

        DrumVoiceParams p909 = DrumVoiceParams::perc();
        p909.character.drive = 0.30f;
        p909.click.level     = 0.68f;

        DrumVoiceParams p2909 = DrumVoiceParams::perc2();
        p2909.noise.level     = 0.30f;

        kit.voices = {
            { "KICK",  k909  },
            { "SUB",   DrumVoiceParams::subKick() },
            { "SNARE", s909  },
            { "CLAP",  DrumVoiceParams::clap() },
            { "CH",    ch909 },
            { "OH",    oh909 },
            { "PERC 1", p909 },
            { "PERC 2", p2909 },
        };
        return kit;
    }

    static DrumKitPatch makeAcousticKit()
    {
        DrumKitPatch kit;
        kit.name = "Acoustic Kit";

        // Acoustic kick: warmer, longer, no pitch drop character
        DrumVoiceParams ak = DrumVoiceParams::kick();
        ak.tone.pitch       = 65.0f;
        ak.tone.pitchstart  = 1.6f;
        ak.tone.pitchdecay  = 0.10f;
        ak.tone.decay       = 0.38f;
        ak.noise.level      = 0.3f;
        ak.noise.hpfreq     = 60.0f;

        // Acoustic snare: less noise color, more body tone
        DrumVoiceParams as = DrumVoiceParams::snare();
        as.tone.pitch       = 200.0f;
        as.tone.decay       = 0.18f;
        as.noise.decay      = 0.20f;
        as.noise.hpfreq     = 200.0f;

        // Acoustic hi-hat: use metal with lower base
        DrumVoiceParams ach = DrumVoiceParams::closedHat();
        ach.metal.basefreq  = 300.0f;
        ach.metal.decay     = 0.06f;

        DrumVoiceParams aoh = DrumVoiceParams::openHat();
        aoh.metal.basefreq  = 300.0f;
        aoh.metal.decay     = 0.5f;

        kit.voices = {
            { "KICK",   ak  },
            { "SUB",    DrumVoiceParams::subKick() },
            { "SNARE",  as  },
            { "CLAP",   DrumVoiceParams::clap() },
            { "CH",     ach },
            { "OH",     aoh },
            { "PERC 1", DrumVoiceParams::tom (70.0f, 41) },
            { "PERC 2", DrumVoiceParams::tom (150.0f, 50) },
        };
        return kit;
    }

    static DrumKitPatch makeLoFiKit()
    {
        DrumKitPatch kit;
        kit.name = "Lo-Fi Kit";

        // Lo-fi kick: low, slow, with extra noise grit
        DrumVoiceParams lk = DrumVoiceParams::kick();
        lk.tone.pitch       = 58.0f;
        lk.tone.pitchstart  = 1.8f;
        lk.tone.decay       = 0.32f;
        lk.noise.level      = 0.4f;
        lk.noise.color      = 0.8f;   // pink noise
        lk.noise.lpfreq     = 600.0f;

        // Lo-fi snare: low-pass filtered, dirty
        DrumVoiceParams ls = DrumVoiceParams::snare();
        ls.noise.color      = 0.7f;
        ls.noise.lpfreq     = 5000.0f;
        ls.noise.hpfreq     = 150.0f;
        ls.tone.pitch       = 140.0f;

        // Lo-fi hat: very short, gritty
        DrumVoiceParams lh = DrumVoiceParams::closedHat();
        lh.metal.decay      = 0.015f;
        lh.noise.level      = 0.6f;
        lh.noise.color      = 0.5f;

        DrumVoiceParams loh = DrumVoiceParams::openHat();
        loh.metal.decay     = 0.12f;
        loh.noise.color     = 0.5f;

        kit.voices = {
            { "KICK",  lk  },
            { "SUB",   DrumVoiceParams::subKick() },
            { "SNARE", ls  },
            { "CLAP",  DrumVoiceParams::clap() },
            { "CH",    lh  },
            { "OH",    loh },
            { "PERC 1", DrumVoiceParams::perc() },
            { "PERC 2", DrumVoiceParams::perc2() },
        };
        return kit;
    }
};

#pragma once

#include "../../Theme.h"
#include <array>

//==============================================================================
/**
    QuickControlSlot — one assignable control slot on the Zone B module faceplate.

    A slot carries:
      - paramId   : routing key forwarded via onQuickParamChanged (stable across UI changes)
      - label     : short display label (max ~6 chars)
      - range     : min / max / default values
      - type      : the physical control type rendered (Slider / Knob / Button)
      - behavior  : Continuous / Toggle / Momentary / EnumStep
      - assigned  : false = slot is empty ("--" shown, no DSP connection)

    Slots are stored in FaceplatePanel and can be reassigned at runtime via
    right-click context menus.  The default assignments per module family are
    provided by QuickFaceplateDefaults below.
*/
struct QuickControlSlot
{
    //==========================================================================
    enum class ControlType { Slider, Knob, Button };

    enum class Behavior
    {
        Continuous, ///< Fader/knob drag — fires value on each movement
        Toggle,     ///< Button alternates 0.0 / 1.0 on each press
        Momentary,  ///< Fires 1.0 on press, 0.0 on release
        EnumStep,   ///< Button increments through discrete integer values
    };

    //==========================================================================
    juce::String paramId;                       ///< DSP routing key
    juce::String label;                         ///< Display label (max ~6 chars)
    float        minVal   { 0.f };
    float        maxVal   { 1.f };
    float        defVal   { 0.f };
    ControlType  type     { ControlType::Slider };
    Behavior     behavior { Behavior::Continuous };
    bool         assigned { false };

    //==========================================================================
    // Convenience factories

    static QuickControlSlot empty (ControlType t = ControlType::Slider) noexcept
    {
        QuickControlSlot s;
        s.type     = t;
        s.label    = "--";
        s.assigned = false;
        return s;
    }

    static QuickControlSlot slider (const juce::String& id,
                                    const juce::String& lbl,
                                    float mn, float mx, float def) noexcept
    {
        return { id, lbl, mn, mx, def, ControlType::Slider, Behavior::Continuous, true };
    }

    static QuickControlSlot knob (const juce::String& id,
                                  const juce::String& lbl,
                                  float mn, float mx, float def) noexcept
    {
        return { id, lbl, mn, mx, def, ControlType::Knob, Behavior::Continuous, true };
    }

    static QuickControlSlot toggle (const juce::String& id,
                                    const juce::String& lbl) noexcept
    {
        return { id, lbl, 0.f, 1.f, 0.f, ControlType::Button, Behavior::Toggle, true };
    }

    static QuickControlSlot momentary (const juce::String& id,
                                       const juce::String& lbl) noexcept
    {
        return { id, lbl, 0.f, 1.f, 0.f, ControlType::Button, Behavior::Momentary, true };
    }
};

//==============================================================================
/**
    QuickFaceplateDefaults — standard surface assignments for each module family.

    Each family has:
      kNumSliders  = 8  vertical-fader slider slots
      kNumKnobs    = 2  rotary-knob slots
      kNumButtons  = 4  button slots (toggle / momentary)

    These defaults populate FaceplatePanel on construction.
    Any slot can be overridden at runtime via right-click reassignment.

    Drum paramId format: "vN:field"
      N = voice index in the current kit
      field = tpitch, tdecay, body, clicklvl, noiselv, noisehp, metaldec,
              drive, grit, snap, outl, outp, trig
*/
namespace QuickFaceplateDefaults
{
    static constexpr int kNumSliders = 8;
    static constexpr int kNumKnobs   = 4;
    static constexpr int kNumButtons = 4;

    //==========================================================================
    // SYNTH

    inline std::array<QuickControlSlot, kNumSliders> synthSliders()
    {
        return {
            QuickControlSlot::slider ("CUT",  "CUT",  20.f, 20000.f, 4000.f),
            QuickControlSlot::slider ("RES",  "RES",   0.f,    1.f,   0.3f ),
            QuickControlSlot::slider ("ENVA", "ENVA", -1.f,    1.f,   0.5f ),
            QuickControlSlot::slider ("DET2", "DET2", -50.f,  50.f,   7.f  ),
            QuickControlSlot::slider ("MIX2", "MIX2",  0.f,   1.f,   0.7f ),
            QuickControlSlot::slider ("ATK",  "ATK",   0.f,   2.f,   0.01f),
            QuickControlSlot::slider ("REL",  "REL",   0.f,   3.f,   0.3f ),
            QuickControlSlot::slider ("LFDP", "LFDP",  0.f,   1.f,   0.f  ),
        };
    }

    inline std::array<QuickControlSlot, kNumKnobs> synthKnobs()
    {
        return {
            QuickControlSlot::knob ("char.drive", "GRIT",  0.f,  1.f, 0.20f),  // pre-filter saturation
            QuickControlSlot::knob ("char.asym",  "ASYM",  0.f,  1.f, 0.15f),  // oscillator harmonic shaper
            QuickControlSlot::knob ("amp.sustain","BODY",  0.f,  1.f, 0.7f ),  // amp sustain → held body
            QuickControlSlot::knob ("lfo.depth",  "MOTN",  0.f,  1.f, 0.f  ),  // LFO depth → movement
        };
    }

    inline std::array<QuickControlSlot, kNumButtons> synthButtons()
    {
        return {
            QuickControlSlot::toggle   ("hold",    "HOLD"),
            QuickControlSlot::toggle   ("mono",    "MONO"),
            QuickControlSlot::toggle   ("lfosync", "SYNC"),
            QuickControlSlot::momentary("init",    "INIT"),
        };
    }

    //==========================================================================
    // DRUM
    // Voice layout: v0=kick  v1=sub  v2=snare  v3=hat  v4=perc1  v5=perc2
    // Field names match DrumVoiceParams members:
    //   tpitch   → tone.pitch      tdecay → tone.decay
    //   clicklvl → click.level     noiselv → noise.level
    //   noisehp  → noise.hpfreq    outl → out.level    outp → out.pan
    //   trig     → triggerVoice

    inline std::array<QuickControlSlot, kNumSliders> drumSliders()
    {
        return {
            QuickControlSlot::slider ("v0:body",     "K.BODY",   0.f,   1.f,   0.88f ),
            QuickControlSlot::slider ("v0:tdecay",   "K.DEC",    0.03f, 0.8f,  0.28f ),
            QuickControlSlot::slider ("v1:tdecay",   "SUBDEC",   0.03f, 0.8f,  0.38f ),
            QuickControlSlot::slider ("v2:snap",     "S.SNAP",   0.f,   1.f,   0.82f ),
            QuickControlSlot::slider ("v3:metaldec", "H.TONE",   0.01f, 0.6f,  0.03f ),
            QuickControlSlot::slider ("v3:noisehp",  "H.CUT", 2000.f,18000.f,6800.f ),
            QuickControlSlot::slider ("v4:tpitch",   "P1.TON",  80.f,  800.f, 320.f  ),
            QuickControlSlot::slider ("v5:clicklvl", "P2.CLK",   0.f,   1.f,   0.78f ),
        };
    }

    inline std::array<QuickControlSlot, kNumKnobs> drumKnobs()
    {
        return {
            QuickControlSlot::knob ("v0:drive",  "K.DRV",  0.f, 1.f, 0.28f),
            QuickControlSlot::knob ("v2:grit",   "S.GRIT", 0.f, 1.f, 0.13f),
            QuickControlSlot::knob ("v3:snap",   "H.SNAP", 0.f, 1.f, 0.92f),
            QuickControlSlot::knob ("v4:grit",   "P1.GRT", 0.f, 1.f, 0.18f),
        };
    }

    inline std::array<QuickControlSlot, kNumButtons> drumButtons()
    {
        return {
            QuickControlSlot::momentary ("v0:trig", "TRIG"),
            QuickControlSlot::momentary ("voicepg", "PAGE"),
            QuickControlSlot::toggle    ("muteall", "MUTE"),
            QuickControlSlot::momentary ("initkit", "INIT"),
        };
    }

    //==========================================================================
    // REEL — seam for a future pass
    // Returns unassigned slots so FaceplatePanel shows "--" placeholders.

    inline std::array<QuickControlSlot, kNumSliders> reelSliders()
    {
        std::array<QuickControlSlot, kNumSliders> a;
        a.fill (QuickControlSlot::empty (QuickControlSlot::ControlType::Slider));
        return a;
    }
    inline std::array<QuickControlSlot, kNumKnobs> reelKnobs()
    {
        std::array<QuickControlSlot, kNumKnobs> a;
        a.fill (QuickControlSlot::empty (QuickControlSlot::ControlType::Knob));
        return a;
    }
    inline std::array<QuickControlSlot, kNumButtons> reelButtons()
    {
        std::array<QuickControlSlot, kNumButtons> a;
        a.fill (QuickControlSlot::empty (QuickControlSlot::ControlType::Button));
        return a;
    }

    //==========================================================================
    // Available-params lists for right-click reassignment menus

    inline juce::Array<QuickControlSlot> synthAvailable()
    {
        juce::Array<QuickControlSlot> s;
        // Oscillator
        s.add (QuickControlSlot::slider ("DET1", "DET1", -50.f,  50.f,   0.f  ));
        s.add (QuickControlSlot::slider ("MIX2", "MIX2",  0.f,   1.f,   0.7f ));
        s.add (QuickControlSlot::slider ("DET2", "DET2", -50.f,  50.f,   7.f  ));
        // Filter
        s.add (QuickControlSlot::slider ("CUT",  "CUT",  20.f, 20000.f, 4000.f));
        s.add (QuickControlSlot::slider ("RES",  "RES",   0.f,   1.f,   0.3f ));
        s.add (QuickControlSlot::slider ("ENVA", "ENVA", -1.f,   1.f,   0.5f ));
        // Amp envelope
        s.add (QuickControlSlot::slider ("ATK",  "ATK",   0.f,   2.f,   0.01f));
        s.add (QuickControlSlot::slider ("DEC",  "DEC",   0.f,   2.f,   0.1f ));
        s.add (QuickControlSlot::slider ("SUS",  "SUS",   0.f,   1.f,   0.7f ));
        s.add (QuickControlSlot::slider ("REL",  "REL",   0.f,   3.f,   0.3f ));
        // Filter envelope
        s.add (QuickControlSlot::slider ("FATK", "FATK",  0.f,   2.f,   0.01f));
        s.add (QuickControlSlot::slider ("FDEC", "FDEC",  0.f,   2.f,   0.2f ));
        s.add (QuickControlSlot::slider ("FSUS", "FSUS",  0.f,   1.f,   0.5f ));
        s.add (QuickControlSlot::slider ("FREL", "FREL",  0.f,   3.f,   0.2f ));
        // LFO
        s.add (QuickControlSlot::slider ("LFRT", "LFRT",  0.1f, 20.f,  1.f  ));
        s.add (QuickControlSlot::slider ("LFDP", "LFDP",  0.f,  1.f,   0.f  ));
        return s;
    }

    inline juce::Array<QuickControlSlot> drumAvailable()
    {
        juce::Array<QuickControlSlot> s;
        const char* voices[6] = { "v0", "v1", "v2", "v3", "v4", "v5" };
        const char* vnames[6] = { "KCK", "SUB", "SNR", "HAT", "P1", "P2" };
        for (int v = 0; v < 6; ++v)
        {
            const juce::String pfx (voices[v]);
            const juce::String nm  (vnames[v]);
            s.add (QuickControlSlot::slider (pfx + ":tpitch",   nm + ".TONE",  20.f,  800.f, 100.f));
            s.add (QuickControlSlot::slider (pfx + ":tdecay",   nm + ".DEC",  0.01f,   2.f,  0.3f));
            s.add (QuickControlSlot::slider (pfx + ":body",     nm + ".BDY",  0.f,     1.f,  0.5f));
            s.add (QuickControlSlot::slider (pfx + ":clicklvl", nm + ".SNAP",  0.f,    1.f,  0.5f));
            s.add (QuickControlSlot::slider (pfx + ":noiselv",  nm + ".NSE",   0.f,    1.f,  0.5f));
            s.add (QuickControlSlot::slider (pfx + ":noisehp",  nm + ".CLR",  80.f, 8000.f, 500.f));
            s.add (QuickControlSlot::slider (pfx + ":drive",    nm + ".DRV",   0.f,    1.f,  0.2f));
            s.add (QuickControlSlot::slider (pfx + ":grit",     nm + ".GRT",   0.f,    1.f,  0.1f));
            s.add (QuickControlSlot::slider (pfx + ":snap",     nm + ".SNP",   0.f,    1.f,  0.2f));
            s.add (QuickControlSlot::slider (pfx + ":outl",     nm + ".LVL",   0.f,   2.f,  1.0f));
            s.add (QuickControlSlot::momentary (pfx + ":trig",  nm + ".TRIG"));
        }
        return s;
    }

} // namespace QuickFaceplateDefaults

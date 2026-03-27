#include "PresetManager.h"
#include "../module/ModuleProcessor.h"

namespace
{
DrumVoiceRole drumRoleFromString (const juce::String& text)
{
    const auto t = text.toLowerCase().trim();
    if (t == "kick")    return DrumVoiceRole::kick;
    if (t == "subkick") return DrumVoiceRole::subKick;
    if (t == "snare")   return DrumVoiceRole::snare;
    if (t == "hat")     return DrumVoiceRole::hat;
    if (t == "perc1")   return DrumVoiceRole::perc1;
    if (t == "perc2")   return DrumVoiceRole::perc2;
    if (t == "clap")    return DrumVoiceRole::clap;
    if (t == "tom")     return DrumVoiceRole::tom;
    if (t == "cymbal")  return DrumVoiceRole::cymbal;
    return DrumVoiceRole::custom;
}

DrumVoiceRole legacyRoleForVoice (const juce::String& name, int midiNote, int index)
{
    const auto upper = name.toUpperCase();
    if (upper.contains ("SUB"))                    return DrumVoiceRole::subKick;
    if (upper.contains ("SNARE"))                  return DrumVoiceRole::snare;
    if (upper == "HAT" || upper.contains ("CH")
        || upper.contains ("OH"))                  return DrumVoiceRole::hat;
    if (upper.contains ("CLAP"))                   return DrumVoiceRole::clap;
    if (upper.contains ("CYM"))                    return DrumVoiceRole::cymbal;
    if (upper.contains ("TOM"))                    return DrumVoiceRole::tom;
    if (upper.contains ("PERC 2"))                 return DrumVoiceRole::perc2;
    if (upper.contains ("PERC"))                   return DrumVoiceRole::perc1;
    if (upper.contains ("KICK"))                   return DrumVoiceRole::kick;
    if (midiNote == 35)                            return DrumVoiceRole::subKick;
    if (midiNote == 38)                            return DrumVoiceRole::snare;
    if (midiNote == 42 || midiNote == 46)          return DrumVoiceRole::hat;
    if (midiNote == 56)                            return DrumVoiceRole::perc2;
    if (index == 1)                                return DrumVoiceRole::subKick;
    if (index == 2)                                return DrumVoiceRole::snare;
    if (index == 3)                                return DrumVoiceRole::hat;
    if (index == 5)                                return DrumVoiceRole::perc2;
    return index >= 4 ? DrumVoiceRole::perc1 : DrumVoiceRole::kick;
}
}

//==============================================================================
// Singleton
//==============================================================================

PresetManager& PresetManager::getInstance()
{
    static PresetManager instance;
    return instance;
}

PresetManager::PresetManager()
{
    buildFactoryPresets();
    scanUserPresets();
}

//==============================================================================
// Internal helpers
//==============================================================================

juce::var PresetManager::makeObj()
{
    return juce::var (new juce::DynamicObject());
}

void PresetManager::setProp (juce::var& obj, const juce::String& key, juce::var val)
{
    if (auto* d = obj.getDynamicObject())
        d->setProperty (juce::Identifier (key), val);
}

float PresetManager::getF (const juce::var& obj, const juce::String& key, float def)
{
    if (auto* d = obj.getDynamicObject())
    {
        const juce::var v = d->getProperty (juce::Identifier (key));
        if (!v.isVoid()) return (float)(double)v;
    }
    return def;
}

int PresetManager::getI (const juce::var& obj, const juce::String& key, int def)
{
    if (auto* d = obj.getDynamicObject())
    {
        const juce::var v = d->getProperty (juce::Identifier (key));
        if (!v.isVoid()) return (int)v;
    }
    return def;
}

int64_t PresetManager::getInt64 (const juce::var& obj, const juce::String& key, int64_t def)
{
    if (auto* d = obj.getDynamicObject())
    {
        const juce::var v = d->getProperty (juce::Identifier (key));
        if (!v.isVoid()) return static_cast<int64_t> (v);
    }
    return def;
}

//==============================================================================
// Synth preset builder
//==============================================================================

juce::var PresetManager::buildSynthPreset (const juce::String& name,
    int o1sh, int o2sh, float o1det, float o2det, int o2oct, float o2lv,
    float fcut, float fres, float fenv,
    float ampA, float ampD, float ampS, float ampR,
    float filtA, float filtD, float filtS, float filtR,
    float lfoR, float lfoD, int lfoT, int mono,
    float charA, float charD, float charDr)
{
    auto obj = makeObj();
    setProp (obj, "name",    name);
    setProp (obj, "type",    "synth");
    setProp (obj, "version", 2);

    auto p = makeObj();
    setProp (p, "osc1.on",       1);
    setProp (p, "osc2.on",       1);
    setProp (p, "osc1.shape",    o1sh);
    setProp (p, "osc2.shape",    o2sh);
    setProp (p, "osc1.detune",   o1det);
    setProp (p, "osc2.detune",   o2det);
    setProp (p, "osc1.octave",   0);
    setProp (p, "osc2.octave",   o2oct);
    setProp (p, "osc1.pulse_width", 0.5f);
    setProp (p, "osc2.pulse_width", 0.5f);
    setProp (p, "osc2.level",    o2lv);
    setProp (p, "filter.cutoff", fcut);
    setProp (p, "filter.res",    fres);
    setProp (p, "filter.env_amt",fenv);
    setProp (p, "amp.attack",    ampA);
    setProp (p, "amp.decay",     ampD);
    setProp (p, "amp.sustain",   ampS);
    setProp (p, "amp.release",   ampR);
    setProp (p, "filt.attack",   filtA);
    setProp (p, "filt.decay",    filtD);
    setProp (p, "filt.sustain",  filtS);
    setProp (p, "filt.release",  filtR);
    setProp (p, "lfo.on",        1);
    setProp (p, "lfo.rate",      lfoR);
    setProp (p, "lfo.depth",     lfoD);
    setProp (p, "lfo.target",    lfoT);
    setProp (p, "voice.mono",    mono);
    setProp (p, "char.asym",     charA);
    setProp (p, "char.drive",    charD);
    setProp (p, "char.drift",    charDr);
    setProp (p, "out.level",     1.0f);
    setProp (p, "out.pan",       0.0f);
    setProp (obj, "params", p);
    return obj;
}

//==============================================================================
// Drum preset builders
//==============================================================================

juce::var PresetManager::voiceToVar (const DrumVoiceState& voice)
{
    auto v = makeObj();
    setProp (v, "name",        juce::String (voice.name));
    setProp (v, "midiNote",    voice.midiNote);
    setProp (v, "midiChannel", voice.midiChannel);
    setProp (v, "velocity",    voice.velocity);
    setProp (v, "muted",       voice.muted ? 1 : 0);
    setProp (v, "mode",        voice.mode == DrumVoiceMode::sample ? "sample" : "midi");
    setProp (v, "samplePath",  juce::String (voice.samplePath));
    setProp (v, "color",       static_cast<int64_t> (voice.colorArgb));
    setProp (v, "stepBits",    static_cast<int64_t> (voice.stepBits));
    setProp (v, "stepCount",   voice.stepCount);
    setProp (v, "role",        juce::String (toString (voice.params.role)));

    const auto& p = voice.params;
    auto t = makeObj();
    setProp (t, "enable",      p.tone.enable      ? 1 : 0);
    setProp (t, "pitch",       p.tone.pitch);
    setProp (t, "pitchstart",  p.tone.pitchstart);
    setProp (t, "pitchdecay",  p.tone.pitchdecay);
    setProp (t, "attack",      p.tone.attack);
    setProp (t, "decay",       p.tone.decay);
    setProp (t, "level",       p.tone.level);
    setProp (t, "body",        p.tone.body);
    setProp (t, "triangleWave",p.tone.triangleWave ? 1 : 0);
    setProp (v, "tone", t);

    auto n = makeObj();
    setProp (n, "enable", p.noise.enable ? 1 : 0);
    setProp (n, "decay",  p.noise.decay);
    setProp (n, "level",  p.noise.level);
    setProp (n, "hpfreq", p.noise.hpfreq);
    setProp (n, "lpfreq", p.noise.lpfreq);
    setProp (n, "color",  p.noise.color);
    setProp (v, "noise", n);

    auto c = makeObj();
    setProp (c, "enable",       p.click.enable ? 1 : 0);
    setProp (c, "level",        p.click.level);
    setProp (c, "decay",        p.click.decay);
    setProp (c, "tone",         p.click.tone);
    setProp (c, "pitch",        p.click.pitch);
    setProp (c, "bursts",       p.click.bursts);
    setProp (c, "burstspacing", p.click.burstspacing);
    setProp (v, "click", c);

    auto m = makeObj();
    setProp (m, "enable",   p.metal.enable ? 1 : 0);
    setProp (m, "voices",   p.metal.voices);
    setProp (m, "basefreq", p.metal.basefreq);
    for (int i = 0; i < static_cast<int> (p.metal.ratios.size()); ++i)
        setProp (m, "ratio" + juce::String (i), p.metal.ratios[static_cast<size_t> (i)]);
    setProp (m, "decay",    p.metal.decay);
    setProp (m, "level",    p.metal.level);
    setProp (m, "hpfreq",   p.metal.hpfreq);
    setProp (v, "metal", m);

    auto ch = makeObj();
    setProp (ch, "drive", p.character.drive);
    setProp (ch, "grit",  p.character.grit);
    setProp (ch, "snap",  p.character.snap);
    setProp (v, "character", ch);

    auto o = makeObj();
    setProp (o, "level",    p.out.level);
    setProp (o, "pan",      p.out.pan);
    setProp (o, "tune",     p.out.tune);
    setProp (o, "velocity", p.out.velocity);
    setProp (v, "out", o);

    return v;
}

juce::var PresetManager::stateToVar (const DrumModuleState& state, const juce::String& name)
{
    auto obj = makeObj();
    setProp (obj, "name",    name);
    setProp (obj, "type",    "drum");
    setProp (obj, "version", 2);
    setProp (obj, "focusedVoiceIndex", state.focusedVoiceIndex);

    juce::Array<juce::var> voices;
    for (const auto& voice : state.voices)
        voices.add (voiceToVar (voice));
    obj.getDynamicObject()->setProperty ("voices", voices);
    return obj;
}

DrumVoiceState PresetManager::varToVoice (const juce::var& v, int fallbackIndex)
{
    DrumVoiceState voice;
    voice.name        = v["name"].toString().isNotEmpty()
        ? v["name"].toString().toStdString()
        : ("VOICE " + juce::String (fallbackIndex + 1)).toStdString();
    voice.midiNote    = getI (v, "midiNote", 36);
    voice.midiChannel = getI (v, "midiChannel", 1);
    voice.velocity    = getF (v, "velocity", 1.0f);
    voice.muted       = getI (v, "muted", 0) != 0;
    voice.samplePath  = v["samplePath"].toString().toStdString();
    voice.colorArgb   = static_cast<std::uint32_t> (static_cast<std::uint64_t> (getInt64 (v, "color", 0xFF4B9EDB)));
    voice.stepBits    = static_cast<std::uint64_t> (getInt64 (v, "stepBits", 0));
    voice.stepCount   = getI (v, "stepCount", 16);
    const auto modeText = v["mode"].toString().toLowerCase();
    voice.mode = (modeText == "sample") ? DrumVoiceMode::sample : DrumVoiceMode::midi;
    voice.params.role = drumRoleFromString (v["role"].toString());

    const auto& t = v["tone"];
    voice.params.tone.enable       = getI (t, "enable", 1) != 0;
    voice.params.tone.pitch        = getF (t, "pitch",        55.f);
    voice.params.tone.pitchstart   = getF (t, "pitchstart",   3.5f);
    voice.params.tone.pitchdecay   = getF (t, "pitchdecay",  0.06f);
    voice.params.tone.attack       = getF (t, "attack",       0.0f);
    voice.params.tone.decay        = getF (t, "decay",        0.6f);
    voice.params.tone.level        = getF (t, "level",        1.0f);
    voice.params.tone.body         = getF (t, "body",         0.5f);
    voice.params.tone.triangleWave = getI (t, "triangleWave", 0) != 0;

    const auto& n = v["noise"];
    voice.params.noise.enable  = getI (n, "enable", 1) != 0;
    voice.params.noise.decay   = getF (n, "decay",  0.02f);
    voice.params.noise.level   = getF (n, "level",  0.15f);
    voice.params.noise.hpfreq  = getF (n, "hpfreq", 80.f);
    voice.params.noise.lpfreq  = getF (n, "lpfreq", 800.f);
    voice.params.noise.color   = getF (n, "color",  0.f);

    const auto& c = v["click"];
    voice.params.click.enable       = getI (c, "enable", 1) != 0;
    voice.params.click.level        = getF (c, "level",       0.6f);
    voice.params.click.decay        = getF (c, "decay",      0.003f);
    voice.params.click.tone         = getF (c, "tone",        0.7f);
    voice.params.click.pitch        = getF (c, "pitch",    2000.0f);
    voice.params.click.bursts       = getI (c, "bursts",      1);
    voice.params.click.burstspacing = getF (c, "burstspacing",0.010f);

    const auto& m = v["metal"];
    voice.params.metal.enable   = getI (m, "enable",  0) != 0;
    voice.params.metal.voices   = getI (m, "voices",  6);
    voice.params.metal.basefreq = getF (m, "basefreq",400.f);
    for (int i = 0; i < static_cast<int> (voice.params.metal.ratios.size()); ++i)
        voice.params.metal.ratios[static_cast<size_t> (i)] =
            getF (m, "ratio" + juce::String (i), voice.params.metal.ratios[static_cast<size_t> (i)]);
    voice.params.metal.decay    = getF (m, "decay",  0.04f);
    voice.params.metal.level    = getF (m, "level",   1.0f);
    voice.params.metal.hpfreq   = getF (m, "hpfreq",2000.f);

    const auto& ch = v["character"];
    voice.params.character.drive = getF (ch, "drive", 0.2f);
    voice.params.character.grit  = getF (ch, "grit",  0.1f);
    voice.params.character.snap  = getF (ch, "snap",  0.2f);

    const auto& o = v["out"];
    voice.params.out.level    = getF (o, "level",    1.0f);
    voice.params.out.pan      = getF (o, "pan",      0.0f);
    voice.params.out.tune     = getF (o, "tune",     0.0f);
    voice.params.out.velocity = getF (o, "velocity", 0.8f);

    if (voice.params.role == DrumVoiceRole::custom)
        voice.params.role = legacyRoleForVoice (v["name"].toString(), voice.midiNote, fallbackIndex);

    const auto defaults = DrumVoiceParams::makeForRole (voice.params.role);
    if (! t.isObject())
        voice.params.tone = defaults.tone;
    if (! c.isObject())
        voice.params.click = defaults.click;
    if (! ch.isObject())
        voice.params.character = defaults.character;

    voice.params.midiNote = voice.midiNote;
    voice.setStepCount (voice.stepCount);
    return voice;
}

DrumModuleState PresetManager::varToDrumState (const juce::var& v)
{
    DrumModuleState state;
    state.name = v["name"].toString().toStdString();
    state.focusedVoiceIndex = getI (v, "focusedVoiceIndex", 0);
    const auto& voices = v["voices"];
    for (int i = 0; i < voices.size(); ++i)
        state.voices.push_back (varToVoice (voices[i], i));

    if (state.voices.empty())
        state = DrumModuleState::fromKitPatch (DrumKitPatch::makeSpoolKit());

    state.clamp();
    return state;
}

//==============================================================================
// Factory preset bank
//==============================================================================

void PresetManager::addFactorySynth (const juce::String& name, juce::var data)
{
    PresetEntry e;
    e.name        = name;
    e.isFactory   = true;
    e.factoryData = data;
    m_synthPresets.add (e);
}

void PresetManager::addFactoryDrum (const juce::String& name, juce::var data)
{
    PresetEntry e;
    e.name        = name;
    e.isFactory   = true;
    e.factoryData = data;
    m_drumPresets.add (e);
}

void PresetManager::buildFactoryPresets()
{
    //----------------------------------------------------------------------
    // Synth: 8 Poly + 8 Mono
    // buildSynthPreset args:
    //  name, o1sh, o2sh, o1det, o2det, o2oct, o2lv,
    //  fcut, fres, fenv,
    //  ampA, ampD, ampS, ampR,
    //  filtA, filtD, filtS, filtR,
    //  lfoR, lfoD, lfoT, mono,
    //  charA(asym), charD(drive), charDr(drift)
    //----------------------------------------------------------------------
    //                           name                  o1  o2  o1d   o2d  oct  lv   fcut  fres fenv  aA    aD   aS   aR    fA    fD   fS   fR    lr  ld   lt  m    cA    cD   cDr
    addFactorySynth ("Init Poly",           buildSynthPreset("Init Poly",           0,  0,  0.f,  7.f, 0, 0.7f, 4000,0.3f,0.5f, 0.01f,0.3f,0.7f,0.4f, 0.01f,0.3f,0.0f,0.3f, 5.f,0.f,0, 0,  0.15f,0.20f,0.15f));
    addFactorySynth ("Warm Pad",            buildSynthPreset("Warm Pad",            0,  0,  0.f, 12.f, 0, 0.8f,  800,0.2f,0.3f, 0.8f, 0.2f,0.9f,1.5f, 0.6f, 0.4f,0.3f,1.0f, 0.8f,0.1f,0, 0,  0.10f,0.15f,0.20f));
    addFactorySynth ("Bright Pad",          buildSynthPreset("Bright Pad",          0,  3,  0.f,  5.f, 1, 0.6f, 2500,0.15f,0.5f,0.4f, 0.3f,0.85f,1.2f,0.3f, 0.5f,0.2f,0.8f, 1.2f,0.08f,1,0,  0.25f,0.25f,0.10f));
    addFactorySynth ("Wide Brass",          buildSynthPreset("Wide Brass",          0,  0,  0.f, 15.f, 0, 0.9f, 5000,0.4f,0.7f, 0.05f,0.1f,0.8f,0.2f, 0.03f,0.2f,0.1f,0.2f, 5.f,0.05f,0,0,  0.30f,0.35f,0.05f));
    addFactorySynth ("Glass Keys",          buildSynthPreset("Glass Keys",          2,  3,  0.f,  3.f, 1, 0.5f, 8000,0.1f,0.2f, 0.01f,0.4f,0.3f,0.6f, 0.01f,0.2f,0.0f,0.2f, 3.f,0.f,0, 0,  0.05f,0.10f,0.05f));
    addFactorySynth ("Soft Pluck",          buildSynthPreset("Soft Pluck",          3,  2,  0.f,  7.f, 0, 0.4f, 3000,0.3f,0.6f, 0.005f,0.5f,0.0f,0.3f,0.005f,0.3f,0.0f,0.2f,0.f,0.f,0, 0,  0.10f,0.05f,0.30f));
    addFactorySynth ("Detuned Saw Stack",   buildSynthPreset("Detuned Saw Stack",   0,  0,-15.f,15.f, 0, 1.0f, 2000,0.25f,0.2f,0.02f,0.1f,0.9f,0.5f, 0.02f,0.2f,0.0f,0.2f, 5.f,0.06f,1,0,  0.30f,0.30f,0.10f));
    addFactorySynth ("Slow Motion Sweep",   buildSynthPreset("Slow Motion Sweep",   0,  1,  0.f,  5.f,-1, 0.7f,  400,0.5f,0.8f, 2.0f, 0.5f,0.8f,3.0f, 1.5f, 0.8f,0.3f,2.0f, 0.3f,0.3f,1,0,  0.00f,0.00f,0.40f));
    addFactorySynth ("Init Mono",           buildSynthPreset("Init Mono",           0,  0,  0.f,  7.f, 0, 0.7f, 4000,0.3f,0.5f, 0.01f,0.3f,0.7f,0.4f, 0.01f,0.3f,0.0f,0.3f, 5.f,0.f,0, 1,  0.15f,0.20f,0.15f));
    addFactorySynth ("Round Bass",          buildSynthPreset("Round Bass",          2,  0,  0.f,  3.f,-1, 0.5f,  600,0.2f,0.3f, 0.01f,0.2f,0.85f,0.25f,0.01f,0.15f,0.0f,0.1f,0.f,0.f,0,1,  0.15f,0.30f,0.10f));
    addFactorySynth ("Acid Line",           buildSynthPreset("Acid Line",           1,  1,  0.f,  0.f, 0, 0.7f, 1500,0.75f,0.85f,0.005f,0.15f,0.7f,0.1f,0.005f,0.12f,0.0f,0.05f,0.f,0.f,0,1, 0.10f,0.40f,0.05f));
    addFactorySynth ("Rubber Bass",         buildSynthPreset("Rubber Bass",         3,  2,  0.f,  7.f,-1, 0.6f, 1200,0.35f,0.6f, 0.01f,0.3f,0.4f,0.4f, 0.005f,0.2f,0.0f,0.15f,0.f,0.f,0,1, 0.25f,0.20f,0.20f));
    addFactorySynth ("Punch Lead",          buildSynthPreset("Punch Lead",          1,  0,  0.f,  0.f, 1, 0.5f, 6000,0.4f,0.3f, 0.005f,0.2f,0.75f,0.15f,0.005f,0.15f,0.0f,0.1f,5.f,0.04f,0,1, 0.20f,0.30f,0.05f));
    addFactorySynth ("Whistle Lead",        buildSynthPreset("Whistle Lead",        2,  2,  0.f, 12.f, 1, 0.3f,12000,0.1f,0.1f, 0.05f,0.1f,0.9f,0.3f, 0.05f,0.1f,0.0f,0.1f, 5.5f,0.1f,0,1,  0.00f,0.00f,0.30f));
    addFactorySynth ("Muted Pluck Mono",    buildSynthPreset("Muted Pluck Mono",    3,  4,  0.f,  0.f, 0, 0.3f, 2000,0.4f,0.7f, 0.005f,0.25f,0.0f,0.15f,0.005f,0.18f,0.0f,0.1f,0.f,0.f,0,1, 0.10f,0.05f,0.25f));
    addFactorySynth ("Sub Driver",          buildSynthPreset("Sub Driver",          2,  2,  0.f,  0.f,-2, 0.6f,  200,0.1f,0.1f, 0.02f,0.5f,0.9f,0.8f, 0.02f,0.3f,0.0f,0.3f, 0.f,0.f,0, 1,  0.00f,0.50f,0.00f));

    //----------------------------------------------------------------------
    // Drums: 8 kits (first 4 use existing DrumKitPatch factory methods)
    //----------------------------------------------------------------------
    addFactoryDrum ("Spool Core",     stateToVar (DrumModuleState::fromKitPatch (DrumKitPatch::makeSpoolKit()),    "Spool Core"));
    addFactoryDrum ("Tight Electro",  stateToVar (DrumModuleState::fromKitPatch (DrumKitPatch::make909Kit()),      "Tight Electro"));
    addFactoryDrum ("Dusty Box",      stateToVar (DrumModuleState::fromKitPatch (DrumKitPatch::makeLoFiKit()),     "Dusty Box"));
    addFactoryDrum ("Punch Kit",      stateToVar (DrumModuleState::fromKitPatch (DrumKitPatch::makeAcousticKit()), "Punch Kit"));

    // Soft Boom Bap — 808 base with slower, softer character
    {
        DrumKitPatch kit = DrumKitPatch::makeSpoolKit();
        kit.name = "Soft Boom Bap";
        // Softer kick
        kit.voices[0].params.tone.decay   = 0.9f;
        kit.voices[0].params.tone.pitch   = 50.f;
        kit.voices[0].params.noise.level  = 0.08f;
        kit.voices[0].params.click.level  = 0.2f;
        // Softer snare
        kit.voices[2].params.noise.level  = 0.6f;
        kit.voices[2].params.noise.hpfreq = 200.f;
        kit.voices[2].params.tone.decay   = 0.18f;
        // Shorter hats
        kit.voices[3].params.metal.decay  = 0.02f;
        addFactoryDrum ("Soft Boom Bap", stateToVar (DrumModuleState::fromKitPatch (kit), "Soft Boom Bap"));
    }

    // Bright Machine — 909 base with higher frequencies
    {
        DrumKitPatch kit = DrumKitPatch::make909Kit();
        kit.name = "Bright Machine";
        kit.voices[0].params.tone.pitch      = 85.f;
        kit.voices[0].params.click.level     = 1.0f;
        kit.voices[2].params.noise.hpfreq    = 600.f;
        kit.voices[2].params.tone.pitch      = 220.f;
        kit.voices[4].params.click.level     = 0.74f;
        kit.voices[3].params.metal.basefreq  = 500.f;
        kit.voices[3].params.metal.hpfreq    = 4000.f;
        addFactoryDrum ("Bright Machine", stateToVar (DrumModuleState::fromKitPatch (kit), "Bright Machine"));
    }

    // Dirty Perc Kit — lo-fi base, heavily textured
    {
        DrumKitPatch kit = DrumKitPatch::makeLoFiKit();
        kit.name = "Dirty Perc Kit";
        kit.voices[0].params.noise.color  = 1.0f;  // max pink
        kit.voices[0].params.noise.level  = 0.6f;
        kit.voices[2].params.noise.color  = 0.9f;
        kit.voices[2].params.noise.lpfreq = 4000.f;
        // Gritty hats
        kit.voices[3].params.noise.level  = 0.8f;
        kit.voices[3].params.noise.color  = 0.7f;
        kit.voices[4].params.character.grit = 0.45f;
        kit.voices[5].params.character.grit = 0.50f;
        addFactoryDrum ("Dirty Perc Kit", stateToVar (DrumModuleState::fromKitPatch (kit), "Dirty Perc Kit"));
    }

    // Minimal Click Kit — transient-only, click layers prominent
    {
        DrumKitPatch kit = DrumKitPatch::makeSpoolKit();
        kit.name = "Minimal Click Kit";
        for (auto& voice : kit.voices)
        {
            voice.params.tone.enable    = false;
            voice.params.noise.level   *= 0.15f;
            voice.params.click.level    = 1.0f;
            voice.params.click.decay    = 0.003f;
            voice.params.out.level     *= 0.9f;
        }
        // Give kick a tiny bit of body back
        kit.voices[0].params.tone.enable = true;
        kit.voices[0].params.tone.decay  = 0.08f;
        kit.voices[0].params.tone.level  = 0.5f;
        kit.voices[1].params.click.enable = false;
        addFactoryDrum ("Minimal Click Kit", stateToVar (DrumModuleState::fromKitPatch (kit), "Minimal Click Kit"));
    }
}

//==============================================================================
// User preset scanning
//==============================================================================

juce::File PresetManager::getUserSynthPresetsDir() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Spool/Presets/Synth");
}

juce::File PresetManager::getUserDrumPresetsDir() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Spool/Presets/Drum");
}

void PresetManager::scanUserPresets()
{
    auto scanDir = [this] (juce::Array<PresetEntry>& list,
                            const juce::File& dir)
    {
        if (!dir.isDirectory()) return;
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.json"))
        {
            // Skip if factory preset with same name already in list (shouldn't happen)
            PresetEntry e;
            e.name      = f.getFileNameWithoutExtension();
            e.isFactory = false;
            e.file      = f;
            list.add (e);
        }
    };

    scanDir (m_synthPresets, getUserSynthPresetsDir());
    scanDir (m_drumPresets,  getUserDrumPresetsDir());
}

void PresetManager::rescanUserPresets()
{
    // Remove all non-factory entries and rescan
    for (int i = m_synthPresets.size() - 1; i >= 0; --i)
        if (!m_synthPresets[i].isFactory)
            m_synthPresets.remove (i);

    for (int i = m_drumPresets.size() - 1; i >= 0; --i)
        if (!m_drumPresets[i].isFactory)
            m_drumPresets.remove (i);

    scanUserPresets();
}

//==============================================================================
// Load preset data
//==============================================================================

juce::var PresetManager::loadPresetData (const PresetEntry& entry)
{
    if (entry.isFactory)
        return entry.factoryData;

    if (entry.file.existsAsFile())
    {
        juce::String text = entry.file.loadFileAsString();
        if (text.isNotEmpty())
            return juce::JSON::parse (text);
    }

    return {};
}

//==============================================================================
// Apply preset to processor
//==============================================================================

void PresetManager::applySynthPreset (const juce::var& preset, ModuleProcessor& proc)
{
    const auto& p = preset["params"];
    if (!p.isObject()) return;

    static const juce::StringRef kIds[] = {
        "osc1.on",     "osc2.on",       "osc1.shape",   "osc2.shape",
        "osc1.detune", "osc2.detune",   "osc1.octave",  "osc2.octave",
        "osc1.pulse_width", "osc2.pulse_width", "osc2.level",    "filter.cutoff","filter.res",
        "filter.env_amt",
        "amp.attack",  "amp.decay",     "amp.sustain",  "amp.release",
        "filt.attack", "filt.decay",    "filt.sustain", "filt.release",
        "lfo.on",      "lfo.rate",      "lfo.depth",    "lfo.target",   "voice.mono",
        "char.asym",   "char.drive",    "char.drift",   "out.level",    "out.pan"
    };

    for (const auto& id : kIds)
        proc.setParam (id, getF (p, id, proc.getParam (id)));
}

void PresetManager::applyDrumPreset (const juce::var& preset, DrumModuleState& state)
{
    state = varToDrumState (preset);
    state.clamp();
}

//==============================================================================
// Capture processor state as JSON
//==============================================================================

juce::var PresetManager::captureSynthPreset (ModuleProcessor& proc,
                                              const juce::String& name)
{
    auto obj = makeObj();
    setProp (obj, "name",    name);
    setProp (obj, "type",    "synth");
    setProp (obj, "version", 2);

    auto p = makeObj();
    static const juce::StringRef kIds[] = {
        "osc1.on",     "osc2.on",       "osc1.shape",   "osc2.shape",
        "osc1.detune", "osc2.detune",   "osc1.octave",  "osc2.octave",
        "osc1.pulse_width", "osc2.pulse_width", "osc2.level",    "filter.cutoff","filter.res",
        "filter.env_amt",
        "amp.attack",  "amp.decay",     "amp.sustain",  "amp.release",
        "filt.attack", "filt.decay",    "filt.sustain", "filt.release",
        "lfo.on",      "lfo.rate",      "lfo.depth",    "lfo.target",   "voice.mono",
        "char.asym",   "char.drive",    "char.drift",   "out.level",    "out.pan"
    };
    for (const auto& id : kIds)
        setProp (p, id, proc.getParam (id));

    setProp (obj, "params", p);
    return obj;
}

juce::var PresetManager::captureDrumPreset (const DrumModuleState& state,
                                             const juce::String& name)
{
    auto namedState = state;
    namedState.name = name.toStdString();
    namedState.clamp();
    return stateToVar (namedState, name);
}

//==============================================================================
// Save user presets
//==============================================================================

bool PresetManager::savePreset (const juce::File& dir,
                                 const juce::String& name,
                                 const juce::var& data)
{
    if (!dir.isDirectory())
        dir.createDirectory();

    const juce::File f = dir.getChildFile (name + ".json");
    const juce::String text = juce::JSON::toString (data, false);
    return f.replaceWithText (text);
}

bool PresetManager::saveUserSynthPreset (const juce::String& name, const juce::var& data)
{
    const bool ok = savePreset (getUserSynthPresetsDir(), name, data);
    if (ok) rescanUserPresets();
    return ok;
}

bool PresetManager::saveUserDrumPreset (const juce::String& name, const juce::var& data)
{
    const bool ok = savePreset (getUserDrumPresetsDir(), name, data);
    if (ok) rescanUserPresets();
    return ok;
}

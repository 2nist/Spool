#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    ChainState — per-slot FX chain data model.

    All 8 chains live simultaneously in ZoneCComponent::m_chains[8].
    Switching slots never discards chain edits.
    No audio processing — params are stored as floats only.
*/

//==============================================================================
struct EffectNode
{
    juce::String       effectType;       // "eq", "compressor", "reverb" etc.
    bool               bypassed = false;
    juce::Array<float> params;           // indexed per effect type (see getEffectDef)
};

//==============================================================================
struct ChainState
{
    int                     slotIndex   = -1;
    juce::Array<EffectNode> nodes;
    bool                    initialised = false;
};

//==============================================================================
/**
    EffectParamDef — describes one parameter slot in an effect type.
*/
struct EffectParamDef
{
    enum class Type { Continuous, Bool, Enum };

    juce::String          name;
    float                 minVal    = 0.0f;
    float                 maxVal    = 1.0f;
    float                 defaultVal = 0.0f;
    Type                  type      = Type::Continuous;
    juce::StringArray     enumValues;   // used when type == Enum
};

//==============================================================================
/**
    EffectTypeDef — everything needed to render and store one effect type.
*/
struct EffectTypeDef
{
    juce::String              id;         // "eq", "compressor" etc.
    juce::String              displayName; // "EQ", "COMPRESSOR" etc.
    juce::String              tag;         // "3-BAND", "VINTAGE" etc.
    juce::Colour              colour;
    juce::Array<EffectParamDef> params;
};

//==============================================================================
/**
    EffectRegistry — static lookup for all 7 built-in effect types.
*/
namespace EffectRegistry
{
    inline juce::Array<EffectTypeDef> all()
    {
        using T = EffectParamDef::Type;

        juce::Array<EffectTypeDef> defs;

        // EQ
        {
            EffectTypeDef d;
            d.id          = "eq";
            d.displayName = "EQ";
            d.tag         = "3-BAND";
            d.colour      = Theme::Signal::audio;
            d.params.add ({ "LO GAIN",  -12.0f, 12.0f,    0.0f,  T::Continuous, {} });
            d.params.add ({ "MID GAIN", -12.0f, 12.0f,    0.0f,  T::Continuous, {} });
            d.params.add ({ "HI GAIN",  -12.0f, 12.0f,    0.0f,  T::Continuous, {} });
            d.params.add ({ "LO FREQ",   20.0f,  500.0f,  80.0f, T::Continuous, {} });
            d.params.add ({ "HI FREQ", 2000.0f,20000.0f,8000.0f, T::Continuous, {} });
            defs.add (d);
        }

        // COMPRESSOR
        {
            EffectTypeDef d;
            d.id          = "compressor";
            d.displayName = "COMPRESSOR";
            d.tag         = "VINTAGE";
            d.colour      = Theme::Colour::accent;
            d.params.add ({ "THRESH", -60.0f,  0.0f, -12.0f, T::Continuous, {} });
            d.params.add ({ "RATIO",    1.0f, 20.0f,   4.0f, T::Continuous, {} });
            d.params.add ({ "ATTACK",   0.1f,100.0f,  10.0f, T::Continuous, {} });
            d.params.add ({ "RELEASE", 10.0f,1000.0f,100.0f, T::Continuous, {} });
            d.params.add ({ "GAIN",     0.0f, 24.0f,   0.0f, T::Continuous, {} });
            defs.add (d);
        }

        // REVERB
        {
            EffectTypeDef d;
            d.id          = "reverb";
            d.displayName = "REVERB";
            d.tag         = "ROOM";
            d.colour      = Theme::Zone::a;
            d.params.add ({ "SIZE",   0.0f, 1.0f, 0.5f, T::Continuous, {} });
            d.params.add ({ "DAMP",   0.0f, 1.0f, 0.5f, T::Continuous, {} });
            d.params.add ({ "MIX",    0.0f, 1.0f, 0.3f, T::Continuous, {} });
            d.params.add ({ "WIDTH",  0.0f, 1.0f, 0.8f, T::Continuous, {} });
            d.params.add ({ "FREEZE", 0.0f, 1.0f, 0.0f, T::Bool,       {} });
            defs.add (d);
        }

        // DELAY
        {
            EffectTypeDef d;
            d.id          = "delay";
            d.displayName = "DELAY";
            d.tag         = "STEREO";
            d.colour      = Theme::Colour::accentWarm;
            d.params.add ({ "TIME",   0.01f, 2.0f,  0.25f, T::Continuous, {} });
            d.params.add ({ "FEED",   0.0f,  0.99f, 0.4f,  T::Continuous, {} });
            d.params.add ({ "SPREAD", 0.0f,  1.0f,  0.5f,  T::Continuous, {} });
            d.params.add ({ "MIX",    0.0f,  1.0f,  0.3f,  T::Continuous, {} });
            d.params.add ({ "SYNC",   0.0f,  1.0f,  1.0f,  T::Bool,       {} });
            defs.add (d);
        }

        // DISTORTION
        {
            EffectTypeDef d;
            d.id          = "distortion";
            d.displayName = "DISTORTION";
            d.tag         = "SATURATION";
            d.colour      = Theme::Colour::error;
            d.params.add ({ "DRIVE", 0.0f, 1.0f, 0.3f, T::Continuous, {} });
            d.params.add ({ "TONE",  0.0f, 1.0f, 0.5f, T::Continuous, {} });
            d.params.add ({ "MIX",   0.0f, 1.0f, 0.5f, T::Continuous, {} });
            d.params.add ({ "MODE",  0.0f, 3.0f, 0.0f, T::Enum,
                            { "SOFT", "HARD", "TAPE", "FOLD" } });
            defs.add (d);
        }

        // CHORUS
        {
            EffectTypeDef d;
            d.id          = "chorus";
            d.displayName = "CHORUS";
            d.tag         = "FLANGER";
            d.colour      = Theme::Semantic::borrowed;
            d.params.add ({ "RATE",  0.01f, 10.0f, 0.5f, T::Continuous, {} });
            d.params.add ({ "DEPTH", 0.0f,  1.0f,  0.3f, T::Continuous, {} });
            d.params.add ({ "FEED",  0.0f,  0.99f, 0.2f, T::Continuous, {} });
            d.params.add ({ "MIX",   0.0f,  1.0f,  0.5f, T::Continuous, {} });
            d.params.add ({ "MODE",  0.0f,  1.0f,  0.0f, T::Enum,
                            { "CHORUS", "FLANGER" } });
            defs.add (d);
        }

        // FILTER
        {
            EffectTypeDef d;
            d.id          = "filter";
            d.displayName = "FILTER";
            d.tag         = "LP24";
            d.colour      = Theme::Semantic::strong;
            d.params.add ({ "CUTOFF", 20.0f,20000.0f,2000.0f, T::Continuous, {} });
            d.params.add ({ "RESO",   0.0f,  1.0f,   0.3f,    T::Continuous, {} });
            d.params.add ({ "ENV",    0.0f,  1.0f,   0.0f,    T::Continuous, {} });
            d.params.add ({ "DRIVE",  0.0f,  1.0f,   0.0f,    T::Continuous, {} });
            d.params.add ({ "MODE",   0.0f,  3.0f,   0.0f,    T::Enum,
                            { "LP", "HP", "BP", "NOTCH" } });
            defs.add (d);
        }

        return defs;
    }

    inline const EffectTypeDef* find (const juce::String& id)
    {
        static const auto defs = all();
        for (auto& d : defs)
            if (d.id == id) return &d;
        return nullptr;
    }

    /** Build a default EffectNode with default param values for the given type. */
    inline EffectNode makeDefault (const juce::String& id)
    {
        EffectNode n;
        n.effectType = id;
        if (auto* def = find (id))
        {
            for (auto& p : def->params)
                n.params.add (p.defaultVal);
        }
        return n;
    }
}

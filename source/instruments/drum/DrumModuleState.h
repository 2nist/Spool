#pragma once

#include "DrumKitPatch.h"
#include <juce_core/juce_core.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

enum class DrumVoiceMode
{
    sample,
    midi
};

struct DrumVoiceState
{
    static constexpr int kMaxSteps = 64;
    static constexpr int kDefaultRatchetCount = 1;
    static constexpr int kDefaultRepeatDivision = 4;

    std::string   name { "VOICE" };
    DrumVoiceParams params {};
    DrumVoiceMode mode { DrumVoiceMode::midi };
    std::string   samplePath;

    int           midiNote    { 36 };
    int           midiChannel { 1 };
    float         velocity    { 1.0f };
    bool          muted       { false };
    std::uint32_t colorArgb   { 0xFF4B9EDB };

    std::uint64_t stepBits    { 0 };
    int           stepCount   { 16 };
    std::array<float, kMaxSteps> stepVelocities {};
    std::array<uint8_t, kMaxSteps> stepRatchets {};
    std::array<uint8_t, kMaxSteps> stepRepeatDivisions {};

    DrumVoiceState()
    {
        stepVelocities.fill (1.0f);
        stepRatchets.fill (kDefaultRatchetCount);
        stepRepeatDivisions.fill (kDefaultRepeatDivision);
    }

    bool stepActive (int step) const noexcept
    {
        if (step < 0 || step >= std::clamp (stepCount, 1, kMaxSteps))
            return false;
        return (stepBits & (1ULL << step)) != 0;
    }

    void setStepActive (int step, bool active) noexcept
    {
        if (step < 0 || step >= kMaxSteps)
            return;

        const auto bit = (1ULL << step);
        if (active)
            stepBits |= bit;
        else
            stepBits &= ~bit;
    }

    float stepVelocity (int step) const noexcept
    {
        if (step < 0 || step >= kMaxSteps)
            return 1.0f;
        return std::clamp (stepVelocities[(size_t) step], 0.05f, 1.0f);
    }

    int stepRatchetCount (int step) const noexcept
    {
        if (step < 0 || step >= kMaxSteps)
            return kDefaultRatchetCount;
        return std::clamp ((int) stepRatchets[(size_t) step], 1, 8);
    }

    int stepRepeatDivision (int step) const noexcept
    {
        if (step < 0 || step >= kMaxSteps)
            return kDefaultRepeatDivision;
        return std::clamp ((int) stepRepeatDivisions[(size_t) step], 1, 8);
    }

    void setStepVelocity (int step, float velocityValue) noexcept
    {
        if (step < 0 || step >= kMaxSteps)
            return;
        stepVelocities[(size_t) step] = std::clamp (velocityValue, 0.05f, 1.0f);
    }

    void setStepRatchetCount (int step, int count) noexcept
    {
        if (step < 0 || step >= kMaxSteps)
            return;
        stepRatchets[(size_t) step] = (uint8_t) std::clamp (count, 1, 8);
    }

    void setStepRepeatDivision (int step, int division) noexcept
    {
        if (step < 0 || step >= kMaxSteps)
            return;
        stepRepeatDivisions[(size_t) step] = (uint8_t) std::clamp (division, 1, 8);
        if (stepRatchets[(size_t) step] > stepRepeatDivisions[(size_t) step])
            stepRatchets[(size_t) step] = stepRepeatDivisions[(size_t) step];
    }

    void toggleStep (int step) noexcept
    {
        if (step < 0 || step >= std::clamp (stepCount, 1, kMaxSteps))
            return;
        stepBits ^= (1ULL << step);
    }

    void setStepCount (int newCount) noexcept
    {
        stepCount = std::clamp (newCount, 1, kMaxSteps);
        if (stepCount < 64)
        {
            const auto mask = (1ULL << stepCount) - 1ULL;
            stepBits &= mask;
        }
    }
};

struct DrumModuleState
{
    std::string                name { "Drum Kit" };
    std::vector<DrumVoiceState> voices;
    int                        focusedVoiceIndex { 0 };
    int                        focusedStepIndex { -1 };

    static DrumVoiceState makeVoiceForRole (DrumVoiceRole role,
                                            std::uint32_t colorArgb,
                                            int stepCount = 16)
    {
        DrumVoiceState voice;
        voice.name = DrumVoiceParams::defaultName (role);
        voice.params = DrumVoiceParams::makeForRole (role);
        voice.midiNote = voice.params.midiNote;
        voice.colorArgb = colorArgb;
        voice.stepCount = stepCount;
        voice.setStepCount (stepCount);
        return voice;
    }

    static DrumModuleState makeDefault()
    {
        DrumModuleState state;
        state.name = "Spool Core";

        struct RolePreset
        {
            DrumVoiceRole role;
            std::uint32_t color;
        };

        const RolePreset defaults[] =
        {
            { DrumVoiceRole::kick,    0xFFDB8E4B },
            { DrumVoiceRole::subKick, 0xFFB86D3C },
            { DrumVoiceRole::snare,   0xFF4B9EDB },
            { DrumVoiceRole::hat,     0xFF9EDB4B },
            { DrumVoiceRole::perc1,   0xFFDB4B9E },
            { DrumVoiceRole::perc2,   0xFF4BDBB3 },
        };

        state.voices.reserve (std::size (defaults));
        for (const auto& preset : defaults)
            state.voices.push_back (makeVoiceForRole (preset.role, preset.color));

        state.clamp();
        return state;
    }

    void clamp() noexcept
    {
        for (auto& voice : voices)
        {
            voice.stepCount   = std::clamp (voice.stepCount, 1, DrumVoiceState::kMaxSteps);
            voice.midiNote    = std::clamp (voice.midiNote, 0, 127);
            voice.midiChannel = std::clamp (voice.midiChannel, 1, 16);
            voice.velocity    = std::clamp (voice.velocity, 0.0f, 1.0f);
            voice.params.midiNote = voice.midiNote;
            voice.setStepCount (voice.stepCount);
        }

        if (voices.empty())
        {
            focusedVoiceIndex = 0;
            focusedStepIndex = -1;
        }
        else
        {
            focusedVoiceIndex = std::clamp (focusedVoiceIndex, 0, static_cast<int> (voices.size()) - 1);
            focusedStepIndex = std::clamp (focusedStepIndex, -1, DrumVoiceState::kMaxSteps - 1);
        }
    }

    DrumKitPatch toKitPatch() const
    {
        DrumKitPatch kit;
        kit.name = name;

        for (const auto& voice : voices)
        {
            DrumKitEntry entry;
            entry.name   = voice.name;
            entry.params = voice.params;
            entry.params.midiNote = voice.midiNote;
            kit.voices.push_back (entry);
        }

        return kit;
    }

    static DrumModuleState fromKitPatch (const DrumKitPatch& kit)
    {
        DrumModuleState state;
        state.name = kit.name;
        state.voices.reserve (kit.voices.size());

        static const std::uint32_t kDefaultColors[] =
        {
            0xFFDB8E4B, 0xFF4B9EDB, 0xFF9EDB4B, 0xFFDB4B9E,
            0xFF4BDBB3, 0xFFB34BDB, 0xFFDBDB4B, 0xFF4B6BDB
        };

        for (size_t i = 0; i < kit.voices.size(); ++i)
        {
            DrumVoiceState voice;
            voice.name      = kit.voices[i].name;
            voice.params    = kit.voices[i].params;
            voice.midiNote  = kit.voices[i].params.midiNote;
            voice.colorArgb = kDefaultColors[i % std::size (kDefaultColors)];
            state.voices.push_back (voice);
        }

        state.clamp();
        return state;
    }

    static juce::String midiNoteName (int note)
    {
        static const char* noteNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        const int safeNote = std::clamp (note, 0, 127);
        const int octave = (safeNote / 12) - 2;
        return juce::String (noteNames[safeNote % 12]) + juce::String (octave);
    }
};

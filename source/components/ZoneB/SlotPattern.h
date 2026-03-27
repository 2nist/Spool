#pragma once

#include "../../Theme.h"
#include <array>

//==============================================================================
/**
    SlotPattern — per-slot sequencer phrase data.

    Each pattern contains variable-length step containers. A step can host
    multiple micro note events, with timing/length stored relative to the
    step duration so the engine can later reinterpret the same phrase using
    different note modes.
*/
struct SlotPattern
{
    static constexpr int MAX_STEPS            = 64;
    static constexpr int MAX_MICRO_EVENTS     = 8;
    static constexpr int NUM_PATTERNS         = 8;
    static constexpr int MAX_STEP_DURATION    = 16;
    static constexpr int DEFAULT_PATTERN_UNITS = 16;
    static constexpr int kUseSlotBasePitch    = -1;
    static constexpr int kMinTheoryValue      = -24;
    static constexpr int kMaxTheoryValue      = 127;

    enum class LaneContext : uint8_t
    {
        melodic = 0,
        bass,
        chord,
        utility,
        drum
    };

    enum class GateMode : uint8_t
    {
        gate = 0,
        tie,
        rest,
        hold
    };

    enum class NoteMode : uint8_t
    {
        absolute = 0,
        degree,
        chordTone,
        interval
    };

    enum class StepRole : uint8_t
    {
        bass = 0,
        chord,
        lead,
        fill,
        transition
    };

    enum class HarmonicSource : uint8_t
    {
        key = 0,
        chord,
        nextChord
    };

    struct MicroEvent
    {
        float timeOffset { 0.0f };  // 0..1 inside the step
        float length     { 0.35f }; // 0..1 of the step duration
        int   pitchValue { kUseSlotBasePitch };
        uint8_t velocity { 100 };
    };

    struct Step
    {
        int      start      { 0 };  // in 16th-note units from pattern start
        int      duration   { 1 };  // in 16th-note units
        GateMode gateMode   { GateMode::gate };
        bool     accent     { false };
        StepRole role       { StepRole::lead };
        NoteMode noteMode   { NoteMode::absolute };
        HarmonicSource harmonicSource { HarmonicSource::key };
        bool     lookAheadToNextChord { false };
        int      anchorValue { kUseSlotBasePitch };
        int      microEventCount { 0 };
        std::array<MicroEvent, MAX_MICRO_EVENTS> microEvents {};
    };

    struct PatternData
    {
        int patternLength { DEFAULT_PATTERN_UNITS };
        LaneContext laneContext { LaneContext::melodic };
        int stepCount { DEFAULT_PATTERN_UNITS };
        std::array<Step, MAX_STEPS> steps {};
    };

    std::array<PatternData, NUM_PATTERNS> patterns {};
    int currentPattern = 0;

    SlotPattern()
    {
        resetToDefault();
    }

    PatternData& current() noexcept
    {
        return patterns[juce::jlimit (0, NUM_PATTERNS - 1, currentPattern)];
    }

    const PatternData& current() const noexcept
    {
        return patterns[juce::jlimit (0, NUM_PATTERNS - 1, currentPattern)];
    }

    int activeStepCount() const noexcept { return current().stepCount; }
    int patternLength() const noexcept   { return current().patternLength; }

    static const char* shortLabel (NoteMode mode) noexcept
    {
        switch (mode)
        {
            case NoteMode::absolute:  return "ABS";
            case NoteMode::degree:    return "DEG";
            case NoteMode::chordTone: return "CHD";
            case NoteMode::interval:  return "INT";
        }

        return "ABS";
    }

    static const char* shortLabel (StepRole role) noexcept
    {
        switch (role)
        {
            case StepRole::bass:       return "BASS";
            case StepRole::chord:      return "CHRD";
            case StepRole::lead:       return "LEAD";
            case StepRole::fill:       return "FILL";
            case StepRole::transition: return "XIT";
        }

        return "LEAD";
    }

    static const char* shortLabel (HarmonicSource source) noexcept
    {
        switch (source)
        {
            case HarmonicSource::key:       return "KEY";
            case HarmonicSource::chord:     return "CHORD";
            case HarmonicSource::nextChord: return "NEXT";
        }

        return "KEY";
    }

    const Step* getStep (int index) const noexcept
    {
        return juce::isPositiveAndBelow (index, current().stepCount) ? &current().steps[(size_t) index] : nullptr;
    }

    Step* getStep (int index) noexcept
    {
        return juce::isPositiveAndBelow (index, current().stepCount) ? &current().steps[(size_t) index] : nullptr;
    }

    bool stepActive (int step) const noexcept
    {
        const auto* s = getStep (step);
        return s != nullptr
            && s->gateMode != GateMode::rest
            && s->microEventCount > 0;
    }

    bool stepAccented (int step) const noexcept
    {
        const auto* s = getStep (step);
        return s != nullptr && s->accent;
    }

    int stepMicroEventCount (int step) const noexcept
    {
        const auto* s = getStep (step);
        return s != nullptr ? s->microEventCount : 0;
    }

    void toggleStep (int step)
    {
        auto* s = getStep (step);
        if (s == nullptr)
            return;

        if (stepActive (step))
        {
            s->gateMode = GateMode::rest;
            s->microEventCount = 0;
        }
        else
        {
            if (s->microEventCount <= 0)
            {
                s->microEventCount = 1;
                s->microEvents[0] = {};
            }
            s->gateMode = GateMode::gate;
            s->noteMode = NoteMode::absolute;
        }
    }

    void setStepCount (int n)
    {
        auto& pat = current();
        const int target = juce::jlimit (1, MAX_STEPS, n);

        if (target > pat.stepCount)
        {
            for (int i = pat.stepCount; i < target; ++i)
            {
                pat.steps[(size_t) i] = {};
                pat.steps[(size_t) i].duration = 1;
            }
        }

        pat.stepCount = target;
        normalizeStepLayout();
    }

    void setPatternLength (int units)
    {
        auto& pat = current();
        const int target = juce::jlimit (1, MAX_STEPS * MAX_STEP_DURATION, units);
        int currentLength = juce::jmax (1, pat.patternLength);

        while (currentLength > target && pat.stepCount > 1)
        {
            auto& last = pat.steps[(size_t) (pat.stepCount - 1)];
            const int over = currentLength - target;

            if (last.duration > over)
            {
                last.duration -= over;
                currentLength -= over;
            }
            else
            {
                currentLength -= last.duration;
                pat.steps[(size_t) (pat.stepCount - 1)] = {};
                pat.stepCount -= 1;
            }
        }

        if (currentLength < target)
        {
            auto& last = pat.steps[(size_t) juce::jmax (0, pat.stepCount - 1)];
            last.duration = juce::jlimit (1, MAX_STEP_DURATION, last.duration + (target - currentLength));
        }

        normalizeStepLayout();
    }

    void setStepDuration (int step, int units)
    {
        auto& pat = current();
        auto* s = getStep (step);
        if (s == nullptr)
            return;

        const int requested = juce::jlimit (1, MAX_STEP_DURATION, units);
        const int currentDuration = s->duration;
        if (requested == currentDuration)
            return;

        const bool hasNextStep = step + 1 < pat.stepCount;

        if (! hasNextStep)
        {
            s->duration = requested;
            normalizeStepLayout();
            return;
        }

        if (requested > currentDuration)
        {
            int extraNeeded = requested - currentDuration;
            int consumed = 0;

            for (int i = step + 1; i < pat.stepCount && extraNeeded > 0;)
            {
                auto& next = pat.steps[(size_t) i];

                if (next.duration <= extraNeeded)
                {
                    consumed += next.duration;
                    extraNeeded -= next.duration;

                    for (int move = i; move < pat.stepCount - 1; ++move)
                        pat.steps[(size_t) move] = pat.steps[(size_t) (move + 1)];

                    pat.steps[(size_t) (pat.stepCount - 1)] = {};
                    pat.stepCount -= 1;
                    continue;
                }

                next.duration -= extraNeeded;
                consumed += extraNeeded;
                extraNeeded = 0;
                break;
            }

            s->duration = currentDuration + consumed;
        }
        else
        {
            int freed = currentDuration - requested;
            s->duration = requested;

            for (int i = step + 1; i < pat.stepCount && freed > 0; ++i)
            {
                auto& next = pat.steps[(size_t) i];
                const int capacity = juce::jmax (0, MAX_STEP_DURATION - next.duration);
                const int give = juce::jmin (capacity, freed);
                next.duration += give;
                freed -= give;
            }

            if (freed > 0)
                pat.steps[(size_t) (pat.stepCount - 1)].duration = juce::jlimit (1,
                                                                                  MAX_STEP_DURATION,
                                                                                  pat.steps[(size_t) (pat.stepCount - 1)].duration + freed);
        }

        normalizeStepLayout();
    }

    void addStepAfter (int step, int durationUnits = 1)
    {
        auto& pat = current();
        if (pat.stepCount >= MAX_STEPS)
            return;

        const int insertIndex = juce::jlimit (0, pat.stepCount, step + 1);
        for (int i = pat.stepCount; i > insertIndex; --i)
            pat.steps[(size_t) i] = pat.steps[(size_t) (i - 1)];

        pat.steps[(size_t) insertIndex] = {};
        pat.steps[(size_t) insertIndex].duration = juce::jlimit (1, MAX_STEP_DURATION, durationUnits);
        pat.stepCount += 1;
        normalizeStepLayout();
    }

    void removeStep (int step)
    {
        auto& pat = current();
        if (pat.stepCount <= 1 || ! juce::isPositiveAndBelow (step, pat.stepCount))
            return;

        for (int i = step; i < pat.stepCount - 1; ++i)
            pat.steps[(size_t) i] = pat.steps[(size_t) (i + 1)];

        pat.steps[(size_t) (pat.stepCount - 1)] = {};
        pat.stepCount -= 1;
        normalizeStepLayout();
    }

    int addMicroEvent (int step, float offset, float length, int pitch, uint8_t velocity)
    {
        auto* s = getStep (step);
        if (s == nullptr || s->microEventCount >= MAX_MICRO_EVENTS)
            return -1;

        auto& event = s->microEvents[(size_t) s->microEventCount];
        event.timeOffset = juce::jlimit (0.0f, 0.98f, offset);
        event.length     = juce::jlimit (0.05f, 1.0f, length);
        event.pitchValue = juce::jlimit (kMinTheoryValue, kMaxTheoryValue, pitch);
        event.velocity   = static_cast<uint8_t> (juce::jlimit (1, 127, (int) velocity));
        s->microEventCount += 1;
        s->gateMode = GateMode::gate;
        return s->microEventCount - 1;
    }

    void removeMicroEvent (int step, int eventIndex)
    {
        auto* s = getStep (step);
        if (s == nullptr || ! juce::isPositiveAndBelow (eventIndex, s->microEventCount))
            return;

        for (int i = eventIndex; i < s->microEventCount - 1; ++i)
            s->microEvents[(size_t) i] = s->microEvents[(size_t) (i + 1)];

        if (s->microEventCount > 0)
            s->microEventCount -= 1;

        if (s->microEventCount == 0)
            s->gateMode = GateMode::rest;
    }

    void updateMicroEvent (int step, int eventIndex, float offset, float length, int pitch, uint8_t velocity)
    {
        auto* s = getStep (step);
        if (s == nullptr || ! juce::isPositiveAndBelow (eventIndex, s->microEventCount))
            return;

        auto& event = s->microEvents[(size_t) eventIndex];
        event.timeOffset = juce::jlimit (0.0f, 0.98f, offset);
        event.length     = juce::jlimit (0.05f, 1.0f, length);
        event.pitchValue = juce::jlimit (kMinTheoryValue, kMaxTheoryValue, pitch);
        event.velocity   = static_cast<uint8_t> (juce::jlimit (1, 127, (int) velocity));
        s->gateMode = GateMode::gate;
    }

    void clearPattern()
    {
        auto& pat = current();
        const auto lane = pat.laneContext;
        pat = {};
        pat.laneContext = lane;
        pat.stepCount = DEFAULT_PATTERN_UNITS;
        for (int i = 0; i < pat.stepCount; ++i)
            pat.steps[(size_t) i].duration = 1;
        normalizeStepLayout();
    }

    void normalizeStepLayout()
    {
        auto& pat = current();
        int cursor = 0;
        for (int i = 0; i < pat.stepCount; ++i)
        {
            auto& step = pat.steps[(size_t) i];
            step.duration = juce::jlimit (1, MAX_STEP_DURATION, step.duration);
            step.start = cursor;
            cursor += step.duration;
        }

        pat.patternLength = juce::jmax (1, cursor);
    }

    void resetToDefault()
    {
        const int previousPattern = currentPattern;
        for (int patternIndex = 0; patternIndex < NUM_PATTERNS; ++patternIndex)
        {
            auto& pat = patterns[(size_t) patternIndex];
            pat = {};
            pat.patternLength = DEFAULT_PATTERN_UNITS;
            pat.stepCount = DEFAULT_PATTERN_UNITS;
            for (int i = 0; i < pat.stepCount; ++i)
                pat.steps[(size_t) i].duration = 1;
            currentPattern = patternIndex;
            normalizeStepLayout();
        }

        currentPattern = juce::jlimit (0, NUM_PATTERNS - 1, previousPattern);
    }

    SlotPattern copyPattern() const { return *this; }
};

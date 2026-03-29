#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

namespace TimelineEditMath
{
enum class EditMode
{
    move,
    trimStart,
    trimEnd
};

struct ClipSegment
{
    float startBeat { 0.0f };
    float lengthBeats { 0.25f };
};

struct EditRequest
{
    EditMode mode { EditMode::move };
    float deltaBeats { 0.0f };
    float loopBeats { 8.0f };
    float minLengthBeats { 0.25f };
    float snapStep { 0.25f };
    bool snapEnabled { true };
};

inline float wrapBeat (float beat, float loopBeats) noexcept
{
    if (loopBeats <= 0.0f)
        return 0.0f;

    float wrapped = std::fmod (beat, loopBeats);
    if (wrapped < 0.0f)
        wrapped += loopBeats;
    return wrapped;
}

inline float snapBeat (float beat, float snapStep) noexcept
{
    if (snapStep <= 0.0f)
        return beat;
    return std::round (beat / snapStep) * snapStep;
}

inline ClipSegment applyEdit (const ClipSegment& original, const EditRequest& request)
{
    ClipSegment result = original;

    const float loopLen = juce::jmax (1.0f, request.loopBeats);
    const float minLen = juce::jmax (0.0001f, request.minLengthBeats);
    const float startUnwrapped = original.startBeat;
    const float endUnwrapped = original.startBeat + juce::jmax (minLen, original.lengthBeats);
    const bool crossesLoopSeam = endUnwrapped > loopLen;

    if (request.mode == EditMode::move)
    {
        result.startBeat = wrapBeat (original.startBeat + request.deltaBeats, loopLen);
        if (request.snapEnabled)
            result.startBeat = wrapBeat (snapBeat (result.startBeat, request.snapStep), loopLen);
        return result;
    }

    if (request.mode == EditMode::trimStart)
    {
        const float minStartUnwrapped = crossesLoopSeam ? (endUnwrapped - loopLen) : 0.0f;
        const float maxStartUnwrapped = endUnwrapped - minLen;
        float trimmedStartUnwrapped = juce::jlimit (minStartUnwrapped,
                                                    maxStartUnwrapped,
                                                    startUnwrapped + request.deltaBeats);
        if (request.snapEnabled)
            trimmedStartUnwrapped = juce::jlimit (minStartUnwrapped,
                                                  maxStartUnwrapped,
                                                  snapBeat (trimmedStartUnwrapped, request.snapStep));

        result.startBeat = wrapBeat (trimmedStartUnwrapped, loopLen);
        result.lengthBeats = juce::jlimit (minLen, loopLen, endUnwrapped - trimmedStartUnwrapped);
        return result;
    }

    const float minEndUnwrapped = startUnwrapped + minLen;
    const float maxEndUnwrapped = crossesLoopSeam ? (startUnwrapped + loopLen) : loopLen;
    float trimmedEndUnwrapped = juce::jlimit (minEndUnwrapped,
                                              maxEndUnwrapped,
                                              endUnwrapped + request.deltaBeats);
    if (request.snapEnabled)
        trimmedEndUnwrapped = juce::jlimit (minEndUnwrapped,
                                            maxEndUnwrapped,
                                            snapBeat (trimmedEndUnwrapped, request.snapStep));

    result.lengthBeats = juce::jlimit (minLen, loopLen, trimmedEndUnwrapped - startUnwrapped);
    return result;
}

inline bool splitOffsetWithinClip (float clipStartBeat,
                                   float clipLengthBeats,
                                   float splitBeat,
                                   float loopBeats,
                                   float minLengthBeats,
                                   float& outSplitOffset) noexcept
{
    const float loopLen = juce::jmax (1.0f, loopBeats);
    const float minLen = juce::jmax (0.0001f, minLengthBeats);
    const float start = wrapBeat (clipStartBeat, loopLen);
    const float split = wrapBeat (splitBeat, loopLen);
    const float length = juce::jlimit (minLen, loopLen, clipLengthBeats);

    float offset = split - start;
    if (offset < 0.0f)
        offset += loopLen;

    if (offset <= minLen || offset >= (length - minLen))
        return false;

    outSplitOffset = offset;
    return true;
}
} // namespace TimelineEditMath

#pragma once

#include "../../dsp/CapturedAudioClip.h"

struct LooperSession
{
    juce::AudioBuffer<float> originalBuffer;
    juce::AudioBuffer<float> workingBuffer;

    double sampleRate { 44100.0 };
    int trimStartSample { 0 };
    int trimEndSample { 0 };
    int loopStartSample { 0 };
    int loopEndSample { 0 };
    float playbackProgress { 0.0f };

    bool hasClip { false };
    bool playing { false };
    bool looping { true };
    bool overdubEnabled { false };
    bool normalized { false };

    juce::String sourceName;
    int sourceSlot { -1 };
    double tempo { 0.0 };
    int bars { 0 };
    juce::String sourcePath;

    void clear();
    bool hasAudio() const noexcept;
    int totalSamples() const noexcept;
    int editedSamples() const noexcept;
    float durationSeconds() const noexcept;
    void loadFromClip (const CapturedAudioClip& clip);
    void resetTrim();
    void setTrimStart (int sample) noexcept;
    void setTrimEnd (int sample) noexcept;
    void normalize();
    void overdubFrom (const CapturedAudioClip& clip);
    CapturedAudioClip makeEditedClip() const;
};

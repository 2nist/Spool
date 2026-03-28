#include "LooperSession.h"

#include <cmath>

void LooperSession::clear()
{
    originalBuffer.setSize (0, 0);
    workingBuffer.setSize (0, 0);
    sampleRate = 44100.0;
    trimStartSample = trimEndSample = 0;
    loopStartSample = loopEndSample = 0;
    playbackProgress = 0.0f;
    hasClip = false;
    playing = false;
    looping = true;
    normalized = false;
    sourceName.clear();
    sourceSlot = -1;
    tempo = 0.0;
    bars = 0;
    sourcePath.clear();
}

bool LooperSession::hasAudio() const noexcept
{
    return hasClip && workingBuffer.getNumSamples() > 0 && workingBuffer.getNumChannels() > 0;
}

int LooperSession::totalSamples() const noexcept
{
    return workingBuffer.getNumSamples();
}

int LooperSession::editedSamples() const noexcept
{
    return juce::jmax (0, trimEndSample - trimStartSample);
}

float LooperSession::durationSeconds() const noexcept
{
    return sampleRate > 0.0 ? static_cast<float> (editedSamples() / sampleRate) : 0.0f;
}

void LooperSession::loadFromClip (const CapturedAudioClip& clip)
{
    clear();
    if (clip.buffer.getNumSamples() <= 0 || clip.buffer.getNumChannels() <= 0)
        return;

    originalBuffer.makeCopyOf (clip.buffer);
    workingBuffer.makeCopyOf (clip.buffer);
    sampleRate = clip.sampleRate > 0.0 ? clip.sampleRate : 44100.0;
    trimStartSample = 0;
    trimEndSample = workingBuffer.getNumSamples();
    loopStartSample = trimStartSample;
    loopEndSample = trimEndSample;
    sourceName = clip.sourceName;
    sourceSlot = clip.sourceSlot;
    tempo = clip.tempo;
    bars = clip.bars;
    hasClip = true;
}

void LooperSession::resetTrim()
{
    if (! hasAudio())
        return;

    trimStartSample = 0;
    trimEndSample = workingBuffer.getNumSamples();
    loopStartSample = trimStartSample;
    loopEndSample = trimEndSample;
}

void LooperSession::setTrimStart (int sample) noexcept
{
    if (! hasAudio())
        return;

    const int maxStart = juce::jmax (0, trimEndSample - 64);
    trimStartSample = juce::jlimit (0, maxStart, sample);
    loopStartSample = trimStartSample;
    loopEndSample = trimEndSample;
}

void LooperSession::setTrimEnd (int sample) noexcept
{
    if (! hasAudio())
        return;

    const int minEnd = juce::jmin (workingBuffer.getNumSamples(), trimStartSample + 64);
    trimEndSample = juce::jlimit (minEnd, workingBuffer.getNumSamples(), sample);
    loopStartSample = trimStartSample;
    loopEndSample = trimEndSample;
}

void LooperSession::normalize()
{
    if (! hasAudio())
        return;

    float peak = 0.0f;
    for (int ch = 0; ch < workingBuffer.getNumChannels(); ++ch)
        peak = juce::jmax (peak, workingBuffer.getMagnitude (ch, 0, workingBuffer.getNumSamples()));

    if (peak <= 0.0001f)
        return;

    workingBuffer.applyGain (0.98f / peak);
    normalized = true;
}

void LooperSession::overdubFrom (const CapturedAudioClip& clip)
{
    if (! hasAudio() || clip.buffer.getNumSamples() <= 0 || clip.buffer.getNumChannels() <= 0)
        return;

    const int targetStart = juce::jlimit (0, workingBuffer.getNumSamples(), loopStartSample);
    const int targetEnd   = juce::jlimit (targetStart + 1, workingBuffer.getNumSamples(), loopEndSample);
    const int targetLen   = juce::jmax (1, targetEnd - targetStart);
    const double sourceRate = clip.sampleRate > 0.0 ? clip.sampleRate : sampleRate;
    const double step = sourceRate > 0.0 ? (sourceRate / sampleRate) : 1.0;

    for (int ch = 0; ch < workingBuffer.getNumChannels(); ++ch)
    {
        auto* dst = workingBuffer.getWritePointer (ch);
        const float* src = clip.buffer.getReadPointer (juce::jmin (ch, clip.buffer.getNumChannels() - 1));
        const int sourceSamples = clip.buffer.getNumSamples();
        double readPos = 0.0;

        for (int i = 0; i < targetLen; ++i)
        {
            if (sourceSamples <= 0)
                break;

            const int dstIndex = targetStart + i;
            const double wrapped = std::fmod (readPos, static_cast<double> (sourceSamples));
            const int idxA = juce::jlimit (0, sourceSamples - 1, static_cast<int> (wrapped));
            const int idxB = juce::jlimit (0, sourceSamples - 1, idxA + 1);
            const float frac = static_cast<float> (wrapped - static_cast<double> (idxA));
            const float sample = juce::jmap (frac, src[idxA], src[idxB]);
            dst[dstIndex] = juce::jlimit (-1.0f, 1.0f, dst[dstIndex] + sample * 0.75f);
            readPos += step;
        }
    }
}

CapturedAudioClip LooperSession::makeEditedClip() const
{
    CapturedAudioClip clip;
    clip.sampleRate = sampleRate;
    clip.sourceName = sourceName.isNotEmpty() ? sourceName : "LOOPER";
    clip.sourceSlot = sourceSlot;
    clip.tempo = tempo;
    clip.bars = bars;

    if (! hasAudio() || editedSamples() <= 0)
        return clip;

    clip.buffer.setSize (workingBuffer.getNumChannels(), editedSamples());
    for (int ch = 0; ch < workingBuffer.getNumChannels(); ++ch)
        clip.buffer.copyFrom (ch, 0, workingBuffer, ch, trimStartSample, editedSamples());

    if (tempo > 0.0 && bars > 0 && totalSamples() > 0)
    {
        const double ratio = static_cast<double> (editedSamples()) / static_cast<double> (totalSamples());
        clip.bars = juce::jmax (1, juce::roundToInt (static_cast<double> (bars) * ratio));
    }

    return clip;
}

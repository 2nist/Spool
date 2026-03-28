#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>

namespace
{
void runBlocks (PluginProcessor& processor, int blockCount, int blockSize = 256)
{
    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    for (int i = 0; i < blockCount; ++i)
    {
        buffer.clear();
        midi.clear();
        processor.processBlock (buffer, midi);
    }
}
}

TEST_CASE ("Playhead is monotonic while transport is playing", "[transport][playhead]")
{
    PluginProcessor processor;
    processor.prepareToPlay (48000.0, 256);
    processor.seekTransport (0.0);
    processor.setBpm (120.0f);
    processor.setPlaying (true);

    double previousBeat = processor.getCurrentSongBeat();
    for (int i = 0; i < 128; ++i)
    {
        runBlocks (processor, 1);
        const auto currentBeat = processor.getCurrentSongBeat();
        REQUIRE (currentBeat >= previousBeat);
        previousBeat = currentBeat;
    }

    processor.setPlaying (false);
    processor.releaseResources();
}

TEST_CASE ("Playhead can move backward only on explicit seek", "[transport][playhead][seek]")
{
    PluginProcessor processor;
    processor.prepareToPlay (48000.0, 256);
    processor.setBpm (120.0f);
    processor.seekTransport (8.0);

    const auto beforeSeek = processor.getCurrentSongBeat();
    processor.seekTransport (2.0);
    const auto afterSeek = processor.getCurrentSongBeat();

    REQUIRE (beforeSeek > afterSeek);
    processor.releaseResources();
}

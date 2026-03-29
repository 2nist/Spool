#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <dsp/SongManager.h>
#include <midi/MidiConstraintEngine.h>
#include <structure/StructureEngine.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cmath>

namespace
{
constexpr double kTestSampleRate = 48000.0;
constexpr int    kTestBlockSize  = 480;
constexpr float   kAudioThreshold = 0.0001f;

CapturedAudioClip makeConstantTimelineClip (int numSamples = 24000, float value = 1.0f)
{
    CapturedAudioClip clip;
    clip.sampleRate = kTestSampleRate;
    clip.sourceName = "TEST";
    clip.sourceSlot = 0;
    clip.tempo = 120.0;
    clip.bars = 1;
    clip.buffer.setSize (2, numSamples);

    for (int ch = 0; ch < clip.buffer.getNumChannels(); ++ch)
        for (int i = 0; i < clip.buffer.getNumSamples(); ++i)
            clip.buffer.setSample (ch, i, value);

    return clip;
}

void deactivateAllSlots (PluginProcessor& processor)
{
    for (int slot = 0; slot < SpoolAudioGraph::kNumSlots; ++slot)
        processor.setSlotActive (slot, false);
}

juce::AudioBuffer<float> processTimelineBlock (PluginProcessor& processor)
{
    juce::AudioBuffer<float> buffer (2, kTestBlockSize);
    buffer.clear();
    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);
    return buffer;
}

bool hasAudibleSample (const juce::AudioBuffer<float>& buffer, float threshold = kAudioThreshold)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (std::abs (buffer.getSample (ch, i)) > threshold)
                return true;

    return false;
}

int firstAudibleSample (const juce::AudioBuffer<float>& buffer, float threshold = kAudioThreshold)
{
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            if (std::abs (buffer.getSample (ch, i)) > threshold)
                return i;

    return -1;
}
} // namespace

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

TEST_CASE ("Plugin instance", "[instance]")
{
    PluginProcessor testPlugin;

    SECTION ("name")
    {
        CHECK_THAT (testPlugin.getName().toStdString(),
            Catch::Matchers::Equals ("Spool"));
    }
}

TEST_CASE ("Timeline clip starts on playhead beat", "[timeline][playback]")
{
    PluginProcessor processor;
    processor.prepareToPlay (kTestSampleRate, kTestBlockSize);
    deactivateAllSlots (processor);
    processor.setTimelineLoopLengthBeats (8.0);
    processor.setPlaying (true);

    const auto clip = makeConstantTimelineClip();
    processor.addTimelineAudioClip (clip, 2.0, 1.0, 0, "AUDIO", "TEST CLIP");

    processor.seekTransport (1.98);
    auto before = processTimelineBlock (processor);
    CHECK_FALSE (hasAudibleSample (before));

    processor.seekTransport (2.0);
    auto atStart = processTimelineBlock (processor);
    CHECK (hasAudibleSample (atStart));
    CHECK (firstAudibleSample (atStart) <= 2);
}

TEST_CASE ("Timeline clip wraps with loop region", "[timeline][loop]")
{
    PluginProcessor processor;
    processor.prepareToPlay (kTestSampleRate, kTestBlockSize);
    deactivateAllSlots (processor);
    processor.setTimelineLoopLengthBeats (4.0);
    processor.setPlaying (true);

    const auto clip = makeConstantTimelineClip();
    processor.addTimelineAudioClip (clip, 3.5, 1.0, 0, "AUDIO", "WRAP CLIP");

    processor.seekTransport (3.40);
    auto preClip = processTimelineBlock (processor);
    CHECK_FALSE (hasAudibleSample (preClip));

    processor.seekTransport (3.99);
    auto nearWrap = processTimelineBlock (processor);
    CHECK (hasAudibleSample (nearWrap));

    auto wrapped = processTimelineBlock (processor);
    CHECK (hasAudibleSample (wrapped));
}

TEST_CASE ("Timeline clip onset stays near expected sample", "[timeline][timing]")
{
    PluginProcessor processor;
    processor.prepareToPlay (kTestSampleRate, kTestBlockSize);
    deactivateAllSlots (processor);
    processor.setTimelineLoopLengthBeats (8.0);
    processor.setPlaying (true);

    const auto clip = makeConstantTimelineClip();
    processor.addTimelineAudioClip (clip, 2.25, 1.0, 0, "AUDIO", "DRIFT CLIP");

    processor.seekTransport (2.24);
    auto block = processTimelineBlock (processor);
    const int onset = firstAudibleSample (block);
    REQUIRE (onset >= 0);
    CHECK (onset >= 238);
    CHECK (onset <= 242);
}

TEST_CASE ("SongManager JSON round-trip", "[song][json]")
{
    SongManager source;

    SongPattern pattern;
    pattern.name = "Verse";
    pattern.lengthBeats = 8.0;
    pattern.baseSteps.addArray ({ 0, 2, 4, 6 });
    const int patternId = source.addPattern (pattern);

    SongSection section;
    section.patternId = patternId;
    section.slotMask = 0b00000011;
    section.startBeat = 0.0;
    section.lengthBeats = 8.0;
    section.repeats = 2;
    section.transpose = 1;
    source.addSection (section);

    ChordEvent chord;
    chord.startBeat = 0.0;
    chord.durationBeats = 4.0;
    chord.root = 65;
    source.addChord (chord);

    TimelineClipPlacement placement;
    placement.laneIndex = 2;
    placement.moduleType = "SYNTH";
    placement.clipName = "SLOT 03";
    placement.clipId = "clip-abc";
    placement.audioAssetPath = "spool-media/Test/timeline-clips/clip-abc.wav";
    placement.startBeat = 16.0f;
    placement.lengthBeats = 8.0f;
    placement.sourceStartBeat = 1.5f;
    placement.sourceTotalBeats = 16.0f;
    source.addTimelinePlacement (placement);
    source.setTimelineLaneArmed (2, true);
    source.setTimelineLaneArmed (6, true);

    auto temp = juce::File::getSpecialLocation (juce::File::tempDirectory)
                  .getChildFile ("pamplejuce-songmanager-roundtrip.json");
    temp.deleteFile();

    REQUIRE (source.saveToFile (temp));

    SongManager loaded;
    REQUIRE (loaded.loadFromFile (temp));
    temp.deleteFile();

    REQUIRE (loaded.getPatterns().size() >= 2);
    CHECK (loaded.getPatterns().getReference (1).name == "Verse");
    CHECK (loaded.getSections().size() == 1);
    CHECK (loaded.getSections().getReference (0).patternId == 1);
    CHECK (loaded.getChords().size() == 1);
    CHECK (loaded.getChords().getReference (0).root == 65);
    REQUIRE (loaded.getTimelinePlacements().size() == 1);
    CHECK (loaded.getTimelinePlacements().getReference (0).laneIndex == 2);
    CHECK (loaded.getTimelinePlacements().getReference (0).moduleType == "SYNTH");
    CHECK (loaded.getTimelinePlacements().getReference (0).clipName == "SLOT 03");
    CHECK (loaded.getTimelinePlacements().getReference (0).clipId == "clip-abc");
    CHECK (loaded.getTimelinePlacements().getReference (0).audioAssetPath == "spool-media/Test/timeline-clips/clip-abc.wav");
    CHECK (std::abs (loaded.getTimelinePlacements().getReference (0).startBeat - 16.0f) < 0.001f);
    CHECK (std::abs (loaded.getTimelinePlacements().getReference (0).lengthBeats - 8.0f) < 0.001f);
    CHECK (std::abs (loaded.getTimelinePlacements().getReference (0).sourceStartBeat - 1.5f) < 0.001f);
    CHECK (std::abs (loaded.getTimelinePlacements().getReference (0).sourceTotalBeats - 16.0f) < 0.001f);
    CHECK (loaded.getTimelineLaneArmed()[2]);
    CHECK (loaded.getTimelineLaneArmed()[6]);
    CHECK_FALSE (loaded.getTimelineLaneArmed()[0]);
}

TEST_CASE ("SongManager beat query mapping", "[song][query]")
{
    SongManager manager;

    SongPattern pattern;
    pattern.name = "BeatMap";
    pattern.lengthBeats = 4.0;
    pattern.baseSteps.addArray ({ 0, 1, 2, 3 });
    const int patternId = manager.addPattern (pattern);

    SongSection section;
    section.patternId = patternId;
    section.slotMask = 0b00000101;
    section.startBeat = 0.0;
    section.lengthBeats = 4.0;
    section.repeats = 1;
    section.transpose = 2;
    manager.addSection (section);

    ChordEvent chord;
    chord.startBeat = 0.0;
    chord.durationBeats = 4.0;
    chord.root = 67;
    manager.addChord (chord);

    const auto state = manager.query (1.5);
    CHECK (state.sectionId == 0);
    CHECK (std::abs (state.localBeat - 1.5) < 0.001);
    CHECK (state.transposeTotal == 2);
    CHECK (state.numActiveSlots == 2);
    CHECK (state.activeSlots[0] == 0);
    CHECK (state.activeSlots[1] == 2);
    CHECK (state.hasActiveChord);
    CHECK (state.activeChord.root == 67);
}

TEST_CASE ("StructureEngine section + chord lookup", "[structure][engine]")
{
    SongManager manager;
    auto& state = manager.getStructureStateForEdit();
    state.sections.clear();
    state.arrangement.clear();

    Section verse;
    verse.id = "verse";
    verse.name = "Verse";
    verse.bars = 4;
    verse.beatsPerBar = 4;
    verse.repeats = 1;
    verse.progression = { { "C", "maj" }, { "F", "maj" }, { "G", "maj" }, { "C", "maj" } };
    state.sections.push_back (verse);

    ArrangementBlock block;
    block.id = "arr-verse";
    block.sectionId = verse.id;
    block.orderIndex = 0;
    block.barsOverride = 4;
    block.repeatsOverride = 1;
    state.arrangement.push_back (block);

    manager.commitAuthoredState();

    StructureEngine engine (manager);
    engine.rebuild();

    const auto& rebuilt = manager.getStructureState();
    REQUIRE (rebuilt.totalBars == 4);

    const auto atTwoBeats = engine.getSectionAtBeat (2.0);
    REQUIRE (atTwoBeats.has_value());
    REQUIRE (atTwoBeats->section != nullptr);
    CHECK (atTwoBeats->section->name == "Verse");
    CHECK_FALSE (engine.getSectionAtBeat (20.0).has_value());

    const auto chordA = engine.getChordAtBeat (0.0);
    const auto chordB = engine.getChordAtBeat (4.1);  // bar 2
    CHECK (chordA.root == "C");
    CHECK (chordB.root == "F");
}

TEST_CASE ("StructureEngine transpose", "[structure][transpose]")
{
    SongManager manager;
    auto& state = manager.getStructureStateForEdit();
    state.sections.clear();
    state.arrangement.clear();

    Section section;
    section.id = "hook";
    section.name = "Hook";
    section.bars = 2;
    section.beatsPerBar = 4;
    section.repeats = 1;
    section.progression = { { "C", "maj" }, { "D#", "min" }, { "A", "maj" } };
    state.sections.push_back (section);

    ArrangementBlock block;
    block.id = "arr-hook";
    block.sectionId = section.id;
    block.orderIndex = 0;
    block.barsOverride = 2;
    block.repeatsOverride = 1;
    state.arrangement.push_back (block);

    manager.commitAuthoredState();

    StructureEngine engine (manager);
    engine.transpose (2);

    const auto& transposed = manager.getStructureState();
    REQUIRE (transposed.sections.size() == 1);
    REQUIRE (transposed.sections[0].progression.size() == 3);
    CHECK (transposed.sections[0].progression[0].root == "D");
    CHECK (transposed.sections[0].progression[1].root == "F");
    CHECK (transposed.sections[0].progression[2].root == "B");
}

TEST_CASE ("MidiConstraintEngine scale lock in C major", "[midi][constraint]")
{
    SongManager manager;
    auto& state = manager.getStructureStateForEdit();
    state.sections.clear();
    state.arrangement.clear();

    Section section;
    section.id = "scale";
    section.name = "Scale";
    section.bars = 4;
    section.beatsPerBar = 4;
    section.repeats = 1;
    section.progression = { { "C", "maj" } };
    state.sections.push_back (section);

    ArrangementBlock block;
    block.id = "arr-scale";
    block.sectionId = section.id;
    block.orderIndex = 0;
    block.barsOverride = 4;
    block.repeatsOverride = 1;
    state.arrangement.push_back (block);

    manager.commitAuthoredState();

    StructureEngine structureEngine (manager);
    structureEngine.rebuild();
    MidiConstraintEngine constraint;
    constraint.setStructure (&manager.getStructureState());
    constraint.setStructureEngine (&structureEngine);
    constraint.setScaleLock (true);
    constraint.setChordLock (false);

    const int in = 61;   // C#
    const int out = constraint.processNote (in, 0.0);
    CHECK (out == 62);    // D
}

TEST_CASE ("MidiConstraintEngine chord lock in A minor", "[midi][constraint]")
{
    SongManager manager;
    auto& state = manager.getStructureStateForEdit();
    state.sections.clear();
    state.arrangement.clear();

    Section section;
    section.id = "chord";
    section.name = "Chord";
    section.bars = 4;
    section.beatsPerBar = 4;
    section.repeats = 1;
    section.progression = { { "A", "min" } };
    state.sections.push_back (section);

    ArrangementBlock block;
    block.id = "arr-chord";
    block.sectionId = section.id;
    block.orderIndex = 0;
    block.barsOverride = 4;
    block.repeatsOverride = 1;
    state.arrangement.push_back (block);

    manager.commitAuthoredState();

    StructureEngine structureEngine (manager);
    structureEngine.rebuild();
    MidiConstraintEngine constraint;
    constraint.setStructure (&manager.getStructureState());
    constraint.setStructureEngine (&structureEngine);
    constraint.setScaleLock (false);
    constraint.setChordLock (true);

    const int in = 71; // B
    const int out = constraint.processNote (in, 0.0);
    CHECK ((out == 69 || out == 72)); // A or C
}

TEST_CASE ("MidiConstraintEngine guide mode preserves performance input", "[midi][constraint][guide]")
{
    SongManager manager;
    auto& state = manager.getStructureStateForEdit();
    state.sections.clear();
    state.arrangement.clear();

    Section section;
    section.id = "guide";
    section.name = "Guide";
    section.bars = 4;
    section.beatsPerBar = 4;
    section.repeats = 1;
    section.progression = { { "C", "maj" } };
    state.sections.push_back (section);

    ArrangementBlock block;
    block.id = "arr-guide";
    block.sectionId = section.id;
    block.orderIndex = 0;
    block.barsOverride = 4;
    block.repeatsOverride = 1;
    state.arrangement.push_back (block);

    manager.commitAuthoredState();

    StructureEngine structureEngine (manager);
    structureEngine.rebuild();
    MidiConstraintEngine constraint;
    constraint.setStructure (&manager.getStructureState());
    constraint.setStructureEngine (&structureEngine);
    constraint.setScaleLock (true);
    constraint.setChordLock (false);
    constraint.setNotePolicy (MidiConstraintEngine::NotePolicy::Guide);

    const int in = 61; // C#
    const int out = constraint.processNote (in, 0.0);
    const auto guide = constraint.analyzeNote (in, 0.0);

    CHECK (out == in);             // no forced rewrite in guide mode
    CHECK (guide.wouldChange);     // engine still provides corrective suggestion
    CHECK (guide.suggestedNote == 62);
}

TEST_CASE ("MidiConstraintEngine adapts with key/chord changes", "[midi][constraint]")
{
    SongManager manager;
    auto& state = manager.getStructureStateForEdit();
    state.sections.clear();
    state.arrangement.clear();

    Section secA;
    secA.id = "sec-a";
    secA.name = "A";
    secA.bars = 1;
    secA.beatsPerBar = 4;
    secA.repeats = 1;
    secA.progression = { { "C", "maj" } };
    state.sections.push_back (secA);

    Section secB;
    secB.id = "sec-b";
    secB.name = "B";
    secB.bars = 1;
    secB.beatsPerBar = 4;
    secB.repeats = 1;
    secB.progression = { { "D", "maj" } };
    state.sections.push_back (secB);

    ArrangementBlock blockA;
    blockA.id = "arr-a";
    blockA.sectionId = secA.id;
    blockA.orderIndex = 0;
    blockA.barsOverride = 1;
    blockA.repeatsOverride = 1;
    state.arrangement.push_back (blockA);

    ArrangementBlock blockB;
    blockB.id = "arr-b";
    blockB.sectionId = secB.id;
    blockB.orderIndex = 1;
    blockB.barsOverride = 1;
    blockB.repeatsOverride = 1;
    state.arrangement.push_back (blockB);

    manager.commitAuthoredState();

    StructureEngine structureEngine (manager);
    structureEngine.rebuild();
    MidiConstraintEngine constraint;
    constraint.setStructure (&manager.getStructureState());
    constraint.setStructureEngine (&structureEngine);
    constraint.setScaleLock (true);
    constraint.setChordLock (false);

    const int in = 61; // C#
    const int outInC = constraint.processNote (in, 0.0);  // C major context
    const int outInD = constraint.processNote (in, 4.0);  // D major context (bar 2)

    CHECK (outInC == 62); // to D in C major
    CHECK (outInD == 61); // keep C# in D major scale
}


#ifdef PAMPLEJUCE_IPP
    #include <ipp.h>

TEST_CASE ("IPP version", "[ipp]")
{
    #if defined(__APPLE__)
        // macOS uses 2021.9.1 from pip wheel (only x86_64 version available)
        CHECK_THAT (ippsGetLibVersion()->Version, Catch::Matchers::Equals ("2021.9.1 (r0x7e208212)"));
    #else
        CHECK_THAT (ippsGetLibVersion()->Version, Catch::Matchers::Equals ("2022.3.0 (r0x0fc08bb1)"));
    #endif
}
#endif

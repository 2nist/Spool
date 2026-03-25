#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <dsp/SongManager.h>
#include <midi/MidiConstraintEngine.h>
#include <structure/StructureEngine.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cmath>

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
    StructureState state;
    Section verse;
    verse.name = "Verse";
    verse.startBar = 0;
    verse.lengthBars = 4;
    verse.progression = { { "C", "maj" }, { "F", "maj" }, { "G", "maj" }, { "C", "maj" } };
    state.sections.push_back (verse);

    StructureEngine engine (state);
    engine.rebuild();

    REQUIRE (state.totalBars == 4);
    REQUIRE (engine.getSectionAtBeat (2.0) != nullptr);
    CHECK (engine.getSectionAtBeat (2.0)->name == "Verse");
    CHECK (engine.getSectionAtBeat (20.0) == nullptr);

    const auto chordA = engine.getChordAtBeat (0.0);
    const auto chordB = engine.getChordAtBeat (4.1);  // bar 2
    CHECK (chordA.root == "C");
    CHECK (chordB.root == "F");
}

TEST_CASE ("StructureEngine transpose", "[structure][transpose]")
{
    StructureState state;
    Section section;
    section.name = "Hook";
    section.startBar = 0;
    section.lengthBars = 2;
    section.progression = { { "C", "maj" }, { "D#", "min" }, { "A", "maj" } };
    state.sections.push_back (section);

    StructureEngine engine (state);
    engine.transpose (2);

    REQUIRE (state.sections.size() == 1);
    REQUIRE (state.sections[0].progression.size() == 3);
    CHECK (state.sections[0].progression[0].root == "D");
    CHECK (state.sections[0].progression[1].root == "F");
    CHECK (state.sections[0].progression[2].root == "B");
}

TEST_CASE ("MidiConstraintEngine scale lock in C major", "[midi][constraint]")
{
    StructureState state;
    Section section;
    section.name = "Scale";
    section.startBar = 0;
    section.lengthBars = 4;
    section.progression = { { "C", "maj" } };
    state.sections.push_back (section);

    StructureEngine structureEngine (state);
    MidiConstraintEngine constraint;
    constraint.setStructure (&state);
    constraint.setStructureEngine (&structureEngine);
    constraint.setScaleLock (true);
    constraint.setChordLock (false);

    const int in = 61;   // C#
    const int out = constraint.processNote (in, 0.0);
    CHECK (out == 62);    // D
}

TEST_CASE ("MidiConstraintEngine chord lock in A minor", "[midi][constraint]")
{
    StructureState state;
    Section section;
    section.name = "Chord";
    section.startBar = 0;
    section.lengthBars = 4;
    section.progression = { { "A", "min" } };
    state.sections.push_back (section);

    StructureEngine structureEngine (state);
    MidiConstraintEngine constraint;
    constraint.setStructure (&state);
    constraint.setStructureEngine (&structureEngine);
    constraint.setScaleLock (false);
    constraint.setChordLock (true);

    const int in = 71; // B
    const int out = constraint.processNote (in, 0.0);
    CHECK ((out == 69 || out == 72)); // A or C
}

TEST_CASE ("MidiConstraintEngine guide mode preserves performance input", "[midi][constraint][guide]")
{
    StructureState state;
    Section section;
    section.name = "Guide";
    section.startBar = 0;
    section.lengthBars = 4;
    section.progression = { { "C", "maj" } };
    state.sections.push_back (section);

    StructureEngine structureEngine (state);
    MidiConstraintEngine constraint;
    constraint.setStructure (&state);
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
    StructureState state;

    Section secA;
    secA.name = "A";
    secA.startBar = 0;
    secA.lengthBars = 1;
    secA.progression = { { "C", "maj" } };
    state.sections.push_back (secA);

    Section secB;
    secB.name = "B";
    secB.startBar = 1;
    secB.lengthBars = 1;
    secB.progression = { { "D", "maj" } };
    state.sections.push_back (secB);

    StructureEngine structureEngine (state);
    MidiConstraintEngine constraint;
    constraint.setStructure (&state);
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

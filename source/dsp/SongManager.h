#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>
#include <array>
#include <memory>
#include <mutex>

struct SongPattern
{
    int id = -1;
    juce::String name;
    double lengthBeats = 16.0;
    juce::Array<int> baseSteps;
};

struct SongSection
{
    int id = -1;
    int patternId = 0;
    uint16_t slotMask = 1;
    double startBeat = 0.0;
    double lengthBeats = 4.0;
    int repeats = 1;
    int transpose = 0;
    double transition = 0.0;
};

struct ChordEvent
{
    int id = -1;
    double startBeat = 0.0;
    double durationBeats = 4.0;
    int root = 60;
    int type = 0;
};

struct ActiveSongState {
    int sectionId = -1;
    double localBeat = 0.0;
    int transposeTotal = 0;
    std::array<int, 8> activeSlots {};
    int numActiveSlots = 0;
    bool hasActiveChord = false;
    ChordEvent activeChord {};
};

class SongManager
{
public:
    SongManager();

    ActiveSongState query (double songBeat) const;

    int getNumPatterns() const;

    const juce::Array<SongPattern>& getPatterns() const;
    const juce::Array<SongSection>& getSections() const;
    const juce::Array<ChordEvent>& getChords() const;

    bool loadFromFile (const juce::File& file);
    bool saveToFile (const juce::File& file) const;

    int addPattern (const SongPattern& p);
    bool overwritePattern (int id, const SongPattern& p);
    bool removePattern (int id);
    int addSection (const SongSection& s);
    bool updateSection (int id, const SongSection& s);
    bool removeSection (int id);
    int addChord (const ChordEvent& c);
    bool updateChord (int id, const ChordEvent& c);
    bool removeChord (int id);

private:
    struct SongData
    {
        juce::Array<SongPattern> patterns;
        juce::Array<SongSection> sections;
        juce::Array<ChordEvent> chords;
        int globalTranspose = 0;
    };

    static int findSection (const SongData& data, double songBeat);
    static bool findFirstChord (const SongData& data, double songBeat, ChordEvent& outChord, double lookAhead = 0.0);
    void publishRtSnapshot();

    SongData m_editData;
    std::atomic<std::shared_ptr<const SongData>> m_rtData;
    mutable std::mutex m_editLock;
};


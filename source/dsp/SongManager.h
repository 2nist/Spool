#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <memory>
#include <mutex>
#include "../structure/StructureState.h"
#include "../state/LyricsState.h"
#include "../state/AutomationState.h"

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

    const juce::String& getSongTitle() const;
    int getTempo() const;
    int getNumerator() const;
    int getDenominator() const;
    const juce::String& getKeyRoot() const;
    const juce::String& getKeyScale() const;
    const juce::String& getNotes() const;
    const LyricsState& getLyricsState() const;
    const AutomationState& getAutomationState() const;

    void setSongTitle (const juce::String& title);
    void setTempo (int bpm);
    void setTimeSignature (int numerator, int denominator);
    void setKeyRoot (const juce::String& keyRoot);
    void setKeyScale (const juce::String& keyScale);
    void setNotes (const juce::String& notes);
    LyricsState& getLyricsStateForEdit();
    AutomationState& getAutomationStateForEdit();
    void resetToDefault();
    void replaceLyricsState (const LyricsState& lyrics);
    void replaceAutomationState (const AutomationState& automation);

    const StructureState& getStructureState() const;
    StructureState& getStructureStateForEdit();
    void commitAuthoredState();
    void replaceStructureState (const StructureState& structure);

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
        juce::String songTitle { "Untitled" };
        int bpm { 120 };
        int numerator { 4 };
        int denominator { 4 };
        juce::String keyRoot { "C" };
        juce::String keyScale { "Major" };
        juce::String notes;
        StructureState structure;
        LyricsState lyrics;
        AutomationState automation;
        juce::Array<SongPattern> patterns;
        juce::Array<SongSection> sections;
        juce::Array<ChordEvent> chords;
        int globalTranspose = 0;
    };

    static int findSection (const SongData& data, double songBeat);
    static bool findFirstChord (const SongData& data, double songBeat, ChordEvent& outChord, double lookAhead = 0.0);
    static SongData makeDefaultSongData();
    void publishRtSnapshot();

    SongData m_editData;
    std::shared_ptr<const SongData> m_rtData;
    mutable juce::SpinLock m_rtLock;
    mutable std::mutex m_editLock;
};

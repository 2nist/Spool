#include "SongManager.h"

#include <cmath>

namespace
{
int readInt (const juce::DynamicObject* obj, const juce::Identifier& key, int fallback = 0)
{
    if (obj == nullptr)
        return fallback;

    const auto value = obj->getProperty (key);
    return value.isVoid() ? fallback : static_cast<int> (value);
}

double readDouble (const juce::DynamicObject* obj, const juce::Identifier& key, double fallback = 0.0)
{
    if (obj == nullptr)
        return fallback;

    const auto value = obj->getProperty (key);
    return value.isVoid() ? fallback : static_cast<double> (value);
}

juce::String readString (const juce::DynamicObject* obj, const juce::Identifier& key, const juce::String& fallback = {})
{
    if (obj == nullptr)
        return fallback;

    const auto value = obj->getProperty (key);
    return value.isVoid() ? fallback : value.toString();
}
} // namespace

SongManager::SongManager()
{
    SongPattern test;
    test.name = "Test";
    test.lengthBeats = 4.0;
    test.baseSteps.add (0);

    m_editData.patterns.add (test);
    m_editData.patterns.getReference (0).id = 0;
    publishRtSnapshot();
}

void SongManager::publishRtSnapshot()
{
    auto snapshot = std::make_shared<SongData> (m_editData);
    m_rtData.store (std::static_pointer_cast<const SongData> (snapshot), std::memory_order_release);
}

bool SongManager::loadFromFile (const juce::File& file)
{
    const auto jsonText = file.loadFileAsString();
    const auto rootVar = juce::JSON::parse (jsonText);
    auto* rootObj = rootVar.getDynamicObject();
    if (rootObj == nullptr)
        return false;

    SongData next;

    const auto patternsVar = rootObj->getProperty ("patterns");
    if (auto* patterns = patternsVar.getArray())
    {
        for (const auto& pVar : *patterns)
        {
            auto* pObj = pVar.getDynamicObject();
            if (pObj == nullptr)
                continue;

            SongPattern pat;
            pat.id = readInt (pObj, "id", next.patterns.size());
            pat.name = readString (pObj, "name", "Pattern");
            pat.lengthBeats = readDouble (pObj, "lengthBeats", 16.0);

            if (auto* steps = pObj->getProperty ("baseSteps").getArray())
                for (const auto& stepVar : *steps)
                    pat.baseSteps.add (static_cast<int> (stepVar));

            next.patterns.add (pat);
        }
    }

    const auto sectionsVar = rootObj->getProperty ("sections");
    if (auto* sections = sectionsVar.getArray())
    {
        for (const auto& sVar : *sections)
        {
            auto* sObj = sVar.getDynamicObject();
            if (sObj == nullptr)
                continue;

            SongSection sec;
            sec.id = readInt (sObj, "id", next.sections.size());
            sec.patternId = readInt (sObj, "patternId", 0);
            sec.slotMask = static_cast<uint16_t> (readInt (sObj, "slotMask", 1));
            sec.startBeat = readDouble (sObj, "startBeat", 0.0);
            sec.lengthBeats = readDouble (sObj, "lengthBeats", 4.0);
            sec.repeats = readInt (sObj, "repeats", 1);
            sec.transpose = readInt (sObj, "transpose", 0);
            sec.transition = readDouble (sObj, "transition", 0.0);
            next.sections.add (sec);
        }
    }

    const auto chordsVar = rootObj->getProperty ("chords");
    if (auto* chords = chordsVar.getArray())
    {
        for (const auto& cVar : *chords)
        {
            auto* cObj = cVar.getDynamicObject();
            if (cObj == nullptr)
                continue;

            ChordEvent chord;
            chord.id = readInt (cObj, "id", next.chords.size());
            chord.startBeat = readDouble (cObj, "startBeat", 0.0);
            chord.durationBeats = readDouble (cObj, "durationBeats", 4.0);
            chord.root = readInt (cObj, "root", 60);
            chord.type = readInt (cObj, "type", 0);
            next.chords.add (chord);
        }
    }

    if (auto* metaObj = rootObj->getProperty ("meta").getDynamicObject())
        next.globalTranspose = readInt (metaObj, "globalTranspose", 0);

    {
        const std::lock_guard<std::mutex> lock (m_editLock);
        m_editData = std::move (next);
        publishRtSnapshot();
    }

    return true;
}

bool SongManager::saveToFile (const juce::File& file) const
{
    SongData data;
    {
        const std::lock_guard<std::mutex> lock (m_editLock);
        data = m_editData;
    }

    juce::DynamicObject::Ptr rootObj = new juce::DynamicObject();

    juce::Array<juce::var> patterns;
    patterns.ensureStorageAllocated (data.patterns.size());
    for (const auto& p : data.patterns)
    {
        juce::DynamicObject::Ptr pObj = new juce::DynamicObject();
        pObj->setProperty ("id", p.id);
        pObj->setProperty ("name", p.name);
        pObj->setProperty ("lengthBeats", p.lengthBeats);

        juce::Array<juce::var> steps;
        for (int s : p.baseSteps)
            steps.add (s);

        pObj->setProperty ("baseSteps", steps);
        patterns.add (juce::var (pObj.get()));
    }
    rootObj->setProperty ("patterns", patterns);

    juce::Array<juce::var> sections;
    sections.ensureStorageAllocated (data.sections.size());
    for (const auto& s : data.sections)
    {
        juce::DynamicObject::Ptr sObj = new juce::DynamicObject();
        sObj->setProperty ("id", s.id);
        sObj->setProperty ("patternId", s.patternId);
        sObj->setProperty ("slotMask", static_cast<int> (s.slotMask));
        sObj->setProperty ("startBeat", s.startBeat);
        sObj->setProperty ("lengthBeats", s.lengthBeats);
        sObj->setProperty ("repeats", s.repeats);
        sObj->setProperty ("transpose", s.transpose);
        sObj->setProperty ("transition", s.transition);
        sections.add (juce::var (sObj.get()));
    }
    rootObj->setProperty ("sections", sections);

    juce::Array<juce::var> chords;
    chords.ensureStorageAllocated (data.chords.size());
    for (const auto& c : data.chords)
    {
        juce::DynamicObject::Ptr cObj = new juce::DynamicObject();
        cObj->setProperty ("id", c.id);
        cObj->setProperty ("startBeat", c.startBeat);
        cObj->setProperty ("durationBeats", c.durationBeats);
        cObj->setProperty ("root", c.root);
        cObj->setProperty ("type", c.type);
        chords.add (juce::var (cObj.get()));
    }
    rootObj->setProperty ("chords", chords);

    juce::DynamicObject::Ptr metaObj = new juce::DynamicObject();
    metaObj->setProperty ("globalTranspose", data.globalTranspose);
    rootObj->setProperty ("meta", juce::var (metaObj.get()));

    const auto jsonText = juce::JSON::toString (juce::var (rootObj.get()), true);
    return file.replaceWithText (jsonText);
}

int SongManager::findSection (const SongData& data, double songBeat)
{
    for (int i = 0; i < data.sections.size(); ++i)
    {
        const auto& sec = data.sections.getReference (i);
        const auto duration = juce::jmax (0.0, sec.lengthBeats) * juce::jmax (1, sec.repeats);
        const auto endBeat = sec.startBeat + duration;
        if (songBeat >= sec.startBeat && songBeat < endBeat)
            return i;
    }

    return -1;
}

bool SongManager::findFirstChord (const SongData& data, double songBeat, ChordEvent& outChord, double lookAhead)
{
    const auto endBeat = songBeat + juce::jmax (0.0, lookAhead);

    for (const auto& chord : data.chords)
    {
        const auto chordEnd = chord.startBeat + juce::jmax (0.0, chord.durationBeats);
        if (chord.startBeat <= endBeat && chordEnd >= songBeat)
        {
            outChord = chord;
            return true;
        }
    }

    return false;
}

int SongManager::addPattern (const SongPattern& p)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    auto copy = p;
    copy.id = m_editData.patterns.size();
    m_editData.patterns.add (copy);
    publishRtSnapshot();
    return copy.id;
}

bool SongManager::overwritePattern (int id, const SongPattern& p)
{
    const std::lock_guard<std::mutex> lock (m_editLock);

    if (id < 0 || id >= m_editData.patterns.size())
        return false;

    auto copy = p;
    copy.id = id;
    m_editData.patterns.set (id, copy);
    publishRtSnapshot();
    return true;
}

bool SongManager::removePattern (int id)
{
    const std::lock_guard<std::mutex> lock (m_editLock);

    if (id < 0 || id >= m_editData.patterns.size())
        return false;

    m_editData.patterns.remove (id);
    for (int i = 0; i < m_editData.patterns.size(); ++i)
        m_editData.patterns.getReference (i).id = i;

    for (int i = 0; i < m_editData.sections.size(); ++i)
    {
        auto& sec = m_editData.sections.getReference (i);
        if (sec.patternId == id)
            sec.patternId = 0;
        else if (sec.patternId > id)
            --sec.patternId;
    }

    publishRtSnapshot();
    return true;
}

int SongManager::addSection (const SongSection& s)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    auto copy = s;
    copy.id = m_editData.sections.size();
    m_editData.sections.add (copy);
    publishRtSnapshot();
    return copy.id;
}

bool SongManager::updateSection (int id, const SongSection& s)
{
    const std::lock_guard<std::mutex> lock (m_editLock);

    if (id < 0 || id >= m_editData.sections.size())
        return false;

    auto copy = s;
    copy.id = id;
    m_editData.sections.set (id, copy);
    publishRtSnapshot();
    return true;
}

bool SongManager::removeSection (int id)
{
    const std::lock_guard<std::mutex> lock (m_editLock);

    if (id < 0 || id >= m_editData.sections.size())
        return false;

    m_editData.sections.remove (id);
    for (int i = 0; i < m_editData.sections.size(); ++i)
        m_editData.sections.getReference (i).id = i;

    publishRtSnapshot();
    return true;
}

int SongManager::addChord (const ChordEvent& c)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    auto copy = c;
    copy.id = m_editData.chords.size();
    m_editData.chords.add (copy);
    publishRtSnapshot();
    return copy.id;
}

bool SongManager::updateChord (int id, const ChordEvent& c)
{
    const std::lock_guard<std::mutex> lock (m_editLock);

    if (id < 0 || id >= m_editData.chords.size())
        return false;

    auto copy = c;
    copy.id = id;
    m_editData.chords.set (id, copy);
    publishRtSnapshot();
    return true;
}

bool SongManager::removeChord (int id)
{
    const std::lock_guard<std::mutex> lock (m_editLock);

    if (id < 0 || id >= m_editData.chords.size())
        return false;

    m_editData.chords.remove (id);
    for (int i = 0; i < m_editData.chords.size(); ++i)
        m_editData.chords.getReference (i).id = i;

    publishRtSnapshot();
    return true;
}

ActiveSongState SongManager::query (double songBeat) const
{
    auto data = m_rtData.load (std::memory_order_acquire);
    if (! data)
        return {};

    ActiveSongState state;
    state.sectionId = findSection (*data, songBeat);

    if (state.sectionId >= 0)
    {
        const auto& sec = data->sections.getReference (state.sectionId);
        const auto sectionLength = juce::jmax (0.001, sec.lengthBeats);
        state.localBeat = std::fmod (songBeat - sec.startBeat, sectionLength);

        for (int slot = 0; slot < 8; ++slot)
        {
            if ((sec.slotMask & static_cast<uint16_t> (1u << slot)) != 0)
                state.activeSlots[state.numActiveSlots++] = slot;
        }

        state.transposeTotal = data->globalTranspose + sec.transpose;
    }

    state.hasActiveChord = findFirstChord (*data, songBeat, state.activeChord);
    return state;
}

int SongManager::getNumPatterns() const
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    return m_editData.patterns.size();
}

const juce::Array<SongPattern>& SongManager::getPatterns() const
{
    return m_editData.patterns;
}

const juce::Array<SongSection>& SongManager::getSections() const
{
    return m_editData.sections;
}

const juce::Array<ChordEvent>& SongManager::getChords() const
{
    return m_editData.chords;
}

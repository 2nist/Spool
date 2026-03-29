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

juce::var chordToVar (const Chord& chord)
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("root", chord.root);
    obj->setProperty ("type", chord.type);
    obj->setProperty ("durationBeats", chord.durationBeats);
    return juce::var (obj.get());
}

Chord chordFromVar (const juce::var& value)
{
    Chord chord;
    if (auto* obj = value.getDynamicObject())
    {
        chord.root = readString (obj, "root");
        chord.type = readString (obj, "type");
        chord.durationBeats = juce::jmax (1, readInt (obj, "durationBeats", 4));
    }
    return chord;
}
} // namespace

SongManager::SongManager()
{
    m_editData = makeDefaultSongData();
    publishRtSnapshot();
}

const juce::String& SongManager::getSongTitle() const
{
    return m_editData.songTitle;
}

int SongManager::getTempo() const
{
    return m_editData.bpm;
}

int SongManager::getNumerator() const
{
    return m_editData.numerator;
}

int SongManager::getDenominator() const
{
    return m_editData.denominator;
}

const juce::String& SongManager::getKeyRoot() const
{
    return m_editData.keyRoot;
}

const juce::String& SongManager::getKeyScale() const
{
    return m_editData.keyScale;
}

const juce::String& SongManager::getNotes() const
{
    return m_editData.notes;
}

const LyricsState& SongManager::getLyricsState() const
{
    return m_editData.lyrics;
}

const AutomationState& SongManager::getAutomationState() const
{
    return m_editData.automation;
}

const juce::Array<TimelineClipPlacement>& SongManager::getTimelinePlacements() const
{
    return m_editData.timelinePlacements;
}

void SongManager::setSongTitle (const juce::String& title)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    const auto cleaned = title.trim();
    m_editData.songTitle = cleaned.isNotEmpty() ? cleaned : juce::String ("Untitled");
    publishRtSnapshot();
}

void SongManager::setTempo (int bpm)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData.bpm = juce::jlimit (20, 999, bpm);
    m_editData.structure.songTempo = m_editData.bpm;
    publishRtSnapshot();
}

void SongManager::setTimeSignature (int numerator, int denominator)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData.numerator = juce::jlimit (1, 32, numerator);
    m_editData.denominator = juce::jlimit (1, 32, denominator);
    publishRtSnapshot();
}

void SongManager::setKeyRoot (const juce::String& keyRoot)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData.keyRoot = keyRoot.trim().isNotEmpty() ? keyRoot.trim() : juce::String ("C");
    m_editData.structure.songKey = m_editData.keyRoot;
    publishRtSnapshot();
}

void SongManager::setKeyScale (const juce::String& keyScale)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData.keyScale = keyScale.trim().isNotEmpty() ? keyScale.trim() : juce::String ("Major");
    m_editData.structure.songMode = m_editData.keyScale;
    publishRtSnapshot();
}

void SongManager::setNotes (const juce::String& notes)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData.notes = notes;
    publishRtSnapshot();
}

LyricsState& SongManager::getLyricsStateForEdit()
{
    return m_editData.lyrics;
}

AutomationState& SongManager::getAutomationStateForEdit()
{
    return m_editData.automation;
}

void SongManager::resetToDefault()
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData = makeDefaultSongData();
    publishRtSnapshot();
}

void SongManager::replaceLyricsState (const LyricsState& lyrics)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData.lyrics = lyrics;
    publishRtSnapshot();
}

void SongManager::replaceAutomationState (const AutomationState& automation)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData.automation = automation;
    publishRtSnapshot();
}

void SongManager::addTimelinePlacement (const TimelineClipPlacement& placement)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    auto copy = placement;
    copy.laneIndex = juce::jlimit (0, 7, copy.laneIndex);
    copy.startBeat = juce::jmax (0.0f, copy.startBeat);
    copy.lengthBeats = juce::jmax (0.25f, copy.lengthBeats);
    copy.clipId = copy.clipId.trim();
    copy.audioAssetPath = copy.audioAssetPath.trim();
    if (copy.clipName.trim().isEmpty())
        copy.clipName = "CAPTURE";
    m_editData.timelinePlacements.add (copy);
    publishRtSnapshot();
}

void SongManager::clearTimelinePlacements()
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData.timelinePlacements.clear();
    publishRtSnapshot();
}

void SongManager::replaceTimelinePlacements (const juce::Array<TimelineClipPlacement>& placements)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData.timelinePlacements.clearQuick();
    for (const auto& placement : placements)
    {
        auto copy = placement;
        copy.laneIndex = juce::jlimit (0, 7, copy.laneIndex);
        copy.startBeat = juce::jmax (0.0f, copy.startBeat);
        copy.lengthBeats = juce::jmax (0.25f, copy.lengthBeats);
        copy.clipId = copy.clipId.trim();
        copy.audioAssetPath = copy.audioAssetPath.trim();
        if (copy.clipName.trim().isEmpty())
            copy.clipName = "CAPTURE";
        m_editData.timelinePlacements.add (copy);
    }
    publishRtSnapshot();
}

const StructureState& SongManager::getStructureState() const
{
    return m_editData.structure;
}

StructureState& SongManager::getStructureStateForEdit()
{
    return m_editData.structure;
}

void SongManager::commitAuthoredState()
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData.structure.songTempo = m_editData.bpm;
    m_editData.structure.songKey = m_editData.keyRoot;
    m_editData.structure.songMode = m_editData.keyScale;
    publishRtSnapshot();
}

void SongManager::replaceStructureState (const StructureState& structure)
{
    const std::lock_guard<std::mutex> lock (m_editLock);
    m_editData.structure = structure;
    m_editData.bpm = structure.songTempo;
    m_editData.keyRoot = structure.songKey.isNotEmpty() ? structure.songKey : juce::String ("C");
    m_editData.keyScale = structure.songMode.isNotEmpty() ? structure.songMode : juce::String ("Major");
    publishRtSnapshot();
}

void SongManager::publishRtSnapshot()
{
    auto snapshot = std::make_shared<SongData> (m_editData);
    auto constSnapshot = std::static_pointer_cast<const SongData> (snapshot);
    const juce::SpinLock::ScopedLockType sl (m_rtLock);
    m_rtData = std::move (constSnapshot);
}

bool SongManager::loadFromFile (const juce::File& file)
{
    const auto jsonText = file.loadFileAsString();
    const auto rootVar = juce::JSON::parse (jsonText);
    auto* rootObj = rootVar.getDynamicObject();
    if (rootObj == nullptr)
        return false;

    SongData next = makeDefaultSongData();

    if (auto* songObj = rootObj->getProperty ("song").getDynamicObject())
    {
        next.songTitle = readString (songObj, "title", next.songTitle);
        next.bpm = juce::jlimit (20, 999, readInt (songObj, "tempo", next.bpm));
        next.numerator = juce::jlimit (1, 32, readInt (songObj, "numerator", next.numerator));
        next.denominator = juce::jlimit (1, 32, readInt (songObj, "denominator", next.denominator));
        next.keyRoot = readString (songObj, "keyRoot", next.keyRoot);
        next.keyScale = readString (songObj, "keyScale", next.keyScale);
        next.notes = readString (songObj, "notes");
    }

    next.patterns.clearQuick();

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

    if (auto* structureObj = rootObj->getProperty ("structure").getDynamicObject())
    {
        StructureState structure;
        structure.songTempo = juce::jlimit (20, 999, readInt (structureObj, "songTempo", next.bpm));
        structure.songKey = readString (structureObj, "songKey", next.keyRoot);
        structure.songMode = readString (structureObj, "songMode", next.keyScale);
        structure.totalBars = juce::jmax (0, readInt (structureObj, "totalBars", 0));
        structure.totalBeats = juce::jmax (0, readInt (structureObj, "totalBeats", 0));
        structure.estimatedDurationSeconds = readDouble (structureObj, "estimatedDurationSeconds", 0.0);

        if (auto* sections = structureObj->getProperty ("sections").getArray())
        {
            for (const auto& sectionVar : *sections)
            {
                auto* sectionObj = sectionVar.getDynamicObject();
                if (sectionObj == nullptr)
                    continue;

                Section section;
                section.id = readString (sectionObj, "id", makeStructureId());
                section.name = readString (sectionObj, "name", "Section");
                section.bars = juce::jmax (1, readInt (sectionObj, "bars", 4));
                section.beatsPerBar = juce::jmax (1, readInt (sectionObj, "beatsPerBar", 4));
                section.repeats = juce::jmax (1, readInt (sectionObj, "repeats", 1));
                section.keyRoot = readString (sectionObj, "keyRoot");
                section.mode = readString (sectionObj, "mode");
                section.harmonicCenter = readString (sectionObj, "harmonicCenter", "I");
                section.transitionIntent = readString (sectionObj, "transitionIntent");

                if (auto* progression = sectionObj->getProperty ("progression").getArray())
                    for (const auto& chordVar : *progression)
                        section.progression.push_back (chordFromVar (chordVar));

                section.bars = computeSectionBarsFromProgression (section);

                structure.sections.push_back (section);
            }
        }

        if (auto* arrangement = structureObj->getProperty ("arrangement").getArray())
        {
            for (const auto& blockVar : *arrangement)
            {
                auto* blockObj = blockVar.getDynamicObject();
                if (blockObj == nullptr)
                    continue;

                ArrangementBlock block;
                block.id = readString (blockObj, "id", makeStructureId());
                block.sectionId = readString (blockObj, "sectionId");
                block.orderIndex = readInt (blockObj, "orderIndex", static_cast<int> (structure.arrangement.size()));
                block.barsOverride = readInt (blockObj, "barsOverride", 0);
                block.repeatsOverride = readInt (blockObj, "repeatsOverride", 0);
                block.transitionIntentOverride = readString (blockObj, "transitionIntentOverride");
                structure.arrangement.push_back (block);
            }
        }

        if (structure.totalBars <= 0)
            structure.totalBars = computeStructureTotalBars (structure);
        if (structure.totalBeats <= 0)
            structure.totalBeats = computeStructureTotalBeats (structure);
        if (structure.estimatedDurationSeconds <= 0.0)
            structure.estimatedDurationSeconds = computeStructureEstimatedDurationSeconds (structure);

        next.structure = structure;
        next.bpm = structure.songTempo;
        next.keyRoot = structure.songKey.isNotEmpty() ? structure.songKey : next.keyRoot;
        next.keyScale = structure.songMode.isNotEmpty() ? structure.songMode : next.keyScale;
    }
    else
    {
        next.structure.songTempo = next.bpm;
        next.structure.songKey = next.keyRoot;
        next.structure.songMode = next.keyScale;
    }

    if (auto* lyricsObj = rootObj->getProperty ("lyrics").getDynamicObject())
    {
        next.lyrics.items.clear();
        next.lyrics.nextId = juce::jmax (1, readInt (lyricsObj, "nextId", 1));
        if (auto* items = lyricsObj->getProperty ("items").getArray())
        {
            for (const auto& itemVar : *items)
            {
                auto* itemObj = itemVar.getDynamicObject();
                if (itemObj == nullptr)
                    continue;

                LyricItem item;
                item.id = readInt (itemObj, "id", next.lyrics.nextId++);
                item.text = readString (itemObj, "text");
                item.beatPosition = readDouble (itemObj, "beatPosition", -1.0);
                item.sectionId = readString (itemObj, "sectionId");
                item.pinnedToTimeline = readInt (itemObj, "pinnedToTimeline", item.beatPosition >= 0.0 ? 1 : 0) != 0;

                if (auto* styleObj = itemObj->getProperty ("style").getDynamicObject())
                {
                    item.style.fontName = readString (styleObj, "fontName", item.style.fontName);
                    item.style.sizeIndex = readInt (styleObj, "sizeIndex", item.style.sizeIndex);
                    const auto colourText = readString (styleObj, "colourArgb");
                    if (colourText.isNotEmpty())
                        item.style.colourArgb = static_cast<uint32_t> (colourText.getHexValue64());
                }

                next.lyrics.items.push_back (item);
                next.lyrics.nextId = juce::jmax (next.lyrics.nextId, item.id + 1);
            }
        }
    }

    if (auto* automationObj = rootObj->getProperty ("automation").getDynamicObject())
    {
        next.automation.lanes.clear();
        next.automation.nextId = juce::jmax (1, readInt (automationObj, "nextId", 1));
        if (auto* lanes = automationObj->getProperty ("lanes").getArray())
        {
            for (const auto& laneVar : *lanes)
            {
                auto* laneObj = laneVar.getDynamicObject();
                if (laneObj == nullptr)
                    continue;

                AutomationLane lane;
                lane.id = readInt (laneObj, "id", next.automation.nextId++);
                lane.targetId = readString (laneObj, "targetId");
                lane.displayName = readString (laneObj, "displayName");
                lane.scope = readString (laneObj, "scope");
                lane.enabled = readInt (laneObj, "enabled", 1) != 0;

                if (auto* points = laneObj->getProperty ("points").getArray())
                {
                    for (const auto& pointVar : *points)
                    {
                        auto* pointObj = pointVar.getDynamicObject();
                        if (pointObj == nullptr)
                            continue;

                        AutomationPoint point;
                        point.beat = readDouble (pointObj, "beat", 0.0);
                        point.value = static_cast<float> (readDouble (pointObj, "value", 0.0));
                        lane.points.push_back (point);
                    }
                }

                next.automation.lanes.push_back (lane);
                next.automation.nextId = juce::jmax (next.automation.nextId, lane.id + 1);
            }
        }
    }

    if (auto* timelineObj = rootObj->getProperty ("timeline").getDynamicObject())
    {
        next.timelinePlacements.clearQuick();
        if (auto* placements = timelineObj->getProperty ("placements").getArray())
        {
            for (const auto& placementVar : *placements)
            {
                auto* placementObj = placementVar.getDynamicObject();
                if (placementObj == nullptr)
                    continue;

                TimelineClipPlacement placement;
                placement.laneIndex = juce::jlimit (0, 7, readInt (placementObj, "laneIndex", 0));
                placement.moduleType = readString (placementObj, "moduleType");
                placement.clipName = readString (placementObj, "clipName", "CAPTURE");
                placement.clipId = readString (placementObj, "clipId");
                placement.audioAssetPath = readString (placementObj, "audioAssetPath");
                placement.startBeat = static_cast<float> (juce::jmax (0.0, readDouble (placementObj, "startBeat", 0.0)));
                placement.lengthBeats = static_cast<float> (juce::jmax (0.25, readDouble (placementObj, "lengthBeats", 0.25)));
                next.timelinePlacements.add (placement);
            }
        }
    }

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

    juce::DynamicObject::Ptr songObj = new juce::DynamicObject();
    songObj->setProperty ("title", data.songTitle);
    songObj->setProperty ("tempo", data.bpm);
    songObj->setProperty ("numerator", data.numerator);
    songObj->setProperty ("denominator", data.denominator);
    songObj->setProperty ("keyRoot", data.keyRoot);
    songObj->setProperty ("keyScale", data.keyScale);
    songObj->setProperty ("notes", data.notes);
    rootObj->setProperty ("song", juce::var (songObj.get()));

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

    juce::DynamicObject::Ptr structureObj = new juce::DynamicObject();
    structureObj->setProperty ("songTempo", data.structure.songTempo);
    structureObj->setProperty ("songKey", data.structure.songKey);
    structureObj->setProperty ("songMode", data.structure.songMode);
    structureObj->setProperty ("totalBars", data.structure.totalBars);
    structureObj->setProperty ("totalBeats", data.structure.totalBeats);
    structureObj->setProperty ("estimatedDurationSeconds", data.structure.estimatedDurationSeconds);

    juce::Array<juce::var> structureSections;
    for (const auto& section : data.structure.sections)
    {
        juce::DynamicObject::Ptr sectionObj = new juce::DynamicObject();
        sectionObj->setProperty ("id", section.id);
        sectionObj->setProperty ("name", section.name);
        sectionObj->setProperty ("bars", computeSectionBarsFromProgression (section));
        sectionObj->setProperty ("beatsPerBar", section.beatsPerBar);
        sectionObj->setProperty ("repeats", section.repeats);
        sectionObj->setProperty ("keyRoot", section.keyRoot);
        sectionObj->setProperty ("mode", section.mode);
        sectionObj->setProperty ("harmonicCenter", section.harmonicCenter);
        sectionObj->setProperty ("transitionIntent", section.transitionIntent);

        juce::Array<juce::var> progression;
        for (const auto& chord : section.progression)
            progression.add (chordToVar (chord));
        sectionObj->setProperty ("progression", progression);
        structureSections.add (juce::var (sectionObj.get()));
    }
    structureObj->setProperty ("sections", structureSections);

    juce::Array<juce::var> arrangement;
    for (const auto& block : data.structure.arrangement)
    {
        juce::DynamicObject::Ptr blockObj = new juce::DynamicObject();
        blockObj->setProperty ("id", block.id);
        blockObj->setProperty ("sectionId", block.sectionId);
        blockObj->setProperty ("orderIndex", block.orderIndex);
        blockObj->setProperty ("barsOverride", block.barsOverride);
        blockObj->setProperty ("repeatsOverride", block.repeatsOverride);
        blockObj->setProperty ("transitionIntentOverride", block.transitionIntentOverride);
        arrangement.add (juce::var (blockObj.get()));
    }
    structureObj->setProperty ("arrangement", arrangement);
    rootObj->setProperty ("structure", juce::var (structureObj.get()));

    juce::DynamicObject::Ptr lyricsObj = new juce::DynamicObject();
    lyricsObj->setProperty ("nextId", data.lyrics.nextId);
    juce::Array<juce::var> lyricItems;
    for (const auto& item : data.lyrics.items)
    {
        juce::DynamicObject::Ptr itemObj = new juce::DynamicObject();
        itemObj->setProperty ("id", item.id);
        itemObj->setProperty ("text", item.text);
        itemObj->setProperty ("beatPosition", item.beatPosition);
        itemObj->setProperty ("sectionId", item.sectionId);
        itemObj->setProperty ("pinnedToTimeline", item.pinnedToTimeline ? 1 : 0);

        juce::DynamicObject::Ptr styleObj = new juce::DynamicObject();
        styleObj->setProperty ("fontName", item.style.fontName);
        styleObj->setProperty ("sizeIndex", item.style.sizeIndex);
        styleObj->setProperty ("colourArgb", juce::String::toHexString (item.style.colourArgb).paddedLeft ('0', 8));
        itemObj->setProperty ("style", juce::var (styleObj.get()));
        lyricItems.add (juce::var (itemObj.get()));
    }
    lyricsObj->setProperty ("items", lyricItems);
    rootObj->setProperty ("lyrics", juce::var (lyricsObj.get()));

    juce::DynamicObject::Ptr automationObj = new juce::DynamicObject();
    automationObj->setProperty ("nextId", data.automation.nextId);
    juce::Array<juce::var> automationLanes;
    for (const auto& lane : data.automation.lanes)
    {
        juce::DynamicObject::Ptr laneObj = new juce::DynamicObject();
        laneObj->setProperty ("id", lane.id);
        laneObj->setProperty ("targetId", lane.targetId);
        laneObj->setProperty ("displayName", lane.displayName);
        laneObj->setProperty ("scope", lane.scope);
        laneObj->setProperty ("enabled", lane.enabled ? 1 : 0);

        juce::Array<juce::var> points;
        for (const auto& point : lane.points)
        {
            juce::DynamicObject::Ptr pointObj = new juce::DynamicObject();
            pointObj->setProperty ("beat", point.beat);
            pointObj->setProperty ("value", point.value);
            points.add (juce::var (pointObj.get()));
        }
        laneObj->setProperty ("points", points);
        automationLanes.add (juce::var (laneObj.get()));
    }
    automationObj->setProperty ("lanes", automationLanes);
    rootObj->setProperty ("automation", juce::var (automationObj.get()));

    juce::DynamicObject::Ptr timelineObj = new juce::DynamicObject();
    juce::Array<juce::var> timelinePlacements;
    for (const auto& placement : data.timelinePlacements)
    {
        juce::DynamicObject::Ptr placementObj = new juce::DynamicObject();
        placementObj->setProperty ("laneIndex", juce::jlimit (0, 7, placement.laneIndex));
        placementObj->setProperty ("moduleType", placement.moduleType);
        placementObj->setProperty ("clipName", placement.clipName);
        placementObj->setProperty ("clipId", placement.clipId);
        placementObj->setProperty ("audioAssetPath", placement.audioAssetPath);
        placementObj->setProperty ("startBeat", juce::jmax (0.0f, placement.startBeat));
        placementObj->setProperty ("lengthBeats", juce::jmax (0.25f, placement.lengthBeats));
        timelinePlacements.add (juce::var (placementObj.get()));
    }
    timelineObj->setProperty ("placements", timelinePlacements);
    rootObj->setProperty ("timeline", juce::var (timelineObj.get()));

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
    std::shared_ptr<const SongData> data;
    {
        const juce::SpinLock::ScopedLockType sl (m_rtLock);
        data = m_rtData;
    }
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

SongManager::SongData SongManager::makeDefaultSongData()
{
    SongData data;
    data.structure.songTempo = data.bpm;
    data.structure.songKey = data.keyRoot;
    data.structure.songMode = data.keyScale;

    SongPattern test;
    test.id = 0;
    test.name = "Test";
    test.lengthBeats = 4.0;
    test.baseSteps.add (0);
    data.patterns.add (test);
    return data;
}

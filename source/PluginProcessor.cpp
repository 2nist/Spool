#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <atomic>
#include <cmath>
#include <limits>

namespace
{
enum class CadenceTemplate
{
    leadingToneToRoot = 0,
    upperNeighborToTarget,
    chromaticPickup,
    fifthToRoot,
    twoFiveLike
};

double wrapBeatToStructure (double beat, const StructureState& structure)
{
    const double totalBeats = static_cast<double> (juce::jmax (0, structure.totalBeats));
    if (totalBeats <= 0.0)
        return beat;
    return std::fmod (juce::jmax (0.0, beat), totalBeats);
}

int snapToNearestPitchClass (const std::vector<int>& allowedPitchClasses, int inputNote)
{
    if (allowedPitchClasses.empty())
        return juce::jlimit (0, 127, inputNote);

    int best = juce::jlimit (0, 127, inputNote);
    int bestDistance = std::numeric_limits<int>::max();
    const int inputOct = inputNote / 12;

    for (const auto pc : allowedPitchClasses)
    {
        for (int oct = inputOct - 1; oct <= inputOct + 1; ++oct)
        {
            const int candidate = oct * 12 + pc;
            if (candidate < 0 || candidate > 127)
                continue;

            const int distance = std::abs (candidate - inputNote);
            if (distance < bestDistance || (distance == bestDistance && candidate > best))
            {
                bestDistance = distance;
                best = candidate;
            }
        }
    }

    return best;
}

int rootToPitchClass (const juce::String& root)
{
    const auto r = root.trim();
    if (r.equalsIgnoreCase ("C")) return 0;
    if (r.equalsIgnoreCase ("C#") || r.equalsIgnoreCase ("Db")) return 1;
    if (r.equalsIgnoreCase ("D")) return 2;
    if (r.equalsIgnoreCase ("D#") || r.equalsIgnoreCase ("Eb")) return 3;
    if (r.equalsIgnoreCase ("E")) return 4;
    if (r.equalsIgnoreCase ("F")) return 5;
    if (r.equalsIgnoreCase ("F#") || r.equalsIgnoreCase ("Gb")) return 6;
    if (r.equalsIgnoreCase ("G")) return 7;
    if (r.equalsIgnoreCase ("G#") || r.equalsIgnoreCase ("Ab")) return 8;
    if (r.equalsIgnoreCase ("A")) return 9;
    if (r.equalsIgnoreCase ("A#") || r.equalsIgnoreCase ("Bb")) return 10;
    if (r.equalsIgnoreCase ("B")) return 11;
    return 0;
}

int nearestNoteForPitchClass (int aroundNote, int targetPc)
{
    return snapToNearestPitchClass ({ (targetPc % 12 + 12) % 12 }, aroundNote);
}

int clampNoteToRangeByOctave (int note, int low, int high)
{
    int result = note;
    while (result < low)
        result += 12;
    while (result > high)
        result -= 12;

    if (result < low)
        result = low;
    if (result > high)
        result = high;
    return juce::jlimit (0, 127, result);
}

int roleRegisterCenter (SlotPattern::StepRole role, int baseNote)
{
    switch (role)
    {
        case SlotPattern::StepRole::bass:       return juce::jlimit (36, 52, baseNote - 12);
        case SlotPattern::StepRole::chord:      return juce::jlimit (48, 72, baseNote + 2);
        case SlotPattern::StepRole::lead:       return juce::jlimit (55, 84, baseNote + 12);
        case SlotPattern::StepRole::fill:       return juce::jlimit (52, 88, baseNote + 7);
        case SlotPattern::StepRole::transition: return juce::jlimit (40, 76, baseNote + 5);
    }

    return baseNote;
}

std::pair<int, int> roleRegisterRange (SlotPattern::StepRole role, int baseNote)
{
    switch (role)
    {
        case SlotPattern::StepRole::bass:       return { juce::jlimit (24, 48, baseNote - 24), juce::jlimit (36, 60, baseNote - 4) };
        case SlotPattern::StepRole::chord:      return { juce::jlimit (42, 60, baseNote - 8), juce::jlimit (60, 84, baseNote + 16) };
        case SlotPattern::StepRole::lead:       return { juce::jlimit (50, 72, baseNote + 2), juce::jlimit (67, 96, baseNote + 28) };
        case SlotPattern::StepRole::fill:       return { juce::jlimit (48, 70, baseNote - 2), juce::jlimit (72, 98, baseNote + 30) };
        case SlotPattern::StepRole::transition: return { juce::jlimit (36, 60, baseNote - 12), juce::jlimit (60, 88, baseNote + 20) };
    }

    return { 36, 84 };
}

int moveTowardTargetBySemitone (int current, int target)
{
    if (current == target)
        return target;
    return current < target ? target - 1 : target + 1;
}

int steerTowardPrevious (int candidate, int previous, int maxDistance)
{
    if (std::abs (candidate - previous) <= maxDistance)
        return candidate;

    return candidate > previous ? previous + maxDistance
                                : previous - maxDistance;
}

int chooseClosestNote (const std::vector<int>& candidates, int aroundNote)
{
    if (candidates.empty())
        return aroundNote;

    int best = candidates.front();
    int bestDistance = std::abs (best - aroundNote);
    for (const auto candidate : candidates)
    {
        const int distance = std::abs (candidate - aroundNote);
        if (distance < bestDistance)
        {
            best = candidate;
            bestDistance = distance;
        }
    }
    return best;
}

std::vector<int> chordIntervalsForType (const juce::String& type)
{
    auto lowerType = type.toLowerCase();
    if (lowerType.isEmpty())
        lowerType = "maj";

    if (lowerType == "min" || lowerType == "m")
        return { 0, 3, 7 };
    if (lowerType == "dim")
        return { 0, 3, 6 };
    if (lowerType == "aug")
        return { 0, 4, 8 };
    if (lowerType == "sus2")
        return { 0, 2, 7 };
    if (lowerType == "sus4")
        return { 0, 5, 7 };
    if (lowerType == "7" || lowerType == "dom7")
        return { 0, 4, 7, 10 };
    if (lowerType == "maj7")
        return { 0, 4, 7, 11 };
    if (lowerType == "min7" || lowerType == "m7")
        return { 0, 3, 7, 10 };

    return { 0, 4, 7 };
}

Chord nextChordForBeat (const StructureState& structure, double beat)
{
    for (const auto& instance : buildResolvedStructure (structure))
    {
        const auto start = static_cast<double> (instance.startBeat);
        const auto end = static_cast<double> (instance.startBeat + instance.totalBeats);
        if (beat < start || beat >= end || instance.section == nullptr)
            continue;

        if (instance.section->progression.empty())
            return { "C", "maj" };

        const auto perRepeatBeats = juce::jmax (1, instance.barsPerRepeat * instance.beatsPerBar);
        const auto localBeat = juce::jmax (0.0, beat - start);
        const auto beatInRepeat = std::fmod (localBeat, static_cast<double> (perRepeatBeats));
        const int slot = chordIndexForLoopBeat (*instance.section, beatInRepeat);
        if (slot < 0)
            return { "C", "maj" };
        const auto nextSlot = (slot + 1) % static_cast<int> (instance.section->progression.size());
        return instance.section->progression[(size_t) nextSlot];
    }

    return { "C", "maj" };
}
} // namespace

//==============================================================================
PluginProcessor::PluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
      m_structureEngine (m_songMgr)
{
    m_midiConstraintEngine.setStructure (&m_songMgr.getStructureState());
    m_midiConstraintEngine.setStructureEngine (&m_structureEngine);
    m_midiConstraintEngine.setScaleLock (true);
    m_midiConstraintEngine.setChordLock (false);
    m_midiConstraintEngine.setNotePolicy (MidiConstraintEngine::NotePolicy::Guide);

    m_appState.transport.bpm = static_cast<double> (m_bpm.load());
    m_appState.transport.isPlaying = m_playing.load();

    m_appState.slots.resize (SpoolAudioGraph::kNumSlots);
    for (int s = 0; s < SpoolAudioGraph::kNumSlots; ++s)
    {
        m_stepBits [s].store (0ULL);
        m_stepCount[s].store (16);
        m_slotNote [s].store (60);  // Default: middle C
        m_drumVoiceCount[s].store (0);
        m_lastNotePlayed[s] = 60;
        m_runtimePatterns[s] = compileRuntimePattern (SlotPattern {});
        resetPhraseMemory (s);

        auto& slotState = m_appState.slots[(size_t) s];
        slotState.slotIndex = s;
        slotState.name = "Slot " + juce::String (s + 1);

        for (int v = 0; v < kMaxDrumVoices; ++v)
        {
            for (int step = 0; step < kMaxDrumStepsPerVoice; ++step)
            {
                m_drumVoices[s][v].stepVelocitiesNorm[(size_t) step].store (100);
                m_drumVoices[s][v].stepRatchets[(size_t) step].store (1);
                m_drumVoices[s][v].stepDivisions[(size_t) step].store (4);
            }
        }
    }

    for (auto& mapped : m_midiNoteMap)
        mapped = -1;

    syncAuthoredSongToRuntime();
}

PluginProcessor::~PluginProcessor() {}

void PluginProcessor::resetPhraseMemory (int slot)
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots)
        return;

    auto& memory = m_phraseMemory[slot];
    memory = {};
    memory.previousNote = m_slotNote[slot].load();
    memory.recentLow = memory.previousNote;
    memory.recentHigh = memory.previousNote;
    memory.previousChordRootPc = 0;
    memory.previousRole = SlotPattern::StepRole::lead;
    memory.previousChordVoiceIndex = 0;
    memory.previousChordTopNote = memory.previousNote;
    memory.preferOpenChordVoicing = false;
    memory.cadenceTemplate = (int) CadenceTemplate::leadingToneToRoot;
    memory.cadenceEventCounter = 0;
}

void PluginProcessor::updatePhraseMemory (int slot, int realizedNote, SlotPattern::StepRole role, const Chord& currentChord)
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots)
        return;

    auto& memory = m_phraseMemory[slot];
    if (memory.hasPreviousNote)
    {
        const int delta = realizedNote - memory.previousNote;
        memory.previousDirection = delta > 0 ? 1 : delta < 0 ? -1 : memory.previousDirection;
        memory.recentLow = juce::jmin (memory.recentLow, realizedNote);
        memory.recentHigh = juce::jmax (memory.recentHigh, realizedNote);
    }
    else
    {
        memory.recentLow = realizedNote;
        memory.recentHigh = realizedNote;
    }

    const int chordRootPc = rootToPitchClass (currentChord.root);
    if (memory.previousChordRootPc != chordRootPc)
    {
        memory.notesSinceChordChange = 0;
        memory.cadenceEventCounter = 0;
        memory.previousChordTopNote = realizedNote;
    }
    else
    {
        memory.notesSinceChordChange += 1;
        memory.cadenceEventCounter += (role == SlotPattern::StepRole::transition) ? 1 : 0;
    }

    memory.previousChordRootPc = chordRootPc;
    memory.previousNote = realizedNote;
    memory.previousRole = role;
    memory.hasPreviousNote = true;
    if (role == SlotPattern::StepRole::chord)
        memory.previousChordTopNote = juce::jmax (memory.previousChordTopNote, realizedNote);
}

PluginProcessor::RuntimeSlotPattern PluginProcessor::compileRuntimePattern (const SlotPattern& pattern)
{
    RuntimeSlotPattern compiled;
    compiled.laneContext = pattern.current().laneContext;
    compiled.patternLengthTicks = juce::jmax (1, pattern.patternLength() * kSequencerTicksPerStep);

    for (int stepIndex = 0; stepIndex < pattern.activeStepCount(); ++stepIndex)
    {
        const auto* step = pattern.getStep (stepIndex);
        if (step == nullptr
            || step->gateMode == SlotPattern::GateMode::rest
            || step->microEventCount <= 0)
            continue;

        const int stepStartTick = step->start * kSequencerTicksPerStep;
        const int stepLengthTicks = juce::jmax (1, step->duration * kSequencerTicksPerStep);

        for (int eventIndex = 0; eventIndex < step->microEventCount; ++eventIndex)
        {
            if (compiled.eventCount >= kMaxSequencerEventsPerSlot)
                break;

            const auto& src = step->microEvents[(size_t) eventIndex];
            auto& dst = compiled.events[(size_t) compiled.eventCount++];
            dst.startTick = juce::jlimit (0,
                                          compiled.patternLengthTicks - 1,
                                          stepStartTick + juce::roundToInt (src.timeOffset * (float) stepLengthTicks));
            dst.lengthTicks = juce::jmax (1, juce::roundToInt (src.length * (float) stepLengthTicks));
            dst.pitchValue = juce::jlimit (SlotPattern::kMinTheoryValue, SlotPattern::kMaxTheoryValue, src.pitchValue);
            dst.anchorValue = juce::jlimit (SlotPattern::kUseSlotBasePitch, SlotPattern::kMaxTheoryValue, step->anchorValue);
            dst.stepIndex = stepIndex;
            dst.eventOrdinalInStep = eventIndex;
            dst.eventsInStep = step->microEventCount;
            dst.velocity = static_cast<uint8_t> (juce::jlimit (1, 127, (int) src.velocity));
            dst.noteMode = step->noteMode;
            dst.role = step->role;
            dst.harmonicSource = step->harmonicSource;
            dst.lookAheadToNextChord = step->lookAheadToNextChord;
        }
    }

    compiled.hasEvents = compiled.eventCount > 0;
    return compiled;
}

void PluginProcessor::releaseFinishedSlotNotes (int slot, int sampleOffset)
{
    bool anyHeld = false;

    for (auto& held : m_heldNotes[slot])
    {
        if (! held.active)
            continue;

        held.ticksRemaining -= 1;
        if (held.ticksRemaining <= 0)
        {
            m_slotMidi[slot].addEvent (juce::MidiMessage::noteOff (1, held.note), sampleOffset);
            held.active = false;
            held.note = -1;
            held.ticksRemaining = 0;
            continue;
        }

        anyHeld = true;
    }

    m_noteHeld[slot] = anyHeld;
}

void PluginProcessor::pushHeldNote (int slot, int note, int lengthTicks)
{
    auto* freeSlot = &m_heldNotes[slot][0];

    for (auto& held : m_heldNotes[slot])
    {
        if (held.active && held.note == note)
        {
            held.ticksRemaining = juce::jmax (1, lengthTicks);
            m_noteHeld[slot] = true;
            m_lastNotePlayed[slot] = note;
            return;
        }

        if (! held.active)
        {
            freeSlot = &held;
            break;
        }
    }

    freeSlot->active = true;
    freeSlot->note = note;
    freeSlot->ticksRemaining = juce::jmax (1, lengthTicks);
    m_noteHeld[slot] = true;
    m_lastNotePlayed[slot] = note;
}

int PluginProcessor::realizeRuntimePitch (int slot,
                                          const RuntimeMicroEvent& event,
                                          int baseNote,
                                          const juce::String& keyRoot,
                                          const juce::String& keyScale,
                                          const Chord& currentChord,
                                          const Chord& nextChord,
                                          bool structureFollow,
                                          const std::vector<int>& structureChordPcs)
{
    auto& memory = m_phraseMemory[slot];
    const int anchorNote = event.anchorValue < 0 ? baseNote : event.anchorValue;
    const auto [roleLow, roleHigh] = roleRegisterRange (event.role, baseNote);
    const int roleCenter = roleRegisterCenter (event.role, baseNote);
    const auto scalePcs = m_theoryAdapter.getScalePitchClasses (keyRoot, keyScale);
    const auto currentChordPcs = m_theoryAdapter.getChordPitchClasses (currentChord, &m_structureEngine);
    const auto nextChordPcs = m_theoryAdapter.getChordPitchClasses (nextChord, &m_structureEngine);
    const bool targetNextHarmony = event.harmonicSource == SlotPattern::HarmonicSource::nextChord || event.lookAheadToNextChord;
    const int currentRootPc = rootToPitchClass (currentChord.root);
    const int nextRootPc = rootToPitchClass (nextChord.root);

    auto chooseCadenceTemplate = [&] () -> CadenceTemplate
    {
        const int delta = (nextRootPc - currentRootPc + 12) % 12;
        if (delta == 7)
            return CadenceTemplate::fifthToRoot;
        if (delta == 2 || delta == 10)
            return CadenceTemplate::twoFiveLike;
        if (delta == 1 || delta == 11)
            return CadenceTemplate::chromaticPickup;
        if (delta == 0)
            return CadenceTemplate::upperNeighborToTarget;
        return CadenceTemplate::leadingToneToRoot;
    };

    if (event.role == SlotPattern::StepRole::transition)
        memory.cadenceTemplate = (int) chooseCadenceTemplate();

    auto realizeCadenceTemplate = [&] (int seedNote) -> int
    {
        const auto templateId = static_cast<CadenceTemplate> (memory.cadenceTemplate);
        const int targetRoot = nearestNoteForPitchClass (roleCenter, nextRootPc);
        const int cadenceIndex = juce::jmax (0, event.eventOrdinalInStep);

        switch (templateId)
        {
            case CadenceTemplate::leadingToneToRoot:
                return cadenceIndex == juce::jmax (0, event.eventsInStep - 1) ? targetRoot : targetRoot - 1;

            case CadenceTemplate::upperNeighborToTarget:
                if (cadenceIndex == 0) return targetRoot + 2;
                return targetRoot;

            case CadenceTemplate::chromaticPickup:
                if (cadenceIndex == 0) return targetRoot - 2;
                if (cadenceIndex == 1 && event.eventsInStep > 2) return targetRoot - 1;
                return targetRoot;

            case CadenceTemplate::fifthToRoot:
                if (cadenceIndex == 0) return nearestNoteForPitchClass (seedNote, (nextRootPc + 7) % 12);
                return targetRoot;

            case CadenceTemplate::twoFiveLike:
                if (cadenceIndex == 0) return nearestNoteForPitchClass (seedNote, (nextRootPc + 2) % 12);
                if (cadenceIndex == 1 && event.eventsInStep > 2) return nearestNoteForPitchClass (seedNote, (nextRootPc + 7) % 12);
                return targetRoot;
        }

        return targetRoot;
    };

    auto roleAdjust = [&] (int note) -> int
    {
        int adjusted = note;
        const int previousNote = memory.hasPreviousNote ? memory.previousNote : roleCenter;

        switch (event.role)
        {
            case SlotPattern::StepRole::bass:
            {
                const std::vector<int> bassTargets = targetNextHarmony && ! nextChordPcs.empty()
                                                   ? std::vector<int> { nextChordPcs.front(), nextChordPcs.size() > 2 ? nextChordPcs[2] : nextChordPcs.front() }
                                                   : std::vector<int> { currentChordPcs.empty() ? rootToPitchClass (currentChord.root) : currentChordPcs.front(),
                                                                        currentChordPcs.size() > 2 ? currentChordPcs[2] : ((currentChordPcs.empty() ? rootToPitchClass (currentChord.root) : currentChordPcs.front()) + 7) % 12 };
                adjusted = snapToNearestPitchClass (bassTargets,
                                                   clampNoteToRangeByOctave (memory.hasPreviousNote ? previousNote : adjusted,
                                                                             roleLow,
                                                                             roleHigh));
                if (targetNextHarmony && ! nextChordPcs.empty() && memory.hasPreviousNote)
                    adjusted = moveTowardTargetBySemitone (adjusted, nearestNoteForPitchClass (adjusted, nextChordPcs.front()));
                adjusted = clampNoteToRangeByOctave (adjusted, roleLow, roleHigh);
                break;
            }

            case SlotPattern::StepRole::chord:
            {
                adjusted = clampNoteToRangeByOctave (memory.hasPreviousNote ? steerTowardPrevious (adjusted, previousNote, 5) : adjusted,
                                                     juce::jmax (46, roleLow),
                                                     roleHigh);
                const int closeTop = juce::jlimit (roleCenter + 3, roleHigh - 2, memory.previousChordTopNote);
                if (memory.preferOpenChordVoicing && adjusted < roleCenter)
                    adjusted = juce::jmin (roleHigh, adjusted + 12);
                if (! memory.preferOpenChordVoicing && adjusted > closeTop)
                    adjusted = juce::jmax (juce::jmax (46, roleLow), adjusted - 12);
                if (adjusted < roleCenter - 5)
                    adjusted += 12;
                break;
            }

            case SlotPattern::StepRole::lead:
                adjusted = clampNoteToRangeByOctave (memory.hasPreviousNote ? steerTowardPrevious (adjusted, previousNote, 4) : adjusted,
                                                     roleLow,
                                                     roleHigh);
                if (memory.hasPreviousNote && memory.previousDirection != 0)
                    adjusted = steerTowardPrevious (adjusted, previousNote + memory.previousDirection * 2, 5);
                if (currentChordPcs.size() >= 3 && std::find (currentChordPcs.begin(), currentChordPcs.end(), adjusted % 12) != currentChordPcs.end())
                    adjusted += ((event.pitchValue >= 0 && (event.pitchValue % 2) == 1) ? 2 : 0);
                break;

            case SlotPattern::StepRole::fill:
                adjusted = clampNoteToRangeByOctave (memory.hasPreviousNote ? steerTowardPrevious (adjusted, previousNote + memory.previousDirection * 4, 9) : adjusted,
                                                     roleLow,
                                                     roleHigh);
                adjusted = clampNoteToRangeByOctave (adjusted + ((event.pitchValue >= 0 && (event.pitchValue % 2) == 1) ? 12 : 0),
                                                     roleLow,
                                                     roleHigh);
                if (targetNextHarmony && ! nextChordPcs.empty())
                    adjusted = snapToNearestPitchClass (nextChordPcs, adjusted);
                break;

            case SlotPattern::StepRole::transition:
            {
                adjusted = clampNoteToRangeByOctave (realizeCadenceTemplate (memory.hasPreviousNote ? previousNote : adjusted),
                                                     roleLow,
                                                     roleHigh);
                break;
            }
        }

        return juce::jlimit (0, 127, adjusted);
    };

    switch (event.noteMode)
    {
        case SlotPattern::NoteMode::absolute:
        {
            int note = event.pitchValue < 0 ? baseNote : event.pitchValue;
            if (structureFollow && ! structureChordPcs.empty())
                note = snapToNearestPitchClass (structureChordPcs, note);
            return juce::jlimit (0, 127, note);
        }

        case SlotPattern::NoteMode::degree:
        {
            const int raw = event.pitchValue < 0 ? 0 : event.pitchValue;
            const int octaveOffset = raw / 12;
            const int degreePc = (rootToPitchClass (keyRoot) + (raw % 12 + 12) % 12) % 12;
            const int contourCenter = memory.hasPreviousNote ? memory.previousNote + memory.previousDirection * 2 : anchorNote;
            const int center = event.role == SlotPattern::StepRole::bass ? roleCenter
                             : event.role == SlotPattern::StepRole::lead ? contourCenter + octaveOffset * 12
                             : anchorNote + octaveOffset * 12;
            int note = nearestNoteForPitchClass (center, degreePc);
            if (! scalePcs.empty() && std::find (scalePcs.begin(), scalePcs.end(), degreePc) == scalePcs.end())
                note = nearestNoteForPitchClass (note, degreePc);
            return roleAdjust (note);
        }

        case SlotPattern::NoteMode::chordTone:
        {
            const auto& chordForEvent = (event.harmonicSource == SlotPattern::HarmonicSource::nextChord || event.lookAheadToNextChord)
                                      ? nextChord
                                      : currentChord;
            const auto intervals = chordIntervalsForType (chordForEvent.type);
            int toneIndex = juce::jmax (0, event.pitchValue < 0 ? 0 : event.pitchValue);

            if (event.role == SlotPattern::StepRole::bass)
            {
                static constexpr int bassChordToneMap[4] = { 0, 2, 0, 1 };
                const int mapped = bassChordToneMap[toneIndex % 4];
                toneIndex = mapped + ((toneIndex / 4) * 2);
            }
            else if (event.role == SlotPattern::StepRole::lead)
            {
                toneIndex = toneIndex % juce::jmax (1, (int) intervals.size());
            }
            else if (event.role == SlotPattern::StepRole::fill)
            {
                toneIndex += 1;
            }
            else if (event.role == SlotPattern::StepRole::chord)
            {
                const int chordSize = juce::jmax (1, (int) intervals.size());
                const int continuityIndex = memory.hasPreviousNote ? memory.previousChordVoiceIndex : toneIndex % chordSize;
                toneIndex = memory.preferOpenChordVoicing
                          ? continuityIndex + ((toneIndex % 2) * chordSize)
                          : continuityIndex;
            }
            else if (event.role == SlotPattern::StepRole::transition)
            {
                memory.cadenceTemplate = (int) chooseCadenceTemplate();
            }

            int octaveOffset = toneIndex / juce::jmax (1, (int) intervals.size());
            int intervalIndex = toneIndex % juce::jmax (1, (int) intervals.size());
            int rootAnchor = event.role == SlotPattern::StepRole::chord ? roleCenter
                          : event.role == SlotPattern::StepRole::lead && memory.hasPreviousNote ? memory.previousNote
                          : anchorNote;
            int rootNote = nearestNoteForPitchClass (rootAnchor,
                                                     rootToPitchClass (chordForEvent.root));
            int note = rootNote + intervals[(size_t) intervalIndex] + octaveOffset * 12;
            if (event.role == SlotPattern::StepRole::chord)
            {
                memory.previousChordVoiceIndex = intervalIndex;
                memory.preferOpenChordVoicing = event.eventsInStep >= 3 || memory.recentHigh - memory.recentLow > 9;
            }
            return roleAdjust (note);
        }

        case SlotPattern::NoteMode::interval:
        default:
        {
            int semitoneOffset = event.pitchValue < 0 ? 0 : event.pitchValue;
            if (event.role == SlotPattern::StepRole::fill)
                semitoneOffset += ((semitoneOffset & 1) ? 12 : 0);
            if (event.role == SlotPattern::StepRole::bass)
                semitoneOffset = juce::jlimit (-12, 12, semitoneOffset);

            return roleAdjust (anchorNote + semitoneOffset);
        }
    }
}

void PluginProcessor::triggerRuntimePatternEvents (int slot,
                                                   int patternTick,
                                                   int sampleOffset,
                                                   bool structureFollow,
                                                   const juce::String& keyRoot,
                                                   const juce::String& keyScale,
                                                   const Chord& currentChord,
                                                   const Chord& nextChord,
                                                   const std::vector<int>& structureChordPcs)
{
    juce::SpinLock::ScopedTryLockType lock (m_runtimePatternLocks[slot]);
    if (! lock.isLocked())
        return;

    const auto pattern = m_runtimePatterns[slot];
    if (! pattern.hasEvents || pattern.patternLengthTicks <= 0)
        return;

    for (int i = 0; i < pattern.eventCount; ++i)
    {
        const auto& event = pattern.events[(size_t) i];
        if (event.startTick != patternTick)
            continue;

        const int baseNote = m_slotNote[slot].load();
        const int note = realizeRuntimePitch (slot, event, baseNote, keyRoot, keyScale, currentChord, nextChord, structureFollow, structureChordPcs);

        for (auto& held : m_heldNotes[slot])
        {
            if (held.active && held.note == note)
            {
                m_slotMidi[slot].addEvent (juce::MidiMessage::noteOff (1, held.note), sampleOffset);
                held.active = false;
                held.note = -1;
                held.ticksRemaining = 0;
            }
        }

        m_slotMidi[slot].addEvent (
            juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (event.velocity)),
            sampleOffset);
        pushHeldNote (slot, note, event.lengthTicks);
        updatePhraseMemory (slot, note, event.role, currentChord);
    }
}

//==============================================================================
const juce::String PluginProcessor::getName() const  { return JucePlugin_Name; }

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const { return 2.0; }

int  PluginProcessor::getNumPrograms()                             { return 1;  }
int  PluginProcessor::getCurrentProgram()                          { return 0;  }
void PluginProcessor::setCurrentProgram (int)                      {}
const juce::String PluginProcessor::getProgramName (int)           { return {}; }
void PluginProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    m_sampleRate = sampleRate;
    m_samplePos  = 0.0;

    m_audioGraph.prepare (sampleRate, samplesPerBlock);

    m_circularBuffer.prepare (sampleRate, 120.0);
    m_circularBuffer.setSource (-1);   // master mix by default
    m_currentBeat = 0.0;
    clearLooperPreview();

    for (auto& mb : m_slotMidi)
        mb.clear();

    for (int slot = 0; slot < SpoolAudioGraph::kNumSlots; ++slot)
        resetPhraseMemory (slot);

    DBG ("PluginProcessor: prepared at " << sampleRate
         << " Hz, " << samplesPerBlock << " buf");
}

void PluginProcessor::releaseResources()
{
    m_audioGraph.reset();
    clearLooperPreview();
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
  #endif
}

//==============================================================================
void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer&          midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    // Constraint pass: incoming MIDI -> structure-aware constrained notes
    juce::MidiBuffer constrainedIncoming;
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        const auto samplePos = metadata.samplePosition;

        if (msg.isNoteOn())
        {
            const int inputNote = msg.getNoteNumber();
            const double rawBeat = m_appState.transport.playheadBeats;
            Chord activeChord { "C", "maj" };
            int constrained = inputNote;
            {
                const juce::ScopedReadLock structureRead (m_structureLock);
                const double structureBeat = wrapBeatToStructure (rawBeat, m_appState.structure);
                activeChord = m_structureEngine.getChordAtBeat (structureBeat);
                constrained = m_midiConstraintEngine.processNote (inputNote, structureBeat);
            }
            m_midiNoteMap[inputNote] = constrained;
            msg = juce::MidiMessage::noteOn (msg.getChannel(), constrained, msg.getFloatVelocity());

            DBG ("Constraint beat=" << rawBeat
                 << " chord=" << activeChord.root << activeChord.type
                 << " in=" << inputNote
                 << " out=" << constrained);
        }
        else if (msg.isNoteOff())
        {
            const int inputNote = msg.getNoteNumber();
            const int mapped = (inputNote >= 0 && inputNote < 128 && m_midiNoteMap[inputNote] >= 0)
                                 ? m_midiNoteMap[inputNote]
                                 : inputNote;
            msg = juce::MidiMessage::noteOff (msg.getChannel(), mapped, msg.getFloatVelocity());
            if (inputNote >= 0 && inputNote < 128)
                m_midiNoteMap[inputNote] = -1;
        }

        constrainedIncoming.addEvent (msg, samplePos);
    }
    midiMessages.swapWith (constrainedIncoming);

    // Clear per-slot MIDI buffers (pre-allocated — no allocation)
    for (auto& mb : m_slotMidi)
        mb.clear();

    // Flush any held notes when transport is paused/stopped
    if (m_flushAllNotes.exchange (false))
    {
        for (int slot = 0; slot < SpoolAudioGraph::kNumSlots; ++slot)
        {
            for (auto& held : m_heldNotes[slot])
            {
                if (! held.active)
                    continue;

                m_slotMidi[slot].addEvent (juce::MidiMessage::noteOff (1, held.note), 0);
                held.active = false;
                held.note = -1;
                held.ticksRemaining = 0;
            }

            m_noteHeld[slot] = false;
        }
    }

    // Route incoming MIDI to the correct slot
    m_midiRouter.route (midiMessages, m_slotMidi, SpoolAudioGraph::kNumSlots);

    // ── Song clock ───────────────────────────────────────────────────────────
    if (m_playing.load())
    {
        const double bpm            = static_cast<double> (m_bpm.load());
        const double bps            = bpm / 60.0;
        const double samplesPerBeat = m_sampleRate / bps;
        const double beatDelta      = static_cast<double> (numSamples) / samplesPerBeat;
        
        m_currentBeat += beatDelta;
        m_currentSongBeat += beatDelta;
        m_appState.transport.playheadBeats = m_currentSongBeat;

        // Minimal integration point: transport can query structure context.
        std::optional<ResolvedSectionInstance> activeSection;
        Chord activeChord { "C", "maj" };
        {
            const juce::ScopedReadLock structureRead (m_structureLock);
            const double structureBeat = wrapBeatToStructure (m_currentSongBeat, m_appState.structure);
            activeSection = m_structureEngine.getSectionAtBeat (structureBeat);
            activeChord = m_structureEngine.getChordAtBeat (structureBeat);
        }
        juce::ignoreUnused (activeSection, activeChord);

        // Per-sample tick advance so micro-events can land inside each step.
        const double samplesPerTick = samplesPerBeat / static_cast<double> (kSequencerTicksPerBeat);
        for (int i = 0; i < numSamples; ++i)
        {
            m_samplePos += 1.0;
            if (m_samplePos >= samplesPerTick)
            {
                m_samplePos -= samplesPerTick;
                advanceSequencerTick (i);
            }
        }
    }

    // ── Audio graph ──────────────────────────────────────────────────────────
    m_audioGraph.process (buffer, m_slotMidi);

    // ── Master gain + pan ────────────────────────────────────────────────────
    const float gain = m_masterGain.load();
    const float pan  = m_masterPan.load();
    constexpr float pi = 3.14159265358979323846f;
    const float angle      = ((pan + 1.0f) * 0.5f) * (pi * 0.5f);
    const float leftGain   = gain * std::cos (angle);
    const float rightGain  = gain * std::sin (angle);

    const int numCh = juce::jmin (getTotalNumOutputChannels(), buffer.getNumChannels());
    if (numCh > 0) buffer.applyGain (0, 0, numSamples, leftGain);
    if (numCh > 1) buffer.applyGain (1, 0, numSamples, rightGain);

    // ── Circular buffer — LAST operation before looper audition ───────────────
    if (m_circularBuffer.isActive())
    {
        const int srcSlot = m_circularBuffer.getSource();
        if (srcSlot == -1)
        {
            // Master mix
            m_circularBuffer.writeBlock (buffer, numSamples);
        }
        else if (srcSlot >= 0 && srcSlot < SpoolAudioGraph::kNumSlots
                 && m_audioGraph.isSlotActive (srcSlot))
        {
            m_circularBuffer.writeBlock (m_audioGraph.getSlotBuffer (srcSlot), numSamples);
        }
    }

    // ── Looper preview audition — post-history so tape capture stays clean ───
    if (m_looperPreviewPlaying.load (std::memory_order_relaxed))
    {
        juce::SpinLock::ScopedTryLockType previewLock (m_looperPreviewLock);
        if (previewLock.isLocked() && m_looperPreviewBuffer.getNumSamples() > 0)
        {
            const int previewSamples = m_looperPreviewBuffer.getNumSamples();
            const int previewChannels = m_looperPreviewBuffer.getNumChannels();
            const double ratio = m_sampleRate > 0.0 ? (m_looperPreviewSourceRate / m_sampleRate) : 1.0;

            for (int sample = 0; sample < numSamples; ++sample)
            {
                if (m_looperPreviewReadPos >= static_cast<double> (previewSamples))
                {
                    if (m_looperPreviewLoopingFlag)
                        m_looperPreviewReadPos = std::fmod (m_looperPreviewReadPos, static_cast<double> (previewSamples));
                    else
                    {
                        m_looperPreviewPlaying.store (false, std::memory_order_relaxed);
                        m_looperPreviewProgress.store (1.0f, std::memory_order_relaxed);
                        break;
                    }
                }

                const int indexA = juce::jlimit (0, previewSamples - 1, static_cast<int> (m_looperPreviewReadPos));
                const int indexB = juce::jlimit (0, previewSamples - 1, indexA + 1);
                const float frac = static_cast<float> (m_looperPreviewReadPos - static_cast<double> (indexA));

                for (int ch = 0; ch < numCh; ++ch)
                {
                    const float* src = m_looperPreviewBuffer.getReadPointer (juce::jmin (ch, previewChannels - 1));
                    const float previewSample = juce::jmap (frac, src[indexA], src[indexB]);
                    buffer.addSample (ch, sample, previewSample * 0.9f);
                }

                m_looperPreviewReadPos += ratio;
            }

            const float progress = previewSamples > 0 ? (float) (m_looperPreviewReadPos / (double) previewSamples) : 0.0f;
            m_looperPreviewProgress.store (juce::jlimit (0.0f, 1.0f, progress), std::memory_order_relaxed);
        }
    }
}

//==============================================================================
void PluginProcessor::advanceSequencerTick (int sampleOffset)
{
    bool structureActive = false;
    double structureBeat = 0.0;
    std::vector<int> structureChordPcs;
    Chord currentChord { "C", "maj" };
    Chord nextChord { "C", "maj" };
    {
        const juce::ScopedReadLock structureRead (m_structureLock);
        structureActive = structureHasScaffold (m_appState.structure);
        if (structureActive)
        {
            structureBeat = wrapBeatToStructure (m_currentSongBeat, m_appState.structure);
            currentChord = m_structureEngine.getChordAtBeat (structureBeat);
            nextChord = nextChordForBeat (m_appState.structure, structureBeat);
            structureChordPcs = m_structureEngine.resolveChordAtBeat (structureBeat);
        }
    }

    for (int slot = 0; slot < SpoolAudioGraph::kNumSlots; ++slot)
        releaseFinishedSlotNotes (slot, sampleOffset);

    const double activeBeat = structureActive ? structureBeat : m_currentSongBeat;
    const int activeTick = juce::jmax (0, static_cast<int> (std::floor (activeBeat * (double) kSequencerTicksPerBeat + 1.0e-6)));
    const int step16 = activeTick / kSequencerTicksPerStep;

    // Drum machine slots: check every sequencer tick so ratchets/divisions can retrigger within the step.
    {
        const int stepSubTick = activeTick % kSequencerTicksPerStep;
        for (int slot = 0; slot < SpoolAudioGraph::kNumSlots; ++slot)
        {
            if (! m_audioGraph.isSlotActive (slot)) continue;
            if (m_audioGraph.getSlotSynthType (slot) != 2) continue;

            const int voiceCount = m_drumVoiceCount[slot].load (std::memory_order_relaxed);
            for (int v = 0; v < voiceCount; ++v)
            {
                const int vsc = juce::jmax (1, m_drumVoices[slot][v].stepCount.load (std::memory_order_relaxed));
                const int voiceStep = step16 % vsc;
                const uint64_t vbit = 1ULL << juce::jlimit (0, 63, voiceStep);
                if ((m_drumVoices[slot][v].stepBits.load (std::memory_order_relaxed) & vbit) == 0)
                    continue;

                const int division = juce::jlimit (1, 8, m_drumVoices[slot][v].stepDivisions[(size_t) voiceStep].load (std::memory_order_relaxed));
                const int ratchetCount = juce::jlimit (1,
                                                       division,
                                                       m_drumVoices[slot][v].stepRatchets[(size_t) voiceStep].load (std::memory_order_relaxed));
                const int velocityNorm = juce::jlimit (5, 100, m_drumVoices[slot][v].stepVelocitiesNorm[(size_t) voiceStep].load (std::memory_order_relaxed));
                const int intervalTicks = juce::jmax (1, kSequencerTicksPerStep / division);

                for (int repeat = 0; repeat < ratchetCount; ++repeat)
                {
                    const int triggerTick = repeat * intervalTicks;
                    if (stepSubTick == triggerTick)
                    {
                        const float velocity = juce::jlimit (0.05f, 1.0f, velocityNorm / 100.0f);
                        m_audioGraph.getDrumMachine (slot).triggerVoice (v, velocity);
                        break;
                    }
                }
            }
        }
    }

    const bool structureFollow = m_useStructureForTracks.load() && structureActive && ! structureChordPcs.empty();

    // Hybrid slot patterns: variable-length steps with micro note events.
    for (int slot = 0; slot < SpoolAudioGraph::kNumSlots; ++slot)
    {
        if (! m_audioGraph.isSlotActive (slot)) continue;
        if (m_audioGraph.getSlotSynthType (slot) == 2) continue;

        const int patternLengthTicks = [&]
        {
            juce::SpinLock::ScopedTryLockType lock (m_runtimePatternLocks[slot]);
            return lock.isLocked() ? m_runtimePatterns[slot].patternLengthTicks : 0;
        }();

        if (patternLengthTicks <= 0)
            continue;

        triggerRuntimePatternEvents (slot,
                                     activeTick % patternLengthTicks,
                                     sampleOffset,
                                     structureFollow,
                                     m_appState.song.keyRoot,
                                     m_appState.song.keyScale,
                                     currentChord,
                                     nextChord,
                                     structureChordPcs);
    }

    auto state = m_songMgr.query (m_currentSongBeat);
    if (state.sectionId < 0)
    {
        if ((activeTick % kSequencerTicksPerStep) == 0)
        {
            m_currentStep.store (step16 % 64);
            triggerAsyncUpdate();
        }
        return;
    }

    const auto& sec = m_songMgr.getSections()[state.sectionId];
    const auto& pat = m_songMgr.getPatterns()[sec.patternId];

    if ((activeTick % kSequencerTicksPerStep) != 0)
        return;

    const int totalPatternSteps = juce::jmax (1, static_cast<int> (pat.lengthBeats * 4.0));
    const int step = static_cast<int> (state.localBeat * 4.0) % totalPatternSteps;

    // Dispatch to active slots
    for (int i = 0; i < state.numActiveSlots; ++i)
    {
        const int slot = state.activeSlots[i];
        if (! m_audioGraph.isSlotActive (slot)) continue;
        if (m_audioGraph.getSlotSynthType (slot) == 2) continue;

        {
            juce::SpinLock::ScopedTryLockType lock (m_runtimePatternLocks[slot]);
            if (lock.isLocked() && m_runtimePatterns[slot].hasEvents)
                continue;
        }

        // Check pattern step active
        if (pat.baseSteps.contains(step))
        {
            int note = m_slotNote[slot].load();
            note += state.transposeTotal;  // apply transposes

            if (state.hasActiveChord)
                note = state.activeChord.root;

            note = juce::jlimit (0, 127, note);

            for (auto& held : m_heldNotes[slot])
            {
                if (held.active && held.note == note)
                {
                    m_slotMidi[slot].addEvent (juce::MidiMessage::noteOff (1, held.note), sampleOffset);
                    held.active = false;
                    held.note = -1;
                    held.ticksRemaining = 0;
                }
            }

            m_slotMidi[slot].addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (100)),
                                       sampleOffset);
            pushHeldNote (slot, note, kSequencerTicksPerStep);
        }
    }

    m_currentStep.store (step16 % 64);
    triggerAsyncUpdate();
}


void PluginProcessor::handleAsyncUpdate()
{
    if (onStepAdvanced)
        onStepAdvanced (m_currentStep.load());
}

//==============================================================================
// Transport
//==============================================================================
void PluginProcessor::setPlaying (bool playing) noexcept
{
    m_playing.store (playing);
    m_appState.transport.isPlaying = playing;
    if (! playing)
    {
        m_flushAllNotes.store (true);
        for (int slot = 0; slot < SpoolAudioGraph::kNumSlots; ++slot)
            resetPhraseMemory (slot);
    }
}

void PluginProcessor::setBpm (float bpm) noexcept
{
    const auto clamped = juce::jlimit (20.0f, 999.0f, bpm);
    m_songMgr.setTempo (juce::roundToInt (clamped));
    syncAuthoredSongToRuntime();
}

void PluginProcessor::syncAuthoredSongToRuntime() noexcept
{
    const auto authoredBpm = juce::jlimit (20, 999, m_songMgr.getTempo());
    m_bpm.store (static_cast<float> (authoredBpm));
    m_appState.song.title = m_songMgr.getSongTitle();
    m_appState.song.bpm = authoredBpm;
    m_appState.song.numerator = m_songMgr.getNumerator();
    m_appState.song.denominator = m_songMgr.getDenominator();
    m_appState.song.keyRoot = m_songMgr.getKeyRoot();
    m_appState.song.keyScale = m_songMgr.getKeyScale();
    m_appState.song.notes = m_songMgr.getNotes();
    m_appState.transport.bpm = static_cast<double> (authoredBpm);

    {
        const juce::ScopedWriteLock structureWrite (m_structureLock);
        m_appState.structure = m_songMgr.getStructureState();
    }

    m_beatsPerBar = static_cast<double> (juce::jmax (1, m_songMgr.getNumerator()));
    m_midiConstraintEngine.setStructure (&m_songMgr.getStructureState());
}

void PluginProcessor::syncTransportFromAppState() noexcept
{
    const auto desiredBpm = juce::jlimit (20.0, 999.0, m_appState.transport.bpm);
    const auto desiredPlaying = m_appState.transport.isPlaying;

    const auto oldBpm = static_cast<double> (m_bpm.load());
    const auto oldPlaying = m_playing.load();

    if (std::abs (oldBpm - desiredBpm) > 0.0001)
    {
        m_songMgr.setTempo (juce::roundToInt (desiredBpm));
        syncAuthoredSongToRuntime();
        DBG ("BPM updated via AppState: " << desiredBpm);
    }

    if (oldPlaying != desiredPlaying)
    {
        m_playing.store (desiredPlaying);
        DBG ("Play state updated via AppState: " << (desiredPlaying ? "playing" : "stopped"));
    }
}

void PluginProcessor::seekTransport (double beat) noexcept
{
    const double clamped = juce::jmax (0.0, beat);
    m_samplePos = 0.0;
    m_currentSongBeat = clamped;
    m_currentBeat = clamped;
    m_appState.transport.playheadBeats = clamped;
    m_currentStep.store (static_cast<int> (std::floor (clamped * 4.0)) % 64);
    for (int slot = 0; slot < SpoolAudioGraph::kNumSlots; ++slot)
        resetPhraseMemory (slot);
}

void PluginProcessor::setConstraintNotePolicy (MidiConstraintEngine::NotePolicy policy) noexcept
{
    m_midiConstraintEngine.setNotePolicy (policy);
}

MidiConstraintEngine::NotePolicy PluginProcessor::getConstraintNotePolicy() const noexcept
{
    return m_midiConstraintEngine.getNotePolicy();
}

void PluginProcessor::setConstraintScaleLock (bool enabled) noexcept
{
    m_midiConstraintEngine.setScaleLock (enabled);
}

void PluginProcessor::setConstraintChordLock (bool enabled) noexcept
{
    m_midiConstraintEngine.setChordLock (enabled);
}

void PluginProcessor::setStructureFollowForTracks (bool enabled) noexcept
{
    m_useStructureForTracks.store (enabled);
}

bool PluginProcessor::isStructureFollowForTracksEnabled() const noexcept
{
    return m_useStructureForTracks.load();
}

//==============================================================================
void PluginProcessor::setFocusedSlot (int slot) noexcept
{
    m_midiRouter.setFocusedSlot (slot);
    m_focusedSlotIndex.store (slot);
}

//==============================================================================
void PluginProcessor::setStepActive (int slot, int step, bool active) noexcept
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots) return;
    if (step < 0 || step >= 64) return;
    const uint64_t mask = 1ULL << step;
    if (active)
        m_stepBits[slot].fetch_or  (mask, std::memory_order_relaxed);
    else
        m_stepBits[slot].fetch_and (~mask, std::memory_order_relaxed);
}

void PluginProcessor::setStepCount (int slot, int count) noexcept
{
    if (slot >= 0 && slot < SpoolAudioGraph::kNumSlots)
        m_stepCount[slot].store (juce::jlimit (1, 64, count));
}

void PluginProcessor::setSlotPattern (int slot, const SlotPattern& pattern) noexcept
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots)
        return;

    const auto compiled = compileRuntimePattern (pattern);
    const juce::SpinLock::ScopedLockType lock (m_runtimePatternLocks[slot]);
    m_runtimePatterns[slot] = compiled;
}

void PluginProcessor::setSlotNote (int slot, int note) noexcept
{
    if (slot >= 0 && slot < SpoolAudioGraph::kNumSlots)
        m_slotNote[slot].store (juce::jlimit (0, 127, note));
}

int PluginProcessor::getSlotNote (int slot) const noexcept
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots) return 60;
    return m_slotNote[slot].load();
}

void  PluginProcessor::setSlotLevel  (int slot, float level) noexcept { m_audioGraph.setSlotLevel  (slot, level);  }
float PluginProcessor::getSlotLevel  (int slot) const noexcept        { return m_audioGraph.getSlotLevel  (slot);  }
void  PluginProcessor::setSlotMuted  (int slot, bool muted)  noexcept { m_audioGraph.setSlotMuted  (slot, muted);  }
bool  PluginProcessor::isSlotMuted   (int slot) const noexcept        { return m_audioGraph.isSlotMuted   (slot);  }
void  PluginProcessor::setSlotSoloed (int slot, bool soloed) noexcept { m_audioGraph.setSlotSoloed (slot, soloed); }
bool  PluginProcessor::isSlotSoloed  (int slot) const noexcept        { return m_audioGraph.isSlotSoloed  (slot);  }
float PluginProcessor::getSlotMeter  (int slot) const noexcept        { return m_audioGraph.getSlotMeter  (slot);  }
void  PluginProcessor::setSlotActive (int slot, bool active) noexcept { m_audioGraph.setSlotActive (slot, active); }

//==============================================================================
void PluginProcessor::setFXChainParam (int slotIndex, int nodeIndex,
                                        int paramIndex, float value) noexcept
{
    if (slotIndex < 0 || slotIndex >= SpoolAudioGraph::kNumSlots) return;
    m_audioGraph.getFXChain (slotIndex).setParam (nodeIndex, paramIndex, value);
}

void PluginProcessor::rebuildFXChain (int slotIndex, const ChainState& chain)
{
    if (slotIndex < 0 || slotIndex >= SpoolAudioGraph::kNumSlots) return;
    m_audioGraph.getFXChain (slotIndex).rebuildFromChain (chain);
}

//==============================================================================
void  PluginProcessor::setMasterGain (float g) noexcept { m_masterGain.store (juce::jlimit (0.0f, 2.0f, g)); }
void  PluginProcessor::setMasterPan  (float p) noexcept { m_masterPan.store  (juce::jlimit (-1.0f, 1.0f, p)); }
float PluginProcessor::getMasterGain() const noexcept   { return m_masterGain.load(); }
float PluginProcessor::getMasterPan()  const noexcept   { return m_masterPan.load();  }

//==============================================================================
// Circular buffer grab
//==============================================================================
void PluginProcessor::grabRegion (double startSecondsAgo, double lengthSeconds)
{
    m_grabbedClip    = m_circularBuffer.getRegion (startSecondsAgo, lengthSeconds);
    m_hasGrabbedClip = true;
    triggerAsyncUpdate();
}

void PluginProcessor::grabLastBars (int numBars)
{
    m_grabbedClip    = m_circularBuffer.getLastBars (numBars,
                                                     static_cast<double> (m_bpm.load()),
                                                     m_beatsPerBar,
                                                     m_currentBeat);
    m_hasGrabbedClip = true;
    triggerAsyncUpdate();
}

void PluginProcessor::setLooperPreviewClip (const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const juce::SpinLock::ScopedLockType previewLock (m_looperPreviewLock);
    m_looperPreviewBuffer.makeCopyOf (buffer);
    m_looperPreviewSourceRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    m_looperPreviewReadPos = 0.0;
    m_looperPreviewProgress.store (0.0f, std::memory_order_relaxed);
}

void PluginProcessor::clearLooperPreview()
{
    const juce::SpinLock::ScopedLockType previewLock (m_looperPreviewLock);
    m_looperPreviewBuffer.setSize (0, 0);
    m_looperPreviewReadPos = 0.0;
    m_looperPreviewSourceRate = 44100.0;
    m_looperPreviewLoopingFlag = true;
    m_looperPreviewPlaying.store (false, std::memory_order_relaxed);
    m_looperPreviewProgress.store (0.0f, std::memory_order_relaxed);
}

void PluginProcessor::setLooperPreviewState (bool playing, bool looping)
{
    const juce::SpinLock::ScopedLockType previewLock (m_looperPreviewLock);
    m_looperPreviewLoopingFlag = looping;
    if (! playing)
        m_looperPreviewReadPos = 0.0;
    m_looperPreviewPlaying.store (playing && m_looperPreviewBuffer.getNumSamples() > 0, std::memory_order_relaxed);
    m_looperPreviewProgress.store (playing ? m_looperPreviewProgress.load (std::memory_order_relaxed) : 0.0f,
                                   std::memory_order_relaxed);
}

//==============================================================================
void PluginProcessor::logRuntimeAction (const juce::String& msg) const
{
    juce::ignoreUnused (msg);
    DBG ("[SPOOL] " << msg);
}

//==============================================================================
// Drum machine step data
//==============================================================================
void PluginProcessor::setDrumVoiceStepBits (int slot, int voice, uint64_t bits) noexcept
{
    if (slot  < 0 || slot  >= SpoolAudioGraph::kNumSlots) return;
    if (voice < 0 || voice >= kMaxDrumVoices)             return;
    m_drumVoices[slot][voice].stepBits.store (bits);
}

void PluginProcessor::setDrumVoiceStepCount (int slot, int voice, int count) noexcept
{
    if (slot  < 0 || slot  >= SpoolAudioGraph::kNumSlots) return;
    if (voice < 0 || voice >= kMaxDrumVoices)             return;
    m_drumVoices[slot][voice].stepCount.store (juce::jlimit (1, 64, count));
}

void PluginProcessor::setDrumVoiceMidiNote (int slot, int voice, int note) noexcept
{
    if (slot  < 0 || slot  >= SpoolAudioGraph::kNumSlots) return;
    if (voice < 0 || voice >= kMaxDrumVoices)             return;
    m_drumVoices[slot][voice].midiNote.store (juce::jlimit (0, 127, note));
}

void PluginProcessor::setDrumVoiceCount (int slot, int count) noexcept
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots) return;
    m_drumVoiceCount[slot].store (juce::jlimit (0, kMaxDrumVoices, count));
}

void PluginProcessor::setDrumStepVelocity (int slot, int voice, int step, float velocity) noexcept
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots) return;
    if (voice < 0 || voice >= kMaxDrumVoices) return;
    if (step < 0 || step >= kMaxDrumStepsPerVoice) return;
    m_drumVoices[slot][voice].stepVelocitiesNorm[(size_t) step].store (juce::jlimit (5, 100, juce::roundToInt (velocity * 100.0f)));
}

void PluginProcessor::setDrumStepRatchet (int slot, int voice, int step, int count) noexcept
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots) return;
    if (voice < 0 || voice >= kMaxDrumVoices) return;
    if (step < 0 || step >= kMaxDrumStepsPerVoice) return;
    m_drumVoices[slot][voice].stepRatchets[(size_t) step].store (juce::jlimit (1, 8, count));
}

void PluginProcessor::setDrumStepDivision (int slot, int voice, int step, int division) noexcept
{
    if (slot < 0 || slot >= SpoolAudioGraph::kNumSlots) return;
    if (voice < 0 || voice >= kMaxDrumVoices) return;
    if (step < 0 || step >= kMaxDrumStepsPerVoice) return;
    m_drumVoices[slot][voice].stepDivisions[(size_t) step].store (juce::jlimit (1, 8, division));
}

uint64_t PluginProcessor::getDrumVoiceStepBits (int slot, int voice) const noexcept
{
    if (slot  < 0 || slot  >= SpoolAudioGraph::kNumSlots) return 0ULL;
    if (voice < 0 || voice >= kMaxDrumVoices)             return 0ULL;
    return m_drumVoices[slot][voice].stepBits.load();
}

int PluginProcessor::getDrumVoiceStepCount (int slot, int voice) const noexcept
{
    if (slot  < 0 || slot  >= SpoolAudioGraph::kNumSlots) return 16;
    if (voice < 0 || voice >= kMaxDrumVoices)             return 16;
    return m_drumVoices[slot][voice].stepCount.load();
}

void PluginProcessor::setRoutingMatrix (const std::array<uint8_t, 8>& m)
{
    std::lock_guard<std::mutex> lock (m_routingLock);
    m_routingState = m;
}

std::array<uint8_t, 8> PluginProcessor::getRoutingMatrix() const
{
    std::lock_guard<std::mutex> lock (m_routingLock);
    return m_routingState;
}

void PluginProcessor::setRoutingState (const RoutingState& state)
{
    std::lock_guard<std::mutex> lock (m_routingLock);
    m_routeModel = state;
}

RoutingState PluginProcessor::getRoutingState() const
{
    std::lock_guard<std::mutex> lock (m_routingLock);
    return m_routeModel;
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement state ("SPOOLState");
    state.setAttribute ("masterGain", static_cast<double> (m_masterGain.load()));
    state.setAttribute ("masterPan",  static_cast<double> (m_masterPan.load()));
    state.setAttribute ("bpm",        static_cast<double> (m_bpm.load()));
    {
        std::lock_guard<std::mutex> lock (m_routingLock);
        juce::String bits;
        for (auto b : m_routingState)
            bits += (b ? '1' : '0');
        state.setAttribute ("routing", bits);
    }
    const juce::String xmlText = state.toString();
    destData.replaceAll (xmlText.toRawUTF8(),
                         static_cast<size_t> (xmlText.getNumBytesAsUTF8()));
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0) return;
    const juce::String s (juce::CharPointer_UTF8 (static_cast<const char*> (data)));
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (s));
    if (xml == nullptr) return;

    m_masterGain.store (static_cast<float> (xml->getDoubleAttribute ("masterGain", 1.0)));
    m_masterPan.store  (static_cast<float> (xml->getDoubleAttribute ("masterPan",  0.0)));
    m_bpm.store        (static_cast<float> (xml->getDoubleAttribute ("bpm",       120.0)));

    const juce::String bits = xml->getStringAttribute ("routing", {});
    if (bits.isNotEmpty())
    {
        std::lock_guard<std::mutex> lock (m_routingLock);
        for (int i = 0; i < (int) m_routingState.size() && i < bits.length(); ++i)
            m_routingState[i] = (bits[i] == '1') ? 1 : 0;
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

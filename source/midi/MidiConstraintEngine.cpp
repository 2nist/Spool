#include "MidiConstraintEngine.h"

#include "../structure/StructureEngine.h"

#include <algorithm>
#include <limits>

namespace
{
std::vector<int> intersectPitchClasses (const std::vector<int>& a, const std::vector<int>& b)
{
    std::vector<int> out;
    for (auto v : a)
    {
        if (std::find (b.begin(), b.end(), v) != b.end())
            out.push_back (v);
    }

    return out;
}

bool containsPitchClass (const std::vector<int>& pitchClasses, int midiNote)
{
    const auto pc = (midiNote % 12 + 12) % 12;
    return std::find (pitchClasses.begin(), pitchClasses.end(), pc) != pitchClasses.end();
}
} // namespace

void MidiConstraintEngine::setStructure (const StructureState* structure)
{
    structureRef = structure;
}

void MidiConstraintEngine::setStructureEngine (const StructureEngine* engine)
{
    engineRef = engine;
}

int MidiConstraintEngine::processNote (int midiNote, double beat)
{
    if (notePolicy == NotePolicy::Bypass)
        return midiNote;

    const auto allowed = buildAllowedPitchClasses (beat, nullptr, nullptr, midiNote);
    if (allowed.empty())
        return midiNote;

    if (notePolicy == NotePolicy::Guide)
        return midiNote;

    return snapToNearest (allowed, midiNote);
}

MidiConstraintEngine::GuidanceResult MidiConstraintEngine::analyzeNote (int midiNote, double beat) const
{
    GuidanceResult result;
    result.inputNote = midiNote;
    result.suggestedNote = midiNote;

    bool inScale = true;
    bool inChord = true;
    const auto allowed = buildAllowedPitchClasses (beat, &inScale, &inChord, midiNote);
    result.inScale = inScale;
    result.inChord = inChord;

    if (! allowed.empty())
    {
        result.suggestedNote = snapToNearest (allowed, midiNote);
        result.wouldChange = (result.suggestedNote != midiNote);
    }

    return result;
}

void MidiConstraintEngine::setScaleLock (bool enabled)
{
    scaleLockEnabled = enabled;
}

void MidiConstraintEngine::setChordLock (bool enabled)
{
    chordLockEnabled = enabled;
}

void MidiConstraintEngine::setNotePolicy (NotePolicy policy)
{
    notePolicy = policy;
}

MidiConstraintEngine::NotePolicy MidiConstraintEngine::getNotePolicy() const noexcept
{
    return notePolicy;
}

std::vector<int> MidiConstraintEngine::buildAllowedPitchClasses (double beat, bool* inScale, bool* inChord, int midiNote) const
{
    if (! scaleLockEnabled && ! chordLockEnabled)
    {
        if (inScale != nullptr) *inScale = true;
        if (inChord != nullptr) *inChord = true;
        return {};
    }

    Chord currentChord { "C", "maj" };
    if (engineRef != nullptr)
        currentChord = engineRef->getChordAtBeat (beat);

    const auto chordPcs = theory.getChordPitchClasses (currentChord, engineRef);
    const auto scalePcs = theory.getScalePitchClasses (currentChord.root, "major");

    if (inScale != nullptr)
        *inScale = containsPitchClass (scalePcs, midiNote);
    if (inChord != nullptr)
        *inChord = containsPitchClass (chordPcs, midiNote);

    if (scaleLockEnabled && chordLockEnabled)
    {
        auto intersection = intersectPitchClasses (scalePcs, chordPcs);
        if (intersection.empty())
            intersection = chordPcs;
        return intersection;
    }

    if (chordLockEnabled)
        return chordPcs;

    return scalePcs;
}

int MidiConstraintEngine::snapToNearest (const std::vector<int>& allowedPitchClasses, int inputNote)
{
    int best = inputNote;
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

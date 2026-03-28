#include "StepGridSingle.h"

#include <cmath>
#include <limits>

namespace
{
std::array<int, 7> intervalsForMode (const juce::String& mode)
{
    auto lower = mode.trim().toLowerCase();
    if (lower == "minor")      return { 0, 2, 3, 5, 7, 8, 10 };
    if (lower == "dorian")     return { 0, 2, 3, 5, 7, 9, 10 };
    if (lower == "mixolydian") return { 0, 2, 4, 5, 7, 9, 10 };
    if (lower == "lydian")     return { 0, 2, 4, 6, 7, 9, 11 };
    if (lower == "phrygian")   return { 0, 1, 3, 5, 7, 8, 10 };
    if (lower == "locrian")    return { 0, 1, 3, 5, 6, 8, 10 };
    return { 0, 2, 4, 5, 7, 9, 11 };
}
}

StepGridSingle::StepGridSingle()
{
    setRepaintsOnMouseActivity (false);
    setWantsKeyboardFocus (true);
}

void StepGridSingle::setPattern (SlotPattern* pattern, juce::Colour groupColor)
{
    m_pattern = pattern;
    m_groupColor = groupColor;
    m_selectedStep = 0;
    m_selectedEvent = -1;
    m_detailOpen = false;
    ensureSelectionValid();
    repaint();
}

void StepGridSingle::clearPattern ()
{
    m_pattern = nullptr;
    m_playhead = -1;
    m_selectedStep = 0;
    m_selectedEvent = -1;
    m_detailOpen = false;
    m_dragMode = DragMode::none;
    m_hoverStep = -1;
    m_hoverEvent = -1;
    m_hoverStepResizeHandle = false;
    repaint();
}

void StepGridSingle::setPlayhead (int stepIndex)
{
    m_playhead = stepIndex;
    repaint();
}

void StepGridSingle::setMusicalContext (const juce::String& keyRoot,
                                        const juce::String& keyScale,
                                        const juce::String& currentChord,
                                        const juce::String& nextChord,
                                        bool followingStructure,
                                        bool locallyOverriding)
{
    m_keyRoot = keyRoot.isNotEmpty() ? keyRoot : juce::String ("C");
    m_keyScale = keyScale.isNotEmpty() ? keyScale : juce::String ("Major");
    m_currentChord = currentChord.isNotEmpty() ? currentChord : juce::String ("Cmaj");
    m_nextChord = nextChord.isNotEmpty() ? nextChord : m_currentChord;
    m_followingStructure = followingStructure;
    m_locallyOverriding = locallyOverriding;
    repaint();
}

juce::Rectangle<int> StepGridSingle::stepLaneRect() const noexcept
{
    return { kPad, kPad, getWidth() - (kPad * 2), kLaneH };
}

juce::Rectangle<int> StepGridSingle::compactBarRect() const noexcept
{
    const auto lane = stepLaneRect();
    return { lane.getX(), lane.getBottom() + kGap, lane.getWidth(), kBtnH };
}

juce::Rectangle<int> StepGridSingle::detailToolbarRect() const noexcept
{
    if (! m_detailOpen)
        return {};

    const auto bar = compactBarRect();
    return { bar.getX(), bar.getBottom() + kGap, bar.getWidth(), kBtnH };
}

juce::Rectangle<int> StepGridSingle::detailRect() const noexcept
{
    if (! m_detailOpen)
        return {};

    const auto toolbar = detailToolbarRect();
    return { toolbar.getX(), toolbar.getBottom() + kGap, toolbar.getWidth(), getHeight() - toolbar.getBottom() - kGap - kPad };
}

juce::Rectangle<int> StepGridSingle::editRect() const noexcept
{
    const auto bar = compactBarRect();
    return { bar.getX(), bar.getY(), 56, bar.getHeight() };
}

juce::Rectangle<int> StepGridSingle::closeDetailRect() const noexcept
{
    const auto bar = m_detailOpen ? detailToolbarRect() : compactBarRect();
    return { bar.getX(), bar.getY(), 56, bar.getHeight() };
}

juce::Rectangle<int> StepGridSingle::modeRect() const noexcept
{
    const auto bar = m_detailOpen ? detailToolbarRect() : compactBarRect();
    auto r = bar.withTrimmedLeft (60);
    return { r.getX(), r.getY(), 66, r.getHeight() };
}

juce::Rectangle<int> StepGridSingle::roleRect() const noexcept
{
    return modeRect().translated (70, 0);
}

juce::Rectangle<int> StepGridSingle::followRect() const noexcept
{
    return roleRect().translated (70, 0);
}

juce::Rectangle<int> StepGridSingle::moreRect() const noexcept
{
    const auto bar = m_detailOpen ? detailToolbarRect() : compactBarRect();
    return { bar.getRight() - 64, bar.getY(), 64, bar.getHeight() };
}

SlotPattern::Step* StepGridSingle::selectedStepData() noexcept
{
    return m_pattern != nullptr ? m_pattern->getStep (m_selectedStep) : nullptr;
}

const SlotPattern::Step* StepGridSingle::selectedStepData() const noexcept
{
    return m_pattern != nullptr ? m_pattern->getStep (m_selectedStep) : nullptr;
}

void StepGridSingle::ensureSelectionValid()
{
    if (m_pattern == nullptr)
        return;

    m_selectedStep = juce::jlimit (0, m_pattern->activeStepCount() - 1, m_selectedStep);
    const auto* step = selectedStepData();
    if (step == nullptr)
    {
        m_selectedEvent = -1;
        return;
    }

    if (step->microEventCount <= 0)
        m_selectedEvent = -1;
    else
        m_selectedEvent = juce::jlimit (0, step->microEventCount - 1, m_selectedEvent < 0 ? 0 : m_selectedEvent);
}

juce::Rectangle<float> StepGridSingle::stepRect (int index) const noexcept
{
    if (m_pattern == nullptr)
        return {};

    const auto* step = m_pattern->getStep (index);
    if (step == nullptr)
        return {};

    const auto lane = stepLaneRect().toFloat();
    const float units = (float) juce::jmax (1, m_pattern->patternLength());
    const float x = lane.getX() + lane.getWidth() * ((float) step->start / units);
    const float w = juce::jmax (8.0f, lane.getWidth() * ((float) step->duration / units) - 1.0f);
    return { x, lane.getY(), w, lane.getHeight() };
}

juce::Rectangle<float> StepGridSingle::stepResizeHandleRect (int stepIndex) const noexcept
{
    const auto rect = stepRect (stepIndex).reduced (1.0f, 2.0f);
    if (rect.isEmpty())
        return {};

    return { rect.getRight() - 10.0f, rect.getY() + 3.0f, 8.0f, rect.getHeight() - 6.0f };
}

juce::Rectangle<float> StepGridSingle::microEventRect (int eventIndex) const noexcept
{
    const auto* step = selectedStepData();
    if (step == nullptr || ! juce::isPositiveAndBelow (eventIndex, step->microEventCount))
        return {};

    const auto detail = detailRect().toFloat();
    const auto& event = step->microEvents[(size_t) eventIndex];
    const float x = detail.getX() + detail.getWidth() * juce::jlimit (0.0f, 0.98f, event.timeOffset);
    const float w = juce::jmax (6.0f, detail.getWidth() * juce::jlimit (0.05f, 1.0f, event.length));
    const int value = event.pitchValue < 0 ? valueForRow (kPitchRows / 2) : event.pitchValue;
    const int row = rowForValue (value);
    const float rowH = detail.getHeight() / (float) kPitchRows;
    const float y = detail.getY() + row * rowH + 1.0f;
    return { x, y, juce::jmin (detail.getRight() - x, w), juce::jmax (6.0f, rowH - 2.0f) };
}

int StepGridSingle::stepAt (juce::Point<int> pos) const noexcept
{
    if (m_pattern == nullptr || ! stepLaneRect().contains (pos))
        return -1;

    for (int i = 0; i < m_pattern->activeStepCount(); ++i)
        if (stepRect (i).contains ((float) pos.x, (float) pos.y))
            return i;

    return -1;
}

int StepGridSingle::microEventAt (juce::Point<int> pos) const noexcept
{
    const auto* step = selectedStepData();
    if (step == nullptr || ! detailRect().contains (pos))
        return -1;

    for (int i = step->microEventCount - 1; i >= 0; --i)
        if (microEventRect (i).contains ((float) pos.x, (float) pos.y))
            return i;

    return -1;
}

float StepGridSingle::normalizedTimeFromDetailX (int x) const noexcept
{
    const auto detail = detailRect();
    if (detail.getWidth() <= 1)
        return 0.0f;

    return juce::jlimit (0.0f, 0.98f, (float) (x - detail.getX()) / (float) detail.getWidth());
}

int StepGridSingle::valueForRow (int row) const noexcept
{
    const auto* step = selectedStepData();
    const auto mode = step != nullptr ? step->noteMode : SlotPattern::NoteMode::absolute;
    const int clampedRow = juce::jlimit (0, kPitchRows - 1, row);

    switch (mode)
    {
        case SlotPattern::NoteMode::absolute:  return juce::jlimit (0, 127, (m_pitchBase + kPitchRows - 1) - clampedRow);
        case SlotPattern::NoteMode::degree:    return kPitchRows - 1 - clampedRow;
        case SlotPattern::NoteMode::chordTone: return kPitchRows - 1 - clampedRow;
        case SlotPattern::NoteMode::interval:  return 6 - clampedRow;
    }

    return 60;
}

int StepGridSingle::rowForValue (int value) const noexcept
{
    const auto* step = selectedStepData();
    const auto mode = step != nullptr ? step->noteMode : SlotPattern::NoteMode::absolute;

    switch (mode)
    {
        case SlotPattern::NoteMode::absolute:
            return juce::jlimit (0, kPitchRows - 1, (m_pitchBase + kPitchRows - 1) - juce::jlimit (0, 127, value));
        case SlotPattern::NoteMode::degree:
        case SlotPattern::NoteMode::chordTone:
            return juce::jlimit (0, kPitchRows - 1, (kPitchRows - 1) - juce::jlimit (0, kPitchRows - 1, value));
        case SlotPattern::NoteMode::interval:
            return juce::jlimit (0, kPitchRows - 1, 6 - juce::jlimit (-5, 6, value));
    }

    return 0;
}

juce::String StepGridSingle::noteValueLabel (int value) const
{
    const auto* step = selectedStepData();
    const auto mode = step != nullptr ? step->noteMode : SlotPattern::NoteMode::absolute;

    switch (mode)
    {
        case SlotPattern::NoteMode::absolute:
            return juce::MidiMessage::getMidiNoteName (juce::jlimit (0, 127, value), true, true, 3);
        case SlotPattern::NoteMode::degree:
        {
            static const char* labels[12] = { "1", "b2", "2", "b3", "3", "4", "#4", "5", "b6", "6", "b7", "7" };
            const int wrapped = (value % 12 + 12) % 12;
            const int octave = value / 12;
            auto label = juce::String (labels[wrapped]);
            if (octave > 0)
                label << "+" << octave;
            return label;
        }
        case SlotPattern::NoteMode::chordTone:
        {
            static const char* tones[12] = { "R", "3", "5", "7", "9", "11", "13", "R+", "3+", "5+", "7+", "9+" };
            return juce::String (tones[juce::jlimit (0, 11, value)]);
        }
        case SlotPattern::NoteMode::interval:
            return value >= 0 ? "+" + juce::String (value) : juce::String (value);
    }

    return {};
}

juce::String StepGridSingle::noteName (int midiNote) const
{
    return juce::MidiMessage::getMidiNoteName (juce::jlimit (0, 127, midiNote), true, true, 3);
}

int StepGridSingle::rootToPitchClass (const juce::String& root) const noexcept
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

juce::String StepGridSingle::chordRootString (const juce::String& chordLabel) const
{
    auto text = chordLabel.trim();
    if (text.isEmpty())
        return m_keyRoot;

    if (text.length() >= 2)
    {
        const auto two = text.substring (0, 2);
        if (two[1] == '#' || two[1] == 'b' || two[1] == 'B')
            return two.replaceCharacter ('B', 'b');
    }

    return text.substring (0, 1);
}

juce::String StepGridSingle::chordTypeString (const juce::String& chordLabel) const
{
    auto text = chordLabel.trim();
    if (text.isEmpty())
        return "maj";

    const auto root = chordRootString (text);
    auto suffix = text.fromFirstOccurrenceOf (root, false, false).trim();
    if (suffix.isEmpty())
        suffix = "maj";
    return suffix;
}

std::vector<int> StepGridSingle::chordIntervalsForLabel (const juce::String& chordLabel) const
{
    auto lowerType = chordTypeString (chordLabel).toLowerCase();
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

std::vector<int> StepGridSingle::scalePitchClassesForKey() const
{
    std::vector<int> pcs;
    pcs.reserve (7);
    const int rootPc = rootToPitchClass (m_keyRoot);
    const auto intervals = intervalsForMode (m_keyScale);
    for (const auto interval : intervals)
        pcs.push_back ((rootPc + interval) % 12);
    return pcs;
}

int StepGridSingle::nearestNoteForPitchClass (int aroundNote, int targetPc) const noexcept
{
    int best = juce::jlimit (0, 127, aroundNote);
    int bestDistance = std::numeric_limits<int>::max();
    const int aroundOctave = aroundNote / 12;

    for (int octave = aroundOctave - 2; octave <= aroundOctave + 2; ++octave)
    {
        const int candidate = octave * 12 + ((targetPc % 12 + 12) % 12);
        if (candidate < 0 || candidate > 127)
            continue;

        const int distance = std::abs (candidate - aroundNote);
        if (distance < bestDistance)
        {
            best = candidate;
            bestDistance = distance;
        }
    }

    return best;
}

juce::String StepGridSingle::chordTargetLabel (SlotPattern::HarmonicSource source) const
{
    switch (source)
    {
        case SlotPattern::HarmonicSource::key:       return m_keyRoot + " " + m_keyScale;
        case SlotPattern::HarmonicSource::chord:     return m_currentChord;
        case SlotPattern::HarmonicSource::nextChord: return m_nextChord;
    }

    return m_keyRoot + " " + m_keyScale;
}

int StepGridSingle::realizedPitchForPreview (const SlotPattern::Step& step, const SlotPattern::MicroEvent& event) const noexcept
{
    const int baseNote = step.anchorValue < 0 ? 60 : juce::jlimit (0, 127, step.anchorValue);
    const int roleCenter = [&step, baseNote]
    {
        switch (step.role)
        {
            case SlotPattern::StepRole::bass:       return juce::jlimit (36, 52, baseNote - 12);
            case SlotPattern::StepRole::chord:      return juce::jlimit (48, 72, baseNote + 2);
            case SlotPattern::StepRole::lead:       return juce::jlimit (55, 84, baseNote + 12);
            case SlotPattern::StepRole::fill:       return juce::jlimit (52, 88, baseNote + 7);
            case SlotPattern::StepRole::transition: return juce::jlimit (40, 76, baseNote + 5);
        }

        return baseNote;
    }();

    const juce::String harmonicChord = (step.harmonicSource == SlotPattern::HarmonicSource::nextChord || step.lookAheadToNextChord)
                                         ? m_nextChord
                                         : m_currentChord;
    const int harmonicRoot = rootToPitchClass (step.harmonicSource == SlotPattern::HarmonicSource::key ? m_keyRoot
                                                                                                        : chordRootString (harmonicChord));

    switch (step.noteMode)
    {
        case SlotPattern::NoteMode::absolute:
        {
            const int stored = event.pitchValue < 0 ? baseNote : event.pitchValue;
            if (! step.followStructure)
                return juce::jlimit (0, 127, stored);

            const auto pcs = step.harmonicSource == SlotPattern::HarmonicSource::key ? scalePitchClassesForKey()
                                                                                      : chordIntervalsForLabel (harmonicChord);
            if (pcs.empty())
                return juce::jlimit (0, 127, stored);

            if (step.harmonicSource == SlotPattern::HarmonicSource::key)
                return nearestNoteForPitchClass (stored, pcs[(size_t) (stored % juce::jmax (1, (int) pcs.size()))]);

            const int nearestPc = (harmonicRoot + pcs[(size_t) (stored % juce::jmax (1, (int) pcs.size()))]) % 12;
            return nearestNoteForPitchClass (stored, nearestPc);
        }

        case SlotPattern::NoteMode::degree:
        {
            const int raw = event.pitchValue < 0 ? 0 : event.pitchValue;
            const int degreePc = (rootToPitchClass (m_keyRoot) + ((raw % 12) + 12) % 12) % 12;
            const int octaveOffset = raw / 12;
            return juce::jlimit (24, 108, nearestNoteForPitchClass (roleCenter + octaveOffset * 12, degreePc));
        }

        case SlotPattern::NoteMode::chordTone:
        {
            const auto intervals = chordIntervalsForLabel (harmonicChord);
            const int toneIndex = juce::jmax (0, event.pitchValue < 0 ? 0 : event.pitchValue);
            const int chordSize = juce::jmax (1, (int) intervals.size());
            const int octaveOffset = toneIndex / chordSize;
            const int intervalIndex = toneIndex % chordSize;
            const int rootNote = nearestNoteForPitchClass (roleCenter, harmonicRoot);
            return juce::jlimit (24, 108, rootNote + intervals[(size_t) intervalIndex] + octaveOffset * 12);
        }

        case SlotPattern::NoteMode::interval:
        default:
        {
            const int offset = event.pitchValue < 0 ? 0 : event.pitchValue;
            return juce::jlimit (24, 108, baseNote + offset);
        }
    }
}

juce::String StepGridSingle::storedEventLabel (const SlotPattern::Step& step, const SlotPattern::MicroEvent& event) const
{
    const int storedValue = event.pitchValue < 0 ? (step.anchorValue < 0 ? 60 : step.anchorValue)
                                                 : event.pitchValue;
    return noteValueLabel (storedValue);
}

juce::String StepGridSingle::realizedEventLabel (const SlotPattern::Step& step, const SlotPattern::MicroEvent& event) const
{
    return noteName (realizedPitchForPreview (step, event));
}

juce::String StepGridSingle::eventTimingLabel (const SlotPattern::MicroEvent& event) const
{
    const int offsetPct = juce::roundToInt (event.timeOffset * 100.0f);
    const int lengthPct = juce::roundToInt (event.length * 100.0f);
    return "at " + juce::String (offsetPct) + "%  len " + juce::String (lengthPct) + "%  vel " + juce::String ((int) event.velocity);
}

juce::String StepGridSingle::stepTimingLabel (const SlotPattern::Step& step) const
{
    return "step " + juce::String (m_selectedStep + 1)
         + "  start " + juce::String (step.start + 1)
         + "  dur " + juce::String (step.duration) + "u";
}

int StepGridSingle::pitchFromDetailY (int y) const noexcept
{
    const auto detail = detailRect();
    if (detail.getHeight() <= 0)
        return 60;

    const float rowH = detail.getHeight() / (float) kPitchRows;
    const int row = juce::jlimit (0, kPitchRows - 1, (int) std::floor ((float) (y - detail.getY()) / rowH));
    return valueForRow (row);
}

void StepGridSingle::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    if (m_pattern == nullptr)
    {
        paintEmpty (g);
        return;
    }

    ensureSelectionValid();
    paintStepLane (g);
    paintCompactBar (g);
    if (m_detailOpen)
    {
        paintDetailToolbar (g);
        paintDetail (g);
    }
}

void StepGridSingle::paintEmpty (juce::Graphics& g) const
{
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("select a slot to edit hybrid steps", getLocalBounds(), juce::Justification::centred, false);
}

void StepGridSingle::paintStepLane (juce::Graphics& g) const
{
    auto lane = stepLaneRect();
    g.setColour (Theme::Colour::surface2);
    g.fillRect (lane);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRect (lane, 1);

    const int patternUnits = juce::jmax (1, m_pattern->patternLength());
    for (int unit = 1; unit < patternUnits; ++unit)
    {
        const int x = lane.getX() + (lane.getWidth() * unit) / patternUnits;
        const bool strong = (unit % 4) == 0;
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (strong ? 0.45f : 0.2f));
        g.fillRect (x, lane.getY(), 1, lane.getHeight());
    }

    for (int i = 0; i < m_pattern->activeStepCount(); ++i)
    {
        const auto rect = stepRect (i).reduced (1.0f, 2.0f);
        const auto* step = m_pattern->getStep (i);
        if (step == nullptr)
            continue;

        juce::Colour fill = Theme::Colour::surface3;
        if (step->microEventCount > 0)
            fill = step->accent ? m_groupColor.brighter (0.22f) : m_groupColor.withMultipliedSaturation (0.9f);
        if (i == m_selectedStep)
            fill = fill.brighter (0.2f);
        if (i == m_hoverStep && i != m_selectedStep)
            fill = fill.brighter (0.1f);

        const bool selected = (i == m_selectedStep);
        const bool hovered = (i == m_hoverStep);
        const bool localOverride = ! step->followStructure;
        g.setColour (fill);
        g.fillRoundedRectangle (rect, 4.0f);

        if (selected)
        {
            g.setColour (m_groupColor.withAlpha (0.18f));
            g.fillRoundedRectangle (rect.expanded (2.5f, 2.5f), 5.5f);
        }

        g.setColour (selected ? Theme::Colour::inkLight
                              : hovered ? m_groupColor.brighter (0.35f)
                                        : Theme::Colour::surfaceEdge);
        g.drawRoundedRectangle (rect, 4.0f, selected ? 2.0f : 1.0f);

        if (localOverride)
        {
            g.setColour (Theme::Colour::warning.withAlpha (0.9f));
            g.drawRoundedRectangle (rect.reduced (2.0f, 2.0f), 3.0f, 1.0f);
        }

        const bool playheadInsideStep = m_playhead >= 0
            && m_playhead >= step->start
            && m_playhead < step->start + step->duration;
        if (playheadInsideStep)
        {
            g.setColour (Theme::Colour::error);
            g.fillRoundedRectangle ({ rect.getX(), rect.getY(), 3.0f, rect.getHeight() }, 1.0f);
            g.fillRoundedRectangle ({ rect.getX(), rect.getY(), rect.getWidth(), 2.0f }, 1.0f);
        }

        g.setColour (i == m_selectedStep ? Theme::Colour::inkLight : Theme::Colour::inkMid);
        g.setFont (Theme::Font::micro());
        g.drawText (juce::String (i + 1), rect.toNearestInt().reduced (4, 2), juce::Justification::topLeft, false);

        auto badgeRect = rect.toNearestInt().reduced (4, 2);
        const auto noteCount = juce::String (step->microEventCount) + "n";
        const auto duration = juce::String (step->duration) + "u";
        g.setColour (selected ? Theme::Colour::inkLight.withAlpha (0.95f) : Theme::Colour::inkGhost.withAlpha (0.85f));
        g.drawText (duration, badgeRect.removeFromTop (10), juce::Justification::topRight, false);
        g.drawText (noteCount, badgeRect.removeFromTop (10), juce::Justification::topRight, false);
        g.drawText (juce::String (step->followStructure ? "FOLLOW" : "LOCAL") + "  " + SlotPattern::shortLabel (step->harmonicSource),
                    badgeRect.removeFromBottom (10),
                    juce::Justification::bottomLeft,
                    false);
        g.drawText (juce::String (SlotPattern::shortLabel (step->role)) + "  " + SlotPattern::shortLabel (step->noteMode),
                    badgeRect.removeFromBottom (10),
                    juce::Justification::bottomLeft,
                    false);

        if (step->microEventCount > 0)
        {
            const auto& event = step->microEvents[0];
            g.setColour (selected ? Theme::Colour::inkLight.withAlpha (0.9f) : Theme::Colour::inkGhost.withAlpha (0.8f));
            g.drawText ("now " + realizedEventLabel (*step, event),
                        rect.toNearestInt().reduced (4, 3).removeFromBottom (10),
                        juce::Justification::bottomRight,
                        false);
        }

        if (selected || hovered)
        {
            const auto handle = stepResizeHandleRect (i);
            g.setColour ((selected || m_hoverStepResizeHandle) ? Theme::Colour::inkLight
                                                               : Theme::Colour::inkMid.withAlpha (0.9f));
            g.fillRoundedRectangle (handle, 2.0f);
            g.setColour (Theme::Colour::inkDark.withAlpha (0.75f));
            const auto handleInt = handle.toNearestInt();
            for (int x = handleInt.getX() + 1; x < handleInt.getRight() - 1; x += 2)
                g.drawVerticalLine (x, (float) handleInt.getY() + 2.0f, (float) handleInt.getBottom() - 2.0f);
        }
    }

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText (m_detailOpen ? "phrase steps  |  detail open" : "phrase steps  |  double-click a step to edit micro notes",
                lane.removeFromTop (12),
                juce::Justification::centredLeft,
                false);
}

void StepGridSingle::paintDetail (juce::Graphics& g) const
{
    auto detail = detailRect();
    g.setColour (Theme::Colour::surface2);
    g.fillRect (detail);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRect (detail, 1);

    const auto* step = selectedStepData();
    if (step == nullptr)
        return;

    auto header = detail.removeFromTop (24);
    g.setColour (Theme::Colour::surface1.withAlpha (0.8f));
    g.fillRect (header);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.drawLine ((float) header.getX(), (float) header.getBottom() - 1.0f, (float) header.getRight(), (float) header.getBottom() - 1.0f, 1.0f);

    g.setFont (Theme::Font::micro());
    g.setColour (step->followStructure ? Theme::Zone::a : Theme::Colour::warning);
    g.drawText (step->followStructure ? "FOLLOWS STRUCTURE" : "LOCAL OVERRIDE",
                header.removeFromLeft (96),
                juce::Justification::centredLeft,
                false);
    g.setColour (Theme::Colour::inkGhost);
    g.drawText (juce::String (SlotPattern::fullLabel (step->role)) + "  |  "
                    + SlotPattern::fullLabel (step->noteMode) + "  |  "
                    + SlotPattern::fullLabel (step->harmonicSource),
                header.removeFromLeft (detail.getWidth() / 2),
                juce::Justification::centredLeft,
                true);
    g.drawText (stepTimingLabel (*step),
                header,
                juce::Justification::centredRight,
                false);

    const float rowH = detail.getHeight() / (float) kPitchRows;
    for (int row = 0; row < kPitchRows; ++row)
    {
        const bool strong = (row == 0 || row == kPitchRows - 1 || valueForRow (row) == 0);
        g.setColour ((strong ? Theme::Colour::surfaceEdge : Theme::Colour::surface3).withAlpha (0.8f));
        g.fillRect (detail.getX(), juce::roundToInt ((float) detail.getY() + row * rowH), detail.getWidth(), 1);
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.7f));
        g.setFont (Theme::Font::micro());
        g.drawText (noteValueLabel (valueForRow (row)),
                    detail.getX() + 2,
                    juce::roundToInt ((float) detail.getY() + row * rowH),
                    28,
                    juce::jmax (10, juce::roundToInt (rowH)),
                    juce::Justification::centredLeft,
                    false);
    }

    for (int div = 1; div < 8; ++div)
    {
        const int x = detail.getX() + (detail.getWidth() * div) / 8;
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (div % 2 == 0 ? 0.65f : 0.35f));
        g.fillRect (x, detail.getY(), 1, detail.getHeight());
    }

    for (int i = 0; i < step->microEventCount; ++i)
    {
        const auto rect = microEventRect (i);
        const bool selected = (i == m_selectedEvent);
        const bool hovered = (i == m_hoverEvent);
        g.setColour (selected ? m_groupColor.brighter (0.35f)
                              : hovered ? m_groupColor.brighter (0.18f)
                                        : m_groupColor);
        g.fillRoundedRectangle (rect, 3.0f);
        g.setColour (selected ? Theme::Colour::inkLight
                              : hovered ? Theme::Colour::inkMid
                                        : Theme::Colour::surfaceEdge);
        g.drawRoundedRectangle (rect, 3.0f, selected ? 2.0f : 1.0f);
        g.setColour (Theme::Colour::inkDark);
        g.setFont (Theme::Font::micro());
        const auto& event = step->microEvents[(size_t) i];
        const int value = event.pitchValue < 0 ? valueForRow (kPitchRows / 2) : event.pitchValue;
        g.drawText (noteValueLabel (value),
                    rect.toNearestInt().reduced (3, 1),
                    juce::Justification::centredLeft,
                    false);

        if (selected)
        {
            g.setColour (Theme::Colour::error.withAlpha (0.95f));
            g.drawRoundedRectangle (rect.expanded (1.5f, 1.5f), 4.0f, 1.3f);
        }
    }

    if (step->microEventCount == 0)
    {
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.8f));
        g.setFont (Theme::Font::micro());
        g.drawText ("click to add note  |  double-click for quick note  |  right-click note for actions",
                    detail.reduced (32, 18),
                    juce::Justification::centred,
                    false);
    }

    auto footer = detail.removeFromTop (14);
    if (m_selectedEvent >= 0 && juce::isPositiveAndBelow (m_selectedEvent, step->microEventCount))
    {
        const auto& event = step->microEvents[(size_t) m_selectedEvent];
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkLight);
        g.drawText ("stored " + storedEventLabel (*step, event)
                        + "  ->  now " + realizedEventLabel (*step, event)
                        + "  |  " + eventTimingLabel (event)
                        + "  |  target " + chordTargetLabel (step->harmonicSource),
                    footer,
                    juce::Justification::centredLeft,
                    true);
    }
    else
    {
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText ("micro note editor  |  stored values on the grid, realized pitch under current harmony in the footer",
                    footer,
                    juce::Justification::centredLeft,
                    true);
    }
}

void StepGridSingle::paintCompactBar (juce::Graphics& g) const
{
    const auto drawBtn = [&] (juce::Rectangle<int> r, const juce::String& label)
    {
        g.setColour (Theme::Colour::surface2);
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRoundedRectangle (r.toFloat(), 3.0f, 1.0f);
        g.setColour (Theme::Colour::inkLight);
        g.setFont (Theme::Font::micro());
        g.drawText (label, r, juce::Justification::centred, false);
    };

    const auto c = compactBarRect();
    g.setColour (Theme::Colour::surface1);
    g.fillRect (c);

    drawBtn (editRect(), m_detailOpen ? "DETAIL" : "EDIT");
    drawBtn (modeRect(), selectedStepData() != nullptr ? juce::String ("MODE ") + SlotPattern::shortLabel (selectedStepData()->noteMode) : "MODE");
    drawBtn (roleRect(), selectedStepData() != nullptr ? juce::String ("ROLE ") + SlotPattern::shortLabel (selectedStepData()->role) : "ROLE");
    drawBtn (followRect(), selectedStepData() != nullptr ? juce::String (selectedStepData()->followStructure ? "FOLLOW" : "LOCAL") : "FOLLOW");
    drawBtn (moreRect(), "MORE");

    if (m_pattern != nullptr && selectedStepData() != nullptr)
    {
        auto info = c.withTrimmedLeft (followRect().getRight() + 8 - c.getX()).withTrimmedRight (72);
        g.setColour (selectedStepData()->followStructure ? Theme::Colour::inkGhost : Theme::Colour::warning);
        g.setFont (Theme::Font::micro());
        auto summary = "step " + juce::String (m_selectedStep + 1) + "/" + juce::String (m_pattern->activeStepCount())
                     + "  pat " + juce::String (m_pattern->patternLength()) + "u"
                     + "  " + SlotPattern::fullLabel (selectedStepData()->harmonicSource)
                     + "  " + (selectedStepData()->followStructure ? "follows structure" : "local phrase");
        if (selectedStepData()->microEventCount > 0)
            summary += "  now " + realizedEventLabel (*selectedStepData(), selectedStepData()->microEvents[0]);
        g.drawFittedText (summary,
                          info,
                          juce::Justification::centredRight,
                          1);
    }
}

void StepGridSingle::paintDetailToolbar (juce::Graphics& g) const
{
    const auto drawBtn = [&] (juce::Rectangle<int> r, const juce::String& label)
    {
        g.setColour (Theme::Colour::surface2);
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRoundedRectangle (r.toFloat(), 3.0f, 1.0f);
        g.setColour (Theme::Colour::inkLight);
        g.setFont (Theme::Font::micro());
        g.drawText (label, r, juce::Justification::centred, false);
    };

    const auto r = detailToolbarRect();
    g.setColour (Theme::Colour::surface1);
    g.fillRect (r);
    drawBtn (closeDetailRect(), "CLOSE");
    drawBtn (modeRect(), selectedStepData() != nullptr ? juce::String ("MODE ") + SlotPattern::shortLabel (selectedStepData()->noteMode) : "MODE");
    drawBtn (roleRect(), selectedStepData() != nullptr ? juce::String ("ROLE ") + SlotPattern::shortLabel (selectedStepData()->role) : "ROLE");
    drawBtn (followRect(), selectedStepData() != nullptr ? juce::String (selectedStepData()->followStructure ? "FOLLOW" : "LOCAL") : "FOLLOW");
    drawBtn (moreRect(), "TOOLS");
}

void StepGridSingle::commitChange()
{
    if (m_pattern != nullptr)
        m_pattern->normalizeStepLayout();

    ensureSelectionValid();
    repaint();
    if (onModified)
        onModified();
}

void StepGridSingle::clearStepContents (int stepIndex)
{
    if (m_pattern == nullptr)
        return;

    auto* step = m_pattern->getStep (stepIndex);
    if (step == nullptr)
        return;

    step->microEventCount = 0;
    step->gateMode = SlotPattern::GateMode::rest;
    step->accent = false;
}

void StepGridSingle::duplicateStep (int stepIndex)
{
    if (m_pattern == nullptr || ! juce::isPositiveAndBelow (stepIndex, m_pattern->activeStepCount()))
        return;

    if (m_pattern->activeStepCount() >= SlotPattern::MAX_STEPS)
        return;

    auto& pat = m_pattern->current();
    const auto source = pat.steps[(size_t) stepIndex];
    m_pattern->addStepAfter (stepIndex, source.duration);
    pat.steps[(size_t) (stepIndex + 1)] = source;
    m_selectedStep = juce::jmin (stepIndex + 1, m_pattern->activeStepCount() - 1);
}

void StepGridSingle::splitStep (int stepIndex)
{
    if (m_pattern == nullptr)
        return;

    auto* step = m_pattern->getStep (stepIndex);
    if (step == nullptr || step->duration < 2 || m_pattern->activeStepCount() >= SlotPattern::MAX_STEPS)
        return;

    const int firstDuration = juce::jmax (1, step->duration / 2);
    const int secondDuration = juce::jmax (1, step->duration - firstDuration);
    const auto original = *step;

    step->duration = firstDuration;
    m_pattern->addStepAfter (stepIndex, secondDuration);

    auto& pat = m_pattern->current();
    auto& newStep = pat.steps[(size_t) (stepIndex + 1)];
    newStep = original;
    newStep.duration = secondDuration;
    newStep.microEventCount = 0;
    newStep.gateMode = SlotPattern::GateMode::rest;
    m_selectedStep = stepIndex + 1;
}

void StepGridSingle::joinStepWithNext (int stepIndex)
{
    if (m_pattern == nullptr)
        return;

    auto* step = m_pattern->getStep (stepIndex);
    auto* next = m_pattern->getStep (stepIndex + 1);
    if (step == nullptr || next == nullptr)
        return;

    const int originalCount = step->microEventCount;
    for (int i = 0; i < next->microEventCount && step->microEventCount < SlotPattern::MAX_MICRO_EVENTS; ++i)
    {
        const auto& src = next->microEvents[(size_t) i];
        const float absoluteUnits = (float) next->duration * src.timeOffset + (float) step->duration;
        auto& dst = step->microEvents[(size_t) step->microEventCount++];
        dst = src;
        dst.timeOffset = juce::jlimit (0.0f,
                                       0.98f,
                                       absoluteUnits / (float) juce::jmax (1, step->duration + next->duration));
    }

    step->duration = juce::jmin (SlotPattern::MAX_STEP_DURATION, step->duration + next->duration);
    if (originalCount > 0 && next->microEventCount > 0)
        step->gateMode = SlotPattern::GateMode::tie;

    m_pattern->removeStep (stepIndex + 1);
    m_selectedStep = juce::jlimit (0, m_pattern->activeStepCount() - 1, stepIndex);
}

void StepGridSingle::showStepContextMenu (int stepIndex)
{
    if (m_pattern == nullptr || ! juce::isPositiveAndBelow (stepIndex, m_pattern->activeStepCount()))
        return;

    juce::PopupMenu menu;
    menu.addItem (12, selectedStepData() != nullptr && selectedStepData()->followStructure ? "Set Local Override" : "Follow Structure");
    menu.addItem (13, "Open Step Detail");
    menu.addSeparator();
    menu.addItem (14, "Pattern Shorter");
    menu.addItem (15, "Pattern Longer");
    menu.addItem (16, "Insert Step After");
    menu.addItem (1, "Delete Step");
    menu.addItem (2, "Duplicate Step");
    menu.addItem (3, "Split Step", selectedStepData() != nullptr && selectedStepData()->duration > 1);
    menu.addItem (4, "Join/Tie With Next", m_pattern->getStep (stepIndex + 1) != nullptr);
    menu.addItem (8, "Copy Step");
    menu.addItem (9, "Paste Step", m_hasStepClipboard);
    menu.addSeparator();
    menu.addItem (5, "Shorten");
    menu.addItem (6, "Lengthen");
    menu.addItem (10, "Transpose Up");
    menu.addItem (11, "Transpose Down");
    menu.addSeparator();
    menu.addItem (7, "Clear Contents");
    juce::PopupMenu roleMenu;
    roleMenu.addItem (20, "Bass");
    roleMenu.addItem (21, "Chord");
    roleMenu.addItem (22, "Lead");
    roleMenu.addItem (23, "Fill");
    roleMenu.addItem (24, "Transition");
    menu.addSubMenu ("Role", roleMenu);

    juce::PopupMenu modeMenu;
    modeMenu.addItem (30, "Absolute");
    modeMenu.addItem (31, "Degree");
    modeMenu.addItem (32, "Chord Tone");
    modeMenu.addItem (33, "Interval");
    menu.addSubMenu ("Note Mode", modeMenu);

    juce::Component::SafePointer<StepGridSingle> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safeThis, stepIndex] (int result)
                        {
                            if (safeThis == nullptr || safeThis->m_pattern == nullptr || result == 0)
                                return;

                            safeThis->m_selectedStep = stepIndex;
                            switch (result)
                            {
                                case 1: safeThis->m_pattern->removeStep (stepIndex); break;
                                case 2: safeThis->duplicateStep (stepIndex); break;
                                case 3: safeThis->splitStep (stepIndex); break;
                                case 4: safeThis->joinStepWithNext (stepIndex); break;
                                case 5:
                                    if (const auto* step = safeThis->m_pattern->getStep (stepIndex))
                                        safeThis->m_pattern->setStepDuration (stepIndex, step->duration - 1);
                                    break;
                                case 6:
                                    if (const auto* step = safeThis->m_pattern->getStep (stepIndex))
                                        safeThis->m_pattern->setStepDuration (stepIndex, step->duration + 1);
                                    break;
                                case 7: safeThis->clearStepContents (stepIndex); break;
                                case 8: safeThis->copySelectedStep(); return;
                                case 9: safeThis->pasteStepToSelection(); return;
                                case 10: safeThis->transposeSelectedStepContents (1); return;
                                case 11: safeThis->transposeSelectedStepContents (-1); return;
                                case 12: safeThis->selectedStepData()->followStructure = ! safeThis->selectedStepData()->followStructure; break;
                                case 13: safeThis->m_detailOpen = true; break;
                                case 14: safeThis->m_pattern->setPatternLength (safeThis->m_pattern->patternLength() - 1); break;
                                case 15: safeThis->m_pattern->setPatternLength (safeThis->m_pattern->patternLength() + 1); break;
                                case 16:
                                {
                                    const int duration = safeThis->selectedStepData() != nullptr ? safeThis->selectedStepData()->duration : 1;
                                    safeThis->m_pattern->addStepAfter (stepIndex, duration);
                                    safeThis->m_selectedStep = juce::jmin (stepIndex + 1, safeThis->m_pattern->activeStepCount() - 1);
                                    break;
                                }
                                case 20: safeThis->selectedStepData()->role = SlotPattern::StepRole::bass; break;
                                case 21: safeThis->selectedStepData()->role = SlotPattern::StepRole::chord; break;
                                case 22: safeThis->selectedStepData()->role = SlotPattern::StepRole::lead; break;
                                case 23: safeThis->selectedStepData()->role = SlotPattern::StepRole::fill; break;
                                case 24: safeThis->selectedStepData()->role = SlotPattern::StepRole::transition; break;
                                case 30: safeThis->selectedStepData()->noteMode = SlotPattern::NoteMode::absolute; break;
                                case 31: safeThis->selectedStepData()->noteMode = SlotPattern::NoteMode::degree; break;
                                case 32: safeThis->selectedStepData()->noteMode = SlotPattern::NoteMode::chordTone; break;
                                case 33: safeThis->selectedStepData()->noteMode = SlotPattern::NoteMode::interval; break;
                                default: break;
                            }

                            safeThis->commitChange();
                        });
}

void StepGridSingle::showNoteContextMenu (int eventIndex)
{
    if (m_pattern == nullptr || eventIndex < 0)
        return;

    juce::PopupMenu menu;
    menu.addItem (1, "Delete Note");
    menu.addItem (2, "Duplicate Note");
    menu.addItem (3, "Nudge Left");
    menu.addItem (4, "Nudge Right");
    menu.addItem (5, "Shorten");
    menu.addItem (6, "Lengthen");
    menu.addItem (7, "Velocity +10");
    menu.addItem (8, "Velocity -10");

    juce::Component::SafePointer<StepGridSingle> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safeThis, eventIndex] (int result)
                        {
                            if (safeThis == nullptr || safeThis->m_pattern == nullptr || result == 0)
                                return;

                            safeThis->m_selectedEvent = eventIndex;
                            switch (result)
                            {
                                case 1:
                                    safeThis->m_pattern->removeMicroEvent (safeThis->m_selectedStep, eventIndex);
                                    safeThis->m_selectedEvent = -1;
                                    safeThis->commitChange();
                                    return;
                                case 2: safeThis->duplicateNote (eventIndex); break;
                                case 3: safeThis->nudgeSelectedEvent (-0.03125f); return;
                                case 4: safeThis->nudgeSelectedEvent (0.03125f); return;
                                case 5: safeThis->adjustSelectedEventLength (-0.05f); return;
                                case 6: safeThis->adjustSelectedEventLength (0.05f); return;
                                case 7: safeThis->adjustSelectedEventVelocity (10); return;
                                case 8: safeThis->adjustSelectedEventVelocity (-10); return;
                                default: return;
                            }

                            safeThis->commitChange();
                        });
}

void StepGridSingle::showModeMenu()
{
    if (selectedStepData() == nullptr)
        return;

    juce::PopupMenu menu;
    menu.addItem (1, "Absolute");
    menu.addItem (2, "Degree");
    menu.addItem (3, "Chord Tone");
    menu.addItem (4, "Interval");

    juce::Component::SafePointer<StepGridSingle> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safeThis] (int result)
                        {
                            if (safeThis == nullptr || safeThis->m_pattern == nullptr || result == 0)
                                return;

                            if (auto* step = safeThis->selectedStepData())
                            {
                                step->noteMode = static_cast<SlotPattern::NoteMode> (result - 1);
                                step->harmonicSource = step->noteMode == SlotPattern::NoteMode::chordTone
                                                     ? SlotPattern::HarmonicSource::chord
                                                     : SlotPattern::HarmonicSource::key;
                                safeThis->commitChange();
                            }
                        });
}

void StepGridSingle::showRoleMenu()
{
    if (selectedStepData() == nullptr)
        return;

    juce::PopupMenu menu;
    menu.addItem (1, "Bass");
    menu.addItem (2, "Chord");
    menu.addItem (3, "Lead");
    menu.addItem (4, "Fill");
    menu.addItem (5, "Transition");

    juce::Component::SafePointer<StepGridSingle> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safeThis] (int result)
                        {
                            if (safeThis == nullptr || safeThis->m_pattern == nullptr || result == 0)
                                return;

                            if (auto* step = safeThis->selectedStepData())
                            {
                                step->role = static_cast<SlotPattern::StepRole> (result - 1);
                                safeThis->commitChange();
                            }
                        });
}

void StepGridSingle::deleteSelection()
{
    if (m_pattern == nullptr)
        return;

    if (m_selectedEvent >= 0)
    {
        m_pattern->removeMicroEvent (m_selectedStep, m_selectedEvent);
        m_selectedEvent = -1;
        commitChange();
        return;
    }

    clearStepContents (m_selectedStep);
    commitChange();
}

void StepGridSingle::copySelectedStep()
{
    if (const auto* step = selectedStepData())
    {
        m_stepClipboard = *step;
        m_hasStepClipboard = true;
        repaint();
    }
}

void StepGridSingle::pasteStepToSelection()
{
    auto* step = selectedStepData();
    if (step == nullptr || ! m_hasStepClipboard)
        return;

    const int preservedStart = step->start;
    const int preservedDuration = step->duration;
    *step = m_stepClipboard;
    step->start = preservedStart;
    step->duration = preservedDuration;
    commitChange();
}

void StepGridSingle::duplicateNote (int eventIndex)
{
    auto* step = selectedStepData();
    if (step == nullptr || ! juce::isPositiveAndBelow (eventIndex, step->microEventCount)
        || step->microEventCount >= SlotPattern::MAX_MICRO_EVENTS)
        return;

    const auto src = step->microEvents[(size_t) eventIndex];
    const int newIndex = m_pattern->addMicroEvent (m_selectedStep,
                                                   juce::jlimit (0.0f, 0.98f, src.timeOffset + 0.06f),
                                                   src.length,
                                                   src.pitchValue,
                                                   src.velocity);
    if (newIndex >= 0)
        m_selectedEvent = newIndex;
}

void StepGridSingle::transposeSelectedStepContents (int semitones)
{
    auto* step = selectedStepData();
    if (step == nullptr)
        return;

    for (int i = 0; i < step->microEventCount; ++i)
    {
        auto& event = step->microEvents[(size_t) i];
        if (step->noteMode == SlotPattern::NoteMode::absolute)
            event.pitchValue = juce::jlimit (0, 127, event.pitchValue + semitones);
        else
            event.pitchValue = juce::jlimit (SlotPattern::kMinTheoryValue, SlotPattern::kMaxTheoryValue, event.pitchValue + semitones);
    }

    commitChange();
}

void StepGridSingle::nudgeSelectedEvent (float delta)
{
    auto* step = selectedStepData();
    if (step == nullptr || ! juce::isPositiveAndBelow (m_selectedEvent, step->microEventCount))
        return;

    auto& event = step->microEvents[(size_t) m_selectedEvent];
    event.timeOffset = juce::jlimit (0.0f, 0.98f, event.timeOffset + delta);
    commitChange();
}

void StepGridSingle::adjustSelectedEventLength (float delta)
{
    auto* step = selectedStepData();
    if (step == nullptr || ! juce::isPositiveAndBelow (m_selectedEvent, step->microEventCount))
        return;

    auto& event = step->microEvents[(size_t) m_selectedEvent];
    event.length = juce::jlimit (0.05f, 1.0f, event.length + delta);
    commitChange();
}

void StepGridSingle::adjustSelectedEventVelocity (int delta)
{
    auto* step = selectedStepData();
    if (step == nullptr || ! juce::isPositiveAndBelow (m_selectedEvent, step->microEventCount))
        return;

    auto& event = step->microEvents[(size_t) m_selectedEvent];
    event.velocity = (uint8_t) juce::jlimit (1, 127, (int) event.velocity + delta);
    commitChange();
}

void StepGridSingle::updateHoverState (juce::Point<int> pos)
{
    const int oldHoverStep = m_hoverStep;
    const int oldHoverEvent = m_hoverEvent;
    const bool oldHoverHandle = m_hoverStepResizeHandle;

    m_hoverStep = stepAt (pos);
    m_hoverEvent = microEventAt (pos);
    m_hoverStepResizeHandle = m_hoverStep >= 0 && stepResizeHandleRect (m_hoverStep).contains ((float) pos.x, (float) pos.y);

    if (oldHoverStep != m_hoverStep || oldHoverEvent != m_hoverEvent || oldHoverHandle != m_hoverStepResizeHandle)
        repaint();
}

void StepGridSingle::mouseMove (const juce::MouseEvent& e)
{
    updateHoverState (e.getPosition());
}

void StepGridSingle::mouseDown (const juce::MouseEvent& e)
{
    if (m_pattern == nullptr)
        return;

    grabKeyboardFocus();
    ensureSelectionValid();
    const auto pos = e.getPosition();
    updateHoverState (pos);

    if ((m_detailOpen ? closeDetailRect() : editRect()).contains (pos))
    {
        m_detailOpen = ! m_detailOpen;
        m_dragMode = DragMode::none;
        repaint();
        return;
    }

    if (modeRect().contains (pos))
    {
        showModeMenu();
        return;
    }

    if (roleRect().contains (pos))
    {
        showRoleMenu();
        return;
    }

    if (followRect().contains (pos))
    {
        if (auto* step = selectedStepData())
        {
            step->followStructure = ! step->followStructure;
            commitChange();
        }
        return;
    }

    if (moreRect().contains (pos))
    {
        showStepContextMenu (m_selectedStep);
        return;
    }

    const int laneStep = stepAt (pos);
    if (laneStep >= 0)
    {
        m_selectedStep = laneStep;
        m_selectedEvent = -1;
        ensureSelectionValid();

        if (e.mods.isRightButtonDown())
        {
            showStepContextMenu (laneStep);
            repaint();
            return;
        }

        if (stepResizeHandleRect (laneStep).contains ((float) pos.x, (float) pos.y))
        {
            m_dragMode = DragMode::resizeStep;
            m_dragStartPos = pos;
            m_dragStepDurationStart = selectedStepData() != nullptr ? selectedStepData()->duration : 1;
            repaint();
            return;
        }

        repaint();
        return;
    }

    if (! m_detailOpen || ! detailRect().contains (pos))
        return;

    auto* step = selectedStepData();
    if (step == nullptr)
        return;

    const int hitEvent = microEventAt (pos);
    if (hitEvent >= 0)
    {
        m_selectedEvent = hitEvent;
        if (e.mods.isRightButtonDown())
        {
            showNoteContextMenu (hitEvent);
            return;
        }

        const auto rect = microEventRect (hitEvent);
        m_dragMode = std::abs (pos.x - juce::roundToInt (rect.getRight())) <= 6 ? DragMode::resizeEvent
                                                                                 : DragMode::moveEvent;
        m_dragStartPos = pos;
        m_dragEventOffsetStart = step->microEvents[(size_t) hitEvent].timeOffset;
        m_dragEventLengthStart = step->microEvents[(size_t) hitEvent].length;
        m_dragEventPitchStart = step->microEvents[(size_t) hitEvent].pitchValue < 0 ? valueForRow (kPitchRows / 2)
                                                                                     : step->microEvents[(size_t) hitEvent].pitchValue;
        repaint();
        return;
    }

    const int pitch = pitchFromDetailY (pos.y);
    m_selectedEvent = m_pattern->addMicroEvent (m_selectedStep,
                                                normalizedTimeFromDetailX (pos.x),
                                                0.18f,
                                                pitch,
                                                100);
    m_dragMode = DragMode::moveEvent;
    m_dragStartPos = pos;
    m_dragEventOffsetStart = normalizedTimeFromDetailX (pos.x);
    m_dragEventLengthStart = 0.18f;
    m_dragEventPitchStart = pitch;
    commitChange();
}

void StepGridSingle::mouseDrag (const juce::MouseEvent& e)
{
    if (m_pattern == nullptr || m_dragMode == DragMode::none)
        return;

    const int dx = e.getPosition().x - m_dragStartPos.x;
    const int dy = e.getPosition().y - m_dragStartPos.y;
    const float deltaNorm = detailRect().getWidth() > 1 ? (float) dx / (float) detailRect().getWidth() : 0.0f;

    if (m_dragMode == DragMode::resizeStep)
    {
        const auto lane = stepLaneRect();
        const auto* selectedStep = selectedStepData();
        if (selectedStep == nullptr || lane.getWidth() <= 0)
            return;

        const float unitsPerPixel = (float) juce::jmax (1, m_pattern->patternLength()) / (float) lane.getWidth();
        const int deltaUnits = juce::roundToInt ((float) dx * unitsPerPixel);
        m_pattern->setStepDuration (m_selectedStep, m_dragStepDurationStart + deltaUnits);
        commitChange();
        return;
    }

    auto* step = selectedStepData();
    if (step == nullptr || ! juce::isPositiveAndBelow (m_selectedEvent, step->microEventCount))
        return;

    float offset = m_dragEventOffsetStart;
    float length = m_dragEventLengthStart;
    int pitch = m_dragEventPitchStart;

    if (m_dragMode == DragMode::moveEvent)
    {
        offset = juce::jlimit (0.0f, 0.98f, m_dragEventOffsetStart + deltaNorm);
        pitch = juce::jlimit (0, 127, m_dragEventPitchStart - (int) std::round ((float) dy / (detailRect().getHeight() / (float) kPitchRows)));
    }
    else if (m_dragMode == DragMode::resizeEvent)
    {
        length = juce::jlimit (0.05f, 1.0f, m_dragEventLengthStart + deltaNorm);
    }

    m_pattern->updateMicroEvent (m_selectedStep, m_selectedEvent, offset, length, pitch, step->microEvents[(size_t) m_selectedEvent].velocity);
    commitChange();
}

void StepGridSingle::mouseUp (const juce::MouseEvent&)
{
    m_dragMode = DragMode::none;
}

void StepGridSingle::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (m_pattern == nullptr)
        return;

    grabKeyboardFocus();
    const auto pos = e.getPosition();
    const int hitEvent = microEventAt (pos);

    if (hitEvent >= 0)
    {
        m_selectedEvent = hitEvent;
        duplicateNote (hitEvent);
        commitChange();
        return;
    }

    const int laneStep = stepAt (pos);
    if (laneStep >= 0)
    {
        m_selectedStep = laneStep;
        m_detailOpen = true;
        const auto* step = selectedStepData();
        if (step != nullptr && step->microEventCount > 0)
        {
            m_selectedEvent = 0;
            repaint();
        }
        else
        {
            m_selectedEvent = m_pattern->addMicroEvent (m_selectedStep, 0.15f, 0.22f, valueForRow (kPitchRows / 2), 108);
            commitChange();
        }
        return;
    }

    if (m_detailOpen && detailRect().contains (pos))
    {
        m_selectedEvent = m_pattern->addMicroEvent (m_selectedStep,
                                                    normalizedTimeFromDetailX (pos.x),
                                                    0.24f,
                                                    pitchFromDetailY (pos.y),
                                                    110);
        commitChange();
    }
}

bool StepGridSingle::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelection();
        return true;
    }

    if (key.getModifiers().isCommandDown() && key.getTextCharacter() == 'c')
    {
        copySelectedStep();
        return true;
    }

    if (key.getModifiers().isCommandDown() && key.getTextCharacter() == 'v')
    {
        pasteStepToSelection();
        return true;
    }

    if (key.getModifiers().isCommandDown() && key.getTextCharacter() == 'd')
    {
        duplicateStep (m_selectedStep);
        commitChange();
        return true;
    }

    if (key.getModifiers().isAltDown() && key.getKeyCode() == juce::KeyPress::upKey)
    {
        transposeSelectedStepContents (1);
        return true;
    }

    if (key.getModifiers().isAltDown() && key.getKeyCode() == juce::KeyPress::downKey)
    {
        transposeSelectedStepContents (-1);
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::leftKey)
    {
        nudgeSelectedEvent (-0.03125f);
        return true;
    }

    if (key.getKeyCode() == juce::KeyPress::rightKey)
    {
        nudgeSelectedEvent (0.03125f);
        return true;
    }

    return false;
}

juce::MouseCursor StepGridSingle::getMouseCursor()
{
    if (m_hoverStepResizeHandle)
        return juce::MouseCursor::LeftRightResizeCursor;
    return juce::MouseCursor::CrosshairCursor;
}

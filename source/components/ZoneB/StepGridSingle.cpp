
#include "StepGridSingle.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
std::array<int, 7> intervalsForMode (const juce::String& mode)
{
    const auto lower = mode.trim().toLowerCase();
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

void StepGridSingle::setPattern (SlotPattern* pattern, juce::Colour groupColor, int ownerSlotIndex)
{
    m_pattern = pattern;
    m_groupColor = groupColor;
    m_ownerSlotIndex = ownerSlotIndex;
    m_selectedUnit = 0;
    m_selectedStep = 0;
    m_selectedEvent = -1;
    m_pageIndex = 0;
    m_rowMuted.fill (false);
    for (auto& row : m_rowMuteHadEvent)
        row.fill (false);
    for (auto& row : m_rowMuteGateSnapshot)
        row.fill (SlotPattern::GateMode::gate);
    ensureSelectionValid();
    repaint();
}

void StepGridSingle::clearPattern()
{
    m_pattern = nullptr;
    m_ownerSlotIndex = -1;
    m_playhead = -1;
    m_selectedUnit = 0;
    m_selectedStep = 0;
    m_selectedEvent = -1;
    m_pageIndex = 0;
    m_hoverColumn = -1;
    m_hoverRow = -1;
    m_rowMuted.fill (false);
    for (auto& row : m_rowMuteHadEvent)
        row.fill (false);
    for (auto& row : m_rowMuteGateSnapshot)
        row.fill (SlotPattern::GateMode::gate);
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

bool StepGridSingle::isSelectedStepFollowingStructure() const noexcept
{
    if (const auto* step = selectedStepData())
        return step->followStructure;

    return m_followingStructure;
}

juce::Rectangle<int> StepGridSingle::contentRect() const noexcept
{
    return getLocalBounds().reduced (kPad);
}

juce::Rectangle<int> StepGridSingle::headerRect() const noexcept
{
    return contentRect().removeFromTop (kHeaderH);
}

juce::Rectangle<int> StepGridSingle::gridRect() const noexcept
{
    auto area = contentRect();
    area.removeFromTop (kHeaderH + 2);
    area.removeFromBottom (kInspectorH + 2);
    return area;
}

juce::Rectangle<int> StepGridSingle::overviewRect() const noexcept
{
    auto area = gridRect();
    area.removeFromBottom (2);
    return area.removeFromTop (juce::jmin (overviewHeight(), juce::jmax (0, area.getHeight())));
}

juce::Rectangle<int> StepGridSingle::noteGridRect() const noexcept
{
    auto area = gridRect();
    area.removeFromTop (juce::jmin (overviewHeight() + 2, juce::jmax (0, area.getHeight())));
    return area;
}

juce::Rectangle<int> StepGridSingle::inspectorRect() const noexcept
{
    auto area = contentRect();
    area.removeFromTop (kHeaderH + 2);
    area.removeFromTop (juce::jmax (0, area.getHeight() - kInspectorH));
    return area;
}

int StepGridSingle::overviewRowCount() const noexcept
{
    return juce::jlimit (1, kOverviewRows, pageCount());
}

int StepGridSingle::overviewHeight() const noexcept
{
    const int rows = overviewRowCount();
    return rows * kOverviewRowH + kOverviewPadY * 2;
}

int StepGridSingle::overviewLabelWidth() const noexcept
{
    return getWidth() < 720 ? 58 : 66;
}

juce::Rectangle<int> StepGridSingle::overviewLabelsRect() const noexcept
{
    const auto ov = overviewRect();
    return ov.withWidth (juce::jmin (overviewLabelWidth(), ov.getWidth()));
}

juce::Rectangle<int> StepGridSingle::overviewCellsRect() const noexcept
{
    const auto ov = overviewRect();
    return ov.withTrimmedLeft (juce::jmin (overviewLabelWidth(), ov.getWidth()));
}

juce::Rectangle<int> StepGridSingle::overviewRowRect (int row) const noexcept
{
    const auto ovCells = overviewCellsRect();
    const int rows = overviewRowCount();
    if (! juce::isPositiveAndBelow (row, rows) || ovCells.isEmpty())
        return {};

    const int rowY = ovCells.getY() + kOverviewPadY + row * kOverviewRowH;
    return { ovCells.getX(), rowY, ovCells.getWidth(), kOverviewRowH };
}

juce::Rectangle<int> StepGridSingle::overviewRowLabelRect (int row) const noexcept
{
    const auto ovLabels = overviewLabelsRect();
    const int rows = overviewRowCount();
    if (! juce::isPositiveAndBelow (row, rows) || ovLabels.isEmpty())
        return {};

    const int rowY = ovLabels.getY() + kOverviewPadY + row * kOverviewRowH;
    return { ovLabels.getX(), rowY, ovLabels.getWidth(), kOverviewRowH };
}

juce::Rectangle<int> StepGridSingle::overviewRowRouteRect (int row) const noexcept
{
    auto rr = overviewRowLabelRect (row);
    if (rr.isEmpty())
        return {};

    rr = rr.reduced (2, 1);
    const int reserveW = rr.getWidth() < 58 ? 14 : 28;
    return rr.withTrimmedRight (reserveW);
}

juce::Rectangle<int> StepGridSingle::overviewRowFollowRect (int row) const noexcept
{
    auto rr = overviewRowLabelRect (row);
    if (rr.isEmpty())
        return {};

    const int btnW = rr.getWidth() < 58 ? 12 : 14;
    const int btnH = juce::jmax (9, rr.getHeight() - 2);
    return { rr.getRight() - btnW * 2 - 3, rr.getY() + (rr.getHeight() - btnH) / 2, btnW, btnH };
}

juce::Rectangle<int> StepGridSingle::overviewRowMuteRect (int row) const noexcept
{
    auto rr = overviewRowLabelRect (row);
    if (rr.isEmpty())
        return {};

    const int btnW = rr.getWidth() < 58 ? 12 : 14;
    const int btnH = juce::jmax (9, rr.getHeight() - 2);
    return { rr.getRight() - btnW - 1, rr.getY() + (rr.getHeight() - btnH) / 2, btnW, btnH };
}

juce::Rectangle<int> StepGridSingle::pagePrevRect() const noexcept
{
    auto h = headerRect();
    return { h.getX(), h.getY(), 14, h.getHeight() };
}

juce::Rectangle<int> StepGridSingle::pageLabelRect() const noexcept
{
    return pagePrevRect().translated (16, 0).withWidth (60);
}

juce::Rectangle<int> StepGridSingle::pageNextRect() const noexcept
{
    return pageLabelRect().translated (62, 0).withWidth (14);
}

juce::Rectangle<int> StepGridSingle::rowChipRect (int rowPage) const noexcept
{
    const bool showRowChips = pageCount() > 1 && headerRect().getWidth() >= 620;
    if (! showRowChips)
        return {};

    const int page = juce::jlimit (0, juce::jmax (0, pageCount() - 1), rowPage);
    const int chipW = 18;
    const int chipGap = 2;
    const auto startX = pageNextRect().getRight() + 4;
    return { startX + page * (chipW + chipGap), headerRect().getY(), chipW, headerRect().getHeight() };
}

juce::Rectangle<int> StepGridSingle::tabRect (LaneView view) const noexcept
{
    const int idx = static_cast<int> (view);
    const auto h = headerRect();
    int left = pageNextRect().getRight() + 8;
    if (const auto lastChip = rowChipRect (pageCount() - 1); ! lastChip.isEmpty())
        left = lastChip.getRight() + 6;

    const auto modeR = modeRect();
    const auto followR = followRect();

    int right = moreRect().getX() - 4;
    if (! modeR.isEmpty())
        right = modeR.getX() - 4;
    else if (! followR.isEmpty())
        right = followR.getX() - 4;

    right = juce::jmax (left + 40, right);
    const int gap = 2;
    const int tabW = juce::jlimit (22, 44, (right - left - gap * 3) / 4);
    const int used = tabW * 4 + gap * 3;
    const int startX = left + juce::jmax (0, (right - left - used) / 2);
    return { startX + idx * (tabW + gap), h.getY(), tabW, h.getHeight() };
}

juce::Rectangle<int> StepGridSingle::modeRect() const noexcept
{
    auto h = headerRect();
    if (h.getWidth() < 450)
        return {};
    if (h.getWidth() < 620)
        return { h.getRight() - 126, h.getY(), 56, h.getHeight() };
    return { h.getRight() - 230, h.getY(), 74, h.getHeight() };
}

juce::Rectangle<int> StepGridSingle::roleRect() const noexcept
{
    if (headerRect().getWidth() < 620)
        return {};
    return modeRect().translated (76, 0).withWidth (62);
}

juce::Rectangle<int> StepGridSingle::followRect() const noexcept
{
    const auto h = headerRect();
    if (h.getWidth() < 450)
        return { h.getRight() - 68, h.getY(), 34, h.getHeight() };
    if (h.getWidth() < 620)
        return { h.getRight() - 68, h.getY(), 56, h.getHeight() };
    return roleRect().translated (64, 0).withWidth (56);
}

juce::Rectangle<int> StepGridSingle::moreRect() const noexcept
{
    auto h = headerRect();
    return { h.getRight() - 34, h.getY(), 34, h.getHeight() };
}

int StepGridSingle::totalUnits() const noexcept
{
    if (m_pattern == nullptr)
        return kCols;

    return juce::jlimit (1, 64, juce::jmax (1, m_pattern->patternLength()));
}

int StepGridSingle::pageCount() const noexcept
{
    return juce::jmax (1, (totalUnits() + kCols - 1) / kCols);
}

int StepGridSingle::pageStartUnit() const noexcept
{
    return juce::jlimit (0, juce::jmax (0, totalUnits() - 1), m_pageIndex * kCols);
}

int StepGridSingle::pageForUnit (int unit) const noexcept
{
    return juce::jlimit (0, pageCount() - 1, juce::jmax (0, unit) / kCols);
}

int StepGridSingle::columnForUnit (int unit) const noexcept
{
    const int start = pageStartUnit();
    if (unit < start || unit >= start + kCols)
        return -1;

    return unit - start;
}

int StepGridSingle::unitForColumn (int column) const noexcept
{
    return juce::jlimit (0, juce::jmax (0, totalUnits() - 1), pageStartUnit() + juce::jlimit (0, kCols - 1, column));
}

int StepGridSingle::cellColumnAt (juce::Point<int> pos) const noexcept
{
    const auto g = noteGridRect();
    const auto cells = g.withTrimmedLeft (kLabelW);
    if (! cells.contains (pos) || cells.getWidth() <= 0)
        return -1;

    const float cellW = cells.getWidth() / (float) kCols;
    return juce::jlimit (0, kCols - 1, (int) std::floor ((pos.x - cells.getX()) / cellW));
}

int StepGridSingle::cellRowAt (juce::Point<int> pos) const noexcept
{
    const auto g = noteGridRect();
    if (! g.contains (pos) || g.getHeight() <= 0)
        return -1;

    const float cellH = g.getHeight() / (float) kRows;
    return juce::jlimit (0, kRows - 1, (int) std::floor ((pos.y - g.getY()) / cellH));
}

juce::Rectangle<int> StepGridSingle::cellRect (int row, int col) const noexcept
{
    const auto g = noteGridRect();
    const auto cells = g.withTrimmedLeft (kLabelW);
    if (cells.isEmpty())
        return {};

    const float cellW = cells.getWidth() / (float) kCols;
    const float cellH = g.getHeight() / (float) kRows;

    const int x1 = juce::roundToInt (cells.getX() + (float) col * cellW);
    const int x2 = juce::roundToInt (cells.getX() + (float) (col + 1) * cellW);
    const int y1 = juce::roundToInt (g.getY() + (float) row * cellH);
    const int y2 = juce::roundToInt (g.getY() + (float) (row + 1) * cellH);

    return { x1, y1, juce::jmax (1, x2 - x1), juce::jmax (1, y2 - y1) };
}

int StepGridSingle::midiForRow (int row) const noexcept
{
    return juce::jlimit (0, 127, m_pitchBase + (kRows - 1 - juce::jlimit (0, kRows - 1, row)));
}

int StepGridSingle::rowForMidi (int midiNote) const noexcept
{
    const int clamped = juce::jlimit (0, 127, midiNote);
    return juce::jlimit (0, kRows - 1, (m_pitchBase + (kRows - 1)) - clamped);
}

int StepGridSingle::velocityForRow (int row) const noexcept
{
    const float t = juce::jlimit (0.0f, 1.0f, (float) juce::jlimit (0, kRows - 1, row) / (float) juce::jmax (1, kRows - 1));
    return juce::jlimit (1, 127, juce::roundToInt (127.0f - t * 126.0f));
}

int StepGridSingle::rowForVelocity (int velocity) const noexcept
{
    const float t = juce::jlimit (0.0f, 1.0f, (127.0f - (float) juce::jlimit (1, 127, velocity)) / 126.0f);
    return juce::jlimit (0, kRows - 1, juce::roundToInt (t * (float) juce::jmax (1, kRows - 1)));
}

const SlotPattern::Step* StepGridSingle::selectedStepData() const noexcept
{
    return m_pattern != nullptr ? m_pattern->getStep (m_selectedStep) : nullptr;
}

SlotPattern::Step* StepGridSingle::selectedStepData() noexcept
{
    return m_pattern != nullptr ? m_pattern->getStep (m_selectedStep) : nullptr;
}

int StepGridSingle::findStepIndexForUnit (int unit) const noexcept
{
    if (m_pattern == nullptr)
        return -1;

    const int clampedUnit = juce::jlimit (0, juce::jmax (0, totalUnits() - 1), unit);
    for (int i = 0; i < m_pattern->activeStepCount(); ++i)
    {
        const auto* step = m_pattern->getStep (i);
        if (step == nullptr)
            continue;

        const int start = step->start;
        const int end = step->start + juce::jmax (1, step->duration);
        if (clampedUnit >= start && clampedUnit < end)
            return i;
    }

    return juce::jlimit (0, juce::jmax (0, m_pattern->activeStepCount() - 1), clampedUnit);
}

int StepGridSingle::eventUnitFor (const SlotPattern::Step& step, const SlotPattern::MicroEvent& event) const noexcept
{
    const int dur = juce::jmax (1, step.duration);
    const int local = juce::jlimit (0, dur - 1, (int) std::floor (event.timeOffset * (float) dur));
    return step.start + local;
}

int StepGridSingle::eventMidiForDisplay (const SlotPattern::Step& step, const SlotPattern::MicroEvent& event) const noexcept
{
    if (step.noteMode == SlotPattern::NoteMode::absolute)
    {
        const int fallback = step.anchorValue < 0 ? 60 : step.anchorValue;
        return juce::jlimit (0, 127, event.pitchValue < 0 ? fallback : event.pitchValue);
    }

    return realizedPitchForPreview (step, event);
}

StepGridSingle::UnitEventRef StepGridSingle::findEventAtCell (int unit, int row) const noexcept
{
    const int stepIdx = findStepIndexForUnit (unit);
    if (stepIdx < 0)
        return {};

    const auto* step = m_pattern->getStep (stepIdx);
    if (step == nullptr)
        return {};

    for (int i = 0; i < step->microEventCount; ++i)
    {
        const auto& event = step->microEvents[(size_t) i];
        if (eventUnitFor (*step, event) != unit)
            continue;

        if (rowForMidi (eventMidiForDisplay (*step, event)) == row)
            return { stepIdx, i };
    }

    return {};
}

StepGridSingle::UnitEventRef StepGridSingle::firstEventInUnit (int unit) const noexcept
{
    const int stepIdx = findStepIndexForUnit (unit);
    if (stepIdx < 0)
        return {};

    const auto* step = m_pattern->getStep (stepIdx);
    if (step == nullptr)
        return {};

    for (int i = 0; i < step->microEventCount; ++i)
    {
        if (eventUnitFor (*step, step->microEvents[(size_t) i]) == unit)
            return { stepIdx, i };
    }

    return {};
}
int StepGridSingle::storedPitchForMode (const SlotPattern::Step& step, int midiNote) const noexcept
{
    const int clampedMidi = juce::jlimit (0, 127, midiNote);

    switch (step.noteMode)
    {
        case SlotPattern::NoteMode::absolute:
            return clampedMidi;

        case SlotPattern::NoteMode::degree:
        {
            const int rootPc = rootToPitchClass (m_keyRoot);
            const int pc = (clampedMidi % 12 + 12) % 12;
            const int semitone = (pc - rootPc + 12) % 12;
            const int octave = clampedMidi / 12;
            return juce::jlimit (SlotPattern::kMinTheoryValue, SlotPattern::kMaxTheoryValue, octave * 12 + semitone);
        }

        case SlotPattern::NoteMode::chordTone:
        {
            const juce::String harmonicChord = (step.harmonicSource == SlotPattern::HarmonicSource::nextChord || step.lookAheadToNextChord)
                                                 ? m_nextChord
                                                 : m_currentChord;
            const auto intervals = chordIntervalsForLabel (harmonicChord);
            const int chordSize = juce::jmax (1, (int) intervals.size());
            const int rootPc = rootToPitchClass (step.harmonicSource == SlotPattern::HarmonicSource::key
                                                   ? m_keyRoot
                                                   : chordRootString (harmonicChord));
            const int rootAnchor = nearestNoteForPitchClass (clampedMidi, rootPc);

            int best = 0;
            int bestDistance = std::numeric_limits<int>::max();
            for (int octave = 0; octave < 5; ++octave)
            {
                for (int idx = 0; idx < chordSize; ++idx)
                {
                    const int candidate = rootAnchor + intervals[(size_t) idx] + octave * 12;
                    const int dist = std::abs (candidate - clampedMidi);
                    if (dist < bestDistance)
                    {
                        bestDistance = dist;
                        best = octave * chordSize + idx;
                    }
                }
            }

            return juce::jlimit (0, SlotPattern::kMaxTheoryValue, best);
        }

        case SlotPattern::NoteMode::interval:
        default:
        {
            const int base = step.anchorValue < 0 ? 60 : juce::jlimit (0, 127, step.anchorValue);
            return juce::jlimit (SlotPattern::kMinTheoryValue,
                                 SlotPattern::kMaxTheoryValue,
                                 clampedMidi - base);
        }
    }
}

int StepGridSingle::addEventAtCell (int unit, int row, int velocity)
{
    if (m_pattern == nullptr)
        return -1;

    const int stepIdx = findStepIndexForUnit (unit);
    auto* step = m_pattern->getStep (stepIdx);
    if (step == nullptr)
        return -1;

    if (step->microEventCount >= SlotPattern::MAX_MICRO_EVENTS)
        return -1;

    if (m_followingStructure && ! m_locallyOverriding)
        step->followStructure = true;

    const int localUnit = juce::jlimit (0, juce::jmax (0, step->duration - 1), unit - step->start);
    const float offset = juce::jlimit (0.0f, 0.98f, ((float) localUnit + 0.5f) / (float) juce::jmax (1, step->duration));
    const float length = juce::jlimit (0.08f, 1.0f, 0.8f / (float) juce::jmax (1, step->duration));
    const int midi = midiForRow (row);
    const int storedPitch = storedPitchForMode (*step, midi);

    const int eventIndex = m_pattern->addMicroEvent (stepIdx,
                                                     offset,
                                                     length,
                                                     storedPitch,
                                                     (uint8_t) juce::jlimit (1, 127, velocity));
    if (eventIndex >= 0)
    {
        m_selectedUnit = unit;
        m_selectedStep = stepIdx;
        m_selectedEvent = eventIndex;
    }

    return eventIndex;
}

void StepGridSingle::addChordAtUnit (int unit, int rootRow, int velocity)
{
    const int stepIdx = findStepIndexForUnit (unit);
    auto* step = m_pattern != nullptr ? m_pattern->getStep (stepIdx) : nullptr;
    if (step == nullptr)
        return;

    const juce::String harmonicChord = (step->harmonicSource == SlotPattern::HarmonicSource::nextChord || step->lookAheadToNextChord)
                                         ? m_nextChord
                                         : m_currentChord;

    auto intervals = chordIntervalsForLabel (harmonicChord);
    if (intervals.empty())
        intervals = { 0, 4, 7 };

    const int count = juce::jmin (3, (int) intervals.size());
    const int rootPc = rootToPitchClass (step->harmonicSource == SlotPattern::HarmonicSource::key
                                           ? m_keyRoot
                                           : chordRootString (harmonicChord));
    const int rootMidi = midiForRow (rootRow);
    const int rootAnchor = nearestNoteForPitchClass (rootMidi, rootPc);

    for (int i = 0; i < count; ++i)
    {
        const int note = juce::jlimit (0, 127, rootAnchor + intervals[(size_t) i]);
        const int row = rowForMidi (note);
        const auto existing = findEventAtCell (unit, row);
        if (existing.stepIndex < 0)
            addEventAtCell (unit, row, velocity);
    }
}

void StepGridSingle::removeEventRef (const UnitEventRef& ref)
{
    if (m_pattern == nullptr || ref.stepIndex < 0 || ref.eventIndex < 0)
        return;

    m_pattern->removeMicroEvent (ref.stepIndex, ref.eventIndex);
    if (m_selectedStep == ref.stepIndex && m_selectedEvent == ref.eventIndex)
        m_selectedEvent = -1;
}

void StepGridSingle::setVelocityAtCell (int unit, int row)
{
    if (m_pattern == nullptr)
        return;

    auto ref = firstEventInUnit (unit);
    if (ref.stepIndex < 0)
    {
        const int created = addEventAtCell (unit, kRows / 2, velocityForRow (row));
        if (created < 0)
            return;

        ref = firstEventInUnit (unit);
    }

    auto* step = m_pattern->getStep (ref.stepIndex);
    if (step == nullptr || ! juce::isPositiveAndBelow (ref.eventIndex, step->microEventCount))
        return;

    auto& event = step->microEvents[(size_t) ref.eventIndex];
    event.velocity = (uint8_t) velocityForRow (row);
    m_selectedUnit = unit;
    m_selectedStep = ref.stepIndex;
    m_selectedEvent = ref.eventIndex;
}

void StepGridSingle::ensureSelectionValid()
{
    if (m_pattern == nullptr)
        return;

    const int units = totalUnits();
    m_selectedUnit = juce::jlimit (0, units - 1, m_selectedUnit);

    m_selectedStep = findStepIndexForUnit (m_selectedUnit);
    m_selectedStep = juce::jlimit (0, juce::jmax (0, m_pattern->activeStepCount() - 1), m_selectedStep);

    if (const auto* step = selectedStepData())
    {
        if (! juce::isPositiveAndBelow (m_selectedEvent, step->microEventCount))
        {
            const auto ref = firstEventInUnit (m_selectedUnit);
            m_selectedEvent = ref.stepIndex == m_selectedStep ? ref.eventIndex : -1;
        }
    }
    else
    {
        m_selectedEvent = -1;
    }

    m_pageIndex = juce::jlimit (0, pageCount() - 1, m_pageIndex);
    const int pageForSelection = m_selectedUnit / kCols;
    if (pageForSelection != m_pageIndex)
        m_pageIndex = pageForSelection;
}

void StepGridSingle::commitChange()
{
    if (m_pattern == nullptr)
        return;

    ensureSelectionValid();
    repaint();
    if (onModified)
        onModified();
}

void StepGridSingle::showModeMenu()
{
    if (selectedStepData() == nullptr)
        return;

    juce::PopupMenu menu;
    menu.addItem (1, "Absolute");
    menu.addItem (2, "Degree");
    menu.addItem (3, "Chord Tone");

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

void StepGridSingle::showEventContextMenu (const UnitEventRef& ref)
{
    if (m_pattern == nullptr || ref.stepIndex < 0 || ref.eventIndex < 0)
        return;

    juce::PopupMenu menu;
    menu.addItem (1, "Delete Note");
    menu.addItem (2, "Velocity +10");
    menu.addItem (3, "Velocity -10");

    juce::Component::SafePointer<StepGridSingle> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safeThis, ref] (int result)
                        {
                            if (safeThis == nullptr || safeThis->m_pattern == nullptr || result == 0)
                                return;

                            auto* step = safeThis->m_pattern->getStep (ref.stepIndex);
                            if (step == nullptr || ! juce::isPositiveAndBelow (ref.eventIndex, step->microEventCount))
                                return;

                            if (result == 1)
                            {
                                safeThis->m_pattern->removeMicroEvent (ref.stepIndex, ref.eventIndex);
                                safeThis->commitChange();
                                return;
                            }

                            auto& event = step->microEvents[(size_t) ref.eventIndex];
                            int velocity = event.velocity;
                            velocity += (result == 2 ? 10 : -10);
                            event.velocity = (uint8_t) juce::jlimit (1, 127, velocity);
                            safeThis->commitChange();
                        });
}

void StepGridSingle::showMoreMenu()
{
    if (m_pattern == nullptr)
        return;

    juce::PopupMenu menu;
    menu.addItem (1, "Copy Step");
    menu.addItem (2, "Paste Step", m_hasStepClipboard);
    menu.addItem (3, "Clear Step Notes");
    menu.addSeparator();
    menu.addItem (4, "Pattern Shorter [");
    menu.addItem (5, "Pattern Longer ]");
    menu.addSeparator();
    menu.addItem (6, "Prev Page");
    menu.addItem (7, "Next Page");

    juce::Component::SafePointer<StepGridSingle> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safeThis] (int result)
                        {
                            if (safeThis == nullptr || safeThis->m_pattern == nullptr || result == 0)
                                return;

                            auto* step = safeThis->selectedStepData();
                            switch (result)
                            {
                                case 1:
                                    if (step != nullptr)
                                    {
                                        safeThis->m_stepClipboard = *step;
                                        safeThis->m_hasStepClipboard = true;
                                        safeThis->repaint();
                                    }
                                    return;

                                case 2:
                                    if (step != nullptr && safeThis->m_hasStepClipboard)
                                    {
                                        const int keepStart = step->start;
                                        const int keepDur = step->duration;
                                        *step = safeThis->m_stepClipboard;
                                        step->start = keepStart;
                                        step->duration = keepDur;
                                        safeThis->commitChange();
                                    }
                                    return;

                                case 3:
                                    if (step != nullptr)
                                    {
                                        step->microEventCount = 0;
                                        step->gateMode = SlotPattern::GateMode::rest;
                                        safeThis->commitChange();
                                    }
                                    return;

                                case 4:
                                    safeThis->m_pattern->setPatternLength (safeThis->m_pattern->patternLength() - 1);
                                    safeThis->commitChange();
                                    return;

                                case 5:
                                    safeThis->m_pattern->setPatternLength (safeThis->m_pattern->patternLength() + 1);
                                    safeThis->commitChange();
                                    return;

                                case 6:
                                    safeThis->m_pageIndex = juce::jmax (0, safeThis->m_pageIndex - 1);
                                    safeThis->repaint();
                                    return;

                                case 7:
                                    safeThis->m_pageIndex = juce::jmin (safeThis->pageCount() - 1, safeThis->m_pageIndex + 1);
                                    safeThis->repaint();
                                    return;
                            }
                        });
}

void StepGridSingle::showRowRouteMenu (int row)
{
    if (m_pattern == nullptr || ! juce::isPositiveAndBelow (row, SlotPattern::kRouteRows))
        return;

    juce::PopupMenu menu;
    const int currentTarget = m_pattern->getRowTargetSlot (row);
    menu.addItem (1, "Route to SELF", true, currentTarget == SlotPattern::kRouteSelf);
    menu.addSeparator();

    static constexpr int kMaxSlots = 8;
    for (int slot = 0; slot < kMaxSlots; ++slot)
    {
        const int itemId = 100 + slot;
        const bool enabled = slot != m_ownerSlotIndex;
        juce::String label = "Route to SLOT " + juce::String (slot + 1);
        if (! enabled)
            label += " (THIS)";

        menu.addItem (itemId, label, enabled, currentTarget == slot);
    }

    juce::Component::SafePointer<StepGridSingle> safeThis (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safeThis, row] (int result)
                        {
                            if (safeThis == nullptr || safeThis->m_pattern == nullptr || result == 0)
                                return;

                            int target = SlotPattern::kRouteSelf;
                            if (result >= 100)
                                target = result - 100;

                            safeThis->m_pattern->setRowTargetSlot (row, target);
                            safeThis->commitChange();
                        });
}

void StepGridSingle::toggleRowFollow (int row)
{
    if (m_pattern == nullptr || ! juce::isPositiveAndBelow (row, SlotPattern::kRouteRows))
        return;

    const int rowStart = row * kCols;
    const int rowEnd = juce::jmin (totalUnits(), rowStart + kCols);
    bool hasEvents = false;
    bool allFollow = true;

    for (int unit = rowStart; unit < rowEnd; ++unit)
    {
        const int stepIndex = findStepIndexForUnit (unit);
        auto* step = m_pattern->getStep (stepIndex);
        if (step == nullptr || step->start != unit || step->microEventCount <= 0)
            continue;

        hasEvents = true;
        if (! step->followStructure)
            allFollow = false;
    }

    if (! hasEvents)
        return;

    const bool setFollow = ! allFollow;
    for (int unit = rowStart; unit < rowEnd; ++unit)
    {
        const int stepIndex = findStepIndexForUnit (unit);
        auto* step = m_pattern->getStep (stepIndex);
        if (step == nullptr || step->start != unit || step->microEventCount <= 0)
            continue;

        step->followStructure = setFollow;
    }

    commitChange();
}

void StepGridSingle::toggleRowMute (int row)
{
    if (m_pattern == nullptr || ! juce::isPositiveAndBelow (row, SlotPattern::kRouteRows))
        return;

    const int rowStart = row * kCols;
    const int rowEnd = juce::jmin (totalUnits(), rowStart + kCols);
    bool changed = false;

    if (m_rowMuted[(size_t) row])
    {
        for (int col = 0; col < kCols; ++col)
        {
            const int unit = rowStart + col;
            if (unit >= rowEnd)
                break;

            const int stepIndex = findStepIndexForUnit (unit);
            auto* step = m_pattern->getStep (stepIndex);
            if (step == nullptr || step->start != unit || step->microEventCount <= 0)
                continue;

            if (m_rowMuteHadEvent[(size_t) row][(size_t) col])
            {
                step->gateMode = m_rowMuteGateSnapshot[(size_t) row][(size_t) col];
                changed = true;
            }
        }

        m_rowMuted[(size_t) row] = false;
        if (changed)
            commitChange();
        else
            repaint();
        return;
    }

    bool hadAnyEvents = false;
    for (int col = 0; col < kCols; ++col)
    {
        m_rowMuteHadEvent[(size_t) row][(size_t) col] = false;
        m_rowMuteGateSnapshot[(size_t) row][(size_t) col] = SlotPattern::GateMode::gate;
    }

    for (int col = 0; col < kCols; ++col)
    {
        const int unit = rowStart + col;
        if (unit >= rowEnd)
            break;

        const int stepIndex = findStepIndexForUnit (unit);
        auto* step = m_pattern->getStep (stepIndex);
        if (step == nullptr || step->start != unit || step->microEventCount <= 0)
            continue;

        hadAnyEvents = true;
        m_rowMuteHadEvent[(size_t) row][(size_t) col] = true;
        m_rowMuteGateSnapshot[(size_t) row][(size_t) col] = step->gateMode;
        step->gateMode = SlotPattern::GateMode::rest;
        changed = true;
    }

    if (! hadAnyEvents)
    {
        repaint();
        return;
    }

    m_rowMuted[(size_t) row] = true;
    if (changed)
        commitChange();
    else
        repaint();
}
void StepGridSingle::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    if (m_pattern == nullptr)
    {
        g.setColour (Theme::Colour::inkGhost);
        g.setFont (Theme::Font::micro());
        g.drawText ("select a slot to edit the sequence timeline", getLocalBounds(), juce::Justification::centred, false);
        return;
    }

    paintHeader (g);
    paintGrid (g);
    paintInspector (g);
}

void StepGridSingle::paintHeader (juce::Graphics& g) const
{
    const auto h = headerRect();
    const bool showRowChips = pageCount() > 1 && h.getWidth() >= 620;
    g.setColour (Theme::Colour::surface0);
    g.fillRect (h);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.75f));
    g.drawRect (h, 1);

    auto drawButton = [&] (juce::Rectangle<int> r, const juce::String& text, bool active, bool enabled = true)
    {
        if (r.isEmpty())
            return;

        g.setColour (active ? m_groupColor.withAlpha (0.24f) : Theme::Colour::surface2.withAlpha (0.9f));
        g.fillRoundedRectangle (r.toFloat().reduced (0.5f, 1.0f), Theme::Radius::chip);
        g.setColour ((active ? m_groupColor : Theme::Colour::surfaceEdge).withAlpha (0.9f));
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f, 1.0f), Theme::Radius::chip, Theme::Stroke::normal);
        g.setColour (enabled ? Theme::Colour::inkMid : Theme::Colour::inkGhost.withAlpha (0.55f));
        g.setFont (Theme::Font::micro());
        g.drawText (text, r, juce::Justification::centred, false);
    };

    drawButton (pagePrevRect(), "<", false, m_pageIndex > 0);
    drawButton (pageLabelRect(), juce::String (m_pageIndex + 1) + "/" + juce::String (pageCount()), false);
    drawButton (pageNextRect(), ">", false, m_pageIndex < pageCount() - 1);

    if (showRowChips)
    {
        for (int rp = 0; rp < pageCount(); ++rp)
        {
            const int rowStart = rp * kCols;
            const int rowEnd = juce::jmin (totalUnits(), rowStart + kCols);
            const bool playheadOnRow = (m_playhead >= rowStart && m_playhead < rowEnd);
            drawButton (rowChipRect (rp),
                        "R" + juce::String (rp + 1),
                        rp == m_pageIndex || playheadOnRow,
                        true);
        }
    }

    drawButton (tabRect (LaneView::notes), laneViewLabel (LaneView::notes), m_view == LaneView::notes);
    drawButton (tabRect (LaneView::chord), laneViewLabel (LaneView::chord), m_view == LaneView::chord);
    drawButton (tabRect (LaneView::velocity), laneViewLabel (LaneView::velocity), m_view == LaneView::velocity);
    drawButton (tabRect (LaneView::midiFx), laneViewLabel (LaneView::midiFx), m_view == LaneView::midiFx);

    const auto* step = selectedStepData();
    const auto modeR = modeRect();
    const auto roleR = roleRect();
    const auto followR = followRect();
    const auto modeText = step != nullptr
                            ? (modeR.getWidth() < 64 ? juce::String ("M ") + SlotPattern::shortLabel (step->noteMode)
                                                     : juce::String ("PITCH ") + SlotPattern::shortLabel (step->noteMode))
                            : (modeR.getWidth() < 64 ? "M" : "PITCH");
    const auto roleText = step != nullptr
                            ? (roleR.getWidth() < 56 ? juce::String ("R ") + SlotPattern::shortLabel (step->role)
                                                     : juce::String ("PART ") + SlotPattern::shortLabel (step->role))
                            : (roleR.getWidth() < 56 ? "R" : "PART");
    const auto followText = step != nullptr
                              ? (followR.getWidth() < 44 ? (step->followStructure ? "F" : "L")
                                                         : (step->followStructure ? "FOLLOW" : "LOCAL"))
                              : (followR.getWidth() < 44 ? "F" : "FLOW");

    drawButton (modeR, modeText, false, step != nullptr);
    drawButton (roleR, roleText, false, step != nullptr);
    drawButton (followR, followText, step != nullptr && ! step->followStructure, step != nullptr);
    drawButton (moreRect(), "MORE", false, true);
}

void StepGridSingle::paintGrid (juce::Graphics& g) const
{
    const auto full = gridRect();
    if (full.isEmpty())
        return;

    g.setColour (Theme::Colour::surface2);
    g.fillRect (full);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.7f));
    g.drawRect (full, 1);

    const auto overview = overviewRect();
    if (! overview.isEmpty())
    {
        const auto ovLabels = overviewLabelsRect();
        const auto ovCells = overviewCellsRect();
        const int rows = overviewRowCount();
        const float cellW = ovCells.getWidth() / (float) juce::jmax (1, kCols);
        const int rowTop = ovCells.getY() + kOverviewPadY;
        const int rowBottom = rowTop + rows * kOverviewRowH;

        g.setColour (Theme::Colour::surface0.withAlpha (0.8f));
        g.fillRect (ovLabels);
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.45f));
        g.drawLine ((float) ovLabels.getRight(),
                    (float) ovLabels.getY(),
                    (float) ovLabels.getRight(),
                    (float) ovLabels.getBottom(),
                    1.0f);

        for (int col = 0; col <= kCols; ++col)
        {
            const int x = juce::roundToInt ((float) ovCells.getX() + (float) col * cellW);
            const int unit = juce::jlimit (0, kCols - 1, col);
            const bool beat = ((unit % 4) == 0);
            g.setColour (Theme::Colour::surfaceEdge.withAlpha (beat ? 0.30f : 0.15f));
            g.drawLine ((float) x, (float) rowTop, (float) x, (float) rowBottom, 1.0f);
        }

        auto drawMiniButton = [&] (juce::Rectangle<int> r, const juce::String& text, juce::Colour fill, juce::Colour edge, juce::Colour ink)
        {
            if (r.isEmpty())
                return;

            g.setColour (fill);
            g.fillRoundedRectangle (r.toFloat(), 2.0f);
            g.setColour (edge);
            g.drawRoundedRectangle (r.toFloat(), 2.0f, 1.0f);
            g.setColour (ink);
            g.setFont (Theme::Font::micro());
            g.drawText (text, r, juce::Justification::centred, false);
        };

        for (int row = 0; row < rows; ++row)
        {
            const int rowStart = row * kCols;
            const int rowEnd = juce::jmin (totalUnits(), rowStart + kCols);
            const bool rowUsed = row < pageCount();
            const bool playheadOnRow = m_playhead >= rowStart && m_playhead < rowEnd;

            const auto rowRect = overviewRowRect (row);
            const auto rowLabelRect = overviewRowLabelRect (row);
            const int y1 = rowRect.getY();
            const int y2 = rowRect.getBottom();

            if (row == m_pageIndex)
            {
                g.setColour (m_groupColor.withAlpha (0.12f));
                g.fillRect (rowRect);
                g.setColour (m_groupColor.withAlpha (0.22f));
                g.fillRect (rowLabelRect);
            }
            else if (! rowUsed)
            {
                g.setColour (Theme::Colour::surface0.withAlpha (0.45f));
                g.fillRect (rowRect);
            }

            bool hasEvents = false;
            bool allFollow = true;
            for (int unit = rowStart; unit < rowEnd; ++unit)
            {
                const int stepIdx = findStepIndexForUnit (unit);
                const auto* step = m_pattern->getStep (stepIdx);
                if (step == nullptr || step->start != unit || step->microEventCount <= 0)
                    continue;

                hasEvents = true;
                if (! step->followStructure)
                    allFollow = false;
            }

            const auto routeRect = overviewRowRouteRect (row);
            const auto followBtn = overviewRowFollowRect (row);
            const auto muteBtn = overviewRowMuteRect (row);
            const bool highlighted = row == m_pageIndex || playheadOnRow;

            drawMiniButton (routeRect,
                            rowRouteLabel (row),
                            highlighted ? m_groupColor.withAlpha (0.22f) : Theme::Colour::surface2.withAlpha (0.92f),
                            highlighted ? m_groupColor.withAlpha (0.9f) : Theme::Colour::surfaceEdge.withAlpha (0.9f),
                            highlighted ? Theme::Colour::inkLight : Theme::Colour::inkMid);

            drawMiniButton (followBtn,
                            hasEvents ? (allFollow ? "F" : "L") : "-",
                            hasEvents ? (allFollow ? Theme::Zone::a.withAlpha (0.26f) : Theme::Colour::warning.withAlpha (0.22f))
                                      : Theme::Colour::surface2.withAlpha (0.7f),
                            hasEvents ? (allFollow ? Theme::Zone::a.withAlpha (0.95f) : Theme::Colour::warning.withAlpha (0.9f))
                                      : Theme::Colour::surfaceEdge.withAlpha (0.7f),
                            hasEvents ? Theme::Colour::inkLight : Theme::Colour::inkGhost);

            drawMiniButton (muteBtn,
                            m_rowMuted[(size_t) row] ? "M" : "-",
                            m_rowMuted[(size_t) row] ? Theme::Colour::error.withAlpha (0.22f) : Theme::Colour::surface2.withAlpha (0.7f),
                            m_rowMuted[(size_t) row] ? Theme::Colour::error.withAlpha (0.9f) : Theme::Colour::surfaceEdge.withAlpha (0.7f),
                            m_rowMuted[(size_t) row] ? Theme::Colour::inkLight : Theme::Colour::inkGhost);

            for (int col = 0; col < kCols; ++col)
            {
                const int unit = rowStart + col;
                const int x1 = juce::roundToInt ((float) ovCells.getX() + (float) col * cellW);
                const int x2 = juce::roundToInt ((float) ovCells.getX() + (float) (col + 1) * cellW);
                auto cell = juce::Rectangle<int> (x1, y1, juce::jmax (1, x2 - x1), juce::jmax (1, y2 - y1)).reduced (1, 1);
                if (cell.isEmpty())
                    continue;

                if (unit >= totalUnits())
                {
                    g.setColour (Theme::Colour::surface0.withAlpha (0.35f));
                    g.fillRect (cell);
                    continue;
                }

                const int stepIdx = findStepIndexForUnit (unit);
                const auto* step = m_pattern->getStep (stepIdx);
                const bool stepActive = step != nullptr
                                     && step->start == unit
                                     && step->gateMode != SlotPattern::GateMode::rest
                                     && step->microEventCount > 0;

                g.setColour (Theme::Colour::surface1.withAlpha (0.75f));
                g.fillRect (cell);

                if (stepActive)
                {
                    g.setColour (m_groupColor.withAlpha (0.70f));
                    g.fillRoundedRectangle (cell.toFloat().reduced (1.0f, 2.0f), 2.0f);
                }

                if (unit == m_selectedUnit)
                {
                    g.setColour (Theme::Colour::inkLight.withAlpha (0.95f));
                    g.drawRect (cell, 1);
                }

                if (m_playhead >= 0 && unit == (m_playhead % juce::jmax (1, totalUnits())))
                {
                    g.setColour (Theme::Colour::error.withAlpha (0.85f));
                    g.fillRect (cell.getX(), cell.getY(), 2, cell.getHeight());
                }
            }
        }

        for (int row = 0; row <= rows; ++row)
        {
            const int y = rowTop + row * kOverviewRowH;
            g.setColour (Theme::Colour::surfaceEdge.withAlpha (row == m_pageIndex || row == m_pageIndex + 1 ? 0.45f : 0.25f));
            g.drawLine ((float) ovCells.getX(), (float) y, (float) ovCells.getRight(), (float) y, 1.0f);
        }

        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.65f));
        g.drawLine ((float) full.getX(), (float) overview.getBottom(), (float) full.getRight(), (float) overview.getBottom(), 1.0f);
    }

    const auto note = noteGridRect();
    if (note.isEmpty())
        return;

    const auto labels = note.withWidth (kLabelW);
    const auto cellsArea = note.withTrimmedLeft (kLabelW);

    g.setColour (Theme::Colour::surface0.withAlpha (0.8f));
    g.fillRect (labels);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.45f));
    g.drawLine ((float) labels.getRight(), (float) labels.getY(), (float) labels.getRight(), (float) labels.getBottom(), 1.0f);

    for (int row = 0; row < kRows; ++row)
    {
        auto r = cellRect (row, 0).withX (labels.getX()).withWidth (labels.getWidth());
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.9f));
        g.drawText (rowLabelForRow (row), r.reduced (2, 0), juce::Justification::centredLeft, false);
    }

    for (int row = 0; row <= kRows; ++row)
    {
        const int y = row == kRows ? cellRect (kRows - 1, 0).getBottom() : cellRect (row, 0).getY();
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (row % 3 == 0 ? 0.40f : 0.18f));
        g.drawLine ((float) cellsArea.getX(), (float) y, (float) cellsArea.getRight(), (float) y, 1.0f);
    }

    for (int col = 0; col <= kCols; ++col)
    {
        const int x = col == kCols ? cellRect (0, kCols - 1).getRight() : cellRect (0, col).getX();
        const int unit = unitForColumn (juce::jlimit (0, kCols - 1, col));
        const bool beat = ((unit % 4) == 0);
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (beat ? 0.45f : 0.2f));
        g.drawLine ((float) x, (float) cellsArea.getY(), (float) x, (float) cellsArea.getBottom(), 1.0f);
    }

    for (int col = 0; col < kCols; ++col)
    {
        const int unit = unitForColumn (col);
        const auto topCell = cellRect (0, col);

        if (unit == m_selectedUnit)
        {
            g.setColour (m_groupColor.withAlpha (0.12f));
            g.fillRect (topCell.getX(), cellsArea.getY(), topCell.getWidth(), cellsArea.getHeight());
        }

        if (m_playhead >= 0 && unit == (m_playhead % juce::jmax (1, totalUnits())))
        {
            g.setColour (Theme::Colour::error.withAlpha (0.65f));
            g.fillRect (topCell.getX(), cellsArea.getY(), 2, cellsArea.getHeight());
        }

        const int stepIdx = findStepIndexForUnit (unit);
        const auto* step = m_pattern->getStep (stepIdx);
        if (step != nullptr && unit == step->start && col > 0)
        {
            g.setColour (Theme::Colour::inkGhost.withAlpha (0.45f));
            g.fillRect (topCell.getX(), cellsArea.getY(), 2, cellsArea.getHeight());
        }
    }

    if (m_view == LaneView::chord)
    {
        const int rootPc = rootToPitchClass (chordRootString (m_currentChord));
        auto chordIntervals = chordIntervalsForLabel (m_currentChord);
        if (chordIntervals.empty())
            chordIntervals = { 0, 4, 7 };

        for (int row = 0; row < kRows; ++row)
        {
            const int midi = midiForRow (row);
            const int pc = (midi % 12 + 12) % 12;
            bool inChord = false;
            for (const int interval : chordIntervals)
            {
                if (((rootPc + interval) % 12) == pc)
                {
                    inChord = true;
                    break;
                }
            }

            if (inChord)
            {
                auto rr = cellRect (row, 0);
                g.setColour (m_groupColor.withAlpha (0.08f));
                g.fillRect (rr.getX(), rr.getY(), cellsArea.getWidth(), rr.getHeight());
            }
        }
    }

    if (m_view == LaneView::velocity)
    {
        for (int col = 0; col < kCols; ++col)
        {
            const int unit = unitForColumn (col);
            const auto ref = firstEventInUnit (unit);
            if (ref.stepIndex < 0)
                continue;

            const auto* step = m_pattern->getStep (ref.stepIndex);
            if (step == nullptr || ! juce::isPositiveAndBelow (ref.eventIndex, step->microEventCount))
                continue;

            const int velocity = step->microEvents[(size_t) ref.eventIndex].velocity;
            const int velocityRow = rowForVelocity (velocity);
            const auto bottomCell = cellRect (kRows - 1, col);
            const auto barTop = cellRect (velocityRow, col);
            const int barH = bottomCell.getBottom() - barTop.getY();
            g.setColour (m_groupColor.withAlpha (0.78f));
            g.fillRoundedRectangle (juce::Rectangle<float> ((float) bottomCell.getX() + 2.0f,
                                                            (float) bottomCell.getBottom() - (float) barH,
                                                            (float) juce::jmax (2, bottomCell.getWidth() - 4),
                                                            (float) barH - 1.0f),
                                    2.0f);

            if (ref.stepIndex == m_selectedStep && ref.eventIndex == m_selectedEvent)
            {
                g.setColour (Theme::Colour::inkLight.withAlpha (0.95f));
                g.drawRect (barTop.withHeight (barH), 1);
            }
        }
    }
    else if (m_view == LaneView::midiFx)
    {
        for (int col = 0; col < kCols; ++col)
        {
            const int unit = unitForColumn (col);
            const int stepIdx = findStepIndexForUnit (unit);
            const auto* step = m_pattern->getStep (stepIdx);
            if (step == nullptr)
                continue;

            auto rr = cellRect (kRows / 2, col).reduced (2, 2);
            g.setColour (step->followStructure ? Theme::Zone::a.withAlpha (0.5f)
                                               : Theme::Colour::warning.withAlpha (0.5f));
            g.fillRoundedRectangle (rr.toFloat(), Theme::Radius::xs);
            g.setColour (Theme::Colour::inkDark.withAlpha (0.9f));
            g.setFont (Theme::Font::micro());
            g.drawFittedText (SlotPattern::shortLabel (step->noteMode), rr, juce::Justification::centred, 1);
        }
    }
    else
    {
        for (int stepIdx = 0; stepIdx < m_pattern->activeStepCount(); ++stepIdx)
        {
            const auto* step = m_pattern->getStep (stepIdx);
            if (step == nullptr)
                continue;

            for (int eventIdx = 0; eventIdx < step->microEventCount; ++eventIdx)
            {
                const auto& event = step->microEvents[(size_t) eventIdx];
                const int unit = eventUnitFor (*step, event);
                const int col = columnForUnit (unit);
                if (col < 0)
                    continue;

                const int midi = eventMidiForDisplay (*step, event);
                const int row = rowForMidi (midi);
                if (! juce::isPositiveAndBelow (row, kRows))
                    continue;

                auto rr = cellRect (row, col).reduced (2, 1);
                const float velAlpha = juce::jmap ((float) event.velocity, 1.0f, 127.0f, 0.40f, 0.95f);
                g.setColour (m_groupColor.withAlpha (velAlpha));
                g.fillRoundedRectangle (rr.toFloat(), Theme::Radius::xs);

                const bool selected = (stepIdx == m_selectedStep && eventIdx == m_selectedEvent);
                g.setColour (selected ? Theme::Colour::inkLight
                                      : Theme::Colour::surfaceEdge.withAlpha (0.9f));
                g.drawRoundedRectangle (rr.toFloat(), Theme::Radius::xs, selected ? Theme::Stroke::thick : Theme::Stroke::normal);
            }
        }
    }

    for (int col = 0; col < kCols; ++col)
    {
        const int unit = unitForColumn (col);
        auto bottomCell = cellRect (kRows - 1, col);
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.75f));
        const juce::Rectangle<int> labelRect { bottomCell.getX(), cellsArea.getBottom() - 10, bottomCell.getWidth(), 10 };
        g.drawText (juce::String (unit + 1),
                    labelRect,
                    juce::Justification::centred,
                    false);
    }
}

void StepGridSingle::paintInspector (juce::Graphics& g) const
{
    const auto rr = inspectorRect();
    if (rr.isEmpty())
        return;

    g.setColour (Theme::Colour::surface0.withAlpha (0.9f));
    g.fillRect (rr);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.7f));
    g.drawRect (rr, 1);

    if (m_pattern == nullptr)
        return;

    const auto* step = selectedStepData();
    if (step == nullptr)
        return;

    const int routeRow = m_pattern->routeRowForUnit (step->start);
    const int selectedPage = pageForUnit (m_selectedUnit);
    juce::String line = "u " + juce::String (m_selectedUnit + 1)
                      + " / " + juce::String (totalUnits())
                      + "  row " + juce::String (selectedPage + 1)
                      + "  step " + juce::String (m_selectedStep + 1)
                      + "  dur " + juce::String (step->duration)
                      + "  " + rowRouteLabel (routeRow)
                      + "  " + (step->followStructure ? "structure" : "local")
                      + "  view " + laneViewLabel (m_view);

    if (m_selectedEvent >= 0 && juce::isPositiveAndBelow (m_selectedEvent, step->microEventCount))
    {
        const auto& event = step->microEvents[(size_t) m_selectedEvent];
        line += "  |  note " + noteName (eventMidiForDisplay (*step, event))
             + "  vel " + juce::String ((int) event.velocity)
             + "  at " + juce::String (juce::roundToInt (event.timeOffset * 100.0f)) + "%"
             + "  len " + juce::String (juce::roundToInt (event.length * 100.0f)) + "%";
    }
    else
    {
        line += "  |  click a cell to add notes";
    }

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkMid);
    g.drawText (line, rr.reduced (4, 0), juce::Justification::centredLeft, true);
}

void StepGridSingle::mouseDown (const juce::MouseEvent& e)
{
    if (m_pattern == nullptr)
        return;

    grabKeyboardFocus();
    ensureSelectionValid();

    const auto pos = e.getPosition();

    if (pagePrevRect().contains (pos))
    {
        m_pageIndex = juce::jmax (0, m_pageIndex - 1);
        repaint();
        return;
    }

    if (pageNextRect().contains (pos))
    {
        m_pageIndex = juce::jmin (pageCount() - 1, m_pageIndex + 1);
        repaint();
        return;
    }

    for (int rp = 0; rp < pageCount(); ++rp)
    {
        if (rowChipRect (rp).contains (pos))
        {
            m_pageIndex = rp;
            repaint();
            return;
        }
    }

    for (int i = 0; i < 4; ++i)
    {
        const auto view = static_cast<LaneView> (i);
        if (tabRect (view).contains (pos))
        {
            m_view = view;
            repaint();
            return;
        }
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
        showMoreMenu();
        return;
    }

    const auto ov = overviewRect();
    const auto ovCells = overviewCellsRect();
    const int rows = overviewRowCount();
    if (ov.contains (pos) && rows > 0 && ovCells.getWidth() > 0 && ovCells.getHeight() > 0)
    {
        const int row = juce::jlimit (0,
                                      rows - 1,
                                      (pos.y - (ov.getY() + kOverviewPadY)) / juce::jmax (1, kOverviewRowH));

        if (overviewRowRouteRect (row).contains (pos))
        {
            showRowRouteMenu (row);
            return;
        }

        if (overviewRowFollowRect (row).contains (pos))
        {
            toggleRowFollow (row);
            return;
        }

        if (overviewRowMuteRect (row).contains (pos))
        {
            toggleRowMute (row);
            return;
        }

        const float cellW = ovCells.getWidth() / (float) juce::jmax (1, kCols);

        if (row >= pageCount())
            return;

        const int clickX = ovCells.contains (pos) ? pos.x : ovCells.getX();
        const int col = juce::jlimit (0,
                                      kCols - 1,
                                      (int) std::floor ((clickX - ovCells.getX()) / cellW));

        m_pageIndex = juce::jmin (pageCount() - 1, row);
        const int targetUnit = juce::jlimit (0, totalUnits() - 1, row * kCols + col);
        m_selectedUnit = targetUnit;
        m_selectedStep = findStepIndexForUnit (targetUnit);
        m_selectedEvent = -1;
        ensureSelectionValid();
        repaint();
        return;
    }

    const int col = cellColumnAt (pos);
    const int row = cellRowAt (pos);
    if (col < 0 || row < 0)
        return;

    const int unit = unitForColumn (col);
    m_selectedUnit = unit;
    m_selectedStep = findStepIndexForUnit (unit);
    m_selectedEvent = -1;

    if (m_view == LaneView::velocity)
    {
        setVelocityAtCell (unit, row);
        commitChange();
        return;
    }

    if (m_view == LaneView::midiFx)
    {
        if (auto* step = selectedStepData())
        {
            step->followStructure = ! step->followStructure;
            commitChange();
        }
        return;
    }

    auto ref = findEventAtCell (unit, row);
    if (e.mods.isRightButtonDown())
    {
        if (ref.stepIndex >= 0)
            showEventContextMenu (ref);
        else
            showMoreMenu();
        return;
    }

    if (m_view == LaneView::chord && ! e.mods.isCommandDown())
    {
        addChordAtUnit (unit, row, e.mods.isShiftDown() ? 120 : 100);
        commitChange();
        return;
    }

    if (ref.stepIndex >= 0)
        removeEventRef (ref);
    else
        addEventAtCell (unit, row, e.mods.isShiftDown() ? 120 : 100);

    commitChange();
}
void StepGridSingle::mouseDrag (const juce::MouseEvent& e)
{
    if (m_pattern == nullptr)
        return;

    const int col = cellColumnAt (e.getPosition());
    const int row = cellRowAt (e.getPosition());
    if (col < 0 || row < 0)
        return;

    const int unit = unitForColumn (col);
    m_selectedUnit = unit;
    m_selectedStep = findStepIndexForUnit (unit);

    if (m_view == LaneView::velocity)
    {
        setVelocityAtCell (unit, row);
        commitChange();
        return;
    }

    if (m_view != LaneView::notes)
        return;

    const auto ref = findEventAtCell (unit, row);
    if (ref.stepIndex < 0)
    {
        addEventAtCell (unit, row, 100);
        commitChange();
    }
}

void StepGridSingle::mouseMove (const juce::MouseEvent& e)
{
    m_hoverColumn = cellColumnAt (e.getPosition());
    m_hoverRow = cellRowAt (e.getPosition());
}

void StepGridSingle::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (m_pattern == nullptr)
        return;

    int delta = 0;
    if (std::abs (wheel.deltaY) > std::abs (wheel.deltaX))
        delta = wheel.deltaY > 0.0f ? 1 : (wheel.deltaY < 0.0f ? -1 : 0);
    else
        delta = wheel.deltaX > 0.0f ? 1 : (wheel.deltaX < 0.0f ? -1 : 0);

    if (delta == 0)
        return;

    if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        m_pitchBase = juce::jlimit (0, 116, m_pitchBase + delta);
        repaint();
        return;
    }

    if (pageLabelRect().contains (e.getPosition()))
    {
        m_pageIndex = juce::jlimit (0, pageCount() - 1, m_pageIndex + (delta < 0 ? 1 : -1));
        repaint();
        return;
    }

    if (m_view == LaneView::velocity)
    {
        auto* step = selectedStepData();
        if (step != nullptr && juce::isPositiveAndBelow (m_selectedEvent, step->microEventCount))
        {
            auto& event = step->microEvents[(size_t) m_selectedEvent];
            event.velocity = (uint8_t) juce::jlimit (1, 127, (int) event.velocity + (delta * 5));
            commitChange();
        }
    }
}

void StepGridSingle::mouseUp (const juce::MouseEvent&)
{
}

bool StepGridSingle::keyPressed (const juce::KeyPress& key)
{
    if (m_pattern == nullptr)
        return false;

    const auto ch = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());

    if (key == juce::KeyPress::leftKey)
    {
        m_selectedUnit = juce::jmax (0, m_selectedUnit - 1);
        ensureSelectionValid();
        repaint();
        return true;
    }

    if (key == juce::KeyPress::rightKey)
    {
        m_selectedUnit = juce::jmin (totalUnits() - 1, m_selectedUnit + 1);
        ensureSelectionValid();
        repaint();
        return true;
    }

    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        if (m_selectedEvent >= 0)
        {
            removeEventRef ({ m_selectedStep, m_selectedEvent });
            commitChange();
            return true;
        }

        const auto ref = firstEventInUnit (m_selectedUnit);
        if (ref.stepIndex >= 0)
        {
            removeEventRef (ref);
            commitChange();
            return true;
        }

        return true;
    }

    if (ch == '[')
    {
        m_pattern->setPatternLength (m_pattern->patternLength() - 1);
        commitChange();
        return true;
    }

    if (ch == ']')
    {
        m_pattern->setPatternLength (m_pattern->patternLength() + 1);
        commitChange();
        return true;
    }

    if (ch == '1') { m_view = LaneView::notes; repaint(); return true; }
    if (ch == '2') { m_view = LaneView::chord; repaint(); return true; }
    if (ch == '3' || ch == 'v') { m_view = LaneView::velocity; repaint(); return true; }
    if (ch == '4') { m_view = LaneView::midiFx; repaint(); return true; }

    if (ch == 'm')
    {
        showModeMenu();
        return true;
    }

    if (ch == 'r')
    {
        showRoleMenu();
        return true;
    }

    if (key.getModifiers().isCommandDown() && key.getTextCharacter() == 'c')
    {
        if (const auto* step = selectedStepData())
        {
            m_stepClipboard = *step;
            m_hasStepClipboard = true;
        }
        return true;
    }

    if (key.getModifiers().isCommandDown() && key.getTextCharacter() == 'v')
    {
        auto* step = selectedStepData();
        if (step != nullptr && m_hasStepClipboard)
        {
            const int keepStart = step->start;
            const int keepDur = step->duration;
            *step = m_stepClipboard;
            step->start = keepStart;
            step->duration = keepDur;
            commitChange();
        }
        return true;
    }

    return false;
}

juce::MouseCursor StepGridSingle::getMouseCursor()
{
    return juce::MouseCursor::CrosshairCursor;
}

juce::String StepGridSingle::rowLabelForRow (int row) const
{
    if (m_view == LaneView::velocity)
        return juce::String (velocityForRow (row));

    if (m_view == LaneView::midiFx)
        return juce::String (kRows - row);

    return noteName (midiForRow (row));
}

juce::String StepGridSingle::laneViewLabel (LaneView view) const
{
    switch (view)
    {
        case LaneView::notes:    return "NOTES";
        case LaneView::chord:    return "CHORD";
        case LaneView::velocity: return "VEL";
        case LaneView::midiFx:   return "FX";
    }

    return "NOTES";
}

juce::String StepGridSingle::rowRouteLabel (int row) const
{
    if (m_pattern == nullptr || ! juce::isPositiveAndBelow (row, SlotPattern::kRouteRows))
        return "R" + juce::String (row + 1) + ">--";

    const int target = m_pattern->getRowTargetSlot (row);
    if (target == SlotPattern::kRouteSelf)
        return "R" + juce::String (row + 1) + ">SELF";

    return "R" + juce::String (row + 1) + ">S" + juce::String (target + 1);
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
            const int offset = (event.pitchValue == SlotPattern::kUseSlotBasePitch)
                                 ? 0
                                 : juce::jlimit (SlotPattern::kMinTheoryValue,
                                                 SlotPattern::kMaxTheoryValue,
                                                 event.pitchValue);
            return juce::jlimit (24, 108, baseNote + offset);
        }
    }
}

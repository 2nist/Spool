# Spool Sequencer — Analysis & Implementation Plan

**Updated:** 2026-03-29
**Status summary:** Core melodic + drum grids functional. Undo/redo wired for melodic. Step resize working. Drum right-click menu expanded. Remaining gaps: inspector controls, velocity/ratchet display, drum undo, chord editing, playhead scrub, constraint enforcement.

---

## Architecture Overview

```
PluginEditor
  └─ ZoneBComponent (orchestrator, focus/routing)
       ├─ SequencerHeader (16px: pattern selector, copy/paste/clear, context)
       ├─ StepGridSingle (melodic: 12-row × 16-col, paged, notes/chord/velocity/midifx views)
       ├─ StepGridDrum   (drum: N voice rows, scrollable viewport)
       └─ ModuleRows / ModuleGroups
            ├─ SlotPattern      (data: 8 patterns, 64 steps, 8 micro-events/step)
            └─ DrumModuleState  (data: N voices, 64 steps/voice, per-step vel/ratchet/div)

PluginProcessor
  └─ compileRuntimePattern() → RuntimeSlotPattern (spinlock-protected, debounced 40ms)
  └─ onStepAdvanced → ZoneBComponent::setPlayheadStep()
```

**Three step grids:**
| Grid | Purpose | Data | Status |
|---|---|---|---|
| `StepGrid` | 32-step basic trigger | SlotPattern (toggle only) | Complete but disconnected |
| `StepGridSingle` | Melodic/piano-roll | SlotPattern (full) | ~85% |
| `StepGridDrum` | Drum machine | DrumModuleState | ~75% |

---

## Completed (this session)

- [x] **Targeted repaints** — playhead column-strip only, no full repaint
- [x] **Playhead page-follow** — StepGridSingle auto-advances page when step goes off-screen
- [x] **Pattern compilation debounce** — 40ms timer, destructor flush, no data loss
- [x] **Undo/redo for melodic edits** — gesture-based (mouseDown→mouseUp = one action), `PatternEditAction` on PluginProcessor's UndoManager
- [x] **Step resize handles** — drag right edge of step block in StepGridSingle to change duration; LeftRightResizeCursor on hover, group-color indicator, undo-integrated
- [x] **Drum right-click menu expanded** — velocity 10%–100% (10 steps, ticked), ratchet 1–8× (ticked), division with current ticked, "Copy Step To Voice" submenu, inspector repaint after action

---

## Current Gaps (Priority-Ordered)

---

### P1 — Drum Step Undo/Redo
**Gap:** StepGridDrum edits are irreversible. StepGridSingle has full gesture undo; drums have none.
**Impact:** High — destroys user work silently.

**Plan:**

1. **Add callbacks to `StepGridDrum`** ([source/components/ZoneB/StepGridDrum.h](source/components/ZoneB/StepGridDrum.h)):
   ```cpp
   std::function<void()> onBeforeEdit;   // fire before first toggle in a drag gesture
   std::function<void()> onGestureEnd;   // fire on mouseUp
   ```

2. **Track gesture state** in `StepGridDrum.h`:
   ```cpp
   bool m_inGesture { false };
   ```

3. **Fire in `voiceMouseDown`** ([source/components/ZoneB/StepGridDrum.cpp](source/components/ZoneB/StepGridDrum.cpp)):
   - Set `m_inGesture = true` at top
   - Call `onBeforeEdit` before first mutation

4. **Fire in `voiceMouseUp`**:
   - Set `m_inGesture = false`
   - Call `onGestureEnd`

5. **Wire in `ZoneBComponent::focusRow`** ([source/components/ZoneB/ZoneBComponent.cpp](source/components/ZoneB/ZoneBComponent.cpp)) for drum rows — mirror the existing melodic gesture wiring, using `DrumModuleState` snapshots instead of `SlotPattern`.
   - `onBeforeEdit`: capture `DrumModuleState` snapshot (copy the whole struct)
   - `onGestureEnd`: diff before/after, push `DrumEditAction : juce::UndoableAction`
   - `DrumEditAction::perform()` / `::undo()` → call `applyDrumStateForUndo()`

6. **Add to `ZoneBComponent`**:
   ```cpp
   void applyDrumStateForUndo (int slotIdx, const DrumModuleState& snap);
   ```

---

### P2 — Velocity Gradient Display in Drum Grid
**Gap:** All active drum steps render at uniform color. `stepVelocities` array holds per-step values but nothing uses them visually.
**Impact:** Medium — users can set velocity via right-click but can't see it.

**Plan:**

In `StepGridDrum::paintVoiceRow` ([source/components/ZoneB/StepGridDrum.cpp](source/components/ZoneB/StepGridDrum.cpp)), line ~293:

```cpp
// Current:
auto fill = (active ? juce::Colour(voice.colorArgb) : ...).withAlpha(alpha);

// Change to:
auto fill = (active ? juce::Colour(voice.colorArgb)
                          .interpolatedWith(juce::Colours::black,
                                           1.0f - voice.stepVelocity(s) * 0.85f)
                    : ...).withAlpha(alpha);
```

Additionally, draw a small velocity bar at the bottom of each active step cell (2px high, width proportional to velocity):
```cpp
if (active)
{
    const float vel = voice.stepVelocity(s);
    const int barW = juce::roundToInt(cr.getWidth() * vel);
    g.setColour(juce::Colour(voice.colorArgb).brighter(0.3f).withAlpha(alpha));
    g.fillRect(cr.getX(), cr.getBottom() - 2, barW, 2);
}
```

Also add ratchet count indicator: if `stepRatchetCount(s) > 1`, draw N small dots across the top of the cell.

---

### P3 — Inspector Bar Controls (StepGridSingle)
**Gap:** The 16px inspector at the bottom of StepGridSingle shows text (`paintInspector`) but has no interactive controls.
**Data available:** selected step's gateMode, role, noteMode, harmonicSource, lookAheadToNextChord, anchorValue.
**Impact:** Medium — structure context visible but not editable inline.

**Plan:**

Expand inspector height from `kInspectorH = 16` to 20px (or keep 16, just use smaller controls).

In `paintInspector` ([source/components/ZoneB/StepGridSingle.cpp](source/components/ZoneB/StepGridSingle.cpp)), replace the current text-only paint with clickable chip buttons:

Layout (left to right):
```
[GATE ▾] [LEAD ▾] [KEY ▾] [F] [MIDI: C3] [VEL: 80%]
```

Where:
- `[GATE ▾]` = GateMode dropdown (gate / tie / rest / hold)
- `[LEAD ▾]` = StepRole dropdown (bass / chord / lead / fill / transition)
- `[KEY ▾]`  = HarmonicSource dropdown (key / chord / nextChord)
- `[F]`      = lookAheadToNextChord toggle
- `MIDI: C3` = realized pitch for selected micro-event (read-only)
- `VEL: 80%` = velocity of selected micro-event (click to cycle or drag to set)

**Mouse hit-testing** in `mouseDown` — add an `inspectorRect().contains(pos)` branch that dispatches to `showGateModeMenu()`, `showRoleMenu()`, etc. depending on sub-rect.

The menus already exist (`showModeMenu`, `showRoleMenu`) — just need to be triggered from the inspector as well as from the header buttons.

---

### P4 — Step Clipboard (Copy/Paste Individual Steps)
**Gap:** `m_stepClipboard` and `m_hasStepClipboard` are declared in StepGridSingle but copy/paste shortcuts are not wired.
**Impact:** Low-Medium — pattern clipboard exists at ZoneBComponent level, but step-level copy is missing.

**Plan:**

In `StepGridSingle::keyPressed` ([source/components/ZoneB/StepGridSingle.cpp](source/components/ZoneB/StepGridSingle.cpp)):

```cpp
// Ctrl+C — copy selected step
if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'C')
{
    if (const auto* step = selectedStepData())
    {
        m_stepClipboard = *step;
        m_hasStepClipboard = true;
    }
    return true;
}

// Ctrl+V — paste to selected step
if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'V')
{
    if (m_hasStepClipboard)
    {
        if (onBeforeEdit) onBeforeEdit();
        if (auto* step = selectedStepData())
        {
            const int savedStart    = step->start;
            const int savedDuration = step->duration;
            *step = m_stepClipboard;
            step->start    = savedStart;
            step->duration = savedDuration;
            commitChange();
        }
    }
    return true;
}
```

Note: preserve `start` and `duration` so pasting doesn't corrupt step layout.

---

### P5 — Gate Mode / Tie Visual in Melodic Grid
**Gap:** GateMode::tie and GateMode::hold exist in data but are not visually distinct in the note grid — they look identical to gate/rest.
**Impact:** Medium — users can set tie/hold via menu but can't tell from the grid.

**Plan:**

In `paintGrid` ([source/components/ZoneB/StepGridSingle.cpp](source/components/ZoneB/StepGridSingle.cpp)), in the notes-view step-event rendering loop (around line 1485):

For micro-events, after drawing the filled block, check if the step's gateMode is `tie` or `hold`:
```cpp
if (step->gateMode == SlotPattern::GateMode::tie)
{
    // Draw a rightward arrow connecting to next cell
    const auto r = cellRect(row, col);
    g.setColour(m_groupColor.withAlpha(0.6f));
    g.fillRect(r.getRight() - 3, r.getCentreY() - 1, 3, 2);
}
else if (step->gateMode == SlotPattern::GateMode::hold)
{
    // Draw a horizontal line to the right edge of the step's last column
    // (same logic as resize handle — uses step->start + step->duration - 1)
}
```

Also in the column loop, shade `rest` steps differently (light cross or grey-out).

---

### P6 — Chord Editing Mode (StepGridSingle)
**Gap:** LaneView::chord tab exists. Clicking the tab switches to chord view which shows chord-tone background highlights, but note entry still works per-event (not chord-aware).
**Impact:** Medium — chord context is displayed but not used for input.

**Plan:**

Chord mode should:
1. **Grid layout** — show chord intervals as horizontal bands (root, 3rd, 5th, 7th highlighted)
2. **Click** — add all chord tones simultaneously at the clicked column (already partially wired via `addChordAtUnit`)
3. **Shift+Click** — add only the root
4. **Visual** — show intervals stacked vertically in one column

The `addChordAtUnit` method in StepGridSingle already exists — the main gap is the **visual distinction** of chord mode in the grid render.

In `paintGrid`, when `m_view == LaneView::chord`, for each column draw stacked chord-tone indicators rather than individual event dots:
```cpp
// In chord view, for each column that has chord events, draw a vertical bar
// spanning all chord-tone rows instead of individual dots
```

This is a rendering change only — data model already supports it (multiple MicroEvents per step).

---

### P7 — Visual Playhead Scrub
**Gap:** Clicking on the note grid or drum grid does not move the playhead. Users have to wait for the transport to reach a step.
**Impact:** Low-Medium — standard DAW behavior; useful for auditioning.

**Plan:**

Add callback to both grids:
```cpp
std::function<void(int stepIndex)> onScrubRequest;
```

In `mouseDown` (both grids) — if Cmd/Ctrl is held while clicking a cell:
```cpp
if (e.mods.isCommandDown() && col >= 0)
{
    if (onScrubRequest)
        onScrubRequest(unit);
    return;
}
```

In `ZoneBComponent`, wire `onScrubRequest` → `processorRef.setTransportPosition(beat)` (if the processor exposes a scrub API).

---

### P8 — Voice Rename (Drum)
**Gap:** TODO comment in `rightClickVoice` at the "Rename voice" menu item. Currently a no-op.
**Impact:** Low — voices stay with generated names.

**Plan:**

In `rightClickVoice`, result == 1:
```cpp
if (result == 1)
{
    juce::AlertWindow::showInputBoxAsync(
        "Rename Voice", "Enter new name:", v.name,
        nullptr,
        [this, vi] (const juce::String& newName)
        {
            if (m_data == nullptr || vi >= (int)m_data->voices.size()) return;
            if (newName.isNotEmpty())
            {
                m_data->voices[(size_t)vi].name = newName.toStdString();
                m_voiceContent->repaint();
                if (onModified) onModified();
            }
        });
}
```

No new state needed — AlertWindow handles the input lifecycle.

---

### P9 — HarmonicSource / lookAheadToNextChord Exposure
**Gap:** These per-step fields are stored in SlotPattern but never exposed in UI beyond what `showModeMenu` / `showRoleMenu` offers (which doesn't include harmonicSource or lookAhead).
**Impact:** Low — advanced feature; most users won't miss it immediately.

**Plan:**

Extend `showMoreMenu` in StepGridSingle to include a "Harmonic Source" submenu:
```
Harmonic Source
  ● Key
  ○ Current Chord
  ○ Next Chord
[ ] Look Ahead to Next Chord
```

These map to `step->harmonicSource` and `step->lookAheadToNextChord`.

---

### P10 — Route Assignment Visual Feedback
**Gap:** The 4-row overview in StepGridSingle shows route buttons that open a menu to assign each row to a target slot. However, after assignment, the route label in the overview shows a slot number/label but the color or connection line is absent — difficult to understand at a glance.
**Impact:** Low — routing works, just not visually obvious.

**Plan:**

In `paintGrid`, in the overview label area for each row, draw a small colored pip matching the target slot's group color (passed in from ZoneBComponent or looked up via a callback).

Add callback:
```cpp
std::function<juce::Colour(int slotIndex)> onGetSlotColor;
```

In `overviewRowLabelRect` rendering, after drawing the route label text, if the route target is not kRouteSelf, draw a `4×4` filled circle using `onGetSlotColor(targetSlot)`.

---

## What Is NOT Worth Doing Now

| Feature | Reason |
|---|---|
| Full piano-roll (drag notes horizontally) | Architectural change — would need a scroll-based coordinate system, not cell-based |
| Polyrhythm / per-voice independent step lengths | Requires DSP-level scheduler change |
| Pattern import/export (MIDI) | `MidiClipExporter` stubbed — needs full DSP stabilization first |
| Multi-pattern layering | Large feature, no current UI affordance |
| Step probability / conditionals | No runtime support in `compileRuntimePattern` |
| Constraint enforcement (force-to-scale) | Requires theory engine integration that may change the musical model |

---

## File Change Map

| File | Changes Needed |
|---|---|
| `StepGridDrum.h` | Add `onBeforeEdit`, `onGestureEnd`, `m_inGesture` |
| `StepGridDrum.cpp` | Fire gesture callbacks in voiceMouseDown/Up; velocity gradient in paintVoiceRow; ratchet dots |
| `ZoneBComponent.h` | Add `applyDrumStateForUndo(int, const DrumModuleState&)` |
| `ZoneBComponent.cpp` | Wire drum gesture callbacks (P1); wire `onGetSlotColor` (P10) |
| `StepGridSingle.h` | No new declarations needed for P3–P5 |
| `StepGridSingle.cpp` | `keyPressed` step clipboard (P4); `paintInspector` controls (P3); `paintGrid` tie/hold visuals (P5); chord view grid render (P6); scrub on Cmd+click (P7) |
| `StepGridDrum.cpp` | `rightClickVoice` rename (P8) |

---

## Execution Order

```
P1  Drum undo/redo          — closes largest correctness gap
P2  Velocity gradient       — immediate visual payoff, small change
P3  Inspector controls      — unlocks GateMode/Role/HarmonicSource editing
P4  Step clipboard          — 20-line keyboard shortcut addition
P5  Tie/hold visual         — rendering only, no logic change
P6  Chord editing mode      — rendering change, addChordAtUnit already works
P7  Playhead scrub          — requires processor scrub API check first
P8  Voice rename            — AlertWindow, 10 lines
P9  HarmonicSource menu     — extend showMoreMenu
P10 Route color pip         — add callback, 15-line paint change
```

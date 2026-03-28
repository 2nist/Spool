# Codex Prompt — Stabilize Zone D and Structure Workflow Before Further Synth/UI Polish

Stop adding synth-layout polish and secondary UI refinements until the following core interaction problems are fixed reliably:

1. Zone D timeline direction is still inconsistent/backwards relative to the chord/structure track
2. simple section drag-and-drop still does not work reliably
3. chord abbreviations still are not working properly in the Structure workflow
4. right-click / contextual chord-theory menus in the section/progression editor still are not dependable

This pass is about **stabilization and correctness**, not more visual experimentation.

---

## Context

Recent progress has improved the Structure tab and Zone D, but several basic behaviors that should already be dependable are still not settled.

These are not minor polish issues. They are blocking usability issues.

Until these are correct, do **not** prioritize:
- more synth control rearrangement
- more knob/slider styling exploration
- additional visual refinement that assumes the structure/timeline workflow is already stable

The app needs the core song-form and timeline behavior to make sense first.

---

## Primary goals

1. Make Zone D timeline direction consistent and correct.
2. Make section drag-and-drop work for the basic section → arrangement workflow.
3. Make chord abbreviation display dependable and readable.
4. Make contextual chord / music-theory editing reachable via dependable right-click/context menus in the section editor / progression area.
5. Reduce repeated failed attempts by fixing the underlying interaction model instead of layering more partial UI over it.

---

## Strict priority order

Work in this order:

### Priority 1
Fix the timeline direction problem so time/placement is not visually contradictory.

### Priority 2
Fix basic section drag-and-drop so the structure workflow is usable.

### Priority 3
Fix chord abbreviation rendering/formatting.

### Priority 4
Fix right-click/context menu behavior for chord/music-theory actions.

Do not jump around randomly between these.

---

# 1. Timeline direction must be correct and consistent

## Required outcome
Zone D and the chord/structure track must agree on time direction.

Canonical rule:
- left = earlier
- right = later
- increasing beat/time maps to increasing x position

## Required checks
Review and correct wherever relevant:
- beat-to-x mapping
- structure/chord track mapping
- clip placement mapping
- playhead interpretation
- ruler/tick direction
- any wrap logic that makes one layer appear reversed relative to another

## Important guidance
Do not accept a partial fix where:
- the main timeline appears left-to-right
- but the chord track or structure projection still reads the opposite way

Zone D and structure/chord projection must agree visually.

---

# 2. Section drag-and-drop must work for the basic workflow

## Required outcome
The basic user story must work:
- create sections
- drag a section into the arrangement area
- get a correct arrangement instance/result

## Required checks
Inspect and fix the actual underlying drag/drop behavior, including:
- drag source ownership
- drop target hit testing
- list refresh / model refresh
- resequencing after drop
- selection synchronization
- redraw/repaint timing
- actual data-model mutation

## Important guidance
Do not fake drag/drop with hover visuals if the model update is still unreliable.

If needed, simplify the first working version so that the basic left-column section → right-column arrangement flow is dependable before adding more advanced reorder cases.

---

# 3. Chord abbreviations must be dependable

## Required outcome
Chord cells/labels should display readable compact abbreviations consistently.

Examples of the kind of outcome expected:
- Cmaj7
- Dm
- G7
- Fsus4
- Bdim

## Required checks
Review:
- chord label generation
- abbreviation formatting
- display width constraints
- progression strip rendering
- any places where chord quality text is inconsistent, too verbose, or not surfacing correctly

## Guidance
This is not just cosmetic. Clear compact chord labels are part of making the progression editor understandable.

---

# 4. Right-click/context menu for chord/music-theory actions must work reliably

## Required outcome
The section/progression editor must support dependable context access for chord/music-theory actions.

At minimum, right-click/context invocation should reliably allow access to things like:
- chord root / quality editing
- duplicate / clear / split cell
- diatonic substitute
- secondary dominant
- borrowed chord
- extend chord

## Required checks
Review and fix:
- mouse event routing
- target cell selection before menu invocation
- context target consistency
- menu display anchoring
- command application to the correct selected chord cell

## Important guidance
If right-click is unreliable in the current widget structure, fix the event flow rather than papering over it.

If necessary, add a fallback context button or explicit menu trigger for the selected cell — but only if right-click still cannot be made dependable.

---

# 5. Reduce repeated failed attempts

## Goal
Stop accumulating partial fixes that do not resolve the actual underlying problem.

## Required direction
For each of the four priority issues above, identify:
- what the current failure actually is
- what underlying assumption or mapping is wrong
- what the minimal dependable fix is

Then implement the fix cleanly.

Do not keep piling small UI changes on top of unresolved interaction-model bugs.

---

## Strong constraints

- Do not spend time on synth panel polish in this pass.
- Do not pivot into knob/slider experimentation yet.
- Do not broaden this into a general styling pass.
- Do not add more timeline features until directionality is correct.
- Do not add more structure UI complexity until drag/drop and context editing are dependable.

---

## Deliverables

Provide:

### A. Timeline-direction fix
A correction so Zone D and the chord/structure track agree on left-to-right time.

### B. Working section drag/drop
A dependable section → arrangement drag-and-drop flow.

### C. Chord abbreviation fix
Readable, compact chord labels in the progression/structure UI.

### D. Working theory context menu
Reliable context-menu access for chord/music-theory editing actions.

### E. Short summary
A concise note describing:
- what was actually broken in each case
- what was fixed
- what, if anything, is still intentionally deferred

---

## Acceptance criteria

- Zone D and the chord/structure track no longer appear to run in opposite directions
- left-to-right time is visually consistent
- basic section drag-and-drop works reliably
- chord abbreviations render clearly and compactly
- right-click/context-menu chord editing is dependable
- the pass improves correctness and usability rather than adding more surface area

---

## Non-goals for this pass

- synth control polish
- knob/slider layout experimentation
- broad styling sweep
- major timeline feature expansion
- unrelated menu/settings work

---

## Deliverable style

Be direct and practical.
This is a stabilization pass. Fix the fundamental timeline/structure interaction problems first so later synth and UI refinement work rests on something solid.
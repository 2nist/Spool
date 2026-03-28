# Theme Editor + Agent Workflow

Use this workflow to style Spool systematically and deliberately.

## Core rule
The user chooses visual direction.
The agent implements structure, propagation, and cleanup.
The Theme Editor becomes the shared source of truth.

---

## Phase 1 — Build the styling foundation
Goal: make the Theme Editor reliable and capable enough to carry real styling choices.

Agent responsibilities:
- wire the Theme Editor correctly
- expose style controls for buttons, sliders, knobs, switches/selectors
- add live preview surfaces
- make save/load/export dependable
- keep ThemeData / ThemeManager / ThemeEditorPanel coherent

User responsibilities:
- do not judge the whole app yet
- only judge whether the editor can express the kinds of style choices you need

Stop condition:
- the Theme Editor can edit real control families and export/persist those choices

---

## Phase 2 — Design inside the Theme Editor
Goal: make the big visual decisions in one place.

User responsibilities:
- adjust control families first
- decide density first
- decide button/sliders/knobs/switches first
- then tune color and semantic states
- save named presets as you test directions

Recommended order inside the editor:
1. buttons
2. sliders
3. knobs
4. switches/selectors
5. spacing/density
6. panel/card chrome
7. colors
8. selected/hovered/focused/playing/scaffold states

Agent responsibilities:
- add or improve preview surfaces if the editor is too abstract
- help expose missing style properties when needed
- not broad-propagate styling yet

Stop condition:
- one or two named presets exist that feel directionally right on preview surfaces

---

## Phase 3 — Representative surface review
Goal: test the chosen style on a few real panels before global propagation.

Agent responsibilities:
- apply the chosen preset/style definitions to 1–2 representative surfaces only
- preferably one strong Zone A panel and one non-Zone-A surface
- summarize what was mapped and what still falls outside the theme system

User responsibilities:
- review actual UI, not just property values
- judge density, hierarchy, readability, and control feel
- reject anything that drifts from the intended feel

Stop condition:
- the chosen style works on real surfaces, not just in isolated previews

---

## Phase 4 — Controlled propagation
Goal: apply the style system globally where the workflows are stable enough.

Recommended propagation order:
1. shared controls
2. small headers / hints / labels
3. panel/card chrome
4. row/list treatment
5. stable zones/components
6. unstable areas later

Agent responsibilities:
- propagate the chosen style system
- reduce hardcoded drift
- keep compactness intact
- avoid redesigning unstable workflows under the name of styling

User responsibilities:
- review each propagation step in chunks
- do not approve a giant all-at-once sweep blindly

Stop condition:
- the app shows family resemblance without losing compactness or zone identity

---

## Phase 5 — Final cleanup
Goal: remove remaining drift and edge-case ugliness.

Agent responsibilities:
- normalize outliers
- clean up local hardcoding
- align remaining controls that should match the system

User responsibilities:
- focus on exceptions, not whole-theme reinvention

Stop condition:
- the app feels cohesive and deliberate rather than patched together

---

## Working rules

### 1. Never propagate before approving control language
Do not style the whole app before buttons/sliders/knobs/switches feel right.

### 2. Preview before sweeping
A preview panel or representative surface is worth more than ten explanations.

### 3. Keep unstable workflows out of the beauty pass
Do not heavily style timeline/structure/synth areas that are still changing layout or interaction logic.

### 4. Use presets as design checkpoints
Save presets with names like:
- Dense Industrial A
- Warm Machine B
- Dark Editorial C

This makes iteration much easier than vague verbal descriptions.

### 5. Make exports real artifacts
Exports should become reusable style assets, not just clipboard snippets.

---

## Best division of labor

User:
- decides feel
- decides density
- approves control families
- rejects wrong taste quickly

Agent:
- wires the editor
- exposes missing style properties
- builds previews
- propagates approved choices
- removes drift

---

## Practical session format

Use this loop:
1. choose one style target inside the Theme Editor
2. adjust it visually
3. save a preset
4. ask the agent to apply it to one representative surface
5. review
6. either refine preset or approve propagation

Do not skip from raw theme edits to full global restyle.

---

## Short version

The Theme Editor should become the place where you choose the look.
The agent should become the thing that makes that chosen look real everywhere else.

That is the least painful way to do this deliberately.
# Codex Prompt — Explain the Music Engine Better in the UI

Implement a focused UI/UX refinement pass in Spool to better explain the sequencer and harmonic engine.

## Context

Spool already has a smart music engine, including concepts such as:
- note modes
- role-based behavior
- harmonic source selection
- follow vs local behavior
- variable-length steps
- micro-events / note events inside steps
- structure-aware realization
- drum and melodic sequencing

The problem is not lack of capability. The problem is that the UI does not explain enough of what the engine is doing.

Users need to better understand:
- what is stored
- what is derived
- what is being realized right now
- why a note or phrase behaves the way it does

This pass should improve legibility and trust without turning the sequencer into a giant DAW piano roll.

## Primary goals

1. Add realized-note preview where it matters.
2. Make follow/local behavior more visible and understandable.
3. Clarify harmonic source and note interpretation context.
4. Improve timing/playhead clarity.
5. Strengthen distinction between selection state and playback state.

## Product intent

The sequencer should feel:
- smart
- understandable
- musically intentional
- less magical/confusing

This pass should reduce the feeling of “the engine did something clever but I do not know why.”

## Scope

Focus on the melodic sequencer/editor experience first, while improving drum clarity where appropriate.

At minimum inspect and improve the UI around:
- `SlotPattern`
- melodic step grid/editor
- detail view for step events / micro-events
- current playhead visualization
- follow/local indicators
- note mode / harmonic source / role display

Do not broaden this into a full sequencer redesign.

## Required improvements

### 1. Realized-note preview
Add a compact preview of the currently realized note/pitch outcome where practical.

Examples of useful information:
- stored note value
- note mode
- harmonic source
- realized note name / octave under current structure/chord

This does not need to be shown everywhere at all times.
It should appear where it is most useful, such as:
- selected step
- selected micro-event
- detail editor
- contextual info strip

Goal:
The user should be able to tell the difference between:
- authored value
- current realized output

### 2. Clearer follow vs local behavior
Make follow/local behavior much more legible.

Current badges or tiny labels are not enough if the behavior materially changes playback.

Improve this through some combination of:
- stronger badge/icon treatment
- more obvious border/state styling
- compact explanatory text in detail/context areas
- clearer naming if current labels are too cryptic

The user should not have to guess whether a step is:
- following structure/chord context
- using local fixed behavior

### 3. Harmonic source clarity
Make harmonic source and note interpretation clearer.

For selected material, surface context such as:
- note mode
- role
- harmonic source
- whether realization is chord-relative, scale-relative, or local

This can be done with compact metadata display rather than large panels.

### 4. Timing clarity
Improve visibility of timing relationships, especially for:
- pattern length
- step duration
- micro-event placement inside a step
- playhead position

Add or improve:
- clearer ruler/grouping logic
- better visual subdivision cues
- stronger distinction between coarse step position and finer intra-step playback position where applicable

The user should more easily understand where events land in time.

### 5. Stronger selection/playhead distinction
Make these clearly different at a glance:
- selected step
- selected note/micro-event
- current playback/playhead position
- hover/focus state

Do not let selection styling and playback styling blur together.

## Strong guidance

This pass is about explanation and trust, not feature sprawl.

Do not solve this by adding giant inspectors or turning the sequencer into a full piano roll clone.
Keep the UI compact and musically focused.

## Suggested implementation direction

### A. Add a compact contextual info strip or detail summary
For selected melodic content, show a compact line or cluster such as:
- role
- note mode
- harmonic source
- follow/local state
- realized note preview

Keep it concise and readable.

### B. Improve badges and labels
Review the current badge language for:
- FOL / LOC
- note mode abbreviations
- role abbreviations
- harmonic source labeling

Retain compactness, but ensure the user can actually infer meaning.

### C. Improve the selected-event detail view
Where a detailed micro-event editor already exists, make it the place where realization becomes understandable.

Good detail view information would include:
- stored authored value
- realized note result
- note length/offset
- role/mode/source context

### D. Improve playhead rendering
Where the current playhead is too coarse, improve the distinction between:
- current step
- current intra-step position

Do this without adding excessive animation or visual noise.

## Drum-side guidance
If the drum editor has similar ambiguity around selection vs playback, improve that as well, but melodic explanation is the priority.

For drums, clarity improvements might include:
- stronger active vs selected vs currently playing distinction
- clearer repeat/ratchet indication
- clearer velocity/accent indication

Do not let drum improvements derail the melodic clarity goal.

## Constraints

- Do not perform a broad sequencer architecture refactor in this pass.
- Do not turn the sequencer into a giant piano roll.
- Do not add large permanent inspector panels.
- Do not bloat Zone B vertically.
- Keep the UI compact and information-dense.
- Prefer contextual information over always-visible clutter.

## Acceptance criteria

- The UI better communicates what the music engine is doing.
- Realized-note preview exists in a useful place for melodic content.
- Follow/local behavior is clearly visible and understandable.
- Harmonic source / note interpretation context is easier to read.
- Timing and playhead relationships are more legible.
- Selection state and playback state are clearly distinct.
- The sequencer remains compact and does not sprawl.

## Non-goals for this pass

- full sequencer redesign
- full piano-roll implementation
- major engine refactor
- adding many new music-theory features
- arrangement/timeline completion

## Deliverable

A focused UI refinement pass that makes Spool’s sequencer and harmonic engine easier to understand, easier to trust, and more musically legible without sacrificing compactness.
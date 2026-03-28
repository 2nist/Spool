# Codex Prompt — Enforce Left-to-Right Timeline and Waveform Flow

Implement a focused timeline-direction correction pass for Spool so that track timelines, clip placement, and waveform flow read **left to right** in the normal expected way.

## Context

Zone D / timeline work is in progress, and it has become clear that some timeline behavior or rendering logic is currently running backwards relative to normal user expectations.

For a music timeline / arrangement view, the expected default is:
- time increases from **left to right**
- earlier events/clips appear on the **left**
- later events/clips appear on the **right**
- waveform progress and clip placement follow the same direction
- playhead movement and time rulers should reinforce that same logic

This pass should correct that directionality consistently.

---

## Primary goals

1. Make Zone D timeline time flow left to right.
2. Make track lanes and clip placement obey the same left-to-right time model.
3. Make waveform rendering/read direction match the same convention.
4. Ensure rulers, beat markers, clip positioning, and playhead movement all agree.
5. Avoid partial fixes where one layer is flipped but another still behaves backwards.

---

## Product intent

The timeline should feel immediately legible and conventional:
- left = earlier
- right = later

Spool can still keep its own personality and loop-aware tape/cylinder concept, but the user should not have to mentally reverse time to understand the arrangement.

---

## Required direction

Review and correct the timeline/time-direction logic across all relevant layers, including where applicable:
- beat-to-x mapping
- clip placement mapping
- waveform rendering direction
- ruler / tick rendering
- playhead visualization
- scroll / wrap logic
- drag/drop placement assumptions
- any helper functions that assume the opposite direction

Do not treat this as a paint-only fix if underlying coordinate logic is reversed.

---

# 1. Canonical rule

Establish one canonical timeline rule:

- increasing beat/time/sample position maps to increasing x position
- earlier content appears farther left
- later content appears farther right

This rule should be the source of truth for Zone D timeline drawing and placement.

If there are helper functions that currently embed the opposite assumption, correct or replace them.

---

# 2. Clip and lane placement

## Goal
Make clip placement behave consistently under the new/correct direction.

## Required direction
Verify or correct:
- clip start position mapping
- clip length rendering direction
- drag/drop placement into lanes
- timeline projection of authored content
- scaffold content placement
- lyrics markers and automation lane constructs if already integrated or in progress

The user should never see a clip whose visual left edge means “later.”

---

# 3. Waveform direction

## Goal
Ensure waveform rendering also reads left to right.

## Required direction
Any waveform shown in track/timeline context should render so that:
- earlier samples are on the left
- later samples are on the right

If any current rendering pass is mirroring or indexing backward, fix it.

This applies to timeline-facing waveform visuals, not necessarily every stylized tape/history visualization elsewhere in the app.

## Guidance
If there are intentional non-timeline visualizations elsewhere (for example a stylized history/tape display), do not casually break those. Focus on timeline/track/lane waveforms and any areas that are supposed to behave like normal arrangement views.

---

# 4. Ruler, ticks, and playhead

## Goal
Make the ruler and playhead reinforce the same time direction.

## Required direction
Review and correct:
- beat markers
- bar markers
- time labels
- section markers if applicable
- playhead movement direction
- wrap/loop indicators if present

If the playhead is centered and the world scrolls around it, the surrounding content still needs to obey left=earlier, right=later.

Do not leave contradictory cues where the ruler implies one direction and the clips imply another.

---

# 5. Loop/cylinder concept

## Important guidance
Spool’s tape/cylinder/loop-aware concept can remain, but it should not invert basic time understanding.

If Zone D uses wrapping or loop-relative visuals:
- preserve the loop-aware concept
- but make local timeline reading obey left-to-right progression

The loop metaphor should not force the arrangement to read backwards.

---

# 6. Interaction checks

## Goal
Make sure user interaction matches the corrected time model.

## Required direction
Verify behaviors such as:
- clicking in the timeline to position or scrub
- dragging clips or markers
- dropping clips into the timeline
- selecting content at earlier/later positions

The interaction model should feel natural under left-to-right time flow.

---

# 7. Strong constraints

- Do not redesign the whole timeline in this pass.
- Do not overbuild new timeline features while correcting directionality.
- Do not apply superficial mirroring if the underlying placement model is still reversed.
- Do not break intentionally stylized non-timeline views unless they are incorrectly acting as timelines.

---

## Deliverables

Provide:

### A. Canonical direction fix
A clear correction so Zone D timeline logic follows left-to-right time progression.

### B. Consistent visual alignment
Clip placement, waveform flow, ruler/ticks, and playhead all agree.

### C. Interaction sanity
Basic timeline interactions feel natural under the corrected direction.

### D. Short summary
A concise note explaining what was corrected and whether any intentionally nonstandard views were left unchanged.

---

## Acceptance criteria

- Zone D timeline time now reads left to right
- clip placement is consistent with left=earlier, right=later
- timeline-facing waveforms also read left to right
- ruler/ticks/playhead agree with the same direction
- the fix is systemic enough to avoid contradictory layers
- the pass does not sprawl into unrelated timeline redesign work

---

## Non-goals for this pass

- full timeline completion
- major arrangement feature expansion
- deep clip editing overhaul
- broad visual restyling unrelated to time direction

---

## Deliverable style

Be direct and practical.
Treat this as a correction pass to normalize timeline directionality before further Zone D work continues.
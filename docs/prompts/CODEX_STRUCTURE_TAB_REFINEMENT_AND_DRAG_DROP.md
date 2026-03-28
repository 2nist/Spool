# Codex Prompt — Structure Tab Refinement and Drag/Drop for Section → Arrangement Flow

Implement the next refinement pass for the Structure tab in Spool with a strong focus on:

1. improving the musical hierarchy and usability of the Structure tab
2. making drag-and-drop work reliably
3. supporting an intuitive two-column Section → Arrangement workflow
4. strengthening progression editing as part of song-form authoring

This is a refinement pass, not a total redesign.

---

## Context

The Structure tab has made real progress and now has a meaningful backbone:
- sections list
- arrangement list
- section editor
- progression strip
- song summary
- structure-follow / rail-seeding hooks

The next problem is not lack of capability. It is refinement.

In particular:
- drag-and-drop was attempted but did not work reliably
- the two-column section-to-arrangement relationship should feel much more natural
- the panel still leans slightly toward a form/property-sheet feel instead of a strong musical structure workflow
- the progression area is promising but should feel more central and more connected to the hierarchy

---

## Primary goals

1. Make drag-and-drop work reliably where it meaningfully improves structure authoring.
2. Strengthen the two-column Section → Arrangement workflow.
3. Differentiate Sections and Arrangement more clearly in purpose and presentation.
4. Make progression editing feel more central to musical structure authoring.
5. Refine the Structure tab without bloating it or turning it into a giant editor.

---

## Product intent

The Structure tab should feel like:
- a musical song-form tool
- a place to build reusable sections
- a place to place/reorder those sections into an arrangement
- a place to shape each section’s harmonic loop and structural role

It should feel less like a generic property sheet.

---

# 1. Drag-and-drop: make it real and dependable

## Goal
Implement drag-and-drop in the Structure tab where it most clearly improves authoring.

## Required direction
Review and repair the drag-and-drop model rather than adding more drag behavior on top of a broken foundation.

At minimum, make drag-and-drop work reliably for meaningful actions such as:
- dragging sections into the arrangement column
- reordering arrangement blocks via drag-and-drop if practical
- reordering sections via drag-and-drop if practical and safe

## Strong guidance
Do not fake drag-and-drop visually if the underlying data model is not actually updated reliably.

If previous attempts failed, inspect:
- hit testing
- source/target ownership
- list model refresh behavior
- selection synchronization
- order-index resequencing
- redraw/update timing

## Desired result
The user should be able to think:
- build sections on the left
- place them into arrangement on the right

That should be the heart of the Structure tab workflow.

## Acceptance criteria
- section → arrangement drag-and-drop works reliably
- dragged items produce correct arrangement/model changes
- visual feedback during drag is clear enough to understand the drop target
- reorder behavior is reliable if implemented

---

# 2. Two-column Section → Arrangement workflow

## Goal
Make the left-to-right relationship between Sections and Arrangement feel obvious and musical.

## Required direction
Refine the UI so that the user understands:
- Sections = reusable building blocks/templates
- Arrangement = song order / playback sequence / form realization

This relationship should be clear visually and behaviorally.

## Guidance
The arrangement column should not feel like just another list that happens to sit below or beside sections.
It should feel like the destination/ordering surface for section instances.

Refine:
- hierarchy
- labels
- button/action wording
- row treatments
- drag/drop affordances
- section-color or identity carry-through into arrangement

## Nice-to-have if practical
- small visual “instance” language in arrangement items
- clearer ordering emphasis in arrangement rows
- visible connection between section identity and its arrangement entries

## Acceptance criteria
- sections and arrangement feel like different roles, not twin lists
- the section-to-arrangement flow is easier to understand at a glance
- the arrangement column feels like song-form construction, not just list management

---

# 3. Progression editing should feel more central

## Goal
Make the progression strip and chord-cell editing feel more musically central to the Structure tab.

## Required direction
The progression area is already promising, but it should feel less like a lower subpanel and more like a core part of section authoring.

Review:
- hierarchy of the progression area relative to other controls
- selected-cell clarity
- loop/cell vs bars explanation
- chord editing flow
- relationship between section length and looping progression cells

## Guidance
Do not necessarily add more controls.
Instead, improve:
- focus
- readability
- semantic importance
- quick-edit flow

The user should more easily understand:
- how many bars are in the section
- how many progression cells exist
- how those cells loop across the section

## Acceptance criteria
- progression editing feels more central and better explained
- selected chord cell is clearer
- looped-cell behavior across bars is easier to understand
- progression editing feels more like musical shaping and less like form-entry plumbing

---

# 4. Reduce the property-sheet feel

## Goal
Refine the panel so it feels more like a musical structure tool and less like a long stacked form.

## Required direction
Review the structure editor hierarchy and grouping.

Possible refinements may include:
- stronger grouping of section identity / phrase-shape controls
- tonal controls as a grouped secondary block
- clearer separation between reusable section editing and arrangement usage
- summary block that reads more like form readback than static metadata

## Guidance
Do not add empty space to solve this.
Use hierarchy, grouping, and better relationship design instead.

## Acceptance criteria
- the panel feels less like a property editor
- the structure workflow feels more musical and intentional
- grouping and hierarchy reduce cognitive friction

---

# 5. Keep compactness

## Strong constraints
- Do not bloat the Structure tab vertically.
- Do not turn it into a giant timeline or document editor.
- Do not overbuild drag-and-drop beyond the most meaningful flows.
- Do not add decorative complexity that weakens usability.
- Keep the panel compact and information-dense.

---

# 6. Suggested implementation priorities

Implement in this order:

1. fix / establish reliable drag-and-drop for section → arrangement
2. improve two-column section/arrangement relationship and visual differentiation
3. strengthen progression strip clarity and editing emphasis
4. refine grouping/hierarchy to reduce property-sheet feel
5. leave a short next-step plan for further refinement

---

## Deliverables

Provide:

### A. Working drag-and-drop
A dependable section → arrangement drag-and-drop flow, plus reordering where practical.

### B. Structure-tab refinement
A clearer two-column musical workflow between Sections and Arrangement.

### C. Progression refinement
A more central, clearer progression editing experience.

### D. Short summary
A concise note describing:
- what drag/drop now supports
- what hierarchy refinements were made
- what remains deferred

---

## Acceptance criteria

- drag-and-drop meaningfully works in the Structure tab
- section → arrangement authoring feels more natural
- sections and arrangement are easier to distinguish conceptually and visually
- progression editing feels more central and more understandable
- the tab remains compact and does not sprawl
- the pass refines the existing strong foundation rather than replacing it unnecessarily

---

## Non-goals for this pass

- full timeline editing inside the Structure tab
- giant structure-system redesign
- deep arrangement/timeline merger
- broad visual restyling unrelated to structure workflow

---

## Deliverable style

Be explicit and practical.
Treat this as a refinement pass for an already promising Structure tab: make drag-and-drop real, strengthen the section-to-arrangement workflow, and improve the musical hierarchy without bloating the panel.
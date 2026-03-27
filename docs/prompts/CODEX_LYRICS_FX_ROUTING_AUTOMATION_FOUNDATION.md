# Codex Prompt — Lyrics Panel, FX Workflow, Routing Matrix, and Automation Foundation

Implement the next foundational development pass for Spool focused on four areas that should be built before deeper timeline and later-stage UI refinement:

1. Lyrics Panel
2. FX Panel workflow completion
3. Routing system foundation
4. Automation foundation

This pass should prioritize practical structure and workflow clarity over visual overgrowth.

---

## Context

Spool already has strong concepts in sequencing, structure, FX, capture, and arrangement. Before pushing further into timeline completion and later UI refinement, several workflow-critical areas need stronger foundations.

These systems should be treated as meaningful product subsystems, not placeholders:
- lyrics capture and later placement into timeline/structure
- focused FX editing and upcoming MIDI-FX support
- authoritative routing for MIDI, audio, and effects
- initial automation model and UI concept

This should be a planning + implementation-oriented pass with compact, scalable architecture.

---

## Primary goals

1. Add a usable Lyrics Panel for jotting and later timestamping ideas into the timeline.
2. Simplify and finish the Zone C FX workflow around one-effect-at-a-time focused editing.
3. Establish a robust routing system with distinct tabs for MIDI, Audio, and FX.
4. Create a real starting point for automation rather than leaving it vague.
5. Keep all of this compact and consistent with Spool’s product direction.

---

## Important guidance

Do not try to fully finish every one of these systems in a giant pass.

Instead:
- define the winning workflow direction
- implement the most important foundation pieces
- leave obvious extension points
- avoid sprawling UI or parallel half-systems

This pass should reduce ambiguity and establish clean authority boundaries.

---

# 1. Lyrics Panel

## Product intent

The Lyrics Panel should function like a compact idea pad where the user can:
- jot down lyric ideas
- keep rough fragments/snippets
- optionally timestamp or place them into the timeline later

It should feel closer to a structured musical notebook than a generic word processor.

## Required direction

Build a compact lyrics/notepad workflow with support for:
- multiline free text entry
- simple formatting choices such as:
  - a small collection of fonts
  - color choices
  - size choices
- lightweight text organization appropriate for song idea capture
- future or initial support for timestamping/placing text into the timeline

## Strong constraints

- Do not turn this into a full rich-text editor.
- Do not create a giant document editor UI.
- Keep it compact and idea-first.
- The formatting system should be intentionally small and musically useful.

## Recommended implementation direction

### A. Lyrics note model
Create a clear model for lyric/note items, for example supporting:
- text content
- optional style attributes (font choice, size, color)
- optional timestamp or beat position
- optional section association later

### B. Panel workflow
The panel should support:
- typing notes/fragments quickly
- selecting or marking text/snippets for timeline placement later
- if timeline insertion is not fully built yet, create a meaningful placeholder workflow and data path for future placement

### C. Timeline relationship
The lyrics system should be built so that future timestamped placement in the timeline is natural.
Do not hardwire it into a dead-end text box model.

## Acceptance criteria for lyrics
- There is a usable Lyrics Panel.
- The user can jot down lyric ideas quickly.
- A small formatting palette exists (fonts, color, size).
- The data model supports future or initial timeline timestamp/placement.
- The panel remains compact and does not become a word processor.

---

# 2. FX Panel

## Product intent

Zone C should stop pretending to be multiple things at once and become a focused effects workflow.

The desired model is:
- one effect shown at a time
- maximize available controls for that effect
- previous effect in the chain shown at the INPUT side
- next effect (or ADD) shown at the OUTPUT side

This should create a more focused sound-design workflow than the current multi-tab ambiguity.

## Required changes

### Remove or eliminate dead UI
Delete or retire the current Settings and Routing tabs in the FX panel if they are not real product surfaces.
Do not keep fake tabs.

### Focused one-effect workflow
Rework the FX editing flow so that:
- one effect is the current focused effect
- the focused effect gets maximum control space
- the previous effect in chain is represented at INPUT
- the next effect or ADD slot is represented at OUTPUT

This should preserve chain awareness without fragmenting attention.

## MIDI effects
Start implementing MIDI effects as a real category in the system.

This does not require a giant finished MIDI-FX ecosystem yet, but it does require:
- a place in the model for MIDI effects
- a clear path for where they sit relative to audio effects
- an intuitive routing relationship to the rest of the app

## Strong guidance for MIDI-FX
Do not bolt MIDI effects awkwardly into the audio-FX mental model.
Think clearly about:
- where MIDI-FX live
- what they process
- what they output to
- how they route into sequencer, instruments, or other zones

## Recommended implementation direction

### A. Focused FX editor model
Add a selected/current effect concept for Zone C.

### B. Chain context UI
Represent:
- previous node at input
- current focused node in center/main view
- next node or add-node at output

### C. MIDI-FX foundation
Add the beginnings of an effect-type distinction such as:
- audio effects
- MIDI effects

This should connect to the routing model rather than becoming a separate island.

## Acceptance criteria for FX
- dead Settings/Routing tabs are removed or retired from Zone C
- one effect at a time is the primary editing workflow
- chain context is preserved via input/output adjacency
- more controls are available for the focused effect
- MIDI-FX have a real model/foundation and are not just mentioned conceptually

---

# 3. Routing

## Product intent

Spool needs a robust, intuitive, authoritative routing system.

Think in the spirit of a routing table like AUM’s routing screen, but adapted to Spool’s zones and internal model.

The routing system should be:
- clear
- tabular
- authoritative
- easy to inspect
- easy to edit
- compatible with future growth

## Required direction

Create a routing subsystem and UI with separate tabs for:
- MIDI
- AUDIO
- FX

Each should be easy to inspect and edit.

## Strong guidance

Routing should not remain implied, scattered, or magical.
It should become a real authoritative subsystem.

Do not hide important routing logic in multiple unrelated zones if it can be represented clearly in one routing surface.

## Recommended implementation direction

### A. Routing model
Define a real routing data model that can represent:
- source
- destination
- signal type
- optional channel/bus context
- enable/disable state
- ordering where relevant

### B. Routing UI
Create a compact but robust tabular screen for:
- MIDI routes
- audio routes
- FX routes

### C. Authority
The routing model should be authoritative for routing relationships rather than just reflecting scattered UI state.

### D. Connection to MIDI-FX and audio-FX
Routing should be designed so that MIDI effects and audio effects can be represented cleanly.

## Acceptance criteria for routing
- there is a real routing subsystem/model
- routing can be inspected in MIDI / AUDIO / FX tabs
- routing is more authoritative and less scattered
- the UI is compact and practical, not ornamental
- the design can support future expansion without becoming opaque

---

# 4. Automation

## Product intent

Automation needs at least a real starting concept and foundation.
It should stop being a vague future idea.

## Required direction

Create an initial automation model and concept that can grow later.
This does not need to fully finish automation, but it should define:
- what automation targets are
- where automation data lives
- how automation relates to timeline/structure/playback
- how the UI will begin to expose it

## Recommended implementation direction

### A. Automation model foundation
Define automation entities such as:
- target reference
- value points or lanes
- timing/beat positions
- enable/disable state
- possibly scope (slot, effect, route, mixer, etc.)

### B. Initial UI concept
Create a practical initial UI concept rather than a full large editor.
For example:
- lane placeholders in relevant contexts
- automation-aware targeting hooks
- compact initial lane or point display where appropriate

### C. Relationship to timeline
Automation should be designed with the future timeline model in mind.
Do not create an automation model that cannot eventually live alongside arrangement/timeline editing.

## Strong guidance

- Do not overbuild the automation UI right now.
- Do not leave automation as a vague comment-only idea.
- Build a real foundation that future timeline work can connect to.

## Acceptance criteria for automation
- there is a real automation model or foundation
- automation targets are clearly defined conceptually and in code
- the UI has an initial meaningful concept or entry point
- the design is compatible with future timeline-based expansion

---

## Cross-cutting constraints

- Keep all new UI compact.
- Do not sprawl vertically unless absolutely necessary.
- Prefer strong foundations over broad unfinished surfaces.
- Avoid fake tabs and dead controls.
- Avoid multiple competing authorities for routing, automation, or lyrics placement.
- Integrate these systems into the existing product direction rather than treating them as isolated features.

---

## Deliverables

Provide:

### A. Architectural direction summary
A concise summary for:
- Lyrics Panel model and placement strategy
- FX workflow direction
- routing authority/model
- automation foundation

### B. Focused implementation
Implement the most important foundation pieces safely and compactly.

### C. Clear next-step plan
List the next safe steps for each subsystem instead of pretending everything is fully complete.

---

## Deliverable style

Be explicit and opinionated.
Do not just list possibilities.
Choose a direction for each subsystem and implement a compact, scalable foundation.
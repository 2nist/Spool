# Spool Future Development Roadmap

## Purpose

This document defines the near- and mid-term development priorities for Spool, the recommended order of work, key architectural and product risks, and practical strategy for turning the current app into a coherent product rather than a growing pile of strong ideas.

This is not a feature wishlist. It is a prioritization and execution guide.

---

## Current State

Spool already has real subsystems and a strong identity:

- Zone A: compact control/navigation concept with an emerging visual language
- Zone B: hybrid sequencer with variable-length phrase steps, micro-events, note modes, and drum sequencing
- Zone C: compact FX chain editor with working DSP chain processing
- Zone D: loop-aware tape-cylinder arrangement surface with structure hooks
- Tape/history capture concept feeding looper / reel / timeline workflows
- Structure-aware sequencing and harmonic realization concepts

What is missing is not more ideas. What is missing is completion, clarity, trust, and workflow cohesion.

---

## Product Direction

### What Spool should be excellent at first

Spool should aim to become excellent at this combined workflow:

1. **Structure-aware phrase sequencing**
2. **Capture / trim / route from tape history**
3. **Loop-aware arrangement scaffolding**

That trio is more distinctive than trying to become a full DAW clone.

### What Spool should not try to be yet

- a full conventional DAW competitor
- a full performance looper workstation
- a giant modular routing environment
- a giant piano-roll editor clone
- a plugin host ecosystem before core workflows are solid

---

## Top-Level Priorities

### Priority 1 — Trust and persistence
Without dependable state recall, the app cannot become a serious product.

Must complete:
- full session save/load
- transport-related state consistency
- slot patterns persistence
- drum state persistence
- FX chain persistence
- structure persistence
- looper/edit state persistence when looper becomes real
- Zone D lane/clip persistence when arrangement content becomes real

### Priority 2 — One authoritative musical model
The app currently has overlapping authorities for structure, song/query state, and runtime sequencing behavior.

Must decide and converge:
- what is the primary song/structure truth model?
- what is scaffold vs playback authority?
- what does Zone D actually represent relative to Zone B and structure?

### Priority 3 — Finished capture/edit/route workflow
Tape history, captured clip transfer, looper, reel, and timeline should form one coherent pipeline.

Core target workflow:
- hear something in tape history
- grab region
- load to looper editor
- trim / normalize / loop / overdub if needed
- drag or send to REEL / TIMELINE

### Priority 4 — Explain the music engine better
The sequencer and harmonic behavior are already smart. The UI does not explain enough.

Must improve:
- realized-note preview
- clearer follow/local behavior
- harmonic source clarity
- timing clarity
- stronger selection/playhead distinction

### Priority 5 — Tighten visual and interaction discipline
The app has good visual ideas, but some surfaces promise more than they currently deliver.

Must reduce drift and dead UI weight:
- standardize Zone A compact styling
- hide or remove fake/incomplete tabs and controls
- stop exposing secondary systems before primary ones are good

---

## Recommended Order of Development

## Phase 1 — Stabilize the core without broad refactors

### Goals
- preserve momentum
- avoid destructive architectural churn
- refine behavior before reorganizing code

### Work items
1. Finish Zone A compact style contract
2. Improve Zone B sequencer legibility and editing flow
3. Finish first real looper/edit pass in Zone B
4. Clarify Zone D clip/lane semantics
5. Improve Zone C parameter access in compact form
6. Expand persistence coverage significantly

### Deliverables
- consistent Zone A visual language
- usable tape → looper → destination workflow
- clearer sequencer behavior and lower confusion
- fewer placeholder UI surfaces
- meaningfully better save/load reliability

---

## Phase 2 — Make the workflows trustworthy

### Goals
- establish reliable end-to-end usage patterns
- make the app feel dependable instead of experimental

### Work items
1. Full session serialization pass
2. Looper save/load + normalized clip workflow
3. Zone D drop / place / select / move / resize fundamentals
4. Chain copy/paste/preset operations in Zone C
5. Better structure-follow behavior and note realization feedback

### Deliverables
- session reload restores meaningful musical work
- captured audio becomes durable material
- Zone D becomes a real destination, not just a pretty monitor
- Zone C becomes a more complete sound-design rack

---

## Phase 3 — Converge the model

### Goals
- reduce overlapping authorities
- prepare for sustainable growth

### Work items
1. Decide primary song/structure authority
2. Clearly define relationship between:
   - StructureState / StructureEngine
   - SongManager
   - SlotPattern runtime sequencing
   - Zone D arrangement clips
3. Reduce dual-authority behavior paths
4. Normalize transport start/stop semantics through one canonical path

### Deliverables
- fewer hidden behavior mismatches
- clearer ownership of song/arrangement data
- safer future development

---

## Phase 4 — Product hardening

### Goals
- make the app easier to understand, demo, and eventually ship

### Work items
1. Write real documentation and onboarding
2. Add chain/sequence/template presets where useful
3. Define stable “great at” workflows
4. Reduce or hide dev-only or incomplete UI
5. Improve naming consistency across zones and features

### Deliverables
- usable README / docs
- demoable workflows
- more coherent product story

---

## Zone-by-Zone Priorities

## Zone A — Navigation / compact control surface

### Keep
- icon-based tabs
- compact density
- Instrument panel as geometry/layout reference

### Improve
- shared Zone A style helper
- deterministic accent mapping
- standardized header/badge/row treatment
- stop local hardcoded color pools
- use Instrument panel density as the baseline

### Avoid
- oversized spacing
- airy card-style expansion
- per-panel styling inventions

### Priority level
**High** for polish and team consistency, but not as high as persistence or looper flow.

---

## Zone B — Sequencer and looper

### Sequencer priorities
- realized-note preview
- clearer timing ruler and playhead feedback
- stronger distinction between:
  - selected step
  - selected event
  - playhead
  - follow/local mode
- drum lane utilities:
  - copy lane
  - paste lane
  - rotate lane
  - constrained randomization / Euclidean tools later

### Looper priorities
The looper should be a **compact clip editor and routing workspace**, not a full classic looper yet.

Build now:
- load captured clip
- waveform view
- trim in/out
- loop audition
- normalize
- save/load
- send to REEL / TIMELINE
- drag-and-drop out
- constrained overdub

Do not prioritize yet:
- multi-looper orchestration
- large performance-looper feature set
- sprawling waveform editor UI

### Priority level
**Very high**. This is one of the strongest opportunities to make Spool feel unique and product-worthy.

---

## Zone C — FX chain editor

### Keep
- compact vertical chain metaphor
- per-slot persistent chains
- add/remove/reorder/bypass flow
- DSP-backed effect processing

### Improve
- full parameter access in a compact way
- selected-node workflow
- chain copy/paste/reset/preset operations
- module-type-aware default chains
- either make tabs real or collapse to CHAIN-only for now

### Avoid
- more effect types before better access/workflow
- giant plugin-style editors per node
- fake tabs that imply subsystems that do not exist yet

### Priority level
**Medium-high**. Strong subsystem already; needs refinement more than expansion.

---

## Zone D — Arrangement / timeline surface

### Keep
- tape-cylinder loop-aware identity
- structure lane
- center-playhead wrapped loop concept
- lane labels and loop-aware feel

### Improve first
- define clip semantics clearly
- define lane meaning clearly
- distinguish scaffold clips from real editable content
- implement first editing actions:
  - select
  - move
  - resize
  - duplicate
- make drop-to-Zone-D from tape/looper meaningful

### Avoid
- too much effort on mixer gutter before clip model is solid
- adding more transport chrome
- letting the cylinder metaphor make editing awkward

### Priority level
**High strategically**, but should follow basic looper and persistence work unless timeline drop/edit is needed immediately.

---

## What Should Go, Shrink, or Stay Hidden for Now

### Hide/remove now
- Zone C Routing tab if not real
- Zone C Settings tab if not real
- visible multi-looper affordance before single-looper workflow is excellent

### De-prioritize heavily
- Zone D mixer gutter complexity
- extra transport-strip surface area and controls that are not central

### Replace later
- fake default full-loop clips as if they are real content
- slot-index-based default FX chain behavior

---

## Strategy to Avoid Pitfalls

## 1. Refine behavior before refactoring structure
Do not perform broad architecture refactors before musical behavior is stable.

Refactor only when one of these is true:
- a bug cannot be fixed cleanly without it
- the same confusion has hit development repeatedly
- the next feature clearly needs a new seam
- two paths that should behave identically are drifting apart

## 2. One axis of change at a time
Do not refactor across transport, sequencing, arrangement, and UI at once.

Change one axis at a time:
- transport
- sequencer
- looper
- FX
- timeline
- persistence

## 3. Keep audio-thread discipline strict
Any new feature touching playback must be checked for:
- allocation in audio paths
- hidden locks
- mutable shared-state hazards
- UI-thread assumptions inside DSP logic

## 4. Kill transitional logic on purpose
Temporary bridges are fine. Unnamed transitional logic that stays forever is not.

For any temporary path, mark it as one of:
- fallback
- bridge
- legacy
- authoritative
- scheduled for deletion

## 5. Hide UI promises before they become expectations
Do not expose controls, tabs, or surfaces that strongly imply mature workflows unless those workflows actually exist.

## 6. Keep compactness as a product rule
Spool is strongest when compact, dense, and purposeful.

General UI rule:
- prefer hierarchy through color, typography, and grouping
- do not solve clarity with large padding and sprawling layouts
- avoid oversized card stacks and excessive whitespace

## 7. Build complete loops of value
Prefer end-to-end workflow completion over scattered feature accumulation.

Good example:
- Tape capture → Looper edit → Send to REEL / TIMELINE

Bad example:
- adding more tabs, more placeholders, more surfaces, more buttons, but not finishing one real workflow

---

## Specific Product Risks

### Risk 1 — Strong ideas, weak cohesion
The app already contains multiple excellent ideas. The danger is that they stay adjacent rather than becoming one coherent system.

### Risk 2 — Clever but under-explained behavior
The sequencer and harmonic systems can become frustrating if the UI does not explain realized musical output clearly.

### Risk 3 — UI surface overreach
Too many semi-real surfaces create expectation debt and slow real progress.

### Risk 4 — Refactor trauma before model convergence
Broad refactors too early could destroy momentum and confidence.

### Risk 5 — Becoming a DAW clone by accident
Spool is more interesting when it leans into structure, capture, and loop-aware arrangement rather than trying to imitate every DAW feature category.

---

## Advice for Tying This Together as a Product

### Product framing
Spool should be framed as a **composition and arrangement workstation built around structure, capture, and loop-aware development**, not as a conventional DAW.

### Core workflow story
A user should be able to understand the app like this:

1. Build or follow structure
2. Author phrases and patterns in slots
3. Shape sound with compact FX chains
4. Capture good moments from tape history
5. Trim and route them quickly
6. Arrange them in a loop-aware timeline

That is a clear and compelling product story.

### What makes it potentially special
- structure-aware sequencing instead of flat loop repetition
- tape-history capture as a first-class creative source
- compact workflow between phrase generation, audio capture, and arrangement
- a product that feels like composition-first, not engineering-first

---

## Practical Near-Term Milestones

### Milestone A — “Trustworthy Core"
- better save/load
- clearer sequencer feedback
- Zone A styling discipline
- Zone C compact param depth

### Milestone B — “Capture Workflow Real"
- working looper waveform editor
- trim / normalize / save / load / drag / send
- tape → looper → destination is reliable

### Milestone C — “Arrangement Starts to Matter"
- meaningful Zone D clip semantics
- first clip editing actions
- scaffold vs content clarity

### Milestone D — “Model Convergence"
- reduced dual-authority logic
- clearer song/structure ownership
- safer future expansion

---

## Final Guidance

Spool does not need more broad invention right now.
It needs:

- finished workflows
- fewer ambiguous models
- stronger persistence
- clearer explanation of musical behavior
- less UI over-promise

If those are handled well, the app has a real chance to become a distinctive product rather than a permanently interesting prototype.

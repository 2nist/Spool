# Codex Prompt — Define One Authoritative Musical Model for Spool

Spool now needs a deliberate architectural pass to define one authoritative musical model.

This is not a broad refactor for cleanliness. It is a convergence pass to reduce overlapping musical authorities and make future sequencing, structure, looper, and arrangement work more dependable.

## Context

Spool currently contains multiple overlapping musical/state authorities, including concepts such as:
- StructureState / StructureEngine
- SongManager
- SlotPattern runtime sequencing
- Zone D lane/clip arrangement concepts
- transport-driven runtime behavior in PluginProcessor

This was reasonable during rapid development, but it is now a source of risk.

The app needs a clear answer to:

**What is the authoritative musical model, and what are the other layers relative to it?**

## Primary goals

1. Define one authoritative musical model for the app.
2. Clarify the roles of existing systems rather than letting them overlap indefinitely.
3. Reduce hidden dual-authority behavior.
4. Create a stable basis for persistence, sequencing, arrangement, and structure-follow behavior.
5. Avoid a destructive rewrite of working behavior.

## Important framing

This is not a request to immediately rewrite the whole app into a new architecture.

This is a request to:
- inspect the current systems
- define the winning model clearly
- classify other systems as authoritative, derived, adapter, runtime, scaffold, or legacy
- make a practical convergence plan

The result should be a concrete architectural direction and a limited implementation pass where appropriate.

## Likely current overlap to analyze

At minimum, inspect and reason about the relationship between:
- `StructureState`
- `StructureEngine`
- `SongManager`
- `SlotPattern` and runtime pattern compilation
- transport/runtime sequencing in `PluginProcessor`
- Zone D clip/lane model
- any persistence/state-restore mechanisms tied to musical content

## Required outcome

The implementation and/or architecture notes should answer these questions clearly:

### 1. What is the primary source of truth for song/form/structure?
Examples:
- sections
- repeats
- progression/chords
- song-level key/mode
- arrangement scaffolding

### 2. What is the primary source of truth for authored sequence material?
Examples:
- phrase/pattern content
- drum steps
- step timing
- micro-events
- slot-local authored content

### 3. What is the primary source of truth for arrangement placement?
Examples:
- Zone D clips
- structure-derived scaffold clips
- audio clip placement
- slot-backed lanes vs independently arranged material

### 4. Which systems are runtime projections only?
Examples:
- compiled runtime patterns
- transport caches
- temporary scheduler state
- playback-only derived views

### 5. Which systems are legacy/bridge systems that should not remain authoritative long-term?

## Strong guidance

The likely best direction is not “everything lives in PluginProcessor” and not “let all models coexist forever.”

A healthy likely split would look something like:
- **authoritative authored model**
- **derived runtime/playback model**
- **derived visual/scaffold model**

You do not need to force this exact split if the code suggests a better one, but the final design should clearly separate:
- authored truth
- runtime playback projection
- UI/arrangement visualization

## What to avoid

- Do not do a broad rewrite just to make the code look cleaner.
- Do not delete working systems without replacing their role clearly.
- Do not leave multiple systems as equal authorities.
- Do not make Zone D, SongManager, and StructureState all silently compete for truth.
- Do not produce vague architecture notes without concrete recommendations.

## Recommended approach

### Phase 1 — Analyze current authority boundaries
Inspect current code and identify:
- where musical truth is authored
- where it is transformed
- where it is merely displayed
- where two systems currently overlap or disagree

### Phase 2 — Define the winning model
Produce a clear mapping such as:
- authoritative
- derived
- runtime-only
- adapter/bridge
- legacy-to-remove

### Phase 3 — Implement minimal convergence steps
Make a focused implementation pass that improves clarity without broad breakage.
Examples may include:
- comments / docs in code
- clearer ownership boundaries
- redirecting one path to another canonical path
- renaming or wrapping misleading accessors
- reducing one obvious dual-authority case

### Phase 4 — Leave a concrete convergence plan
Document what should happen next, in order, rather than pretending the whole convergence is complete in one pass.

## Deliverables

Provide the following:

### A. Architectural decision summary
A concise summary of:
- the chosen authoritative musical model
- the role of each existing subsystem
- what is still transitional

### B. Code-level implementation where practical
Implement small but meaningful convergence improvements where safe.
Do not destabilize working behavior.

### C. Clear classification of current systems
For each major system, classify it as one of:
- Authoritative
- Derived
- Runtime-only
- UI/scaffold
- Legacy/bridge

### D. Next-step convergence plan
List the next small safe steps required to continue convergence.

## Specific acceptance criteria

- There is a clearly stated authoritative musical model.
- Existing overlapping systems are classified instead of left ambiguous.
- At least one concrete dual-authority problem is reduced or clarified.
- The result helps persistence, sequencing, and arrangement work become more dependable.
- The implementation avoids a destructive full rewrite.

## Product intent

This work should help Spool become:
- easier to reason about
- safer to extend
- more dependable for save/load and playback
- better able to unify structure, sequencing, capture, and arrangement

## Non-goals for this pass

- full app-wide refactor
- rewriting every subsystem into a new architecture immediately
- redesigning the UI around the new model
- adding major new features unrelated to model convergence

## Deliverable style

Be explicit and opinionated.
Do not just describe possibilities.
Choose a direction, classify the current systems, implement safe convergence where possible, and leave a practical plan for the next pass.
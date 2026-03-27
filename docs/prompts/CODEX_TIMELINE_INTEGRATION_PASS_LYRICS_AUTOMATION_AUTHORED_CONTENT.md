# Codex Prompt — Timeline Integration Pass for Lyrics, Automation, and Authored Content

Implement the next Zone D / timeline pass in Spool as an **integration pass**, not a full timeline completion pass.

## Context

Recent work has established important foundations:
- lyrics as authored song data
- automation as authored song data
- routing as an explicit authority/model
- focused FX workflow direction
- convergence toward a more authoritative musical model

The next step is **not** to fully finish the timeline as a giant DAW-like editor.

The next step is to make Zone D the place where these new authored systems become visibly real and musically useful.

This pass should connect the newly established models into the timeline in a compact, legible way.

---

## Primary goals

1. Integrate lyrics into Zone D as meaningful timeline constructs.
2. Integrate automation into Zone D as real lane/scaffold constructs.
3. Clarify Zone D clip/content semantics so authored content and scaffold content are visually distinct.
4. Strengthen Zone D as a visual projection of authored musical data without overbuilding the editor.
5. Keep the timeline compact and avoid sprawling into a full arrangement workstation in one pass.

---

## Important framing

This is a **timeline integration pass**.
It is **not** a request to:
- fully finish the timeline
- build a giant clip editor
- complete automation editing in full
- turn Zone D into a DAW clone

This pass should focus on making Zone D the first meaningful visual integration surface for:
- lyrics
- automation
- authored musical content
- structure-derived scaffold

---

## Product intent

Zone D should begin to feel like:
- the place where authored song content becomes visible in time
- the place where structure, lyrics, automation, and clips start to coexist
- a loop-aware arrangement surface with clear semantic layers

It should **not** yet become a huge editing monster.

---

# 1. Lyrics → Zone D integration

## Goal
Make beat-pinned or timestamped lyrics visibly appear in the timeline in a meaningful, compact way.

## Required direction
Use the authored lyrics data/model to project lyric content into Zone D.

### At minimum
Support:
- lyric markers or lyric notes in Zone D based on stored beat/timestamp position
- a compact visual representation for pinned lyric items
- enough labeling/context to make them useful without overwhelming the lane

## Guidance
- Keep lyric visuals compact.
- Distinguish them from audio/midi clips and structure scaffold.
- Do not turn this into a giant karaoke editor yet.
- Use the stored `beatPosition` or equivalent authored placement data meaningfully.

## Nice-to-have if practical
- section-aware or structure-aware lyric grouping if the current model supports it safely
- selection/focus behavior for lyric markers

## Acceptance criteria
- authored lyric items can appear in Zone D when pinned/timestamped
- lyric constructs are visually distinct from clips and scaffold
- the implementation uses authored lyric placement data rather than fake placeholders

---

# 2. Automation → Zone D integration

## Goal
Make automation become visually real in the timeline without building a full editor yet.

## Required direction
Use the automation model/foundation to project automation into Zone D as compact lanes or lane-like constructs.

### At minimum
Support:
- visible automation lane scaffolding or markers
- target/scope labeling for automation lanes
- clear indication that automation belongs to a specific target/domain

## Guidance
- Do not build a giant automation editor in this pass.
- Focus on presence, identity, and basic temporal existence.
- Show that automation has a place in the timeline model.

## Strong suggestion
Use lane placeholders or lightweight rendered point/curve hints rather than full editing mechanics.

## Acceptance criteria
- automation appears in Zone D in a meaningful way
- automation constructs identify their target or scope clearly
- the result feels like the start of a real automation system, not a decorative placeholder

---

# 3. Clarify Zone D content semantics

## Goal
Make it visually and conceptually clear what kind of timeline content the user is looking at.

Zone D should distinguish between at least these categories where they exist:
- structure-derived scaffold content
- authored clips/content
- lyric markers/notes
- automation lanes or automation constructs

## Required direction
Review current clip/lane visual semantics and ensure these categories do not blur together.

## Guidance
This may involve:
- different styling treatments
- different card/marker forms
- different lane roles
- ghosted or secondary styling for scaffold material

The user should not confuse:
- guide/scaffold content
- actual authored content

## Acceptance criteria
- scaffold vs authored content is visually distinguishable
- lyrics and automation do not look like ordinary clips
- Zone D reads more clearly as a layered musical timeline rather than one generic clip soup

---

# 4. Keep timeline scope disciplined

## Strong constraints
Do **not** use this pass to implement all of the following:
- full clip editing overhaul
- giant arrangement editor
- rich automation editor
- lyric rich-text editing inside Zone D
- broad DAW-style clip manipulation system
- deep audio-region editing

If a small amount of selection/focus behavior is needed, keep it minimal and in support of clarity.

The purpose of this pass is integration and semantic clarity.

---

# 5. Relationship to the authored model

## Required direction
Where possible, Zone D should now behave more clearly as a **visual projection of authored data**, not a competing silent authority.

This means:
- lyrics shown in Zone D should derive from authored lyrics data
- automation shown in Zone D should derive from authored automation data
- structure scaffold should remain visibly derived/scaffold-like

Do not create new hidden competing authorities for these constructs inside Zone D unless absolutely necessary.

---

# 6. Implementation guidance

## A. Lyrics lane/marker integration
Choose a compact representation such as:
- markers on an existing lane
- a dedicated lyrics lane
- lightweight labeled note markers

Whatever you choose, it should be compact, readable, and beat-aware.

## B. Automation lane integration
Choose a compact representation such as:
- per-target automation lanes
- small automation sub-lanes
- lane overlays where safe

Prefer clarity over decorative complexity.

## C. Content classification
Review current `ClipData` / lane semantics and introduce clearer distinctions where needed.

## D. Minimal interaction
Only add interaction that directly improves understanding, such as:
- selection/focus
- hover identity
- target/label visibility

Do not overbuild editing.

---

## Acceptance criteria

- Zone D now visibly integrates authored lyrics data.
- Zone D now visibly integrates automation data in an initial meaningful form.
- scaffold vs authored content is more clearly distinguished.
- lyrics, automation, and clips do not all read as the same generic object type.
- the timeline remains compact and does not sprawl.
- the pass strengthens Zone D as the visual integration surface for authored song data.

---

## Non-goals for this pass

- finishing the entire timeline
- full arrangement editing system
- full automation editor
- lyric document editing in the timeline
- large clip manipulation redesign
- deep audio editing in Zone D

---

## Deliverable

A focused Zone D integration pass that makes lyrics, automation, and authored musical content visibly real in the timeline, clarifies semantic layering, and prepares the timeline for later deeper editing without prematurely turning it into a giant arrangement editor.
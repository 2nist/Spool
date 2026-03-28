# Codex Prompt — Controlled Global Style Matching from Established Zone A Patterns

Now that several Zone A panels have a stronger styling direction, begin a **controlled global style matching pass** across the app.

This is **not** permission to redesign everything.
It is a propagation/refinement pass to carry the strongest established visual patterns into the rest of the app in a disciplined way.

---

## Context

A few Zone A surfaces now provide the clearest current design language in the app.
These should be treated as the current visual reference points.

The goal now is to start matching that language more globally so the app feels like one product rather than multiple visual dialects.

However, this must be done carefully:
- not every surface is equally mature
- not every subsystem should be restyled in one shot
- the app still has core workflow issues in some areas
- compactness is still non-negotiable

So this pass should be **style matching**, not full visual invention.

---

## Primary goals

1. Use the strongest current Zone A patterns as the visual reference language.
2. Start propagating shared styling decisions globally where they are stable enough.
3. Reduce obvious visual drift between zones/components.
4. Keep the pass controlled, compact, and reversible.
5. Avoid broad redesign or workflow churn disguised as style work.

---

## Important framing

This pass should answer:
- where can we safely match style now?
- what existing Zone A patterns are mature enough to reuse?
- what visual mismatches are most worth cleaning up first?

This pass should **not** answer:
- what should the final design of every part of the app be forever?

---

## Reference-source rule

Use the best current Zone A panels/components as the visual source of truth for matching patterns such as:
- control density
- button treatment
- spacing discipline
- small header styling
- chip/badge language
- control grouping
- compact row behavior
- border/stroke logic

Where Zone A already solved something well, prefer matching it rather than inventing new local styles elsewhere.

Do not blindly copy Zone A everywhere if a surface has different functional needs.
Translate the language, do not clone it mechanically.

---

# 1. Identify what should be matched globally now

## Goal
Choose the visual patterns that are mature enough to propagate.

## Likely candidates
Focus on stable/shared patterns such as:
- text button styling
- compact combo/select styling
- text editor styling
- slider/bar-slider treatment where already standardized
- section header treatment
- inline hint/secondary label treatment
- panel/card chrome
- row selection/hover treatment where safe

## Guidance
Do not start with the most experimental surfaces.
Start with the patterns already repeated and already working well.

## Acceptance criteria
- the pass identifies and reuses stable styling patterns
- global matching is based on proven components, not guesses

---

# 2. Match style globally in safe layers first

## Goal
Propagate style in the safest order rather than touching every component equally.

## Recommended order
1. shared controls
2. small headers / labels / hints
3. panel chrome / card treatment
4. row/list styling
5. zone-specific refinements only after the shared layer is cleaner

## Guidance
This means:
- match control language first
- then typography hierarchy and secondary text language
- then panel surfaces/borders
- only then deal with local exceptions

This avoids the app looking globally “sort of matched” while the controls still all disagree.

---

# 3. Keep compactness intact

## Non-negotiable rule
Do not let global style matching bloat the UI.

## Required direction
When matching styling globally:
- do not increase row heights casually
- do not enlarge padding just to make things feel polished
- do not add whitespace as a substitute for hierarchy
- do not make controls larger unless functionally justified

The goal is:
- more coherent
- not more spacious

---

# 4. Control language should be the first global match target

## Goal
Now that some Zone A controls are styled well, start matching that control language more broadly.

## Required direction
Prioritize matching for:
- buttons
- combos/selectors
- sliders where the same control family exists elsewhere
- switches/toggles where possible
- text-entry fields

## Guidance
If a Zone A control treatment is clearly the best current version, use it as the reference.
If a different zone has a control with different functional needs, adapt the shared language rather than pasting an exact clone.

## Acceptance criteria
- control families feel more related across the app
- the app feels less like each zone invented its own control universe

---

# 5. Typography and information hierarchy should follow

## Goal
Match the text hierarchy globally using the stronger current Zone A examples.

## Required direction
Review and align:
- small headers
- secondary hints
- summary labels
- body/value text treatment
- section-label emphasis

## Guidance
This is not a global font redesign.
It is about making labels, hints, and headers feel like they belong to the same product.

---

# 6. Panel/card/row treatment should be normalized where safe

## Goal
Reduce obvious visual drift in panel chrome and row treatments.

## Required direction
Where appropriate, align:
- panel background treatment
- rounded-rect/card treatment
- border/stroke use
- selected row treatment
- hover treatment
- list row density

## Guidance
Do not force identical styling into places where the semantics are different.
But do remove obvious one-off visual drift.

---

# 7. Do not overreach into unstable areas

## Goal
Keep this pass from becoming a disguised redesign.

## Required direction
Avoid pushing heavy style propagation into areas that are still functionally unstable, especially if they are still changing structurally.

Examples of areas to be cautious with:
- timeline/Zone D if interaction semantics are still being corrected
- structure interactions if drag/drop and context behavior are still in flux
- synth/editor areas that are mid-layout redesign

You may align shared control language there lightly, but do not perform deep local restyling until their workflows are stable enough.

---

# 8. Deliver a matching pass, not a style explosion

## Required outcome
The result should feel like:
- clearer family resemblance across the app
- less local styling drift
- more consistent control language
- stronger product coherence

It should **not** feel like:
- an unrelated new design suddenly dropped on the whole app
- every zone flattened into the same look
- more padding and chrome for the sake of polish

---

## Deliverables

Provide:

### A. Matching strategy summary
A concise note explaining which Zone A patterns were treated as the current reference language.

### B. Focused global propagation
A controlled implementation of style matching across the safest reusable layers.

### C. Drift cleanup summary
A short list of the most important visual mismatches that were normalized.

### D. Deferred areas
A clear note of which areas were intentionally left for later because they are still structurally unstable.

---

## Acceptance criteria

- stable Zone A styling patterns are reused more globally
- control language feels more consistent across the app
- typography/hints/headers feel more unified
- panel/card/row drift is reduced where safe
- the app remains compact
- unstable areas are not over-restyled prematurely
- the pass improves coherence without becoming a full redesign

---

## Non-goals for this pass

- full app-wide restyle
- replacing all local exceptions in one shot
- broad workflow/layout redesign
- forcing identical styling onto every zone regardless of function
- polishing unstable timeline/structure surfaces as if they are already final

---

## Deliverable style

Be disciplined and selective.
Use the best current Zone A patterns as the source language, propagate them globally where they are mature enough, reduce obvious drift, and leave unstable areas for later rather than overreaching.
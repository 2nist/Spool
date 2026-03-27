# Codex Prompt — Styling Pass 0: System, Rules, and Helper Layer Before the Big Sweep

Implement a **Styling Pass 0** for Spool before attempting a broad styling/refinement sweep across the whole app.

This pass is not the big restyle itself.
It is the pass that makes the big restyle safer, more consistent, and less likely to turn into patchwork.

---

## Context

Spool already has a strong theme foundation in `Theme.h` and related theme infrastructure:
- colors
- spacing scale
- radii
- strokes
- typography
- zone accents
- helper drawing functions
- live theme editing / theme manager support

However, a large styling pass still needs:
- clearer rules
- stronger shared helper coverage
- compactness guardrails
- semantic visual-state standards
- fewer local one-off decisions

Without this setup work, a big styling pass will drift into inconsistent local fixes, oversized layouts, and hard-to-maintain exceptions.

---

## Primary goals

1. Define the styling rules clearly enough that future large and small styling changes follow the same system.
2. Build the shared helper layer needed for a broad restyle.
3. Establish compactness rules so the styling pass does not accidentally bloat the app.
4. Define semantic visual-state rules that apply consistently across the UI.
5. Prepare the app for zone-by-zone styling passes instead of an uncontrolled full-app repaint.

---

## Important framing

This pass should **not** try to fully restyle the entire application.

This pass should:
- codify the styling contract
- add missing reusable primitives/helpers
- reduce hardcoded styling drift
- establish visual semantics
- leave the app ready for later shell / Zone A / Zone B / Zone C / Zone D styling passes

Think of this as:
**build the brush kit and paint rules before repainting the house**.

---

## Product intent

Spool should remain:
- compact
- information-dense
- zone-distinct
- musically purposeful
- visually coherent

The styling system should help the app feel unified without making every zone look identical.

---

# 1. Define the styling contract

## Goal
Create a short internal styling specification in code/comments/docs form that future styling passes can follow.

## Required areas to define
At minimum, codify the rules for:
- shell bars
- panel headers
- row/list items
- badges/chips/toggles
- hover/selected/focused/playing/armed/disabled states
- scaffold vs authored content treatment
- typography hierarchy
- compact control sizing
- zone accent usage

## Guidance
This does not need to become a giant design bible.
It should be concise, practical, and specific enough to guide implementation.

A short markdown doc and/or strong code comments near the helper layer is fine.

## Acceptance criteria
- there is a clearly stated styling contract for the most reused UI patterns
- the contract is practical enough to guide later styling passes
- the contract reinforces compactness and consistency

---

# 2. Strengthen the shared helper layer

## Goal
Add the missing reusable drawing/styling helpers needed for the broad styling pass.

## Context
`Theme::Helper` already provides a few useful helpers such as panel drawing, zone stripe drawing, and step-pad drawing. That is a good start, but not enough for a large consistency pass.

## Required direction
Add practical shared helpers for commonly repeated UI patterns.

Good likely candidates include functions conceptually like:
- standard header drawing
- badge/chip drawing
- row background drawing
- selection outline/focus ring drawing
- compact info chip/toggle drawing
- maybe zone-specific helper wrappers where justified

These do not need to be named exactly this way, but the capability should exist.

## Guidance
- Keep the helper layer practical.
- Do not build a giant abstract UI framework.
- Focus on the patterns that are already repeated across the app.
- Prefer using Theme tokens everywhere rather than local hardcoded styling.

## Acceptance criteria
- the helper layer now covers more of the app’s common visual patterns
- future styling passes can reuse helpers instead of reimplementing look-and-feel locally
- helper APIs stay compact and understandable

---

# 3. Compactness rules (non-negotiable)

## Goal
Prevent the big styling pass from accidentally bloating the app.

## Required direction
Establish explicit compactness rules that later styling work must follow.

These should include guidance such as:
- do not enlarge panels unless functionally necessary
- do not increase row heights casually
- do not solve clarity with large empty padding
- do not add extra wrapper containers just for visual separation
- prefer hierarchy through typography, contrast, borders, and grouping rather than whitespace

## Guidance
Spool already has strong vertical-space pressure and compact workflows.
The styling system should protect that rather than undo it.

## Acceptance criteria
- compactness rules are explicitly established
- future styling work has clear guardrails against sprawl

---

# 4. Semantic visual-state rules

## Goal
Define one stable visual language for important UI states.

## Required direction
Create or codify consistent treatment for semantic states such as:
- hover
- selected
- focused
- playing/current playhead
- armed/record-enabled
- disabled/inactive
- scaffold/guide content
- authored/real content

## Guidance
A user should not have to relearn what “selected” or “playing” means in each zone.

These states do not need identical visuals everywhere, but they should share a stable logic.

Examples:
- selected should always read as selection, not sometimes hover and sometimes playback
- scaffold should always feel more ghosted/secondary than authored content
- disabled should consistently reduce contrast/alpha in a similar way

## Acceptance criteria
- semantic state styling is more clearly defined
- a stable visual language exists for the most important recurring states

---

# 5. Eliminate obvious local styling drift

## Goal
Use this pass to clean up the most obvious hardcoded local styling patterns that directly conflict with the theme system.

## Required direction
Inspect for local hardcoded values that should instead come from the shared theme/helper system, especially in areas like:
- repeated color pools
- repeated panel chrome
- repeated row hover/select logic
- repeated badge treatments
- local spacing/radius/stroke inventions

Do not try to sweep every single component in this pass.
Focus on the most harmful drift and on the foundations that make future cleanup easier.

## Acceptance criteria
- the worst styling drift is reduced
- the app relies more on the central theme/helper system
- future cleanup becomes easier rather than harder

---

# 6. Prepare zone-by-zone follow-up passes

## Goal
Leave the app ready for controlled styling passes by zone rather than one giant all-at-once repaint.

## Required output
At the end of this pass, leave a short recommended next-order plan such as:
1. shell
2. Zone A
3. Zone C
4. Zone B
5. Zone D
6. final cleanup pass

Or another order if the code suggests something better.

## Guidance
The purpose is to avoid a chaotic “style everything everywhere” pass.

---

# 7. Strong constraints

- Do not fully restyle the whole app in this pass.
- Do not introduce a giant custom UI framework.
- Do not increase layout sizes casually.
- Do not leave every component free to invent its own micro-style.
- Do not create style helpers so abstract that no one will use them.
- Do not let the app lose its compactness in the name of polish.

---

## Deliverables

Provide:

### A. Styling contract
A concise practical styling spec/rule set for the app.

### B. Shared helper expansion
A stronger helper layer for repeated visual patterns.

### C. Compactness and semantic-state rules
Explicit rules that later styling passes should follow.

### D. Limited drift cleanup
Targeted cleanup of the worst current local styling drift.

### E. Next styling pass order
A short recommended order for the major styling passes that should follow.

---

## Acceptance criteria

- the app has a clearer styling system contract
- the helper layer is stronger and more reusable
- compactness rules are explicit
- semantic visual-state rules are clearer
- future styling passes are easier to execute consistently
- this pass improves the system without prematurely repainting the whole app

---

## Non-goals for this pass

- restyling every component
- full shell redesign
- full Zone A/Zone B/Zone C/Zone D polish in one pass
- introducing broad workflow changes disguised as style work
- replacing the theme system with something new

---

## Deliverable style

Be explicit and practical.
This is a foundation pass for a larger styling effort. Build the rules, the helper layer, and the guardrails first so later styling work is faster, cleaner, and more consistent.
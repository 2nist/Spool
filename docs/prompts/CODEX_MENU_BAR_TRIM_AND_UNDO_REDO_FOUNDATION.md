# Codex Prompt — Trim Menu Bar Import/Export for Now and Add Undo/Redo Foundation

Implement a focused shell/workflow pass in Spool with two goals:

1. Remove or trim menu bar Import/Export actions that do not yet represent real media workflows.
2. Add a real undo/redo foundation if possible, with practical first coverage.

This should be a disciplined pass, not a broad command-system rewrite.

---

## Context

The menu bar is now becoming a real workflow surface, and Save/Open are working well enough to be useful.

However:
- Import/Export currently suggest media workflows that are not ready yet.
- The app still needs undo/redo support to feel dependable during editing.

This pass should make the shell more honest and introduce a realistic undo/redo foundation.

---

## Primary goals

1. Remove misleading Import/Export menu expectations for now.
2. Add a real undo/redo model where it can be done safely.
3. Keep the menu bar product-facing and honest.
4. Avoid fake or decorative undo/redo if the underlying change model is not yet covered.

---

# 1. Trim menu bar Import/Export for now

## Goal
If Import/Export do not yet represent strong real media workflows, remove or simplify them for now.

## Required direction
Review the current menu bar structure and trim it so it reflects what is actually dependable today.

If media import/export is not truly ready, prefer:
- removing the top-level Import and Export menus for now, or
- reducing them to only actions that are genuinely meaningful today

## Guidance
Do not keep menu items that imply finished workflows such as broad media import/export if those are not actually product-ready.

Theme-related import/export should live under Settings / Appearance / Theme rather than pretending to be a general media workflow if that is the more accurate product structure.

## Desired result
The menu bar should be more honest.
It should not over-promise import/export capability before those workflows are real.

## Acceptance criteria
- misleading or premature Import/Export menu items are removed or reduced
- theme-related import/export remains discoverable through the correct settings/theme path
- the top-level menu bar feels cleaner and more truthful

---

# 2. Add undo/redo foundation

## Goal
Add real undo/redo support if possible, beginning with the most practical and user-visible editing actions.

## Important framing
Do not add fake undo/redo that only lights up menu items without actually covering meaningful edits.

If full app-wide undo/redo is too broad for one pass, build a real foundation and cover a sensible initial subset of actions.

## Product intent
Undo/redo should make the app feel safer during authoring.
The user should be able to edit without feeling like every click is permanent.

## Recommended initial coverage
Prioritize coverage for authored editing actions that are:
- common
- local enough to track reliably
- valuable to the user immediately

Good likely starting candidates:
- lyrics edits / note additions or placement changes
- structure edits if already command-like
- sequencer step/pattern edits where practical
- routing table edits if they are discrete and model-based
- automation lane/point changes if simple enough

Do not force full coverage of every subsystem if the change model is not ready yet.

## Strong guidance
Use a real command/change model where possible.
Avoid scattered ad hoc “store previous value in UI component” hacks.

A good direction would include:
- a central undo manager or command history owner
- explicit transactions/commands for meaningful authored changes
- a way for menus and shortcuts to call Undo and Redo consistently

## Architectural guidance
- Reuse JUCE undo infrastructure if that is the most practical fit.
- Keep the first implementation compact and real.
- Prefer model-level undoable actions over paint/UI-level hacks.
- Do not attempt to wrap the entire app in one pass.

## Menu bar integration
Add Undo and Redo to the correct menu location.
A likely good structure is:
- File
- Edit
- View
- Settings

If adding Edit is the right move, do it.
Undo/Redo should not feel buried or improvised.

Also support standard shortcuts where practical:
- Undo
- Redo

## Required outcome
At the end of this pass, the app should have:
- a clear undo/redo foundation
- menu access to Undo/Redo
- real coverage for at least one meaningful set of authored edits
- a clear statement of which edits are covered now and which are deferred

## Strong constraints
- Do not claim app-wide undo if only a few subsystems are covered.
- Do not build fake menu items with no real state tracking.
- Do not create a giant refactor just to support undo everywhere at once.
- Do not bury undo logic inside unrelated UI classes if model-level actions are possible.

---

## Deliverables

Provide:

### A. Menu-bar cleanup
A cleaner, more honest top-level menu structure reflecting currently real workflows.

### B. Undo/redo foundation
A real first-pass undo/redo implementation with meaningful initial coverage.

### C. Coverage summary
A concise summary of:
- what actions are undoable now
- what actions are not yet covered
- what the next safe expansion steps are

---

## Acceptance criteria

- misleading Import/Export menu items are removed or reduced for now
- theme import/export remains accessible through the right shell/settings path
- Undo and Redo are accessible from the menu bar
- Undo and Redo are backed by a real state/change model
- at least one meaningful authored editing workflow is actually undoable/redoable
- the implementation is honest about what is and is not covered

---

## Non-goals for this pass

- complete media import/export workflow
- full app-wide undo/redo coverage in one pass
- a giant command system rewrite across every subsystem
- fake placeholder undo/redo menu items

---

## Deliverable style

Be explicit and practical.
Trim the menu bar to what is real, add a real undo/redo foundation, cover the highest-value authored edits first, and leave a clear expansion plan for later.
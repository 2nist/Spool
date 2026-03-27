# Codex Prompt — Menu Bar Workflow Pass: Load, Save, Import, Export, and Theme Access

Implement the next shell/workflow pass for Spool around the menu bar.

## Context

Spool now has multiple meaningful subsystems and user-facing workflows:
- authored musical content
- settings/preferences
- theme controls and a custom theme generator interface
- lyrics, FX, routing, automation, and timeline integration work in progress

At this point, the app needs a clearer shell-level workflow for:
- creating/opening/loading things
- saving/exporting things
- importing useful content
- finding theme and customization controls

The menu bar should become the dependable top-level workflow surface for these actions.

This pass should improve product usability and discoverability, not just add decorative menus.

---

## Primary goals

1. Make the menu bar a real workflow surface.
2. Clarify how users load, save, import, and export content.
3. Make theme/customization access easy to find from the menu bar.
4. Establish better shell-level information architecture.
5. Avoid stuffing too many unfinished actions into the menu.

---

## Product intent

The menu bar should answer questions like:
- How do I start a new song/session?
- How do I open or load something?
- How do I save what I made?
- How do I import useful material?
- How do I export useful material?
- Where do I find theme and appearance controls?

It should make Spool feel more like a coherent product and less like a powerful internal tool.

---

## Required direction

Design and implement a more intentional menu-bar structure with practical workflows.

At minimum, define and support clear menu groupings for:
- File / Project
- Import
- Export
- View / Window / Layout where appropriate
- Settings / Preferences / Theme access

Do not simply dump every possible action into the menu bar.

---

# 1. File / Project workflow

## Goal
Make it obvious how a user starts, opens, saves, and manages work.

## Required actions to consider
At minimum, evaluate and implement the most relevant of:
- New song / new project
- Open / Load
- Save
- Save As
- possibly recent files/projects if practical

## Guidance
Be clear about what is being saved/opened.
If the app currently has one main session/project concept, reflect that consistently.
Do not create multiple confusing save concepts without clear meaning.

## Acceptance criteria
- The menu bar gives the user a clear path for new/open/save actions.
- Save-related actions are easy to find.
- Terminology is consistent and product-facing.

---

# 2. Import workflow

## Goal
Make it clear how external material enters the app.

## Examples to consider
Depending on current app capabilities and realistic next-step support, evaluate menu entries for importing things such as:
- MIDI
- audio clips
- lyrics/text
- theme/preset data if applicable later

## Guidance
Only add import entries for workflows that are real or immediately meaningful.
Do not create fake menu actions for unfinished systems.

If a workflow is partially implemented, the menu action should still lead to something coherent and not feel dead.

## Acceptance criteria
- Import actions are easy to find.
- Import menu items match real or near-real workflows.
- The menu is not padded with fake future actions.

---

# 3. Export workflow

## Goal
Make it clear how users get useful things out of Spool.

## Examples to consider
Depending on current real capabilities, evaluate export entries for things such as:
- rendered/exported audio
- exported MIDI
- saved clips
- song/session data
- theme exports if relevant later

## Guidance
Again: only expose what is real or nearly real.
Do not create fake export promises.

## Acceptance criteria
- Export actions are clearly grouped.
- The menu structure helps users understand what kinds of output the app supports.
- Terminology is clear and not overcomplicated.

---

# 4. Theme / appearance / settings discoverability

## Goal
Make theme access and the custom theme generator easy to find.

## Required direction
The menu bar should provide a clear route to:
- Settings / Preferences
- Theme / Appearance controls
- the custom theme generator interface

This should reinforce the settings work already underway and make customization discoverable at the shell level.

## Guidance
Do not bury theme controls in a place users would never look.
A practical menu path might be:
- Settings / Preferences
- Appearance
- Theme
- Custom Theme Generator

Reuse the actual settings/preferences workflow rather than creating disconnected theme entry points.

## Acceptance criteria
- A user can discover theme/customization controls from the menu bar.
- The custom theme generator is reachable from the menu shell.
- The menu path reinforces the real settings architecture instead of bypassing it.

---

# 5. View / layout / shell control

## Goal
Make shell-level visibility/customization more discoverable where useful.

## Examples to consider
If these are now meaningful and wired through settings/preferences, menu entries may expose or lead to:
- show/hide system feed
- show/hide song header
- compact shell/title bar mode
- open settings/preferences

## Guidance
Avoid turning the menu bar into a second inconsistent settings system.
Use it as an entry point or quick-access surface where appropriate, not as a duplicate configuration universe.

---

# 6. Information architecture and naming

## Goal
Choose a menu structure that feels product-ready.

## Guidance
Be explicit and practical about naming.
Choose a clear top-level structure rather than an arbitrary collection of labels.

A likely good direction is something like:
- File
- Import
- Export
- View
- Settings / Preferences
- Help (if there is enough real help/documentation to justify it)

Do not overbuild niche menu categories yet.

---

# 7. Strong constraints

- Do not add a huge number of dead or placeholder menu items.
- Do not expose fake workflows that are not wired.
- Do not make the menu structure more complicated than the current product maturity supports.
- Do not create a second disconnected settings/theme system via the menu.
- Keep the menu product-facing and practical.

---

# 8. Deliverables

Provide:

### A. Menu-bar structure recommendation
A concise summary of the chosen top-level menu structure and why.

### B. Focused implementation
Implement the most important real menu actions and entry points safely.

### C. Theme/settings discoverability
Ensure theme controls and the custom theme generator are easy to reach via the menu bar.

### D. Clear next-step plan
List which menu actions are still intentionally deferred until their workflows become real.

---

## Acceptance criteria

- The menu bar better explains how to load, save, import, export, and customize the app.
- Theme and custom theme generator access are discoverable from the menu bar.
- The menu structure feels more intentional and product-facing.
- The implementation avoids fake unfinished menu entries.
- The result improves shell usability without adding clutter.

---

## Non-goals for this pass

- fully finishing every load/save/import/export backend workflow if those systems are not ready
- adding large documentation/help subsystems unless they already exist
- building a giant command palette or advanced shell framework
- replacing the settings panel with menu-only controls

---

## Deliverable style

Be explicit and opinionated.
Do not just list menu possibilities.
Choose a practical menu structure, implement the real actions that should exist now, connect the settings/theme flow properly, and leave a clear plan for what remains deferred.
# Codex Prompt — Build a Working Settings Panel with Optional UI Toggles

Implement a real settings panel for Spool that allows optional UI elements to be shown/hidden and theme-related settings to be controlled from one coherent place.

## Context

Spool has a strong theming system and several UI surfaces that are useful in some situations but expensive in others, especially vertically. Examples include:
- top status/system feed strip
- song header metadata density
- optional decorative or secondary UI surfaces
- future optional per-zone affordances

The app needs a real settings panel so the user can turn these elements on or off instead of hardcoding a single layout choice.

This should become the foundation for user-facing UI/layout preferences.

## Primary goals

1. Create a real working settings panel.
2. Make selected UI elements optional via settings toggles.
3. Persist those settings across launches.
4. Integrate with the existing theme/settings architecture cleanly.
5. Keep the implementation practical and compact.

## Product intent

The settings panel should support the idea that Spool can be:
- compact and minimal
- more informative / verbose
- more theme-driven and customizable

This is not just a debug panel. It should be a real product-facing preferences surface.

## Scope

Build a first useful version of settings focused on:
- UI visibility toggles
- layout compactness / metadata visibility
- theme-related choices already supported by the codebase

Do not try to solve every future setting now.

## First settings to support

At minimum, add support for toggling these UI surfaces if they exist in the current layout:

### Top-level layout toggles
- Show system/status feed strip
- Show song header strip
- Compact song header mode (reduced metadata density)

### Zone/secondary UI toggles
Where practical and already present in the UI:
- Show optional non-essential decorative/status elements
- Keep placeholders hidden unless explicitly enabled

Only expose toggles for things that are real and meaningful. Do not add fake settings for unfinished systems.

## Persistence requirements

Settings must persist across launches using an appropriate existing preferences/settings mechanism in the app.

Use the same general persistence style as other user preferences where possible.
Do not make these session-only unless that is clearly already how app preferences are handled.

## Integration requirements

### 1. Settings panel entry point
Wire the settings panel to an existing appropriate entry point if available, such as:
- transport/settings gear button
- menu bar/preferences entry
- another already-established settings access point

Prefer reusing an existing “open settings” action if one is already stubbed or partially wired.

### 2. PluginEditor/layout integration
Make the toggles actually affect layout.
For example:
- if system feed is disabled, reclaim its height
- if song header is disabled, reclaim its height
- if compact song header mode is enabled, reduce visual density rather than keeping full spacing

The settings must change real layout behavior, not just internal flags.

### 3. Theme integration
If the app already has theme or appearance controls, surface those through this panel where practical.
Do not invent a second disconnected theme system.

## UX guidance

The settings panel should be:
- compact
- readable
- utilitarian
- consistent with Spool’s design language

Avoid:
- giant modal complexity
- over-designed preferences UI
- debug-looking checkbox dumps

Think of this as the start of a real preferences surface, not a temporary developer panel.

## Suggested structure

A practical first version might include sections like:

### Appearance
- Theme selection if already supported
- UI density / compactness mode if practical

### Layout
- Show system feed
- Show song header
- Compact song header

### Workflow
- Hide unfinished/secondary UI by default
- Other real workflow-affecting toggles only if already meaningful

Keep the first version small and real.

## Architectural guidance

- Reuse the existing theming/settings infrastructure where possible.
- Add a small persistent app-preferences model if needed.
- Avoid scattering ad hoc booleans throughout the editor.
- Centralize settings state enough that future toggles have a clear home.
- Do not perform a broad refactor of unrelated code.

## Specific implementation goals

1. Add a settings/preferences model for UI/layout preferences.
2. Add persistence for that model.
3. Create a visible settings panel component.
4. Wire it to a real app entry point.
5. Make top-level layout respond to:
   - show/hide system feed
   - show/hide song header
   - compact song header mode
6. Keep the diff focused and practical.

## Acceptance criteria

- There is a real settings panel in the app.
- It opens from a clear entry point.
- It can toggle at least the system feed strip and song header strip.
- Layout height is actually reclaimed when those elements are hidden.
- Settings persist across launches.
- The implementation fits the existing app/theming architecture.
- The UI remains compact and not overly complicated.

## Non-goals for this pass

- exhaustive settings coverage
- every possible zone customization
- complex theme editor if not already present
- redesigning the whole shell layout
- turning settings into a large standalone subsystem

## Deliverable

A focused implementation of a working settings panel that introduces persistent optional UI toggles and establishes a clean foundation for future user-configurable layout and theme preferences.
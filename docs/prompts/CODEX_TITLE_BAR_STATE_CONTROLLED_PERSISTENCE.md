# Codex Prompt — Dependable State-Controlled Persistence for Title Bar and Top Shell UI

Implement dependable state-controlled persistence for the title bar area in Spool, with particular attention to top-shell UI visibility, compactness, and settings-driven behavior.

## Context

Spool now has or is gaining:
- a title/song header area
- optional top-shell surfaces such as the system/status feed
- a settings panel with UI visibility toggles
- theme-related preferences and optional shell density choices

The next requirement is to make the title bar and related top-shell UI behavior dependable, state-controlled, and persistent.

This means:
- the UI should always reflect a real persisted settings/state model
- layout should be derived from state, not ad hoc booleans scattered in the editor
- show/hide/compact behavior should survive relaunches
- state changes should apply consistently and immediately

## Primary goals

1. Make title bar visibility and compactness fully state-controlled.
2. Make top-shell UI visibility derive from a single persistent preferences/state model.
3. Ensure layout reclamation is dependable when shell elements are hidden.
4. Make the title bar reflect real persisted state after relaunch.
5. Avoid fragile one-off layout flags and duplicated visibility logic.

## Scope

Focus specifically on the title bar / top shell area, including things such as:
- song header strip visibility
- compact song header mode
- system/status feed visibility
- any related top-offset / layout calculations in PluginEditor

Do not broaden this into all app persistence. This pass is specifically about dependable shell/title-bar state behavior.

## Product intent

The title bar area should support multiple valid shell modes, for example:
- minimal shell
- informative shell
- compact shell

But these modes must be reliable and persistent.

The user should not feel like the app randomly remembers some shell settings and forgets others.

## Requirements

### 1. Central state ownership
Create or use a single clear preferences/state owner for shell/title-bar settings.

At minimum, include state such as:
- showSongHeader
- compactSongHeader
- showSystemFeed

Do not let these remain as scattered local booleans owned independently by unrelated UI components.

### 2. Persistence across launches
These settings must persist across launches using the app’s real preferences mechanism.

Expected behavior:
- user hides song header → relaunch → still hidden
- user enables compact header → relaunch → still compact
- user hides system feed → relaunch → still hidden

### 3. State-driven layout
Top-level layout in PluginEditor must be derived from persisted shell state.

For example:
- if song header is hidden, its height should become 0 in layout
- if system feed is hidden, its height should become 0 in layout
- if compact song header mode is enabled, the header should reduce density/space in a real way

Do not leave phantom reserved height.
Do not rely on “visible but zero-alpha” type tricks.

### 4. Immediate application of state changes
When the user changes these settings in the UI:
- the shell should update immediately
- layout should be recalculated immediately
- the visible state should stay in sync with the persisted model

There should not be drift between:
- what settings say
- what the UI currently shows
- what will be restored next launch

### 5. Dependable title bar data behavior
Where title bar content itself depends on app/session state, make sure it updates in a predictable way.

Examples to verify or improve if needed:
- song title shown consistently
- BPM shown consistently
- save/unsaved state shown consistently
- compact mode still preserves the intended essential information

If compact mode reduces displayed metadata, it should do so intentionally rather than by accidental clipping.

## Architectural guidance

- Reuse the settings/preferences model already introduced for the settings panel if present.
- Make PluginEditor derive top offsets and shell heights from that state model.
- Avoid duplicated calculations in multiple places.
- Keep state flow clear: persisted preferences → shell state → layout → paint/visibility.
- Do not perform a broad unrelated refactor.

## Suggested implementation approach

1. Identify all current top-shell layout members and flags related to:
   - song header
n   - system feed
   - top offset calculation
   - compact header behavior
2. Move or normalize those decisions behind a single preferences/state access path.
3. Make layout helpers derive from current persisted state.
4. Ensure settings changes trigger a clean relayout/repaint cycle.
5. Verify persisted restore on startup.
6. Verify no dead vertical space remains when elements are disabled.

## Quality bar

After implementation:
- shell visibility is controlled by one reliable state source
- state survives relaunches
- title bar and system feed visibility behave predictably
- compact mode is intentional and visually stable
- top layout no longer feels fragile or ad hoc

## Acceptance criteria

- song header visibility is state-controlled and persistent
- compact song header mode is state-controlled and persistent
- system feed visibility is state-controlled and persistent
- PluginEditor layout fully reclaims hidden top-shell space
- settings changes apply immediately without restart
- restored state after relaunch matches prior user choice
- no duplicated or contradictory visibility logic remains in the top shell

## Non-goals for this pass

- full application-wide persistence overhaul
- redesigning the entire title bar UX
- adding large new metadata surfaces
- broad refactors outside top-shell state/layout behavior

## Deliverable

A focused implementation that makes the title bar and related top-shell UI behavior dependable, persistent, and driven by one coherent settings/state model rather than fragile ad hoc flags.
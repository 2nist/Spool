# Codex Prompt — Fully Wire the Theme Editor and Expand It Into a Real Control-Styling Tool

Implement the next major theme-system pass for Spool by making the existing theme editor fully wired, more complete, and more useful for real styling work.

This pass should shift the Theme Editor from being mostly a color/size playground into a real **style authoring tool** for the app.

---

## Context

Spool already has a real theme system foundation and a live theme editor:
- `ThemeManager`
- `ThemeData`
- `ThemeEditorPanel`
- runtime repaint/broadcast support
- preset/save/load concepts
- export functionality

However, the current Theme Editor is still limited in important ways:
- it is focused mostly on colours, fonts, spacing, and tape values
- it does not yet meaningfully expose style decisions for controls like buttons, sliders, knobs, and switches
- the export path is still closer to “copy Theme.h snippet to clipboard” than a full styling workflow
- the editor needs to become the actual home for styling choices rather than forcing those choices to be described vaguely in prompts

The goal is to let the user shape real control styling through the theme/styling tool and export those choices in a useful way.

---

## Primary goals

1. Make sure the existing Theme Editor is fully wired and discoverable.
2. Expand the theme/style model to cover control styling, not just colours and scalar sizes.
3. Add theme-editor support for styling families such as buttons, sliders, knobs, and switches/selectors.
4. Improve the export workflow so it can export styling choices meaningfully, not just describe colours in isolation.
5. Keep the result practical and grounded in the current app architecture.

---

## Important framing

This is **not** a request to build a giant Figma clone inside the app.

This is a request to:
- make the existing theme editor truly useful
- expose styling decisions that matter to the real UI
- use exports/presets as real style outputs
- reduce the need to describe design choices abstractly in chat every time

---

## What exists already

The agent should begin from the existing implementation, not reinvent it.

Relevant existing files include:
- `source/theme/ThemeManager.h`
- `source/theme/ThemeManager.cpp`
- `source/theme/ThemeData.h`
- `source/theme/ThemeEditorPanel.h`
- `source/theme/ThemeEditorPanel.cpp`
- `source/theme/ThemePresets.*`
- `source/theme/ColourRow.*`
- `source/theme/FloatRow.*`
- `source/Theme.h`

The current Theme Editor already has:
- tabs for Surface / Ink / Accent / Zones / Signal / Type / Space / Tape
- preset combo
- save-as / reset
- export button
- runtime theme broadcasting

Build on this, do not replace it casually.

---

# 1. Fully wire and verify the existing theme editor

## Goal
Make sure the existing Theme Editor is actually reachable, coherent, and working as the app’s styling hub.

## Required direction
Verify and fix where needed:
- Theme Editor entry points
- Zone A / settings / theme-designer discoverability
- live theme change propagation
- preset switching
- save/load workflow
- export workflow
- repaint/update flow after changes

## Acceptance criteria
- the Theme Editor is easy to reach
- live changes reliably affect the app where intended
- preset/save/load/export flows work coherently

---

# 2. Expand the theme model beyond colours and scalar spacing

## Goal
Make the theme/style data model capable of expressing actual control styling choices.

## Required direction
Extend `ThemeData` and related theme infrastructure so it can represent styling decisions for common control families.

At minimum, add a practical styling model for some or all of:
- button family
- slider family
- knob family
- switch/toggle family
- selector/dropdown family
- panel/card chrome options where needed

## Guidance
This does not mean adding every possible aesthetic parameter.
It means adding the parameters needed to express the app’s actual reusable control language.

Examples of useful style properties may include:
- corner radius choices
- border weight/strength
- fill strength
- track thickness
- knob ring thickness
- thumb/cap size
- selected-state intensity
- hover-state intensity
- control height classes
- chip/button density variants

Use a compact, meaningful set of properties rather than exploding the data model with endless knobs.

---

# 3. Add Theme Editor UI for control styling

## Goal
Let the user style buttons, sliders, knobs, and switches/selectors from the Theme Editor.

## Required direction
Add tabs/sections/subpanels as needed so the Theme Editor can control the reusable control families.

A good direction would be to add one or more new tabs such as:
- Controls
- Buttons
- Sliders
- Knobs
- Switches

The exact tab structure is flexible as long as it stays practical.

## Required editing scope
Expose real styling controls for at least:
- button treatment
- slider treatment
- knob treatment
- switch/selector treatment

## Guidance
Do not turn the editor into a giant control-property maze.
Keep it focused on the parameters that really define the visual language.

## Acceptance criteria
- the Theme Editor can now adjust actual control-family styling
- buttons/sliders/knobs/switches are part of the styling workflow
- the UI remains understandable and not overgrown

---

# 4. Add better style-preview capability

## Goal
Let the user see styling choices in more meaningful context than isolated scalar rows.

## Required direction
Add or improve preview surfaces inside the Theme Editor so the user can see how style choices affect representative controls.

A practical direction would be a small live preview area showing sample:
- buttons
- sliders
- knobs
- switches/selectors
- panel header / row treatment if useful

## Guidance
This preview does not need to be huge.
But it should be good enough that the user does not have to imagine what a “button corner radius” means in the abstract.

## Acceptance criteria
- the Theme Editor provides a more meaningful preview of control styling choices
- the user can make design decisions by looking, not just by reading property names

---

# 5. Improve export workflow so it exports style choices meaningfully

## Goal
Move beyond a narrow “clipboard snippet of colour constants” workflow.

## Required direction
Improve the export path so it can export the broader styling choices in a useful, reproducible format.

Possible good directions:
- export a fuller theme/style preset file
- export a compile-time theme/style definition more comprehensively
- support both runtime preset saving and a more complete export artifact

## Important guidance
The export should capture not just raw colours, but the actual styling choices the user made for control families where possible.

The goal is:
- user styles the app in the Theme Editor
- user exports/persists those choices as a real theme/style artifact
- future styling passes can use that artifact rather than guessing

## Acceptance criteria
- export reflects more of the real style system, not only a narrow subset
- exported styling choices are reusable and meaningful
- the workflow reduces the need for vague manual description of design choices

---

# 6. Keep the editor practical

## Strong constraints
- Do not build an enormous abstract design tool.
- Do not flood the editor with hundreds of tiny style properties.
- Do not make the UI harder to use than the app it is styling.
- Do not replace the existing theme system; extend it.
- Keep the result compact and product-appropriate.

---

# 7. Relationship to future styling passes

## Goal
Make the Theme Editor the real source of truth for styling direction as much as practical.

## Required direction
This pass should set up the app so that future styling work can increasingly be driven by:
- theme/style data
- control-family styling definitions
- reusable exportable presets

rather than repeated one-off design descriptions.

The Theme Editor does not need to solve everything, but it should become much more central to styling work.

---

## Deliverables

Provide:

### A. Wiring verification/fixes
A concise note on what was fixed so the Theme Editor is properly reachable and reliable.

### B. Theme/style model expansion
An extension of the theme data/model to cover real control-family styling choices.

### C. Theme Editor UI expansion
New or improved editor sections for buttons, sliders, knobs, and switches/selectors.

### D. Better preview
A live preview area or equivalent representative sample surfaces for style decisions.

### E. Better export flow
A more meaningful export path for style choices and presets.

### F. Short summary
A concise explanation of:
- what style families can now be edited
- what the export contains now
- what is still intentionally deferred

---

## Acceptance criteria

- the Theme Editor is fully wired and easy to reach
- live theme changes work reliably
- control-family styling can be edited meaningfully
- buttons/sliders/knobs/switches are now part of the theme workflow
- previews help the user see actual styling decisions
- exports capture more than just isolated colours
- the result is practical and helps future styling work move faster

---

## Non-goals for this pass

- building a massive universal GUI skinning system
- exposing every possible JUCE paint parameter
- full app-wide style propagation in the same pass
- redesigning every component immediately

---

## Deliverable style

Be practical and implementation-focused.
Use the existing Theme Editor as the foundation, wire it properly, expand it into a real control-styling tool, and make exports/presets useful enough that future styling work can be driven by actual style artifacts instead of vague descriptions.
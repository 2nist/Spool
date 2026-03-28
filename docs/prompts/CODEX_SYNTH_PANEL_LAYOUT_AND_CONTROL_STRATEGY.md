# Codex Prompt — Design the Zone A Synth Panel Layout and Control Strategy

Implement the next synth-panel refinement pass for Spool with a focus on **layout hierarchy, control choice, and readability**.

This is a design-and-implementation pass for the Zone A synth editor, not a broad styling sweep and not a random control shuffle.

---

## Context

The synth editor in Zone A is now important enough that its layout needs to be designed intentionally.

The goals are:
- make OSC 1 and OSC 2 clearer and more refined
- improve the control hierarchy
- choose better control types where they make the synth feel more musical and tactile
- keep the panel compact
- avoid polishing into chaos while more core app work continues elsewhere

There is also a specific layout direction to try:
- keep OSC 1 and OSC 2 as mirrored primary source blocks
- move `DRV`, `ASYM`, and `DRFT` out of awkward limbo and make them a shared character section
- avoid treating those controls as if they belong to the amp envelope

---

## Primary goals

1. Design a clearer synth control hierarchy.
2. Refine OSC 1 and OSC 2 into mirrored, readable source blocks.
3. Place `DRV`, `ASYM`, and `DRFT` in a better shared position.
4. Use a better mix of control types instead of defaulting to all sliders.
5. Keep the Zone A synth editor compact and product-quality.

---

## Product intent

The Zone A synth editor should feel like:
- the deeper, more intentional control surface
- tactile and musically shaped
- compact but not cramped
- more like an instrument editor and less like a spreadsheet of parameters

It should not become a giant knob wall or a random mixture of control styles.

---

# 1. Overall synth hierarchy

## Goal
Make the synth read in a more musical signal/control order.

## Desired high-level order
Use this hierarchy as the design direction unless the code strongly requires a minor variation:

1. **OSC 1 / OSC 2** — primary sound sources
2. **Character row** — shared controls: `DRV`, `ASYM`, `DRFT`
3. **Filter / tone shaping**
4. **Filter envelope**
5. **Amp envelope**

This is the intended reading model:
- source generation first
- shared tone/instability/character next
- shaping and envelopes after

## Important guidance
Do **not** group `DRV`, `ASYM`, and `DRFT` as if they are part of the amp envelope just for visual symmetry.
They are better understood as shared synth/voice character controls.

---

# 2. OSC 1 and OSC 2 refinement

## Goal
Make the oscillator blocks clearer and more symmetrical.

## Required direction
Refine OSC 1 and OSC 2 so they:
- feel like paired/mirrored source sections
- use consistent ordering of controls
- are easy to compare at a glance
- visually anchor the synth editor

## Guidance
The exact control list depends on the current implementation, but the layout should make the two oscillators feel related and easy to scan.

Use a consistent control order for both oscillator sections wherever possible.

Examples of likely per-oscillator concerns:
- waveform/shape
- detune
- octave
- level/mix
- related tone/source controls already present in the editor

## Acceptance criteria
- OSC 1 and OSC 2 feel intentionally paired
- the control order is more consistent
- the source section reads more clearly at a glance

---

# 3. Shared character row: `DRV`, `ASYM`, `DRFT`

## Goal
Move `DRV`, `ASYM`, and `DRFT` into a better shared placement.

## Required direction
Create a compact shared **Character** row or block placed:
- below the oscillator source area
- above the filter/envelope shaping area

This row should clearly feel like shared synth/voice character, not oscillator-specific and not amp-envelope-specific.

## Guidance
These controls are strong candidates for a more tactile control style.
They should not be visually buried or made to feel like afterthoughts.

## Acceptance criteria
- `DRV`, `ASYM`, and `DRFT` are grouped together intentionally
- they sit in a musically sensible place in the panel hierarchy
- they no longer feel misplaced relative to the envelopes

---

# 4. Mixed control strategy: not all sliders

## Goal
Use control types that fit the meaning of the parameter.

## Required direction
Do **not** assume every deep control should remain a slider.
Use a more intentional mixed-control strategy.

## Recommended control philosophy
Use this rule:
- **knobs** for character and dial-in style controls
- **sliders** for staged/linear/comparative controls
- **switches/segmented/selectors** for mode/type/state choices

## Good likely knob candidates
Where practical and visually sound, use knobs or rotary controls for parameters like:
- `DRV`
- `ASYM`
- `DRFT`
- detune
- mix/level style controls
- resonance
- env amount
- LFO rate/depth if present and appropriate

## Good likely slider candidates
Retain sliders where they improve readability, especially for:
- amp ADSR
- filter ADSR
- strongly comparative linear controls where reading relative positions matters

## Important guidance
Do not turn the panel into a knob farm.
The goal is better control semantics, not decorative synth cliché.

## Acceptance criteria
- the panel uses a more intentional mix of control types
- knobs appear where they actually improve tactile/semantic reading
- sliders remain where they are still the better choice
- the result feels more instrument-like and less generic

---

# 5. Compactness and usability constraints

## Strong constraints
- Keep the synth panel compact.
- Do not bloat Zone A vertically.
- Do not add controls just to make the layout look fuller.
- Do not create a giant wall of knobs.
- Do not sacrifice readability for style.

The synth editor should feel deeper and better, not larger and more cluttered.

---

# 6. Styling relationship

## Guidance
This pass is about synth editor layout and control choice, not the entire global styling pass.

You may improve local synth control styling as needed to support the new layout, but do not broaden this into an app-wide styling rewrite.

Use the existing theme/control system as much as practical.

---

# 7. Suggested implementation approach

1. identify the current synth editor control groups
2. reorganize the panel into the intended hierarchy
3. mirror/refine the OSC 1 and OSC 2 sections
4. introduce the shared Character row for `DRV` / `ASYM` / `DRFT`
5. convert only the best candidate controls to knobs/rotary controls
6. preserve compactness and test readability
7. leave a short summary of what was changed and why

---

## Deliverables

Provide:

### A. Refined synth hierarchy
A clearer Zone A synth layout with better musical grouping.

### B. Mirrored oscillator sections
A cleaner paired presentation for OSC 1 and OSC 2.

### C. Shared character section
A proper grouping for `DRV`, `ASYM`, and `DRFT`.

### D. Mixed control strategy
A justified mix of knobs/sliders/selectors that improves the synth editor.

### E. Short summary
A concise note describing:
- how the synth hierarchy was changed
- which controls were converted to knobs or kept as sliders
- any further refinements still deferred

---

## Acceptance criteria

- the synth editor has a clearer hierarchy
- OSC 1 and OSC 2 feel more intentional and mirrored
- `DRV`, `ASYM`, and `DRFT` are grouped in a better shared position
- the control mix is more thoughtful and tactile
- the editor remains compact and usable
- the result feels more like a designed instrument panel and less like parameter drift

---

## Non-goals for this pass

- broad app-wide styling pass
- full synth engine redesign
- adding major new synthesis features
- turning the panel into a giant knob-heavy interface

---

## Deliverable style

Be explicit and practical.
Design the synth panel with a strong hierarchy and intentional control strategy rather than making isolated control swaps.
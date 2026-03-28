# Style Decision Checklist for Matthew — Lock These Before the Big Styling Pass

Use this as a decision checklist before or during the styling pass.

The goal is simple:
- Matthew chooses the visual direction
- the agent implements and propagates it consistently

This checklist is not for the agent to answer on its own.
It is for the user to review and decide so the styling pass does not drift into invented taste.

---

## 1. Overall feel
Choose the closest intended feel for the app:

- more industrial / machine-like
- more instrument-like / tactile
- more editorial / clean / designed
- more retro hardware / warm / rugged
- hybrid: dense and technical, but still musical

Write 1–2 sentences describing the intended feel in plain language.

---

## 2. Density target
Decide how compact the app should remain.

Questions:
- should controls stay as tight as possible unless readability clearly suffers?
- what is the maximum acceptable row height increase, if any?
- should the styling pass prefer tighter grouping over breathing room?

Recommended default if undecided:
- keep compactness as a hard rule
- do not increase row heights unless functionally necessary

---

## 3. Button family
Decide the button character.

Questions:
- sharper corners or softer corners?
- flatter or slightly tactile?
- more label-driven or more chip-like?
- stronger border or more subtle border?
- should primary actions feel clearly different from secondary actions?

Lock decisions for:
- default button
- primary/accent button
- selected/toggled button
- disabled button

---

## 4. Slider family
Decide the slider character.

Questions:
- narrow and technical, or slightly chunkier?
- should the track be subtle or obvious?
- should the thumb feel minimal or tactile?
- do bar sliders and ADSR sliders use the same visual language or related variants?

Lock decisions for:
- default slider
- selected/focused slider
- disabled slider

---

## 5. Knob family
Decide whether knobs should become part of the visual language, and where.

Questions:
- should knobs be used sparingly for character/tone/dial-in controls?
- should they stay small and dense, or slightly larger and more tactile?
- flat ring style, hardware-like cap style, or minimal indicator style?

Lock decisions for:
- whether knobs are part of the standard Zone A language
- which parameter types should use knobs vs not

---

## 6. Switch / toggle / selector family
Decide how mode/state controls should feel.

Questions:
- segmented and crisp, or softer toggle chips?
- should state changes be bold or understated?
- should selectors feel more like tabs, pills, or hardware toggles?

Lock decisions for:
- binary toggle style
- segmented selector style
- dropdown/select style

---

## 7. Typography hierarchy
Decide the text mood.

Questions:
- should headers feel stronger and more branded, or quiet and technical?
- should body/value text stay mono-heavy for technical clarity?
- where should mono vs proportional fonts dominate?

Lock decisions for:
- section headers
- labels
- hints
- numeric/value text

---

## 8. Color philosophy
Decide how color should behave globally.

Questions:
- neutral dark base with restrained accents?
- stronger per-zone identity?
- should accents be rare and meaningful, or more present throughout?
- should the app feel warmer, cooler, darker, or higher-contrast than it does now?

Lock decisions for:
- base surface mood
- accent intensity
- zone-color intensity
- whether selected states are subtle or obvious

---

## 9. Semantic states
Decide how visually assertive states should be.

Questions:
- how obvious should selected be?
- how subtle should hover be?
- should focused feel similar to selected or clearly different?
- how should playing/current-step/current-playhead read relative to selection?
- how ghosted should scaffold content be compared to authored content?

Lock decisions for:
- hover
- selected
- focused
- playing/current
- scaffold vs authored
- disabled

---

## 10. Panel and card chrome
Decide the panel language.

Questions:
- harder structural panels or softer card feel?
- more border-led or more fill-led?
- do headers need stronger framing or lighter framing?
- how much chrome is too much?

Lock decisions for:
- panel edge strength
- rounding amount
- header treatment
- card treatment for list rows / grouped modules

---

## 11. Zone identity
Decide how much each zone should visually keep its own identity.

Questions:
- should zones share almost all styling except accent colors?
- should some zones remain more distinct because of function?
- should Zone A remain the most editorial/controlled reference language?

Lock decisions for:
- how strong zone accent carry-through should be
- whether some zones get toned down to improve global cohesion

---

## 12. What not to change
This is important.

Write down what should be protected during the styling pass.

Examples:
- keep vertical density tight
- do not make the app airy
- do not turn it into a generic plugin UI
- do not over-round everything
- do not reduce contrast on selected states
- do not flatten zone identity too much

This section is the guardrail against the agent making “tasteful” but wrong decisions.

---

## 13. Review format requirement for the agent
Require the agent to present styling work in this order:

1. control family proposal
2. one or two representative panel examples
3. concise list of design choices made
4. only then broader propagation

Do not allow a full sweep before the user approves the control language.

---

## 14. Recommended practical use
Matthew should fill this out briefly, even with short answers.
Then the agent should use it as the styling brief.

The better approach is:
- user chooses feel and guardrails
- agent implements consistency

Not:
- agent invents taste and the user cleans up after it

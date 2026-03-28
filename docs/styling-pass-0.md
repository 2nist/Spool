# Styling Pass 0

This file defines the practical styling contract for Spool before the broader zone-by-zone styling sweep.

## Core rules

- Keep layouts compact by default.
- Do not increase row heights, header heights, or padding unless the control cannot function otherwise.
- Solve hierarchy with contrast, typography, borders, and grouping before adding whitespace.
- Prefer shared helpers and theme tokens over local color, radius, or stroke decisions.
- Zone identity should come from accent usage and semantic emphasis, not from wholly different component languages.

## Reused patterns

### Shell bars
- Use flat compact bars with a subtle bottom divider.
- Use zone accent sparingly: hover fills, active dots, or compact chips.
- Avoid tall wrappers or decorative containers.

### Panel headers
- Compact, single-line headers.
- Accent stripe or accent text indicates identity.
- Divider belongs at the bottom edge of the header, not as extra padding.

### Rows and list items
- Default row: low-contrast surface.
- Hover: slightly elevated or brightened fill.
- Selected: stronger fill plus accent cue.
- Playing/current: distinct from selected; use active accent, playhead color, or animated indicator, not the same selection treatment.

### Badges and chips
- Compact and short-label first.
- Use badge/chip styling for status, mode, or category, not as large buttons.
- Accent may tint a chip, but authored/active states should still read stronger than scaffold or disabled states.

### Scaffold vs authored
- Scaffold content must read as secondary, ghosted, or guide-like.
- Authored content must read as concrete and higher-confidence.
- Do not style scaffold content with the same contrast as authored material.

## Semantic state rules

- `hover`: temporary interest, subtle lift only.
- `selected`: explicit user selection, persistent and clear.
- `focused`: keyboard/edit focus, uses focus ring or focused outline.
- `playing/current`: transport/runtime state, not the same as selection.
- `armed`: hot/ready state, strong accent or warning emphasis.
- `disabled`: reduced contrast and alpha, still legible.
- `scaffold`: ghosted, secondary, structurally informative.
- `authored`: primary, confident, highest-content contrast.

## Typography hierarchy

- `display` and `heading`: only for true headers and strong labels.
- `label`: default UI control and panel content label.
- `body` / `value`: authored values, transport values, and dense content readouts.
- `micro`: hints, row metadata, badges, supporting text.

## Compactness guardrails

- Do not add wrapper cards solely for decoration.
- Do not widen or heighten compact Zone A controls casually.
- InstrumentPanel remains the density reference for Zone A.
- If a new helper introduces size defaults, they should follow Theme/ZoneA compact token ranges.

## Recommended styling pass order

1. Shell
2. Zone A
3. Zone C
4. Zone B
5. Zone D
6. Final consistency cleanup

# Codex Prompt — Replace Broken Unicode Glyph Labels with Plain Text

Replace broken Unicode symbol glyphs used in compact UI labels with plain ASCII text labels, starting with the Zone B looper strip.

## Context

Spool currently uses Unicode symbol glyphs in some button and label text, for example:
- arrows
- play triangle
- plus / loop-style symbols
- dropdown triangles

These are rendering poorly with the current font setup and showing as broken/fallback characters in the UI. The immediate goal is to remove font-dependent symbol glyphs and use clean text-only labels.

This is a text-only cleanup pass, not an icon system implementation.

## Primary goal

Make the UI render reliably by replacing Unicode symbol glyphs in visible control labels with compact ASCII text.

## Scope

Start with `source/components/ZoneB/LooperStrip.cpp` and any closely related looper UI code that uses glyph-style text labels.

Search for labels using symbol escapes like these patterns:
- `\xe2\x97...`
- `\xe2\x96...`
- `\xe2\x86...`
- `\xe2\x8a...`
- other non-ASCII button-label glyphs being used as icons

Also search for any similar Unicode glyph usage in nearby UI code where compact controls are rendered as text.

## Constraints

- Do not introduce icon fonts.
- Do not add Lucide or SVG icon rendering in this pass.
- Do not redesign layout broadly.
- Do not enlarge controls unless absolutely necessary.
- Keep the looper strip compact.
- Keep labels short and readable.
- Prefer ASCII-safe text only.
- Preserve behavior; this is a rendering and labeling cleanup pass.

## Replacement guidance

Use plain text replacements like these:

- `◀ 4` → `LAST 4`
- `◀ 8` → `LAST 8`
- `◀ 16` → `LAST 16`
- `▶ PLAY` → `PLAY`
- `⊕ LOOP` → `LOOP`
- `→ REEL` → `REEL`
- `→ TMLN` → `TMLN`
- dropdown triangle suffixes like `▼` → either `v` or remove entirely if spacing is tighter and still clear

For source selectors:
- `SRC: MIX ▼` → `SRC: MIX v` or `SRC: MIX`

For snap selectors:
- `BAR ▼` → `BAR v` or `BAR`

Choose whichever looks best while staying compact and visually consistent.

## Specific file guidance

### `source/components/ZoneB/LooperStrip.cpp`
Replace all symbol-glyph button text with ASCII-safe labels.

Be sure to update both:
- paint-time label text
- hit-testing width calculations that currently depend on glyph label strings

Make sure button widths still fit the new text without overlap.

## Quality bar

After the change:
- no visible broken glyph/fallback characters should remain in the looper strip
- labels should remain compact and legible
- no behavior should change
- layout should remain tight

## Nice-to-have

If there are a few additional obvious instances of the same problem in nearby UI controls, clean them up too — but keep the diff focused.

## Deliverable

A focused diff that removes Unicode icon-glyph dependency from the relevant compact UI labels and replaces them with clean text-only labels.
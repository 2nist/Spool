# Song Structure Schema

- Schema file: `docs/song-structure.schema.json`
- Example file: `docs/song-structure.example.json`
- Draft: JSON Schema 2020-12

## Goals

- Preserve current runtime compatibility with existing `SongManager` fields.
- Provide a stable contract for arrangement development.
- Add first-class harmonic metadata for diatonic and Roman numeral workflows.
- Keep forward compatibility via `x-*` extension fields on every object.

## Current Runtime Contract (Implemented)

The current loader/saver in `source/dsp/SongManager.cpp` uses these fields directly:

- `patterns[]`: `id`, `name`, `lengthBeats`, `baseSteps`
- `sections[]`: `id`, `patternId`, `slotMask`, `startBeat`, `lengthBeats`, `repeats`, `transpose`, `transition`
- `chords[]`: `id`, `startBeat`, `durationBeats`, `root`, `type`
- `meta`: `globalTranspose`

Additional schema fields are safe for storage and future tooling, but are not yet consumed by runtime playback.

## Future-Ready Blocks

- `arrangement`: timeline tracks, clips, markers
- `harmony.defaultKey`: global key context
- `harmony.keyTimeline`: modulations/key regions over time
- `harmony.romanTimeline`: Roman numeral events with harmonic function
- Enriched chord model:
  - `symbol`, `quality`, `extensions`, `alterations`, `inversion`, `romanNumeral`, `secondaryOf`, `borrowedFrom`, `keyContext`

## Suggested Evolution Plan

1. Keep writing current core fields so old builds remain compatible.
2. Begin writing `schemaVersion` and `meta.key` in all new files.
3. Add optional writer support for `harmony.romanTimeline` from progression UI.
4. Add parser adapters in `SongManager` for enriched chord fields while retaining legacy `type`.
5. Add schema validation in tests for regression safety.

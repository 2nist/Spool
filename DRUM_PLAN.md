# Spool Drum Engine Improvement Plan

**Goal**: Advanced Drum Engine – FM Modulation, Kit Browser, Velocity/Round-Robin Layers  
**Priority**: All (FM first for DSP perf).

## Executive Summary
Current: Solid 4-layer voice (tone/noise/click/metal), 16 voices/slot, atomic UI sync.  
**Target**: FM operator layer, kit presets, multi-sample velocity switching.

## Roadmap
### Phase 1: FM Layer (DSP)
- [ ] FmDrumVoice.{h,cpp}
- [ ] Edit DrumVoiceDSP → Integrate FM
- [ ] Edit DrumVoiceParams → FM params

### Phase 2: Kit Browser (UI)
- [ ] ZoneA/KitBrowser.{h,cpp}
- [ ] DrumKitPatch serialization (.json kits)

### Phase 3: Velocity Layers
- [ ] Multi-sample support (SFZ-lite)
- [ ] Round-robin per velocity bin

## Code Examples
**FmDrumVoice**:
```cpp
class FmDrumVoice {
    float m_modIndex, m_ratio;
    double m_carPhase, m_modPhase;
    void render (float* out, int numSamples);
};
```

**Integration**:
```cpp
DrumVoiceDSP::renderFm() { /* 2-op FM */ }
```

**Status**: Approved – Proceed to Phase 1.


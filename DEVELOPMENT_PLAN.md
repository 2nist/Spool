# Spool Development Plan: Enhance DAW Integration (MIDI Output + Timeline Editing)

**Date**: Current  
**Goal**: Add VST3/AU MIDI Output (Goal 4) + Full Multi-Lane Timeline Editing (Goal 5)  
**Scope**: MIDI clip/pattern export to DAW; timeline → clip editor (warp, quantize, automation).  
**Timeline**: MIDI out (~1 week); Timeline editor (~2-3 weeks).  
**Author**: BLACKBOXAI Analysis

## Executive Summary
Current: Audio-focused plugin w/ internal sequencer/MIDI routing.  
**Target**: Full DAW integration – export MIDI from timeline/patterns; editable timeline w/ warping/automation.  
**Benefits**: Seamless DAW workflow (e.g., export Spool patterns to Logic/Ableton tracks).

## Current Codebase State
- **Tech**: C++/JUCE, CMake, RT-safe atomics.  
- **Architecture**: `PluginProcessor` (DSP) → `SpoolAudioGraph` → FX/Master.  
- **Timeline**: Audio clips (`TimelineClipPlacement`), basic UI (Zone D). No MIDI export/editing.  
- **MIDI**: Internal (`MidiRouter`); `producesMidi()=false`.

## Gap Analysis
| Feature | Current | Missing |
|---------|---------|---------|
| **MIDI Output** | Internal routing | VST3/AU `MidiBuffer` export; pattern/timeline → DAW MIDI. |
| **Timeline** | Audio clip placement | Multi-lane editing (warp/stretch, quantize, split); automation lanes; MIDI render. |
| **Persistence** | Song JSON/XML | MIDI clip export; timeline assets. |
| **UI** | Zone D basic | Clip editor panel; drag handles. |

**Risks**: RT-safety (MIDI render); JUCE MIDI perf; timeline complexity (warping algos).  
**Mitigations**: Atomic snapshots; JUCE `MidiBuffer`; simple linear warp first.

## Actionable Roadmap (Atomic Steps, 15-30min)
1. **Prep (1h)**: Set `JucePlugin_ProducesMidiOutput=1`; build/test.
2. **MIDI Output (4h)**:
   - `PluginProcessor`: Add `m_midiOut`; `processBlock` render patterns → MIDI.
   - `SongManager`: `getMidiSnapshot(double beat)`.
3. **Timeline MIDI (6h)**:
   - Extend `TimelineClipPlacement` (MIDI data).
   - `TimelineMidiRenderer`: Clip → `MidiBuffer`.
4. **UI Edits (8h)**:
   - `ZoneD`: `TimelineEditor` component.
   - `PluginEditor`: Integrate + callbacks.
5. **Editing Features (12h)**:
   - Clip warp/quantize/split.
   - Automation lanes.
6. **Persistence/Export (4h)**:
   - Save MIDI clips to song.
   - Export `.mid` files.
7. **Tests/Polish (4h)**: Unit tests; DAW validation.

**Total**: ~40h (2-3 weeks pt).

## Risk & Mitigation
| Risk | Mitigation |
|------|------------|
| RT perf | Profile `processBlock`; cap events. |
| JUCE MIDI bugs | Test VST3/AU in multiple DAWs. |
| Timeline complexity | MVP: Linear warp + note quantize. |
| State sync | Atomics + `AsyncUpdater`.

## Code Examples
### 1. PluginProcessor.h (Add MIDI Out)
```cpp
class PluginProcessor : public juce::AudioProcessor
{
    juce::MidiBuffer m_midiOut;
    //...
    void renderMidiForBlock(juce::MidiBuffer& out, double startBeat);
};
```

### 2. processBlock Snippet
```cpp
void PluginProcessor::processBlock(...) {
    // Existing audio...
    renderMidiForBlock(m_midiOut, blockStartBeat);
    // Forward to DAW
    // (JUCE handles output automatically when producesMidi=1)
}
```

### 3. TimelineClipPlacement (Extend)
```cpp
struct TimelineClipPlacement {
    // Existing...
    juce::Array<int> midiNotes;  // Or MidiBuffer snapshot
    double quantization = 0.25;
};
```

### 4. New: TimelineEditor UI
```cpp
class TimelineEditor : public juce::Component {
    void mouseDrag(const MouseEvent&);  // Warp clips
    void paintClips(Graphics&);         // Draw lanes/clips
};
```

## Implementation Files
| Action | Path | Details |
|--------|------|---------|
| **New** | `source/components/ZoneD/TimelineEditor.{h,cpp}` | Clip editor. |
| **New** | `source/dsp/MidiClipExporter.{h,cpp}` | Pattern→MIDI. |
| **Edit** | `source/PluginProcessor.cpp` | MIDI render/output. |
| **Edit** | `dsp/SongManager.cpp` | MIDI snapshots. |

## Post-Implementation
- Build: `cmake --build .`
- Test: Load in DAW → Arm timeline → See MIDI on track.
- CI: Update workflows.

**Status**: Ready for TODO.md + step-by-step edits.


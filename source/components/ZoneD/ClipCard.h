#pragma once

#include "../../Theme.h"
#include "ClipData.h"

//==============================================================================
/**
    ClipCard — paints a single clip rectangle on the tape surface.

    Not a Component — used as a pure paint helper by CylinderBand.
    Clips are painted directly onto CylinderBand's Graphics context.

    All positions are tape-surface-local coordinates.
    Caller handles modulo wrap — ClipCard just paints at given x.
*/
class ClipCard
{
public:
    // Local tape colour constants (same values as ZoneDComponent)
    static const juce::Colour tapeClipBg;
    static const juce::Colour tapeClipBorder;

    /**
        Paint the clip at the given tape-local rect.
        Called once per visible portion (may be split for wrap).
    */
    static void paint (juce::Graphics& g,
                       const juce::Rectangle<float>& rect,
                       const Clip& clip,
                       float loopLengthBeats,
                       float pxPerBeat);

private:
    static void paintAudio   (juce::Graphics& g,
                               const juce::Rectangle<float>& contentR,
                               const Clip& clip,
                               float loopLengthBeats,
                               float pxPerBeat);

    static void paintMidi    (juce::Graphics& g,
                               const juce::Rectangle<float>& contentR,
                               const Clip& clip,
                               float loopLengthBeats,
                               float pxPerBeat);

    static void paintOutput  (juce::Graphics& g,
                               const juce::Rectangle<float>& contentR,
                               const Clip& clip);
};

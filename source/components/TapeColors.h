#pragma once

// NOTE: This header uses juce::Colour. Always include after Theme.h
// (or after any header that brings in <JuceHeader.h>).

//==============================================================================
/**
    TapeColors — shared warm-tan tape visual constants.

    Used by both CylinderBand (Zone D tape) and SystemFeedCylinder (system feed).
    CylinderBand also declares these as static const members; both sets must
    stay in sync.
*/
namespace TapeColors
{
    inline const juce::Colour tapeBase    { 0xFFbfaa82 };   // warm tan tape surface
    inline const juce::Colour tapeSeam    { 0xFF9a8660 };   // seam line
    inline const juce::Colour housingEdge { 0xFF080604 };   // near-black rim/housing
    inline const juce::Colour inkOnTape   { 0xFF3a2e18 };   // dark ink on tan surface
}

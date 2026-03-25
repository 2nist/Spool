#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    ThemeData — plain data struct holding every runtime-editable theme value.

    This struct is the single source of truth for all visual values in SPOOL.
    ThemeManager owns one instance and broadcasts when any value changes.
    Component paint() methods read from ThemeManager::get().theme() directly.

    Rule: no methods, no inheritance, no virtual functions — pure data only.
*/
struct ThemeData
{
    // ── SURFACES ─────────────────────────────────────────────────────────────
    juce::Colour surface0     { 0xFF0f0d09 };   // deepest bg
    juce::Colour surface1     { 0xFF1c1610 };   // page bg
    juce::Colour surface2     { 0xFF26201a };   // panel bg
    juce::Colour surface3     { 0xFF322a22 };   // elevated cards
    juce::Colour surface4     { 0xFF3e342a };   // raised elements
    juce::Colour surfaceEdge  { 0xFF4a3e32 };   // borders / dividers

    // ── INK ──────────────────────────────────────────────────────────────────
    juce::Colour inkLight  { 0xFFf8f4ec };   // primary text — near white
    juce::Colour inkMid    { 0xFFd4c9b0 };   // secondary text
    juce::Colour inkMuted  { 0xFFa89880 };   // tertiary / hints
    juce::Colour inkGhost  { 0xFF6a6050 };   // barely visible annotations
    juce::Colour inkDark   { 0xFF2a1f0e };   // text on light surfaces

    // ── ACCENT ───────────────────────────────────────────────────────────────
    juce::Colour accent     { 0xFFc4822a };   // primary interactive — amber ochre
    juce::Colour accentWarm { 0xFFd4603a };   // hot accent — burnt orange
    juce::Colour accentDim  { 0xFF8a5a1e };   // dimmed accent — inactive

    // ── ZONE ACCENTS ─────────────────────────────────────────────────────────
    juce::Colour zoneA    { 0xFF4a9eff };   // Zone A — nav panel — blue
    juce::Colour zoneB    { 0xFFc4822a };   // Zone B — modules — amber
    juce::Colour zoneC    { 0xFF4aee8a };   // Zone C — fx engine — green
    juce::Colour zoneD    { 0xFFeec44a };   // Zone D — timeline — yellow
    juce::Colour zoneMenu { 0xFFd4603a };   // Menu bar — burnt orange

    // ── ZONE BACKGROUNDS ─────────────────────────────────────────────────────
    juce::Colour zoneBgA  { 0xFF26201a };   // Zone A background (= surface2)
    juce::Colour zoneBgB  { 0xFF26201a };   // Zone B background — editable independently
    juce::Colour zoneBgC  { 0xFF26201a };   // Zone C background
    juce::Colour zoneBgD  { 0xFF1c1610 };   // Zone D background (= surface1)

    // ── SIGNAL ───────────────────────────────────────────────────────────────
    juce::Colour signalAudio { 0xFF4a9eff };   // audio — blue
    juce::Colour signalMidi  { 0xFFee7c4a };   // MIDI — orange
    juce::Colour signalCv    { 0xFF4aee8a };   // CV — green
    juce::Colour signalGate  { 0xFFeec44a };   // gate — yellow

    // ── TYPOGRAPHY ───────────────────────────────────────────────────────────
    float sizeDisplay   { 14.0f };
    float sizeHeading   { 13.0f };
    float sizeLabel     { 11.0f };
    float sizeBody      { 11.0f };
    float sizeMicro     { 10.0f };
    float sizeValue     { 11.0f };
    float sizeTransport { 13.0f };

    // ── SPACING (runtime editable subset) ────────────────────────────────────
    float spaceXs  {  4.0f };
    float spaceSm  {  8.0f };
    float spaceMd  { 12.0f };
    float spaceLg  { 16.0f };
    float spaceXl  { 20.0f };
    float spaceXxl { 24.0f };

    // Layout dimensions
    float menuBarHeight    {  28.0f };
    float zoneAWidth       { 160.0f };
    float zoneCWidth       { 200.0f };
    float zoneDNormHeight  { 180.0f };
    float moduleSlotHeight {  80.0f };
    float stepHeight       {  24.0f };
    float stepWidth        {  24.0f };
    float knobSize         {  32.0f };

    // ── TAPE (Zone D) ────────────────────────────────────────────────────────
    juce::Colour tapeBase       { 0xFFbfaa82 };
    juce::Colour tapeClipBg     { 0xFFe8dbc0 };
    juce::Colour tapeClipBorder { 0xFF7a6438 };
    juce::Colour tapeSeam       { 0xFF9a8660 };
    juce::Colour tapeBeatTick   { 0xFF41300e };
    juce::Colour playheadColor  { 0xFF694412 };
    juce::Colour housingEdge    { 0xFF080604 };

    // ── PRESET METADATA ──────────────────────────────────────────────────────
    juce::String presetName { "Espresso" };
};

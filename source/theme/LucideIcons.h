#pragma once

//==============================================================================
/**
    LucideIcons — SVG path strings for Zone A theme editor icon tabs.

    Each constant is a compact SVG path string suitable for
    juce::Path::parsePathString().  All paths are designed for a 24×24 viewBox
    and will be scaled at draw time.
*/
namespace LucideIcons
{
    // Surface — layers icon
    inline constexpr const char* surface =
        "M12 2 L2 7 L12 12 L22 7 Z "
        "M2 17 L12 22 L22 17 "
        "M2 12 L12 17 L22 12";

    // Ink — type/T icon
    inline constexpr const char* ink =
        "M4 7 L4 4 L20 4 L20 7 "
        "M9 20 L15 20 "
        "M12 4 L12 20";

    // Accent — zap/lightning icon
    inline constexpr const char* accent =
        "M13 2 L3 14 L12 14 L11 22 L21 10 L12 10 Z";

    // Zones — layout-dashboard icon
    inline constexpr const char* zones =
        "M3 3 L10 3 L10 10 L3 10 Z "
        "M14 3 L21 3 L21 10 L14 10 Z "
        "M14 14 L21 14 L21 21 L14 21 Z "
        "M3 14 L10 14 L10 21 L3 21 Z";

    // Signal — activity/pulse icon
    inline constexpr const char* signal =
        "M22 12 L18 12 L15 21 L9 3 L6 12 L2 12";

    // Typography — baseline/text icon
    inline constexpr const char* typography =
        "M4 20 L20 20 "
        "M6 20 L12 4 L18 20 "
        "M8.5 14 L15.5 14";

    // Spacing — move/arrows icon
    inline constexpr const char* spacing =
        "M5 9 L2 12 L5 15 "
        "M9 5 L12 2 L15 5 "
        "M15 19 L12 22 L9 19 "
        "M19 9 L22 12 L19 15 "
        "M2 12 L22 12 "
        "M12 2 L12 22";

    // Tape — disc/spool icon
    inline constexpr const char* tape =
        "M12 2 A10 10 0 1 0 12 22 A10 10 0 0 0 12 2 Z "
        "M12 8 A4 4 0 1 0 12 16 A4 4 0 0 0 12 8 Z";
}

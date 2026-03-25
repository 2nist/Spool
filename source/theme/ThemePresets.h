#pragma once

#include "ThemeData.h"

//==============================================================================
/**
    ThemePresets — built-in named themes and user preset management.

    Five built-in presets:
      1. Espresso     — original SPOOL aesthetic (warm espresso surfaces)
      2. Midnight     — deep blue-black, clinical, low-light
      3. Warm Studio  — amber wood tones, analog warmth
      4. High Contrast — maximum readability, accessibility-focused
      5. Neon         — dark charcoal with vivid cyberpunk accents
*/
class ThemePresets
{
public:
    /** All five built-in presets in order. */
    static juce::Array<ThemeData> builtInPresets();

    /** Returns preset by name, or Espresso if name not found. */
    static ThemeData presetByName (const juce::String& name);

    /** All presets from getUserThemeDir() that have .spool-theme extension. */
    static juce::Array<ThemeData> userPresets();

    /** Save a theme to getUserThemeDir()/<name>.spool-theme. */
    static void saveUserPreset  (const ThemeData& theme);

    /** Delete a user preset by name. */
    static void deleteUserPreset (const juce::String& name);

private:
    static ThemeData espresso();
    static ThemeData midnight();
    static ThemeData warmStudio();
    static ThemeData highContrast();
    static ThemeData neon();
};

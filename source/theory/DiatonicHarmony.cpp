#include "DiatonicHarmony.h"

namespace DiatonicHarmony
{
namespace
{
constexpr const char* kRoots[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
constexpr const char* kRomanBase[] = { "I", "II", "III", "IV", "V", "VI", "VII" };

juce::String normalizedMode (const juce::String& mode)
{
    return mode.trim().toLowerCase();
}

juce::String romanForQuality (const juce::String& base, const juce::String& quality)
{
    if (quality.startsWithIgnoreCase ("min"))
        return base.toLowerCase();
    if (quality.startsWithIgnoreCase ("dim"))
        return base.toLowerCase() + "o";
    return base;
}
}

std::array<int, 7> modeIntervals (const juce::String& mode)
{
    const auto normalized = normalizedMode (mode);
    if (normalized == "minor" || normalized == "aeolian")
        return { 0, 2, 3, 5, 7, 8, 10 };
    if (normalized == "dorian")
        return { 0, 2, 3, 5, 7, 9, 10 };
    if (normalized == "mixolydian")
        return { 0, 2, 4, 5, 7, 9, 10 };
    if (normalized == "lydian")
        return { 0, 2, 4, 6, 7, 9, 11 };
    if (normalized == "phrygian")
        return { 0, 1, 3, 5, 7, 8, 10 };
    if (normalized == "locrian")
        return { 0, 1, 3, 5, 6, 8, 10 };
    return { 0, 2, 4, 5, 7, 9, 11 };
}

std::array<juce::String, 7> modeQualities (const juce::String& mode)
{
    const auto normalized = normalizedMode (mode);
    if (normalized == "minor" || normalized == "aeolian")
        return { "min", "dim", "maj", "min", "min", "maj", "maj" };
    if (normalized == "dorian")
        return { "min", "min", "maj", "maj", "min", "dim", "maj" };
    if (normalized == "mixolydian")
        return { "maj", "min", "dim", "maj", "min", "min", "maj" };
    if (normalized == "lydian")
        return { "maj", "maj", "min", "dim", "maj", "min", "min" };
    if (normalized == "phrygian")
        return { "min", "maj", "maj", "min", "dim", "maj", "min" };
    if (normalized == "locrian")
        return { "dim", "maj", "min", "min", "maj", "maj", "min" };
    return { "maj", "min", "min", "maj", "maj", "min", "dim" };
}

std::array<juce::String, 7> modeFunctions (const juce::String& mode)
{
    juce::ignoreUnused (mode);
    return { "Tonic", "Predominant", "Tonic", "Predominant", "Dominant", "Tonic", "Dominant" };
}

std::array<juce::String, 7> modeRomanNumerals (const juce::String& mode)
{
    const auto qualities = modeQualities (mode);
    std::array<juce::String, 7> out;
    for (int i = 0; i < 7; ++i)
        out[(size_t) i] = romanForQuality (kRomanBase[i], qualities[(size_t) i]);
    return out;
}

int pitchClassForRoot (const juce::String& root)
{
    const auto r = root.trim();
    if (r.equalsIgnoreCase ("C")) return 0;
    if (r.equalsIgnoreCase ("C#") || r.equalsIgnoreCase ("Db")) return 1;
    if (r.equalsIgnoreCase ("D")) return 2;
    if (r.equalsIgnoreCase ("D#") || r.equalsIgnoreCase ("Eb")) return 3;
    if (r.equalsIgnoreCase ("E")) return 4;
    if (r.equalsIgnoreCase ("F")) return 5;
    if (r.equalsIgnoreCase ("F#") || r.equalsIgnoreCase ("Gb")) return 6;
    if (r.equalsIgnoreCase ("G")) return 7;
    if (r.equalsIgnoreCase ("G#") || r.equalsIgnoreCase ("Ab")) return 8;
    if (r.equalsIgnoreCase ("A")) return 9;
    if (r.equalsIgnoreCase ("A#") || r.equalsIgnoreCase ("Bb")) return 10;
    return 11;
}

juce::String rootNameForPitchClass (int pitchClass)
{
    return kRoots[(pitchClass % 12 + 12) % 12];
}

int degreeIndexForRoman (const juce::String& roman)
{
    const auto normalized = roman.trim().toUpperCase().retainCharacters ("IV");
    for (int i = 0; i < 7; ++i)
        if (normalized == kRomanBase[i])
            return i;
    return 0;
}

DiatonicChordInfo buildDiatonicChord (const juce::String& keyRoot,
                                      const juce::String& mode,
                                      int degreeIndex)
{
    const int index = juce::jlimit (0, 6, degreeIndex);
    const auto intervals = modeIntervals (mode);
    const auto qualities = modeQualities (mode);
    const auto romans = modeRomanNumerals (mode);
    const auto functions = modeFunctions (mode);

    DiatonicChordInfo chord;
    chord.degreeIndex = index;
    chord.root = rootNameForPitchClass (pitchClassForRoot (keyRoot) + intervals[(size_t) index]);
    chord.type = qualities[(size_t) index];
    chord.roman = romans[(size_t) index];
    chord.function = functions[(size_t) index];
    return chord;
}

Chord buildChordForDegree (const juce::String& keyRoot,
                           const juce::String& mode,
                           int degreeIndex)
{
    const auto info = buildDiatonicChord (keyRoot, mode, degreeIndex);
    return { info.root, info.type };
}

Chord buildChordForCenter (const juce::String& keyRoot,
                           const juce::String& mode,
                           const juce::String& harmonicCenter)
{
    return buildChordForDegree (keyRoot, mode, degreeIndexForRoman (harmonicCenter));
}

std::vector<DiatonicChordInfo> buildPalette (const juce::String& keyRoot,
                                             const juce::String& mode)
{
    std::vector<DiatonicChordInfo> palette;
    palette.reserve (7);
    for (int i = 0; i < 7; ++i)
        palette.push_back (buildDiatonicChord (keyRoot, mode, i));
    return palette;
}

const std::vector<ProgressionTemplate>& progressionPresets()
{
    static const std::vector<ProgressionTemplate> presets
    {
        { "axis-pop", "Axis Pop", "I-V-vi-IV", { 0, 4, 5, 3 } },
        { "soul-lift", "Soul Lift", "I-vi-IV-V", { 0, 5, 3, 4 } },
        { "doo-wop", "Doo-Wop", "I-vi-ii-V", { 0, 5, 1, 4 } },
        { "two-five-one", "ii-V-I", "Predominant to dominant to tonic", { 1, 4, 0 } },
        { "vamp-tonic", "Tonic Vamp", "I-IV loop", { 0, 3 } },
        { "minor-walk", "Minor Walk", "i-bVII-bVI-bVII feel", { 0, 6, 5, 6 } }
    };

    return presets;
}

const std::vector<ProgressionTemplate>& cadencePresets()
{
    static const std::vector<ProgressionTemplate> cadences
    {
        { "authentic", "Authentic", "V-I", { 4, 0 } },
        { "plagal", "Plagal", "IV-I", { 3, 0 } },
        { "deceptive", "Deceptive", "V-vi", { 4, 5 } },
        { "half", "Half", "ii-V", { 1, 4 } }
    };

    return cadences;
}

std::vector<Chord> buildProgressionFromTemplate (const juce::String& keyRoot,
                                                 const juce::String& mode,
                                                 const ProgressionTemplate& preset)
{
    std::vector<Chord> out;
    out.reserve (preset.degrees.size());
    for (const auto degree : preset.degrees)
        out.push_back (buildChordForDegree (keyRoot, mode, degree));
    return out;
}
}

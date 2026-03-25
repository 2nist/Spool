#include "ThemePresets.h"
#include "ThemeManager.h"

//==============================================================================
// Built-in preset definitions
//==============================================================================

ThemeData ThemePresets::espresso()
{
    ThemeData t;
    t.presetName = "Espresso";
    // All defaults match ThemeData struct defaults — this is the original design
    return t;
}

ThemeData ThemePresets::midnight()
{
    ThemeData t;
    t.presetName   = "Midnight";

    t.surface0     = juce::Colour (0xFF080810);
    t.surface1     = juce::Colour (0xFF0e0e1a);
    t.surface2     = juce::Colour (0xFF161626);
    t.surface3     = juce::Colour (0xFF1e1e32);
    t.surface4     = juce::Colour (0xFF26263e);
    t.surfaceEdge  = juce::Colour (0xFF32324a);

    t.inkLight     = juce::Colour (0xFFf0f0ff);
    t.inkMid       = juce::Colour (0xFFb0b0d0);
    t.inkMuted     = juce::Colour (0xFF7070a0);
    t.inkGhost     = juce::Colour (0xFF404060);
    t.inkDark      = juce::Colour (0xFF0a0a20);

    t.accent       = juce::Colour (0xFF5a8aff);
    t.accentWarm   = juce::Colour (0xFF8860ff);
    t.accentDim    = juce::Colour (0xFF3a5aaa);

    t.zoneA        = juce::Colour (0xFF5a8aff);
    t.zoneB        = juce::Colour (0xFF5a8aff);
    t.zoneC        = juce::Colour (0xFF40d0a0);
    t.zoneD        = juce::Colour (0xFF80a0ff);
    t.zoneMenu     = juce::Colour (0xFF8860ff);

    t.signalAudio  = juce::Colour (0xFF5a8aff);
    t.signalMidi   = juce::Colour (0xFFff6080);
    t.signalCv     = juce::Colour (0xFF40d0a0);
    t.signalGate   = juce::Colour (0xFFffcc40);

    t.tapeBase       = juce::Colour (0xFF1a1a2e);
    t.tapeClipBg     = juce::Colour (0xFF252540);
    t.tapeClipBorder = juce::Colour (0xFF404070);
    t.tapeSeam       = juce::Colour (0xFF303058);
    t.tapeBeatTick   = juce::Colour (0xFF5060aa);
    t.playheadColor  = juce::Colour (0xFF5a8aff);
    t.housingEdge    = juce::Colour (0xFF04040c);

    return t;
}

ThemeData ThemePresets::warmStudio()
{
    ThemeData t;
    t.presetName   = "Warm Studio";

    t.surface0     = juce::Colour (0xFF120e08);
    t.surface1     = juce::Colour (0xFF1e1810);
    t.surface2     = juce::Colour (0xFF2c2418);
    t.surface3     = juce::Colour (0xFF3a3020);
    t.surface4     = juce::Colour (0xFF48402a);
    t.surfaceEdge  = juce::Colour (0xFF5a5030);

    t.inkLight     = juce::Colour (0xFFfff8e8);
    t.inkMid       = juce::Colour (0xFFd4c090);
    t.inkMuted     = juce::Colour (0xFFa08860);
    t.inkGhost     = juce::Colour (0xFF604830);
    t.inkDark      = juce::Colour (0xFF200e04);

    t.accent       = juce::Colour (0xFFe8a030);
    t.accentWarm   = juce::Colour (0xFFe06020);
    t.accentDim    = juce::Colour (0xFF906020);

    t.zoneA        = juce::Colour (0xFFe8a030);
    t.zoneB        = juce::Colour (0xFFe8a030);
    t.zoneC        = juce::Colour (0xFF80c870);
    t.zoneD        = juce::Colour (0xFFe8c060);
    t.zoneMenu     = juce::Colour (0xFFe06020);

    t.signalAudio  = juce::Colour (0xFFe8a030);
    t.signalMidi   = juce::Colour (0xFFff7040);
    t.signalCv     = juce::Colour (0xFF80c870);
    t.signalGate   = juce::Colour (0xFFe8c060);

    t.tapeBase       = juce::Colour (0xFFd4b880);
    t.tapeClipBg     = juce::Colour (0xFFe8d0a0);
    t.tapeClipBorder = juce::Colour (0xFF907040);
    t.tapeSeam       = juce::Colour (0xFFb09050);
    t.tapeBeatTick   = juce::Colour (0xFF603010);
    t.playheadColor  = juce::Colour (0xFF804010);
    t.housingEdge    = juce::Colour (0xFF080604);

    return t;
}

ThemeData ThemePresets::highContrast()
{
    ThemeData t;
    t.presetName   = "High Contrast";

    t.surface0     = juce::Colour (0xFF000000);
    t.surface1     = juce::Colour (0xFF0a0a0a);
    t.surface2     = juce::Colour (0xFF141414);
    t.surface3     = juce::Colour (0xFF1e1e1e);
    t.surface4     = juce::Colour (0xFF282828);
    t.surfaceEdge  = juce::Colour (0xFF404040);

    t.inkLight     = juce::Colour (0xFFffffff);
    t.inkMid       = juce::Colour (0xFFe0e0e0);
    t.inkMuted     = juce::Colour (0xFFb0b0b0);
    t.inkGhost     = juce::Colour (0xFF707070);
    t.inkDark      = juce::Colour (0xFF000000);

    t.accent       = juce::Colour (0xFFffaa00);
    t.accentWarm   = juce::Colour (0xFFff6600);
    t.accentDim    = juce::Colour (0xFFaa7000);

    t.zoneA        = juce::Colour (0xFF00aaff);
    t.zoneB        = juce::Colour (0xFFffaa00);
    t.zoneC        = juce::Colour (0xFF00ff88);
    t.zoneD        = juce::Colour (0xFFffdd00);
    t.zoneMenu     = juce::Colour (0xFFff6600);

    t.signalAudio  = juce::Colour (0xFF00aaff);
    t.signalMidi   = juce::Colour (0xFFff6644);
    t.signalCv     = juce::Colour (0xFF00ff88);
    t.signalGate   = juce::Colour (0xFFffdd00);

    t.tapeBase       = juce::Colour (0xFF2a2a2a);
    t.tapeClipBg     = juce::Colour (0xFF3a3a3a);
    t.tapeClipBorder = juce::Colour (0xFF808080);
    t.tapeSeam       = juce::Colour (0xFF606060);
    t.tapeBeatTick   = juce::Colour (0xFFaaaaaa);
    t.playheadColor  = juce::Colour (0xFFffaa00);
    t.housingEdge    = juce::Colour (0xFF000000);

    return t;
}

ThemeData ThemePresets::neon()
{
    ThemeData t;
    t.presetName   = "Neon";

    t.surface0     = juce::Colour (0xFF0a0a0c);
    t.surface1     = juce::Colour (0xFF121216);
    t.surface2     = juce::Colour (0xFF1a1a20);
    t.surface3     = juce::Colour (0xFF22222a);
    t.surface4     = juce::Colour (0xFF2a2a34);
    t.surfaceEdge  = juce::Colour (0xFF363644);

    t.inkLight     = juce::Colour (0xFFf0f0f8);
    t.inkMid       = juce::Colour (0xFFc0b0d0);
    t.inkMuted     = juce::Colour (0xFF806090);
    t.inkGhost     = juce::Colour (0xFF503060);
    t.inkDark      = juce::Colour (0xFF08020a);

    t.accent       = juce::Colour (0xFFff2090);
    t.accentWarm   = juce::Colour (0xFFff6020);
    t.accentDim    = juce::Colour (0xFF901060);

    t.zoneA        = juce::Colour (0xFF00e5ff);
    t.zoneB        = juce::Colour (0xFFff2090);
    t.zoneC        = juce::Colour (0xFF39ff14);
    t.zoneD        = juce::Colour (0xFFffe600);
    t.zoneMenu     = juce::Colour (0xFFff6020);

    t.signalAudio  = juce::Colour (0xFF00e5ff);
    t.signalMidi   = juce::Colour (0xFFff2090);
    t.signalCv     = juce::Colour (0xFF39ff14);
    t.signalGate   = juce::Colour (0xFFffe600);

    t.tapeBase       = juce::Colour (0xFF1a0a20);
    t.tapeClipBg     = juce::Colour (0xFF2a1030);
    t.tapeClipBorder = juce::Colour (0xFF8030c0);
    t.tapeSeam       = juce::Colour (0xFF6020a0);
    t.tapeBeatTick   = juce::Colour (0xFF9040d0);
    t.playheadColor  = juce::Colour (0xFFff2090);
    t.housingEdge    = juce::Colour (0xFF04020a);

    return t;
}

//==============================================================================

juce::Array<ThemeData> ThemePresets::builtInPresets()
{
    return { espresso(), midnight(), warmStudio(), highContrast(), neon() };
}

ThemeData ThemePresets::presetByName (const juce::String& name)
{
    for (auto& p : builtInPresets())
        if (p.presetName == name)
            return p;

    // Check user presets
    for (auto& p : userPresets())
        if (p.presetName == name)
            return p;

    return espresso();
}

juce::Array<ThemeData> ThemePresets::userPresets()
{
    juce::Array<ThemeData> result;
    const auto dir = ThemeManager::get().getUserThemeDir();
    if (!dir.isDirectory()) return result;

    for (const auto& f : juce::RangedDirectoryIterator (dir, false, "*.spool-theme"))
    {
        if (auto xml = juce::XmlDocument::parse (f.getFile()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            if (vt.isValid())
                result.add (ThemeManager::themeFromValueTree (vt));
        }
    }
    return result;
}

void ThemePresets::saveUserPreset (const ThemeData& theme)
{
    ThemeManager::get().getUserThemeDir().createDirectory();
    const auto safeName = theme.presetName.replaceCharacters ("/\\:*?\"<>|", "_________");
    const auto file = ThemeManager::get().getUserThemeDir()
                          .getChildFile (safeName + ".spool-theme");

    // Temporarily apply theme, save, then restore
    const auto current = ThemeManager::get().theme();
    ThemeManager::get().applyTheme (theme);
    ThemeManager::get().saveToFile (file);
    ThemeManager::get().applyTheme (current);
}

void ThemePresets::deleteUserPreset (const juce::String& name)
{
    const auto safeName = name.replaceCharacters ("/\\:*?\"<>|", "_________");
    const auto file = ThemeManager::get().getUserThemeDir()
                          .getChildFile (safeName + ".spool-theme");
    file.deleteFile();
}

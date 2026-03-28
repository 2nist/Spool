#include "ThemeManager.h"

//==============================================================================
// Helpers

static juce::String colourToString (juce::Colour c)
{
    return "0x" + c.toString().toUpperCase();
}

static juce::Colour stringToColour (const juce::String& s)
{
    if (s.startsWith ("0x") || s.startsWith ("0X"))
        return juce::Colour (static_cast<uint32_t> (s.substring (2).getHexValue64()));
    return {};
}

//==============================================================================

juce::ValueTree ThemeManager::toValueTree() const
{
    const auto& t = m_theme;
    juce::ValueTree vt ("ThemeData");

    // Surfaces
    vt.setProperty ("surface0",    colourToString (t.surface0),    nullptr);
    vt.setProperty ("surface1",    colourToString (t.surface1),    nullptr);
    vt.setProperty ("surface2",    colourToString (t.surface2),    nullptr);
    vt.setProperty ("surface3",    colourToString (t.surface3),    nullptr);
    vt.setProperty ("surface4",    colourToString (t.surface4),    nullptr);
    vt.setProperty ("surfaceEdge", colourToString (t.surfaceEdge), nullptr);

    // Ink
    vt.setProperty ("inkLight",  colourToString (t.inkLight),  nullptr);
    vt.setProperty ("inkMid",    colourToString (t.inkMid),    nullptr);
    vt.setProperty ("inkMuted",  colourToString (t.inkMuted),  nullptr);
    vt.setProperty ("inkGhost",  colourToString (t.inkGhost),  nullptr);
    vt.setProperty ("inkDark",   colourToString (t.inkDark),   nullptr);

    // Accent
    vt.setProperty ("accent",     colourToString (t.accent),     nullptr);
    vt.setProperty ("accentWarm", colourToString (t.accentWarm), nullptr);
    vt.setProperty ("accentDim",  colourToString (t.accentDim),  nullptr);

    // UI chrome
    vt.setProperty ("headerBg",      colourToString (t.headerBg),      nullptr);
    vt.setProperty ("cardBg",        colourToString (t.cardBg),        nullptr);
    vt.setProperty ("rowBg",         colourToString (t.rowBg),         nullptr);
    vt.setProperty ("rowHover",      colourToString (t.rowHover),      nullptr);
    vt.setProperty ("rowSelected",   colourToString (t.rowSelected),   nullptr);
    vt.setProperty ("badgeBg",       colourToString (t.badgeBg),       nullptr);
    vt.setProperty ("controlBg",     colourToString (t.controlBg),     nullptr);
    vt.setProperty ("controlOnBg",   colourToString (t.controlOnBg),   nullptr);
    vt.setProperty ("controlText",   colourToString (t.controlText),   nullptr);
    vt.setProperty ("controlTextOn", colourToString (t.controlTextOn), nullptr);
    vt.setProperty ("focusOutline",  colourToString (t.focusOutline),  nullptr);
    vt.setProperty ("sliderTrack",   colourToString (t.sliderTrack),   nullptr);
    vt.setProperty ("sliderThumb",   colourToString (t.sliderThumb),   nullptr);

    // Zones
    vt.setProperty ("zoneA",    colourToString (t.zoneA),    nullptr);
    vt.setProperty ("zoneB",    colourToString (t.zoneB),    nullptr);
    vt.setProperty ("zoneC",    colourToString (t.zoneC),    nullptr);
    vt.setProperty ("zoneD",    colourToString (t.zoneD),    nullptr);
    vt.setProperty ("zoneMenu", colourToString (t.zoneMenu), nullptr);

    // Zone backgrounds
    vt.setProperty ("zoneBgA", colourToString (t.zoneBgA), nullptr);
    vt.setProperty ("zoneBgB", colourToString (t.zoneBgB), nullptr);
    vt.setProperty ("zoneBgC", colourToString (t.zoneBgC), nullptr);
    vt.setProperty ("zoneBgD", colourToString (t.zoneBgD), nullptr);

    // Signal
    vt.setProperty ("signalAudio", colourToString (t.signalAudio), nullptr);
    vt.setProperty ("signalMidi",  colourToString (t.signalMidi),  nullptr);
    vt.setProperty ("signalCv",    colourToString (t.signalCv),    nullptr);
    vt.setProperty ("signalGate",  colourToString (t.signalGate),  nullptr);

    // Typography
    vt.setProperty ("sizeDisplay",   t.sizeDisplay,   nullptr);
    vt.setProperty ("sizeHeading",   t.sizeHeading,   nullptr);
    vt.setProperty ("sizeLabel",     t.sizeLabel,     nullptr);
    vt.setProperty ("sizeBody",      t.sizeBody,      nullptr);
    vt.setProperty ("sizeMicro",     t.sizeMicro,     nullptr);
    vt.setProperty ("sizeValue",     t.sizeValue,     nullptr);
    vt.setProperty ("sizeTransport", t.sizeTransport, nullptr);

    // Compact UI density
    vt.setProperty ("zoneAHeaderHeight",        t.zoneAHeaderHeight,        nullptr);
    vt.setProperty ("zoneAGroupHeaderHeight",   t.zoneAGroupHeaderHeight,   nullptr);
    vt.setProperty ("zoneARowHeight",           t.zoneARowHeight,           nullptr);
    vt.setProperty ("zoneABadgeHeight",         t.zoneABadgeHeight,         nullptr);
    vt.setProperty ("zoneASectionHeaderHeight", t.zoneASectionHeaderHeight, nullptr);
    vt.setProperty ("zoneAControlBarHeight",    t.zoneAControlBarHeight,    nullptr);
    vt.setProperty ("zoneAControlHeight",       t.zoneAControlHeight,       nullptr);
    vt.setProperty ("zoneACardRadius",          t.zoneACardRadius,          nullptr);
    vt.setProperty ("zoneACompactGap",          t.zoneACompactGap,          nullptr);

    // Spacing
    vt.setProperty ("spaceXs",  t.spaceXs,  nullptr);
    vt.setProperty ("spaceSm",  t.spaceSm,  nullptr);
    vt.setProperty ("spaceMd",  t.spaceMd,  nullptr);
    vt.setProperty ("spaceLg",  t.spaceLg,  nullptr);
    vt.setProperty ("spaceXl",  t.spaceXl,  nullptr);
    vt.setProperty ("spaceXxl", t.spaceXxl, nullptr);

    // Layout
    vt.setProperty ("menuBarHeight",    t.menuBarHeight,    nullptr);
    vt.setProperty ("zoneAWidth",       t.zoneAWidth,       nullptr);
    vt.setProperty ("zoneCWidth",       t.zoneCWidth,       nullptr);
    vt.setProperty ("zoneDNormHeight",  t.zoneDNormHeight,  nullptr);
    vt.setProperty ("moduleSlotHeight", t.moduleSlotHeight, nullptr);
    vt.setProperty ("stepHeight",       t.stepHeight,       nullptr);
    vt.setProperty ("stepWidth",        t.stepWidth,        nullptr);
    vt.setProperty ("knobSize",         t.knobSize,         nullptr);

    // Tape
    vt.setProperty ("tapeBase",       colourToString (t.tapeBase),       nullptr);
    vt.setProperty ("tapeClipBg",     colourToString (t.tapeClipBg),     nullptr);
    vt.setProperty ("tapeClipBorder", colourToString (t.tapeClipBorder), nullptr);
    vt.setProperty ("tapeSeam",       colourToString (t.tapeSeam),       nullptr);
    vt.setProperty ("tapeBeatTick",   colourToString (t.tapeBeatTick),   nullptr);
    vt.setProperty ("playheadColor",  colourToString (t.playheadColor),  nullptr);
    vt.setProperty ("housingEdge",    colourToString (t.housingEdge),    nullptr);

    // Metadata
    vt.setProperty ("presetName", t.presetName, nullptr);

    return vt;
}

void ThemeManager::fromValueTree (const juce::ValueTree& vt)
{
    if (!vt.isValid() || vt.getType().toString() != "ThemeData")
        return;

    auto& t = m_theme;

    auto readColour = [&] (const char* name, juce::Colour& dest)
    {
        if (vt.hasProperty (name))
            dest = stringToColour (vt [name].toString());
    };
    auto readFloat = [&] (const char* name, float& dest)
    {
        if (vt.hasProperty (name))
            dest = static_cast<float> (vt [name]);
    };
    auto readString = [&] (const char* name, juce::String& dest)
    {
        if (vt.hasProperty (name))
            dest = vt [name].toString();
    };

    readColour ("surface0",    t.surface0);
    readColour ("surface1",    t.surface1);
    readColour ("surface2",    t.surface2);
    readColour ("surface3",    t.surface3);
    readColour ("surface4",    t.surface4);
    readColour ("surfaceEdge", t.surfaceEdge);

    readColour ("inkLight",  t.inkLight);
    readColour ("inkMid",    t.inkMid);
    readColour ("inkMuted",  t.inkMuted);
    readColour ("inkGhost",  t.inkGhost);
    readColour ("inkDark",   t.inkDark);

    readColour ("accent",     t.accent);
    readColour ("accentWarm", t.accentWarm);
    readColour ("accentDim",  t.accentDim);

    readColour ("headerBg",      t.headerBg);
    readColour ("cardBg",        t.cardBg);
    readColour ("rowBg",         t.rowBg);
    readColour ("rowHover",      t.rowHover);
    readColour ("rowSelected",   t.rowSelected);
    readColour ("badgeBg",       t.badgeBg);
    readColour ("controlBg",     t.controlBg);
    readColour ("controlOnBg",   t.controlOnBg);
    readColour ("controlText",   t.controlText);
    readColour ("controlTextOn", t.controlTextOn);
    readColour ("focusOutline",  t.focusOutline);
    readColour ("sliderTrack",   t.sliderTrack);
    readColour ("sliderThumb",   t.sliderThumb);

    readColour ("zoneA",    t.zoneA);
    readColour ("zoneB",    t.zoneB);
    readColour ("zoneC",    t.zoneC);
    readColour ("zoneD",    t.zoneD);
    readColour ("zoneMenu", t.zoneMenu);

    readColour ("zoneBgA", t.zoneBgA);
    readColour ("zoneBgB", t.zoneBgB);
    readColour ("zoneBgC", t.zoneBgC);
    readColour ("zoneBgD", t.zoneBgD);

    readColour ("signalAudio", t.signalAudio);
    readColour ("signalMidi",  t.signalMidi);
    readColour ("signalCv",    t.signalCv);
    readColour ("signalGate",  t.signalGate);

    readFloat ("sizeDisplay",   t.sizeDisplay);
    readFloat ("sizeHeading",   t.sizeHeading);
    readFloat ("sizeLabel",     t.sizeLabel);
    readFloat ("sizeBody",      t.sizeBody);
    readFloat ("sizeMicro",     t.sizeMicro);
    readFloat ("sizeValue",     t.sizeValue);
    readFloat ("sizeTransport", t.sizeTransport);

    readFloat ("zoneAHeaderHeight",        t.zoneAHeaderHeight);
    readFloat ("zoneAGroupHeaderHeight",   t.zoneAGroupHeaderHeight);
    readFloat ("zoneARowHeight",           t.zoneARowHeight);
    readFloat ("zoneABadgeHeight",         t.zoneABadgeHeight);
    readFloat ("zoneASectionHeaderHeight", t.zoneASectionHeaderHeight);
    readFloat ("zoneAControlBarHeight",    t.zoneAControlBarHeight);
    readFloat ("zoneAControlHeight",       t.zoneAControlHeight);
    readFloat ("zoneACardRadius",          t.zoneACardRadius);
    readFloat ("zoneACompactGap",          t.zoneACompactGap);

    readFloat ("spaceXs",  t.spaceXs);
    readFloat ("spaceSm",  t.spaceSm);
    readFloat ("spaceMd",  t.spaceMd);
    readFloat ("spaceLg",  t.spaceLg);
    readFloat ("spaceXl",  t.spaceXl);
    readFloat ("spaceXxl", t.spaceXxl);

    readFloat ("menuBarHeight",    t.menuBarHeight);
    readFloat ("zoneAWidth",       t.zoneAWidth);
    readFloat ("zoneCWidth",       t.zoneCWidth);
    readFloat ("zoneDNormHeight",  t.zoneDNormHeight);
    readFloat ("moduleSlotHeight", t.moduleSlotHeight);
    readFloat ("stepHeight",       t.stepHeight);
    readFloat ("stepWidth",        t.stepWidth);
    readFloat ("knobSize",         t.knobSize);

    readColour ("tapeBase",       t.tapeBase);
    readColour ("tapeClipBg",     t.tapeClipBg);
    readColour ("tapeClipBorder", t.tapeClipBorder);
    readColour ("tapeSeam",       t.tapeSeam);
    readColour ("tapeBeatTick",   t.tapeBeatTick);
    readColour ("playheadColor",  t.playheadColor);
    readColour ("housingEdge",    t.housingEdge);

    readString ("presetName", t.presetName);

    broadcast();
}

void ThemeManager::saveToFile (const juce::File& f) const
{
    getUserThemeDir().createDirectory();
    auto vt  = toValueTree();
    auto xml = vt.createXml();
    if (xml != nullptr)
        xml->writeTo (f);
}

bool ThemeManager::loadFromFile (const juce::File& f)
{
    if (!f.existsAsFile()) return false;
    if (auto xml = juce::XmlDocument::parse (f))
    {
        auto vt = juce::ValueTree::fromXml (*xml);
        if (vt.isValid())
        {
            fromValueTree (vt);
            return true;
        }
    }
    return false;
}

ThemeData ThemeManager::themeFromValueTree (const juce::ValueTree& vt)
{
    ThemeData t;  // start with defaults

    if (!vt.isValid() || vt.getType().toString() != "ThemeData")
        return t;

    auto readColour = [&] (const char* name, juce::Colour& dest)
    {
        if (vt.hasProperty (name))
            dest = stringToColour (vt [name].toString());
    };
    auto readFloat = [&] (const char* name, float& dest)
    {
        if (vt.hasProperty (name))
            dest = static_cast<float> (vt [name]);
    };
    auto readString = [&] (const char* name, juce::String& dest)
    {
        if (vt.hasProperty (name))
            dest = vt [name].toString();
    };

    readColour ("surface0",    t.surface0);    readColour ("surface1",    t.surface1);
    readColour ("surface2",    t.surface2);    readColour ("surface3",    t.surface3);
    readColour ("surface4",    t.surface4);    readColour ("surfaceEdge", t.surfaceEdge);
    readColour ("inkLight",    t.inkLight);    readColour ("inkMid",      t.inkMid);
    readColour ("inkMuted",    t.inkMuted);    readColour ("inkGhost",    t.inkGhost);
    readColour ("inkDark",     t.inkDark);
    readColour ("accent",      t.accent);      readColour ("accentWarm",  t.accentWarm);
    readColour ("accentDim",   t.accentDim);
    readColour ("headerBg",    t.headerBg);    readColour ("cardBg",      t.cardBg);
    readColour ("rowBg",       t.rowBg);       readColour ("rowHover",    t.rowHover);
    readColour ("rowSelected", t.rowSelected); readColour ("badgeBg",     t.badgeBg);
    readColour ("controlBg",   t.controlBg);   readColour ("controlOnBg", t.controlOnBg);
    readColour ("controlText", t.controlText); readColour ("controlTextOn", t.controlTextOn);
    readColour ("focusOutline", t.focusOutline); readColour ("sliderTrack", t.sliderTrack);
    readColour ("sliderThumb", t.sliderThumb);
    readColour ("zoneA",    t.zoneA);       readColour ("zoneB",       t.zoneB);
    readColour ("zoneC",       t.zoneC);       readColour ("zoneD",       t.zoneD);
    readColour ("zoneMenu",    t.zoneMenu);
    readColour ("zoneBgA",     t.zoneBgA);    readColour ("zoneBgB",     t.zoneBgB);
    readColour ("zoneBgC",     t.zoneBgC);    readColour ("zoneBgD",     t.zoneBgD);
    readColour ("signalAudio", t.signalAudio); readColour ("signalMidi",  t.signalMidi);
    readColour ("signalCv",    t.signalCv);    readColour ("signalGate",  t.signalGate);
    readFloat  ("sizeDisplay",   t.sizeDisplay);  readFloat ("sizeHeading",   t.sizeHeading);
    readFloat  ("sizeLabel",     t.sizeLabel);    readFloat ("sizeBody",      t.sizeBody);
    readFloat  ("sizeMicro",     t.sizeMicro);    readFloat ("sizeValue",     t.sizeValue);
    readFloat  ("sizeTransport", t.sizeTransport);
    readFloat  ("zoneAHeaderHeight",        t.zoneAHeaderHeight);
    readFloat  ("zoneAGroupHeaderHeight",   t.zoneAGroupHeaderHeight);
    readFloat  ("zoneARowHeight",           t.zoneARowHeight);
    readFloat  ("zoneABadgeHeight",         t.zoneABadgeHeight);
    readFloat  ("zoneASectionHeaderHeight", t.zoneASectionHeaderHeight);
    readFloat  ("zoneAControlBarHeight",    t.zoneAControlBarHeight);
    readFloat  ("zoneAControlHeight",       t.zoneAControlHeight);
    readFloat  ("zoneACardRadius",          t.zoneACardRadius);
    readFloat  ("zoneACompactGap",          t.zoneACompactGap);
    readFloat  ("spaceXs",  t.spaceXs);  readFloat ("spaceSm",  t.spaceSm);
    readFloat  ("spaceMd",  t.spaceMd);  readFloat ("spaceLg",  t.spaceLg);
    readFloat  ("spaceXl",  t.spaceXl);  readFloat ("spaceXxl", t.spaceXxl);
    readFloat  ("menuBarHeight",    t.menuBarHeight);
    readFloat  ("zoneAWidth",       t.zoneAWidth);
    readFloat  ("zoneCWidth",       t.zoneCWidth);
    readFloat  ("zoneDNormHeight",  t.zoneDNormHeight);
    readFloat  ("moduleSlotHeight", t.moduleSlotHeight);
    readFloat  ("stepHeight",       t.stepHeight);  readFloat ("stepWidth", t.stepWidth);
    readFloat  ("knobSize",         t.knobSize);
    readColour ("tapeBase",       t.tapeBase);     readColour ("tapeClipBg",     t.tapeClipBg);
    readColour ("tapeClipBorder", t.tapeClipBorder); readColour ("tapeSeam",    t.tapeSeam);
    readColour ("tapeBeatTick",   t.tapeBeatTick); readColour ("playheadColor",  t.playheadColor);
    readColour ("housingEdge",    t.housingEdge);
    readString ("presetName", t.presetName);

    return t;
}

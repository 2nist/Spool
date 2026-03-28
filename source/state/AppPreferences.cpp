#include "AppPreferences.h"

AppPreferences& AppPreferences::get()
{
    static AppPreferences prefs;
    return prefs;
}

AppPreferences::AppPreferences()
{
    juce::PropertiesFile::Options opts;
    opts.applicationName     = "SPOOL";
    opts.filenameSuffix      = ".settings";
    opts.osxLibrarySubFolder = "Application Support";
    m_props = std::make_unique<juce::PropertiesFile> (opts);

    m_showSystemFeed    = m_props->getBoolValue ("uiShowSystemFeed", true);
    m_showSongHeader    = m_props->getBoolValue ("uiShowSongHeader", true);
    m_showHistoryTape   = m_props->getBoolValue ("uiShowHistoryTape", true);
    m_showLooper        = m_props->getBoolValue ("uiShowLooper", true);
    m_showZoneCMacros   = m_props->getBoolValue ("uiShowZoneCMacros", true);
    m_compactSongHeader = m_props->getBoolValue ("uiCompactSongHeader", false);
    m_themePresetName   = m_props->getValue ("uiThemePreset", "Espresso");
}

void AppPreferences::setShowSystemFeed (bool shouldShow)
{
    if (m_showSystemFeed == shouldShow)
        return;

    m_showSystemFeed = shouldShow;
    save();
    broadcast();
}

void AppPreferences::setShowSongHeader (bool shouldShow)
{
    if (m_showSongHeader == shouldShow)
        return;

    m_showSongHeader = shouldShow;
    save();
    broadcast();
}

void AppPreferences::setShowHistoryTape (bool shouldShow)
{
    if (m_showHistoryTape == shouldShow)
        return;

    m_showHistoryTape = shouldShow;
    save();
    broadcast();
}

void AppPreferences::setShowLooper (bool shouldShow)
{
    if (m_showLooper == shouldShow)
        return;

    m_showLooper = shouldShow;
    save();
    broadcast();
}

void AppPreferences::setShowZoneCMacros (bool shouldShow)
{
    if (m_showZoneCMacros == shouldShow)
        return;

    m_showZoneCMacros = shouldShow;
    save();
    broadcast();
}

void AppPreferences::setCompactSongHeader (bool shouldCompact)
{
    if (m_compactSongHeader == shouldCompact)
        return;

    m_compactSongHeader = shouldCompact;
    save();
    broadcast();
}

void AppPreferences::setThemePresetName (const juce::String& presetName)
{
    const auto cleaned = presetName.trim();
    if (cleaned.isEmpty() || m_themePresetName == cleaned)
        return;

    m_themePresetName = cleaned;
    save();
    broadcast();
}

void AppPreferences::broadcast()
{
    m_listeners.call ([] (Listener& listener) { listener.appPreferencesChanged(); });
}

void AppPreferences::save()
{
    if (m_props == nullptr)
        return;

    m_props->setValue ("uiShowSystemFeed", m_showSystemFeed);
    m_props->setValue ("uiShowSongHeader", m_showSongHeader);
    m_props->setValue ("uiShowHistoryTape", m_showHistoryTape);
    m_props->setValue ("uiShowLooper", m_showLooper);
    m_props->setValue ("uiShowZoneCMacros", m_showZoneCMacros);
    m_props->setValue ("uiCompactSongHeader", m_compactSongHeader);
    m_props->setValue ("uiThemePreset", m_themePresetName);
    m_props->saveIfNeeded();
}

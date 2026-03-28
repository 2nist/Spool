#pragma once

#include <juce_data_structures/juce_data_structures.h>

class AppPreferences
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void appPreferencesChanged() = 0;
    };

    static AppPreferences& get();

    bool getShowSystemFeed() const noexcept     { return m_showSystemFeed; }
    bool getShowSongHeader() const noexcept     { return m_showSongHeader; }
    bool getShowHistoryTape() const noexcept    { return m_showHistoryTape; }
    bool getShowLooper() const noexcept         { return m_showLooper; }
    bool getShowZoneCMacros() const noexcept    { return m_showZoneCMacros; }
    bool getCompactSongHeader() const noexcept  { return m_compactSongHeader; }
    juce::String getThemePresetName() const     { return m_themePresetName; }

    void setShowSystemFeed (bool shouldShow);
    void setShowSongHeader (bool shouldShow);
    void setShowHistoryTape (bool shouldShow);
    void setShowLooper (bool shouldShow);
    void setShowZoneCMacros (bool shouldShow);
    void setCompactSongHeader (bool shouldCompact);
    void setThemePresetName (const juce::String& presetName);

    void addListener (Listener* listener)    { m_listeners.add (listener); }
    void removeListener (Listener* listener) { m_listeners.remove (listener); }

private:
    AppPreferences();

    void broadcast();
    void save();

    bool m_showSystemFeed    { true };
    bool m_showSongHeader    { true };
    bool m_showHistoryTape   { true };
    bool m_showLooper        { true };
    bool m_showZoneCMacros   { true };
    bool m_compactSongHeader { false };
    juce::String m_themePresetName { "Espresso" };

    std::unique_ptr<juce::PropertiesFile> m_props;
    juce::ListenerList<Listener> m_listeners;
};

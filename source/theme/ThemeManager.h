#pragma once

#include "ThemeData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>

//==============================================================================
/**
    ThemeManager — runtime singleton holding all theme values.

    Usage from components:
        auto& t = ThemeManager::get().theme();
        g.setColour (t.inkLight);

    ThemeManager::Listener is implemented by PluginEditor (top-level).
    PluginEditor::themeChanged() triggers a recursive repaint of the component tree.

    Rule: components NEVER cache theme values between paint calls — always read
    fresh from ThemeManager::get().theme() in paint() and resized().
*/
class ThemeManager : private juce::Timer
{
public:
    //==========================================================================
    // Singleton access

    static ThemeManager& get()
    {
        static ThemeManager instance;
        return instance;
    }

    ThemeManager (const ThemeManager&)            = delete;
    ThemeManager& operator= (const ThemeManager&) = delete;

    //==========================================================================
    // Data access (read-only from components)

    const ThemeData& theme() const noexcept { return m_theme; }

    //==========================================================================
    // Mutation — call from ThemeEditorPanel only

    /** Update a single juce::Colour field and broadcast. */
    void setColour (juce::Colour ThemeData::* member, juce::Colour value)
    {
        m_theme.*member = value;
        broadcast();
    }

    /** Update a single float field and broadcast. */
    void setFloat (float ThemeData::* member, float value)
    {
        m_theme.*member = value;
        broadcast();
    }

    /** Replace the entire theme at once (preset switch). */
    void applyTheme (const ThemeData& newTheme)
    {
        m_theme = newTheme;
        broadcast();
    }

    //==========================================================================
    // Listener

    class Listener
    {
    public:
        virtual ~Listener()              = default;
        virtual void themeChanged()      = 0;
    };

    void addListener    (Listener* l) { m_listeners.add    (l); }
    void removeListener (Listener* l) { m_listeners.remove (l); }

    //==========================================================================
    // Serialization

    juce::ValueTree toValueTree()                   const;
    void            fromValueTree (const juce::ValueTree&);
    void            saveToFile    (const juce::File&)         const;
    bool            loadFromFile  (const juce::File&);

    /** Deserialize a ThemeData from a ValueTree without modifying the live singleton. */
    static ThemeData themeFromValueTree (const juce::ValueTree&);

    /** Directory where user preset .spool-theme files are stored. */
    juce::File getUserThemeDir() const
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                           .getChildFile ("SPOOL/themes");
    }

    /** Path to the auto-saved current state file. */
    juce::File getCurrentThemeFile() const
    {
        return getUserThemeDir().getChildFile ("current-theme.spool-theme");
    }

private:
    ThemeManager() = default;

    ThemeData                      m_theme;
    juce::ListenerList<Listener>   m_listeners;

    void broadcast()
    {
        m_listeners.call (&Listener::themeChanged);
        startTimer (500);   // debounce — saves 500 ms after the last change
    }

    // juce::Timer
    void timerCallback() override
    {
        stopTimer();
        saveToFile (getCurrentThemeFile());
    }
};

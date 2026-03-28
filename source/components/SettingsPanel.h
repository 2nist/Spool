#pragma once

#include "../Theme.h"
#include "../state/AppPreferences.h"
#include "../theme/ThemeManager.h"
#include "../theme/ThemePresets.h"
#include "ZoneA/ZoneAControlStyle.h"
#include "ZoneA/ZoneAStyle.h"

class SettingsPanel final : public juce::Component,
                            private AppPreferences::Listener,
                            private ThemeManager::Listener
{
public:
    SettingsPanel();
    ~SettingsPanel() override;

    std::function<void (juce::Rectangle<int> anchorBounds)> onOpenThemeDesigner;
    std::function<void()> onImportTheme;
    std::function<void()> onExportTheme;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void refreshFromPreferences();
    void populateThemePresets();
    void appPreferencesChanged() override;
    void themeChanged() override;

    juce::Label m_themeLabel;
    juce::ComboBox m_themeCombo;
    juce::TextButton m_customizeThemeButton { "THEME UTILITY" };
    juce::TextButton m_importThemeButton { "IMPORT" };
    juce::TextButton m_exportThemeButton { "EXPORT" };
    juce::Label m_themeHint;

    juce::ToggleButton m_showFeedToggle;
    juce::ToggleButton m_showSongHeaderToggle;
    juce::ToggleButton m_showHistoryTapeToggle;
    juce::ToggleButton m_showLooperToggle;
    juce::ToggleButton m_showZoneCMacrosToggle;
    juce::ToggleButton m_compactSongHeaderToggle;

    juce::Label m_layoutHint;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsPanel)
};

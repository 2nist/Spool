#include "SettingsPanel.h"

namespace
{
void styleSectionLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (Theme::Font::micro());
    label.setColour (juce::Label::textColourId, Theme::Colour::inkGhost);
    label.setJustificationType (juce::Justification::centredLeft);
}

void styleToggle (juce::ToggleButton& button)
{
    button.setColour (juce::ToggleButton::textColourId, Theme::Colour::inkLight);
    button.setColour (juce::ToggleButton::tickColourId, Theme::Zone::a);
    button.setColour (juce::ToggleButton::tickDisabledColourId, Theme::Colour::inkGhost);
    button.setClickingTogglesState (true);
    button.setTooltip (button.getButtonText());
}
}

SettingsPanel::SettingsPanel()
{
    setSize (264, 282);

    styleSectionLabel (m_themeLabel, "Theme Preset");
    addAndMakeVisible (m_themeLabel);

    ZoneAControlStyle::styleComboBox (m_themeCombo, Theme::Zone::a);
    m_themeCombo.onChange = [this]
    {
        const auto selected = m_themeCombo.getText().trim();
        if (selected.isEmpty())
            return;

        ThemeManager::get().applyTheme (ThemePresets::presetByName (selected));
        AppPreferences::get().setThemePresetName (selected);
    };
    addAndMakeVisible (m_themeCombo);
    populateThemePresets();

    ZoneAControlStyle::styleTextButton (m_customizeThemeButton, Theme::Zone::a);
    m_customizeThemeButton.onClick = [this]
    {
        if (onOpenThemeDesigner)
            onOpenThemeDesigner (m_customizeThemeButton.getScreenBounds());
    };
    addAndMakeVisible (m_customizeThemeButton);

    ZoneAControlStyle::styleTextButton (m_importThemeButton, Theme::Zone::a);
    m_importThemeButton.onClick = [this]
    {
        if (onImportTheme)
            onImportTheme();
    };
    addAndMakeVisible (m_importThemeButton);

    ZoneAControlStyle::styleTextButton (m_exportThemeButton, Theme::Zone::a);
    m_exportThemeButton.onClick = [this]
    {
        if (onExportTheme)
            onExportTheme();
    };
    addAndMakeVisible (m_exportThemeButton);

    m_themeHint.setText ("Theme utility opens the full generator/editor for live testing.", juce::dontSendNotification);
    m_themeHint.setFont (Theme::Font::micro());
    m_themeHint.setColour (juce::Label::textColourId, Theme::Colour::inkGhost);
    m_themeHint.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (m_themeHint);

    m_showFeedToggle.setButtonText ("Show System Feed Strip");
    styleToggle (m_showFeedToggle);
    m_showFeedToggle.onClick = [this]
    {
        AppPreferences::get().setShowSystemFeed (m_showFeedToggle.getToggleState());
    };
    addAndMakeVisible (m_showFeedToggle);

    m_showSongHeaderToggle.setButtonText ("Show Song Header Strip");
    styleToggle (m_showSongHeaderToggle);
    m_showSongHeaderToggle.onClick = [this]
    {
        AppPreferences::get().setShowSongHeader (m_showSongHeaderToggle.getToggleState());
    };
    addAndMakeVisible (m_showSongHeaderToggle);

    m_showHistoryTapeToggle.setButtonText ("Show History Tape");
    styleToggle (m_showHistoryTapeToggle);
    m_showHistoryTapeToggle.onClick = [this]
    {
        AppPreferences::get().setShowHistoryTape (m_showHistoryTapeToggle.getToggleState());
    };
    addAndMakeVisible (m_showHistoryTapeToggle);

    m_showLooperToggle.setButtonText ("Show Looper");
    styleToggle (m_showLooperToggle);
    m_showLooperToggle.onClick = [this]
    {
        AppPreferences::get().setShowLooper (m_showLooperToggle.getToggleState());
    };
    addAndMakeVisible (m_showLooperToggle);

    m_showZoneCMacrosToggle.setButtonText ("Show Zone C Macros");
    styleToggle (m_showZoneCMacrosToggle);
    m_showZoneCMacrosToggle.onClick = [this]
    {
        AppPreferences::get().setShowZoneCMacros (m_showZoneCMacrosToggle.getToggleState());
    };
    addAndMakeVisible (m_showZoneCMacrosToggle);

    m_compactSongHeaderToggle.setButtonText ("Compact Song Header");
    styleToggle (m_compactSongHeaderToggle);
    m_compactSongHeaderToggle.onClick = [this]
    {
        AppPreferences::get().setCompactSongHeader (m_compactSongHeaderToggle.getToggleState());
    };
    addAndMakeVisible (m_compactSongHeaderToggle);

    m_layoutHint.setText ("Shell, looper, and macro-strip toggles reclaim layout space immediately.", juce::dontSendNotification);
    m_layoutHint.setFont (Theme::Font::micro());
    m_layoutHint.setColour (juce::Label::textColourId, Theme::Colour::inkGhost);
    m_layoutHint.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (m_layoutHint);

    AppPreferences::get().addListener (this);
    ThemeManager::get().addListener (this);
    refreshFromPreferences();
}

SettingsPanel::~SettingsPanel()
{
    ThemeManager::get().removeListener (this);
    AppPreferences::get().removeListener (this);
}

void SettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    auto bounds = getLocalBounds().reduced (8);
    ZoneAStyle::drawHeader (g, bounds.removeFromTop (ZoneAStyle::compactHeaderHeight()), "APP SETTINGS", Theme::Zone::a,
                            { juce::Justification::centredLeft, true });

    auto appearanceCard = bounds.removeFromTop (100);
    ZoneAStyle::drawCard (g, appearanceCard, Theme::Zone::a, ZoneAStyle::cardRadius());
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("Appearance", appearanceCard.reduced (8, 4).removeFromTop (12), juce::Justification::centredLeft, false);

    bounds.removeFromTop (6);
    auto layoutCard = bounds.removeFromTop (160);
    ZoneAStyle::drawCard (g, layoutCard, Theme::Colour::accentWarm, ZoneAStyle::cardRadius());
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("Layout", layoutCard.reduced (8, 4).removeFromTop (12), juce::Justification::centredLeft, false);
}

void SettingsPanel::resized()
{
    const int gap = ZoneAControlStyle::compactGap() + 2;
    const int labelH = ZoneAControlStyle::sectionHeaderHeight() - 2;
    const int rowH = ZoneAControlStyle::controlHeight();
    const int headerH = ZoneAStyle::compactHeaderHeight();

    auto bounds = getLocalBounds().reduced (gap + 4);
    bounds.removeFromTop (headerH);

    auto appearanceCard = bounds.removeFromTop (labelH + rowH + rowH + labelH + gap * 5).reduced (gap + 4, gap + 2);
    appearanceCard.removeFromTop (labelH);
    m_themeLabel.setBounds (appearanceCard.removeFromTop (labelH));
    m_themeCombo.setBounds (appearanceCard.removeFromTop (rowH));
    appearanceCard.removeFromTop (gap + 1);
    auto themeButtonRow = appearanceCard.removeFromTop (rowH);
    m_customizeThemeButton.setBounds (themeButtonRow.removeFromLeft (96));
    themeButtonRow.removeFromLeft (gap);
    m_importThemeButton.setBounds (themeButtonRow.removeFromLeft (58));
    themeButtonRow.removeFromLeft (gap);
    m_exportThemeButton.setBounds (themeButtonRow.removeFromLeft (58));
    appearanceCard.removeFromTop (gap + 1);
    m_themeHint.setBounds (appearanceCard.removeFromTop (labelH));

    bounds.removeFromTop (gap + 1);
    auto layoutCard = bounds.reduced (gap + 4, gap + 2);
    layoutCard.removeFromTop (labelH);
    m_showFeedToggle.setBounds (layoutCard.removeFromTop (rowH));
    layoutCard.removeFromTop (gap);
    m_showSongHeaderToggle.setBounds (layoutCard.removeFromTop (rowH));
    layoutCard.removeFromTop (gap);
    m_showHistoryTapeToggle.setBounds (layoutCard.removeFromTop (rowH));
    layoutCard.removeFromTop (gap);
    m_showLooperToggle.setBounds (layoutCard.removeFromTop (rowH));
    layoutCard.removeFromTop (gap);
    m_showZoneCMacrosToggle.setBounds (layoutCard.removeFromTop (rowH));
    layoutCard.removeFromTop (gap);
    m_compactSongHeaderToggle.setBounds (layoutCard.removeFromTop (rowH));
    layoutCard.removeFromTop (gap);
    m_layoutHint.setBounds (layoutCard.removeFromTop (labelH + 2));
}

void SettingsPanel::refreshFromPreferences()
{
    const auto& prefs = AppPreferences::get();
    m_showFeedToggle.setToggleState (prefs.getShowSystemFeed(), juce::dontSendNotification);
    m_showSongHeaderToggle.setToggleState (prefs.getShowSongHeader(), juce::dontSendNotification);
    m_showHistoryTapeToggle.setToggleState (prefs.getShowHistoryTape(), juce::dontSendNotification);
    m_showLooperToggle.setToggleState (prefs.getShowLooper(), juce::dontSendNotification);
    m_showZoneCMacrosToggle.setToggleState (prefs.getShowZoneCMacros(), juce::dontSendNotification);
    m_compactSongHeaderToggle.setToggleState (prefs.getCompactSongHeader(), juce::dontSendNotification);
    m_compactSongHeaderToggle.setEnabled (prefs.getShowSongHeader());

    const auto presetName = prefs.getThemePresetName().trim();
    if (presetName.isNotEmpty())
        m_themeCombo.setText (presetName, juce::dontSendNotification);
    else
        m_themeCombo.setText (ThemeManager::get().theme().presetName, juce::dontSendNotification);

    repaint();
}

void SettingsPanel::populateThemePresets()
{
    m_themeCombo.clear (juce::dontSendNotification);

    int itemId = 1;
    for (auto& preset : ThemePresets::builtInPresets())
        m_themeCombo.addItem (preset.presetName, itemId++);

    for (auto& preset : ThemePresets::userPresets())
        m_themeCombo.addItem (preset.presetName, itemId++);
}

void SettingsPanel::appPreferencesChanged()
{
    refreshFromPreferences();
}

void SettingsPanel::themeChanged()
{
    populateThemePresets();
    const auto presetName = ThemeManager::get().theme().presetName.trim();
    if (presetName.isNotEmpty())
        m_themeCombo.setText (presetName, juce::dontSendNotification);
    repaint();
}

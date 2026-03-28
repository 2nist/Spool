#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeManager.h"
#include "ColourRow.h"
#include "FloatRow.h"
#include "../Theme.h"
#include "../state/AppPreferences.h"
#include "IconTabButton.h"

//==============================================================================
/**
    ThemeEditorPanel — live theme editor hosted in Zone A's panel content view.

    Layout:
        [Header: preset combo + EXPORT button]
        [Tab bar: SURFACE | INK | ACCENT | ZONES | SIGNAL | TYPE | SPACE | TAPE]
        [Scrollable content area with ColourRow / FloatRow widgets]
        [Footer: SAVE AS | RESET]

    Wire-up:
        - PanelContentView creates this when panelId == "theme"
        - PluginEditor::themeChanged() calls refreshAllRows() on this panel
          (or just repaint() since all rows read ThemeManager directly)
*/
class ThemeEditorPanel final : public juce::Component,
                                public ThemeManager::Listener
{
public:
    ThemeEditorPanel();
    ~ThemeEditorPanel() override;

    //==========================================================================
    void resized() override;
    void paint   (juce::Graphics&) override;

    // ThemeManager::Listener
    void themeChanged() override;

private:
    struct TabPage;

    //==========================================================================
    // Tab identifiers
    enum Tab { kSurface, kInk, kAccent, kZones, kSignal, kType, kSpace, kUi, kControls, kStyle, kPreview, kTape, kTabCount };
    static const char* tabName (Tab t);

    void buildAllTabs();
    void buildSurfaceTab  (juce::Component& container);
    void buildInkTab      (juce::Component& container);
    void buildAccentTab   (juce::Component& container);
    void buildZonesTab    (juce::Component& container);
    void buildSignalTab   (juce::Component& container);
    void buildTypeTab     (juce::Component& container);
    void buildSpaceTab    (juce::Component& container);
    void buildUiTab       (juce::Component& container);
    void buildControlsTab (juce::Component& container);
    void buildStyleTab    (juce::Component& container);
    void buildPreviewTab  (juce::Component& container);
    void buildTapeTab     (juce::Component& container);
    void relayoutPage     (TabPage& page);

    void switchTab (int newTab);
    void refreshAllRows();

    void populatePresetCombo();
    void onPresetSelected (int comboId);
    void onExportClicked();
    void onSaveAsClicked();
    void onResetClicked();

    //==========================================================================
    // Header
    juce::Label          m_titleLabel;
    juce::ComboBox       m_presetCombo;
    juce::TextButton     m_exportBtn  { "EXPORT" };

    // Tab bar
    juce::OwnedArray<IconTabButton> m_tabButtons;
    int                                m_activeTab = kSurface;

    // Content — one viewport+container per tab
    struct TabPage
    {
        juce::Viewport                  viewport;
        juce::Component                 container;
        juce::OwnedArray<ColourRow>     colourRows;
        juce::OwnedArray<FloatRow>      floatRows;
        std::unique_ptr<juce::Component> customContent;
        int contentHeight { 0 };
    };
    std::array<std::unique_ptr<TabPage>, kTabCount> m_pages;

    // Footer
    juce::TextButton m_saveAsBtn { "SAVE AS" };
    juce::TextButton m_resetBtn  { "RESET"   };

    static constexpr int kHeaderH = 36;
    static constexpr int kTabBarH = 28;
    static constexpr int kFooterH = 32;
    static constexpr int kRowH    = 26;
    static constexpr int kRowGap  = 2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThemeEditorPanel)
};

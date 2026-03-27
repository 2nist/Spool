#include "PresetBar.h"

//==============================================================================

PresetBar::PresetBar()
{
    // Name editor
    m_nameEditor.setMultiLine (false);
    m_nameEditor.setReturnKeyStartsNewLine (false);
    m_nameEditor.setScrollbarsShown (false);
    m_nameEditor.setPopupMenuEnabled (false);
    ZoneAControlStyle::styleTextEditor (m_nameEditor);
    m_nameEditor.addListener (this);
    addAndMakeVisible (m_nameEditor);

    // ▾ browse button
    styleButton (m_browseBtn);
    m_browseBtn.onClick = [this] { showBrowsePopup(); };
    addAndMakeVisible (m_browseBtn);

    // INIT
    styleButton (m_initBtn);
    m_initBtn.onClick = [this] { loadInitPreset(); };
    addAndMakeVisible (m_initBtn);

    // SAVE
    styleButton (m_saveBtn);
    m_saveBtn.onClick = [this] { saveCurrentPreset(); };
    addAndMakeVisible (m_saveBtn);

    displayPreset ("Init Poly", true);
}

//==============================================================================

void PresetBar::styleButton (juce::TextButton& b)
{
    ZoneAControlStyle::styleTextButton (b, Theme::Zone::a);
}

//==============================================================================
// Configuration
//==============================================================================

void PresetBar::setModuleType (PresetManager::ModuleType type)
{
    m_moduleType = type;
    // Show the init preset name for this type
    auto& pm      = PresetManager::getInstance();
    const auto& presets = (type == PresetManager::ModuleType::Synth)
                          ? pm.getSynthPresets()
                          : pm.getDrumPresets();
    if (!presets.isEmpty())
        displayPreset (presets[0].name, true);
}

void PresetBar::displayPreset (const juce::String& name, bool /*isFactory*/)
{
    m_committedName = name;
    m_nameEditor.setText (name, juce::dontSendNotification);
    m_nameEditor.setColour (juce::TextEditor::textColourId,
                             juce::Colour (Theme::Colour::inkLight));
    repaint();
}

juce::String PresetBar::getCurrentName() const
{
    return m_nameEditor.getText().trim();
}

bool PresetBar::isDirty() const
{
    return getCurrentName() != m_committedName;
}

//==============================================================================
// Layout
//==============================================================================

void PresetBar::resized()
{
    constexpr int kBtnW    = 38;
    constexpr int kBrowseW = 20;
    constexpr int kGap     = 2;
    constexpr int kPad     = 4;
    constexpr int kButtonH = 16;
    const int h = getHeight();
    const int w = getWidth();
    const int by = (h - kButtonH) / 2;

    int x = w - kPad;

    x -= kBtnW;
    m_saveBtn.setBounds (x, by, kBtnW, kButtonH);

    x -= kGap + kBtnW;
    m_initBtn.setBounds (x, by, kBtnW, kButtonH);

    x -= kGap + kBrowseW;
    m_browseBtn.setBounds (x, by, kBrowseW, kButtonH);

    const int availableNameW = x - kGap - kPad;
    const int targetNameW = juce::jlimit (72, availableNameW, w * 2 / 5);
    m_nameEditor.setBounds (kPad, by - 1, targetNameW, kButtonH + 2);
}

//==============================================================================
// Paint
//==============================================================================

void PresetBar::paint (juce::Graphics& g)
{
    g.setColour (Theme::Colour::surface1);
    g.fillRect  (getLocalBounds());

    g.setColour (juce::Colour (Theme::Colour::surfaceEdge).withAlpha (0.4f));
    g.fillRect  (0, 0, getWidth(), 1);
    g.setColour (Theme::Zone::a.withAlpha (0.85f));
    g.fillRect (0, 0, 3, getHeight());

    // Dirty indicator — subtle asterisk after name
    if (isDirty())
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (juce::Colour (Theme::Colour::inkGhost));
        const auto nb = m_nameEditor.getBounds();
        g.drawText  ("*", nb.getRight() + 1, 0, 8, getHeight(),
                     juce::Justification::centredLeft, false);
    }
}

//==============================================================================
// Mouse
//==============================================================================

void PresetBar::mouseDown (const juce::MouseEvent&)
{
    // Allow clicking the non-editor area to focus the name field
}

//==============================================================================
// Actions
//==============================================================================

void PresetBar::showBrowsePopup()
{
    auto& pm            = PresetManager::getInstance();
    const auto& presets = (m_moduleType == PresetManager::ModuleType::Synth)
                          ? pm.getSynthPresets()
                          : pm.getDrumPresets();

    juce::PopupMenu menu;

    // Factory group
    menu.addSectionHeader ("Factory");
    for (int i = 0; i < presets.size(); ++i)
        if (presets[i].isFactory)
            menu.addItem (i + 1, presets[i].name);

    // User group (if any)
    bool hasUser = false;
    for (const auto& p : presets)
        if (!p.isFactory) { hasUser = true; break; }

    if (hasUser)
    {
        menu.addSeparator();
        menu.addSectionHeader ("User");
        for (int i = 0; i < presets.size(); ++i)
            if (!presets[i].isFactory)
                menu.addItem (i + 1, presets[i].name);
    }

    menu.showMenuAsync (
        juce::PopupMenu::Options()
            .withTargetComponent (this)
            .withMaximumNumColumns (1),
        [this] (int result)
        {
            if (result <= 0) return;
            auto& pm2            = PresetManager::getInstance();
            const auto& presets2 = (m_moduleType == PresetManager::ModuleType::Synth)
                                   ? pm2.getSynthPresets()
                                   : pm2.getDrumPresets();
            const int idx = result - 1;
            if (idx < 0 || idx >= presets2.size()) return;

            const auto& entry = presets2[idx];
            const auto  data  = PresetManager::loadPresetData (entry);
            if (data.isObject() && onLoadPreset)
            {
                onLoadPreset (data);
                displayPreset (entry.name, entry.isFactory);
            }
        });
}

void PresetBar::loadInitPreset()
{
    auto& pm            = PresetManager::getInstance();
    const auto& presets = (m_moduleType == PresetManager::ModuleType::Synth)
                          ? pm.getSynthPresets()
                          : pm.getDrumPresets();

    if (presets.isEmpty()) return;

    // First factory preset is always the "Init" preset
    const auto& entry = presets[0];
    const auto  data  = PresetManager::loadPresetData (entry);
    if (data.isObject() && onLoadPreset)
    {
        onLoadPreset (data);
        displayPreset (entry.name, true);
    }
}

void PresetBar::saveCurrentPreset()
{
    const juce::String name = getCurrentName().trim();
    if (name.isEmpty()) return;
    if (!onRequestState) return;

    const auto data = onRequestState();
    if (!data.isObject()) return;

    auto& pm = PresetManager::getInstance();

    if (m_moduleType == PresetManager::ModuleType::Synth)
        pm.saveUserSynthPreset (name, data);
    else
        pm.saveUserDrumPreset (name, data);

    displayPreset (name, false);
}

void PresetBar::commitNameChanges()
{
    const juce::String newName = getCurrentName();
    if (newName.isEmpty() || newName == m_committedName) return;

    // Auto-save with the new name
    saveCurrentPreset();
}

//==============================================================================
// TextEditor::Listener
//==============================================================================

void PresetBar::textEditorReturnKeyPressed (juce::TextEditor&)
{
    commitNameChanges();
    m_nameEditor.giveAwayKeyboardFocus();
}

void PresetBar::textEditorFocusLost (juce::TextEditor&)
{
    commitNameChanges();
}

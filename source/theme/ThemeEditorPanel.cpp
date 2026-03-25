#include "ThemeEditorPanel.h"
#include "ThemePresets.h"

//==============================================================================
// Helpers

static void styleSmallButton (juce::TextButton& b)
{
    b.setLookAndFeel (nullptr);
    b.setColour (juce::TextButton::buttonColourId,   Theme::Colour::surface3);
    b.setColour (juce::TextButton::buttonOnColourId, Theme::Colour::accent.withAlpha (0.3f));
    b.setColour (juce::TextButton::textColourOffId,  Theme::Colour::inkMuted);
    b.setColour (juce::TextButton::textColourOnId,   Theme::Colour::inkLight);
}

//==============================================================================

ThemeEditorPanel::ThemeEditorPanel()
{
    // Title
    m_titleLabel.setText ("THEME EDITOR", juce::dontSendNotification);
    m_titleLabel.setFont (Theme::Font::micro());
    m_titleLabel.setColour (juce::Label::textColourId, Theme::Colour::inkMuted);
    m_titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (m_titleLabel);

    // Preset combo
    m_presetCombo.setTextWhenNothingSelected ("Preset...");
    m_presetCombo.setColour (juce::ComboBox::backgroundColourId, Theme::Colour::surface3);
    m_presetCombo.setColour (juce::ComboBox::textColourId,       Theme::Colour::inkMid);
    m_presetCombo.setColour (juce::ComboBox::outlineColourId,    Theme::Colour::surfaceEdge);
    m_presetCombo.onChange = [this] { onPresetSelected (m_presetCombo.getSelectedId()); };
    addAndMakeVisible (m_presetCombo);
    populatePresetCombo();

    // Export button
    styleSmallButton (m_exportBtn);
    m_exportBtn.onClick = [this] { onExportClicked(); };
    addAndMakeVisible (m_exportBtn);

    // Tab buttons — icon tabs using Lucide SVG paths
    static const char* kIcons[kTabCount] = {
        LucideIcons::surface,    // kSurface
        LucideIcons::ink,        // kInk
        LucideIcons::accent,     // kAccent
        LucideIcons::zones,      // kZones
        LucideIcons::signal,     // kSignal
        LucideIcons::typography, // kType
        LucideIcons::spacing,    // kSpace
        LucideIcons::tape,       // kTape
    };
    static const char* kTooltips[kTabCount] = {
        "Surface Colors", "Ink & Text", "Accent Colors", "Zone Colors",
        "Signal Colors",  "Typography", "Spacing",       "Tape Colors",
    };
    for (int i = 0; i < kTabCount; ++i)
    {
        auto* btn = m_tabButtons.add (new IconTabButton (kIcons[i], kTooltips[i]));
        btn->onClick = [this, i] { switchTab (i); };
        addAndMakeVisible (btn);
    }

    // Footer
    styleSmallButton (m_saveAsBtn);
    m_saveAsBtn.onClick = [this] { onSaveAsClicked(); };
    addAndMakeVisible (m_saveAsBtn);

    styleSmallButton (m_resetBtn);
    m_resetBtn.onClick = [this] { onResetClicked(); };
    addAndMakeVisible (m_resetBtn);

    // Build pages
    buildAllTabs();

    // Show first tab
    switchTab (kSurface);

    ThemeManager::get().addListener (this);
}

ThemeEditorPanel::~ThemeEditorPanel()
{
    ThemeManager::get().removeListener (this);
}

//==============================================================================

void ThemeEditorPanel::resized()
{
    auto r = getLocalBounds();

    // Header
    auto header = r.removeFromTop (kHeaderH);
    m_titleLabel .setBounds (header.removeFromLeft (90).reduced (4, 6));
    m_exportBtn  .setBounds (header.removeFromRight (52).reduced (2, 5));
    m_presetCombo.setBounds (header.reduced (4, 5));

    // Tab bar
    auto tabBar = r.removeFromTop (kTabBarH);
    const int tabW = tabBar.getWidth() / kTabCount;
    for (int i = 0; i < kTabCount; ++i)
        m_tabButtons[i]->setBounds (tabBar.removeFromLeft (tabW).reduced (1, 2));

    // Footer
    auto footer = r.removeFromBottom (kFooterH);
    m_resetBtn .setBounds (footer.removeFromRight (60).reduced (3, 5));
    m_saveAsBtn.setBounds (footer.removeFromRight (60).reduced (3, 5));

    // Content
    for (auto& page : m_pages)
        if (page != nullptr)
            page->viewport.setBounds (r);
}

void ThemeEditorPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    // Header separator
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawHorizontalLine (kHeaderH, 0.f, (float) getWidth());

    // Tab bar bottom line
    g.drawHorizontalLine (kHeaderH + kTabBarH, 0.f, (float) getWidth());

    // Footer top line
    g.drawHorizontalLine (getHeight() - kFooterH, 0.f, (float) getWidth());

    // Active tab indicator
    if (m_activeTab >= 0 && m_activeTab < kTabCount)
    {
        const auto tabBounds = m_tabButtons[m_activeTab]->getBounds();
        g.setColour (Theme::Colour::accent);
        g.fillRect (tabBounds.getX(), kHeaderH + kTabBarH - 2, tabBounds.getWidth(), 2);
    }
}

//==============================================================================

void ThemeEditorPanel::themeChanged()
{
    refreshAllRows();
    repaint();
}

void ThemeEditorPanel::refreshAllRows()
{
    for (auto& page : m_pages)
    {
        if (page == nullptr) continue;
        for (auto* r : page->colourRows) r->refresh();
        for (auto* r : page->floatRows)  r->refresh();
    }
}

//==============================================================================

const char* ThemeEditorPanel::tabName (Tab t)
{
    switch (t)
    {
        case kSurface: return "SURFACE";
        case kInk:     return "INK";
        case kAccent:  return "ACCENT";
        case kZones:   return "ZONES";
        case kSignal:  return "SIGNAL";
        case kType:    return "TYPE";
        case kSpace:   return "SPACE";
        case kTape:    return "TAPE";
        default:       return "?";
    }
}

void ThemeEditorPanel::switchTab (int newTab)
{
    m_activeTab = newTab;
    for (int i = 0; i < kTabCount; ++i)
    {
        const bool isActive = (i == newTab);
        m_pages[i]->viewport.setVisible (isActive);
        m_tabButtons[i]->setToggleState (isActive, juce::dontSendNotification);
    }
    repaint();
}

//==============================================================================
// Tab builders

static void layoutRows (juce::Component& container,
                        juce::OwnedArray<ColourRow>& crows,
                        juce::OwnedArray<FloatRow>&  frows,
                        int rowH, int gap)
{
    const int total = crows.size() + frows.size();
    const int h     = total * (rowH + gap) + gap;
    container.setSize (container.getWidth() > 0 ? container.getWidth() : 200, h);

    int y = gap;
    for (auto* r : crows) { r->setBounds (4, y, container.getWidth() - 8, rowH); y += rowH + gap; }
    for (auto* r : frows) { r->setBounds (4, y, container.getWidth() - 8, rowH); y += rowH + gap; }
}

void ThemeEditorPanel::buildAllTabs()
{
    for (int i = 0; i < kTabCount; ++i)
    {
        m_pages[i] = std::make_unique<TabPage>();
        auto& page = *m_pages[i];
        page.viewport.setViewedComponent (&page.container, false);
        page.viewport.setScrollBarsShown (true, false);
        page.container.setSize (200, 400);
        addAndMakeVisible (page.viewport);

        switch (static_cast<Tab> (i))
        {
            case kSurface: buildSurfaceTab (page.container); break;
            case kInk:     buildInkTab     (page.container); break;
            case kAccent:  buildAccentTab  (page.container); break;
            case kZones:   buildZonesTab   (page.container); break;
            case kSignal:  buildSignalTab  (page.container); break;
            case kType:    buildTypeTab    (page.container); break;
            case kSpace:   buildSpaceTab   (page.container); break;
            case kTape:    buildTapeTab    (page.container); break;
            default: break;
        }

        layoutRows (page.container, page.colourRows, page.floatRows, kRowH, kRowGap);
    }
}

static ColourRow* addCR (juce::Component& c, juce::OwnedArray<ColourRow>& arr,
                          const char* label, juce::Colour ThemeData::* ptr)
{
    auto* row = arr.add (new ColourRow (label, ptr));
    c.addAndMakeVisible (row);
    return row;
}

static FloatRow* addFR (juce::Component& c, juce::OwnedArray<FloatRow>& arr,
                         const char* label, float ThemeData::* ptr,
                         float minV, float maxV, float step = 0.5f)
{
    auto* row = arr.add (new FloatRow (label, ptr, minV, maxV, step));
    c.addAndMakeVisible (row);
    return row;
}

void ThemeEditorPanel::buildSurfaceTab (juce::Component& c)
{
    auto& cr = m_pages[kSurface]->colourRows;
    addCR (c, cr, "Surface 0 (base)",      &ThemeData::surface0);
    addCR (c, cr, "Surface 1 (page bg)",   &ThemeData::surface1);
    addCR (c, cr, "Surface 2 (panel)",     &ThemeData::surface2);
    addCR (c, cr, "Surface 3 (elevated)",  &ThemeData::surface3);
    addCR (c, cr, "Surface 4 (raised)",    &ThemeData::surface4);
    addCR (c, cr, "Surface Edge",          &ThemeData::surfaceEdge);
    addCR (c, cr, "Housing Edge",          &ThemeData::housingEdge);
}

void ThemeEditorPanel::buildInkTab (juce::Component& c)
{
    auto& cr = m_pages[kInk]->colourRows;
    addCR (c, cr, "Ink Light (primary)",   &ThemeData::inkLight);
    addCR (c, cr, "Ink Mid (secondary)",   &ThemeData::inkMid);
    addCR (c, cr, "Ink Muted (tertiary)",  &ThemeData::inkMuted);
    addCR (c, cr, "Ink Ghost (hints)",     &ThemeData::inkGhost);
    addCR (c, cr, "Ink Dark (on paper)",   &ThemeData::inkDark);
}

void ThemeEditorPanel::buildAccentTab (juce::Component& c)
{
    auto& cr = m_pages[kAccent]->colourRows;
    addCR (c, cr, "Accent (primary)",  &ThemeData::accent);
    addCR (c, cr, "Accent Warm",       &ThemeData::accentWarm);
    addCR (c, cr, "Accent Dim",        &ThemeData::accentDim);
}

void ThemeEditorPanel::buildZonesTab (juce::Component& c)
{
    auto& cr = m_pages[kZones]->colourRows;
    addCR (c, cr, "Zone A accent",     &ThemeData::zoneA);
    addCR (c, cr, "Zone A background", &ThemeData::zoneBgA);
    addCR (c, cr, "Zone B accent",     &ThemeData::zoneB);
    addCR (c, cr, "Zone B background", &ThemeData::zoneBgB);
    addCR (c, cr, "Zone C accent",     &ThemeData::zoneC);
    addCR (c, cr, "Zone C background", &ThemeData::zoneBgC);
    addCR (c, cr, "Zone D accent",     &ThemeData::zoneD);
    addCR (c, cr, "Zone D background", &ThemeData::zoneBgD);
    addCR (c, cr, "Menu Bar",          &ThemeData::zoneMenu);
}

void ThemeEditorPanel::buildSignalTab (juce::Component& c)
{
    auto& cr = m_pages[kSignal]->colourRows;
    addCR (c, cr, "Audio",  &ThemeData::signalAudio);
    addCR (c, cr, "MIDI",   &ThemeData::signalMidi);
    addCR (c, cr, "CV",     &ThemeData::signalCv);
    addCR (c, cr, "Gate",   &ThemeData::signalGate);
}

void ThemeEditorPanel::buildTypeTab (juce::Component& c)
{
    auto& fr = m_pages[kType]->floatRows;
    addFR (c, fr, "Display size",   &ThemeData::sizeDisplay,   8.f, 24.f);
    addFR (c, fr, "Heading size",   &ThemeData::sizeHeading,   8.f, 24.f);
    addFR (c, fr, "Label size",     &ThemeData::sizeLabel,     7.f, 18.f);
    addFR (c, fr, "Body size",      &ThemeData::sizeBody,      7.f, 18.f);
    addFR (c, fr, "Micro size",     &ThemeData::sizeMicro,     6.f, 16.f);
    addFR (c, fr, "Value size",     &ThemeData::sizeValue,     7.f, 18.f);
    addFR (c, fr, "Transport size", &ThemeData::sizeTransport, 8.f, 24.f);
}

void ThemeEditorPanel::buildSpaceTab (juce::Component& c)
{
    auto& fr = m_pages[kSpace]->floatRows;
    addFR (c, fr, "XS (4px base)",  &ThemeData::spaceXs,  2.f, 12.f, 1.f);
    addFR (c, fr, "SM (8px)",       &ThemeData::spaceSm,  4.f, 20.f, 1.f);
    addFR (c, fr, "MD (12px)",      &ThemeData::spaceMd,  6.f, 24.f, 1.f);
    addFR (c, fr, "LG (16px)",      &ThemeData::spaceLg,  8.f, 32.f, 1.f);
    addFR (c, fr, "XL (20px)",      &ThemeData::spaceXl,  10.f, 40.f, 1.f);
    addFR (c, fr, "XXL (24px)",     &ThemeData::spaceXxl, 12.f, 48.f, 1.f);
    addFR (c, fr, "Knob size",      &ThemeData::knobSize,      20.f, 60.f, 1.f);
    addFR (c, fr, "Module slot H",  &ThemeData::moduleSlotHeight, 40.f, 140.f, 2.f);
    addFR (c, fr, "Step H",         &ThemeData::stepHeight, 12.f, 48.f, 1.f);
    addFR (c, fr, "Step W",         &ThemeData::stepWidth,  12.f, 48.f, 1.f);
}

void ThemeEditorPanel::buildTapeTab (juce::Component& c)
{
    auto& cr = m_pages[kTape]->colourRows;
    addCR (c, cr, "Tape Base",        &ThemeData::tapeBase);
    addCR (c, cr, "Clip Background",  &ThemeData::tapeClipBg);
    addCR (c, cr, "Clip Border",      &ThemeData::tapeClipBorder);
    addCR (c, cr, "Seam",             &ThemeData::tapeSeam);
    addCR (c, cr, "Beat Tick",        &ThemeData::tapeBeatTick);
    addCR (c, cr, "Playhead",         &ThemeData::playheadColor);
}

//==============================================================================
// Preset management

void ThemeEditorPanel::populatePresetCombo()
{
    m_presetCombo.clear (juce::dontSendNotification);
    int id = 1;

    m_presetCombo.addSectionHeading ("Built-in");
    for (auto& p : ThemePresets::builtInPresets())
        m_presetCombo.addItem (p.presetName, id++);

    const auto userP = ThemePresets::userPresets();
    if (!userP.isEmpty())
    {
        m_presetCombo.addSeparator();
        m_presetCombo.addSectionHeading ("Saved");
        for (auto& p : userP)
            m_presetCombo.addItem (p.presetName, id++);
    }
}

void ThemeEditorPanel::onPresetSelected (int comboId)
{
    if (comboId <= 0) return;

    // Build combined list in same order as populatePresetCombo
    juce::Array<ThemeData> all;
    for (auto& p : ThemePresets::builtInPresets()) all.add (p);
    for (auto& p : ThemePresets::userPresets())    all.add (p);

    const int idx = comboId - 1;
    if (idx >= 0 && idx < all.size())
        ThemeManager::get().applyTheme (all[idx]);
}

void ThemeEditorPanel::onExportClicked()
{
    // Generate a compilable Theme.h snippet with current values
    const auto& t  = ThemeManager::get().theme();
    juce::String s = "// Generated by SPOOL Theme Editor — " + t.presetName + "\n";
    s += "// Paste into source/Theme.h namespace Colour block\n\n";

    auto hex = [] (juce::Colour c) { return "0xFF" + c.toString().toUpperCase().substring (2); };
    s += "// Surfaces\n";
    s += "inline const juce::Colour surface0    { " + hex (t.surface0)    + " };\n";
    s += "inline const juce::Colour surface1    { " + hex (t.surface1)    + " };\n";
    s += "inline const juce::Colour surface2    { " + hex (t.surface2)    + " };\n";
    s += "inline const juce::Colour surface3    { " + hex (t.surface3)    + " };\n";
    s += "inline const juce::Colour surface4    { " + hex (t.surface4)    + " };\n";
    s += "inline const juce::Colour surfaceEdge { " + hex (t.surfaceEdge) + " };\n\n";
    s += "// Ink\n";
    s += "inline const juce::Colour inkLight    { " + hex (t.inkLight)    + " };\n";
    s += "inline const juce::Colour inkMid      { " + hex (t.inkMid)      + " };\n";
    s += "inline const juce::Colour inkMuted    { " + hex (t.inkMuted)    + " };\n";
    s += "inline const juce::Colour inkGhost    { " + hex (t.inkGhost)    + " };\n";
    s += "inline const juce::Colour inkDark     { " + hex (t.inkDark)     + " };\n\n";
    s += "// Accent\n";
    s += "inline const juce::Colour accent      { " + hex (t.accent)      + " };\n";
    s += "inline const juce::Colour accentWarm  { " + hex (t.accentWarm)  + " };\n";
    s += "inline const juce::Colour accentDim   { " + hex (t.accentDim)   + " };\n\n";
    s += "// Zones\n";
    s += "inline const juce::Colour zoneA       { " + hex (t.zoneA)    + " };\n";
    s += "inline const juce::Colour zoneB       { " + hex (t.zoneB)    + " };\n";
    s += "inline const juce::Colour zoneC       { " + hex (t.zoneC)    + " };\n";
    s += "inline const juce::Colour zoneD       { " + hex (t.zoneD)    + " };\n";
    s += "inline const juce::Colour zoneMenu    { " + hex (t.zoneMenu) + " };\n";
    s += "// Zone backgrounds\n";
    s += "inline const juce::Colour zoneBgA     { " + hex (t.zoneBgA)  + " };\n";
    s += "inline const juce::Colour zoneBgB     { " + hex (t.zoneBgB)  + " };\n";
    s += "inline const juce::Colour zoneBgC     { " + hex (t.zoneBgC)  + " };\n";
    s += "inline const juce::Colour zoneBgD     { " + hex (t.zoneBgD)  + " };\n";

    juce::SystemClipboard::copyTextToClipboard (s);
    juce::AlertWindow::showMessageBoxAsync (
        juce::MessageBoxIconType::InfoIcon,
        "Theme Exported",
        "Colour values copied to clipboard.\n\nPaste into source/Theme.h to lock the theme at compile time.",
        "OK");
}

void ThemeEditorPanel::onSaveAsClicked()
{
    auto* w = new juce::AlertWindow ("Save Theme", "Enter preset name:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", ThemeManager::get().theme().presetName, "Name");
    w->addButton ("Save",   1);
    w->addButton ("Cancel", 0);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int result)
    {
        if (result == 1)
        {
            const auto name = w->getTextEditorContents ("name").trim();
            if (name.isNotEmpty())
            {
                ThemeData td  = ThemeManager::get().theme();
                td.presetName = name;
                ThemePresets::saveUserPreset (td);
                populatePresetCombo();
            }
        }
        delete w;
    }), false);  // false: we delete manually in callback
}

void ThemeEditorPanel::onResetClicked()
{
    ThemeManager::get().applyTheme (ThemePresets::presetByName ("Espresso"));
}

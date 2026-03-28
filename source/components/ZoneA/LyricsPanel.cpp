#include "LyricsPanel.h"
#include "../../PluginProcessor.h"
#include "../../PluginEditor.h"

namespace
{
juce::String fontNameForId (int id)
{
    switch (id)
    {
        case 1: return "Mono";
        case 2: return "Serif";
        case 3: return "Marker";
        default: return "Mono";
    }
}

juce::Colour colorForId (int id)
{
    switch (id)
    {
        case 1: return juce::Colour (0xFFE6DCCF);
        case 2: return Theme::Colour::accentWarm;
        case 3: return Theme::Signal::audio;
        case 4: return Theme::Semantic::strong;
        default: return juce::Colour (0xFFE6DCCF);
    }
}

int sizeForIndex (int index)
{
    switch (index)
    {
        case 0: return 12;
        case 1: return 15;
        case 2: return 18;
        default: return 15;
    }
}

struct LyricsStateUndoAction final : public juce::UndoableAction
{
    LyricsStateUndoAction (PluginProcessor& processorIn, const LyricsState& beforeIn, const LyricsState& afterIn)
        : processor (processorIn), beforeState (beforeIn), afterState (afterIn) {}

    bool perform() override
    {
        processor.getSongManager().replaceLyricsState (afterState);
        processor.syncAuthoredSongToRuntime();
        if (auto* editor = dynamic_cast<PluginEditor*> (processor.getActiveEditor()))
            editor->refreshFromAuthoredSong();
        return true;
    }

    bool undo() override
    {
        processor.getSongManager().replaceLyricsState (beforeState);
        processor.syncAuthoredSongToRuntime();
        if (auto* editor = dynamic_cast<PluginEditor*> (processor.getActiveEditor()))
            editor->refreshFromAuthoredSong();
        return true;
    }

    int getSizeInUnits() override { return 1; }

    PluginProcessor& processor;
    LyricsState beforeState;
    LyricsState afterState;
};
}

LyricsPanel::LyricsPanel (PluginProcessor& processor) : processorRef (processor)
{
    auto accent = ZoneAStyle::accentForTabId ("lyrics");
    for (auto* c : { static_cast<juce::Component*> (&itemList), static_cast<juce::Component*> (&editorViewport), static_cast<juce::Component*> (&addItemButton),
                     static_cast<juce::Component*> (&duplicateButton), static_cast<juce::Component*> (&deleteButton), static_cast<juce::Component*> (&pinButton),
                     static_cast<juce::Component*> (&listHeader), static_cast<juce::Component*> (&editorHeader), static_cast<juce::Component*> (&editorHint) })
        addAndMakeVisible (*c);

    editorViewport.setViewedComponent (&editorContent, false);
    editorViewport.setScrollBarsShown (true, false);

    for (auto* c : { static_cast<juce::Component*> (&textLabel), static_cast<juce::Component*> (&fontLabel), static_cast<juce::Component*> (&sizeLabel),
                     static_cast<juce::Component*> (&colorLabel), static_cast<juce::Component*> (&beatLabel), static_cast<juce::Component*> (&textEditor),
                     static_cast<juce::Component*> (&fontBox), static_cast<juce::Component*> (&sizeBox), static_cast<juce::Component*> (&colorBox),
                     static_cast<juce::Component*> (&beatSlider), static_cast<juce::Component*> (&beatValue), static_cast<juce::Component*> (&placementSummary) })
        editorContent.addAndMakeVisible (*c);

    listHeader.setText ("LYRIC PAD", juce::dontSendNotification);
    editorHeader.setText ("SELECTED NOTE", juce::dontSendNotification);
    editorHint.setText ("Capture fragments now, then pin them to beats or sections later.", juce::dontSendNotification);
    textLabel.setText ("TEXT", juce::dontSendNotification);
    fontLabel.setText ("FONT", juce::dontSendNotification);
    sizeLabel.setText ("SIZE", juce::dontSendNotification);
    colorLabel.setText ("COLOR", juce::dontSendNotification);
    beatLabel.setText ("BEAT / TIMELINE PIN", juce::dontSendNotification);

    for (auto* label : { &listHeader, &editorHeader })
    {
        label->setFont (Theme::Font::micro());
        label->setColour (juce::Label::textColourId, accent);
    }

    for (auto* label : { &editorHint, &textLabel, &fontLabel, &sizeLabel, &colorLabel, &beatLabel, &beatValue, &placementSummary })
    {
        label->setFont (Theme::Font::micro());
        label->setColour (juce::Label::textColourId, Theme::Colour::inkGhost);
    }

    ZoneAControlStyle::styleTextButton (addItemButton, accent);
    ZoneAControlStyle::styleTextButton (duplicateButton, accent);
    ZoneAControlStyle::styleTextButton (deleteButton, accent);
    ZoneAControlStyle::styleTextButton (pinButton, accent);
    ZoneAControlStyle::styleTextEditor (textEditor);
    for (auto* box : { &fontBox, &sizeBox, &colorBox })
        ZoneAControlStyle::styleComboBox (*box, accent);
    ZoneAControlStyle::initBarSlider (beatSlider, "BEAT");
    ZoneAControlStyle::tintBarSlider (beatSlider, accent);
    beatSlider.setRange (0.0, 256.0, 1.0);
    beatSlider.setDoubleClickReturnValue (true, -1.0);

    textEditor.setMultiLine (true);
    textEditor.setReturnKeyStartsNewLine (true);

    fontBox.addItem ("Mono", 1);
    fontBox.addItem ("Serif", 2);
    fontBox.addItem ("Marker", 3);
    sizeBox.addItem ("SM", 1);
    sizeBox.addItem ("MD", 2);
    sizeBox.addItem ("LG", 3);
    colorBox.addItem ("PAPER", 1);
    colorBox.addItem ("WARM", 2);
    colorBox.addItem ("BLUE", 3);
    colorBox.addItem ("GREEN", 4);

    itemList.setModel (this);
    itemList.setRowHeight (26);
    itemList.setColour (juce::ListBox::backgroundColourId, Theme::Colour::surface2);

    addItemButton.onClick = [this] { addItem(); };
    duplicateButton.onClick = [this] { duplicateItem(); };
    deleteButton.onClick = [this] { deleteItem(); };
    pinButton.onClick = [this] { pinSelectedToCurrentBeat(); };

    textEditor.onTextChange = [this]
    {
        if (suppressCallbacks)
            return;
        if (auto* item = selectedItem())
        {
            const auto beforeState = lyrics();
            item->text = textEditor.getText();
            commitChanges (beforeState, "Edit Lyrics");
            refreshList();
            refreshEditor();
        }
    };

    fontBox.onChange = [this]
    {
        if (suppressCallbacks)
            return;
        if (auto* item = selectedItem())
        {
            const auto beforeState = lyrics();
            item->style.fontName = fontNameForId (fontBox.getSelectedId());
            commitChanges (beforeState, "Change Lyric Font");
            refreshList();
        }
    };

    sizeBox.onChange = [this]
    {
        if (suppressCallbacks)
            return;
        if (auto* item = selectedItem())
        {
            const auto beforeState = lyrics();
            item->style.sizeIndex = juce::jlimit (0, 2, sizeBox.getSelectedId() - 1);
            commitChanges (beforeState, "Change Lyric Size");
            refreshEditor();
        }
    };

    colorBox.onChange = [this]
    {
        if (suppressCallbacks)
            return;
        if (auto* item = selectedItem())
        {
            const auto beforeState = lyrics();
            item->style.colourArgb = colorForId (colorBox.getSelectedId()).getARGB();
            commitChanges (beforeState, "Change Lyric Colour");
            refreshList();
            refreshEditor();
        }
    };

    beatSlider.onValueChange = [this]
    {
        if (suppressCallbacks)
            return;
        if (auto* item = selectedItem())
        {
            const auto beforeState = lyrics();
            item->beatPosition = beatSlider.getValue() < 0.0 ? -1.0 : beatSlider.getValue();
            item->pinnedToTimeline = item->beatPosition >= 0.0;
            commitChanges (beforeState, "Move Lyric Pin");
            refreshEditor();
        }
    };

    refreshList();
    refreshEditor();
}

LyricsState& LyricsPanel::lyricsForEdit()
{
    return processorRef.getSongManager().getLyricsStateForEdit();
}

const LyricsState& LyricsPanel::lyrics() const
{
    return processorRef.getSongManager().getLyricsState();
}

LyricItem* LyricsPanel::selectedItem()
{
    auto& state = lyricsForEdit();
    if (selectedIndex < 0 || selectedIndex >= static_cast<int> (state.items.size()))
        return nullptr;
    return &state.items[(size_t) selectedIndex];
}

const LyricItem* LyricsPanel::selectedItem() const
{
    return const_cast<LyricsPanel*> (this)->selectedItem();
}

void LyricsPanel::commitChanges (const LyricsState& beforeState, const juce::String& actionName)
{
    const auto afterState = lyrics();
    processorRef.getUndoManager().beginNewTransaction (actionName);
    processorRef.getUndoManager().perform (new LyricsStateUndoAction (processorRef, beforeState, afterState));
}

int LyricsPanel::getNumRows()
{
    return static_cast<int> (lyrics().items.size());
}

void LyricsPanel::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    const auto& state = lyrics();
    if (row < 0 || row >= static_cast<int> (state.items.size()))
        return;

    const auto& item = state.items[(size_t) row];
    const auto accent = ZoneAStyle::accentForTabId ("lyrics");
    ZoneAStyle::drawRowBackground (g, { 0, 0, width, height }, false, selected, accent);
    g.setColour (juce::Colour (item.style.colourArgb));
    g.setFont ((float) juce::jlimit (11, 18, sizeForIndex (item.style.sizeIndex)));
    g.drawText (item.text.upToFirstOccurrenceOf ("\n", false, false),
                8, 0, width - 78, height, juce::Justification::centredLeft, true);
    g.setColour (Theme::Colour::inkGhost);
    g.setFont (Theme::Font::micro());
    const auto pinText = item.beatPosition >= 0.0 ? ("Beat " + juce::String (juce::roundToInt (item.beatPosition))) : "Loose";
    g.drawText (pinText, width - 72, 0, 64, height, juce::Justification::centredRight, false);
}

void LyricsPanel::selectedRowsChanged (int lastRowSelected)
{
    selectedIndex = lastRowSelected;
    refreshEditor();
}

void LyricsPanel::refreshList()
{
    itemList.updateContent();
    if (selectedIndex >= 0)
        itemList.selectRow (selectedIndex, false, false);
}

void LyricsPanel::refreshEditor()
{
    suppressCallbacks = true;
    const auto* item = selectedItem();
    const bool has = item != nullptr;
    duplicateButton.setEnabled (has);
    deleteButton.setEnabled (has);
    pinButton.setEnabled (has);
    textEditor.setEnabled (has);
    fontBox.setEnabled (has);
    sizeBox.setEnabled (has);
    colorBox.setEnabled (has);
    beatSlider.setEnabled (has);

    if (! has)
    {
        textEditor.clear();
        fontBox.setSelectedId (1, juce::dontSendNotification);
        sizeBox.setSelectedId (2, juce::dontSendNotification);
        colorBox.setSelectedId (1, juce::dontSendNotification);
        beatSlider.setValue (-1.0, juce::dontSendNotification);
        beatValue.setText ("Loose note", juce::dontSendNotification);
        placementSummary.setText ("No lyric selected. Add a note, then pin it later if it belongs on the timeline.", juce::dontSendNotification);
        suppressCallbacks = false;
        return;
    }

    textEditor.setText (item->text, juce::dontSendNotification);
    fontBox.setSelectedId (item->style.fontName == "Serif" ? 2 : item->style.fontName == "Marker" ? 3 : 1, juce::dontSendNotification);
    sizeBox.setSelectedId (item->style.sizeIndex + 1, juce::dontSendNotification);
    const auto colour = juce::Colour (item->style.colourArgb);
    int colourId = 1;
    if (colour == Theme::Colour::accentWarm) colourId = 2;
    else if (colour == Theme::Signal::audio) colourId = 3;
    else if (colour == Theme::Semantic::strong) colourId = 4;
    colorBox.setSelectedId (colourId, juce::dontSendNotification);
    beatSlider.setValue (item->beatPosition, juce::dontSendNotification);
    beatValue.setText (item->beatPosition >= 0.0 ? ("Beat " + juce::String (juce::roundToInt (item->beatPosition))) : "Loose note", juce::dontSendNotification);
    placementSummary.setText (item->pinnedToTimeline
                                  ? "Pinned for later timeline placement. This item can become a lyric marker or note card."
                                  : "Loose lyric note. It stays in the pad until you pin it to a beat or section.",
                              juce::dontSendNotification);
    suppressCallbacks = false;
}

void LyricsPanel::addItem()
{
    const auto beforeState = lyrics();
    auto& state = lyricsForEdit();
    LyricItem item;
    item.id = state.nextId++;
    item.text = "new lyric fragment";
    state.items.push_back (item);
    selectedIndex = static_cast<int> (state.items.size() - 1);
    commitChanges (beforeState, "Add Lyric Note");
    refreshList();
    refreshEditor();
}

void LyricsPanel::duplicateItem()
{
    const auto* source = selectedItem();
    if (source == nullptr)
        return;

    const auto beforeState = lyrics();
    auto copy = *source;
    auto& state = lyricsForEdit();
    copy.id = state.nextId++;
    copy.text << " copy";
    state.items.push_back (copy);
    selectedIndex = static_cast<int> (state.items.size() - 1);
    commitChanges (beforeState, "Duplicate Lyric Note");
    refreshList();
    refreshEditor();
}

void LyricsPanel::deleteItem()
{
    const auto beforeState = lyrics();
    auto& state = lyricsForEdit();
    if (selectedIndex < 0 || selectedIndex >= static_cast<int> (state.items.size()))
        return;
    state.items.erase (state.items.begin() + selectedIndex);
    selectedIndex = juce::jmin (selectedIndex, static_cast<int> (state.items.size()) - 1);
    commitChanges (beforeState, "Delete Lyric Note");
    refreshList();
    refreshEditor();
}

void LyricsPanel::pinSelectedToCurrentBeat()
{
    if (auto* item = selectedItem())
    {
        const auto beforeState = lyrics();
        item->beatPosition = processorRef.getCurrentSongBeat();
        item->pinnedToTimeline = true;
        commitChanges (beforeState, "Pin Lyric To Beat");
        refreshEditor();
        refreshList();
    }
}

void LyricsPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);
    auto accent = ZoneAStyle::accentForTabId ("lyrics");
    const int headerH = ZoneAStyle::compactHeaderHeight();
    const int cardPad = ZoneAControlStyle::compactGap() + 2;
    ZoneAStyle::drawHeader (g, { 0, 0, getWidth(), headerH }, "LYRICS", accent);
    ZoneAStyle::drawCard (g, itemList.getBounds().expanded (cardPad, headerH + 6), accent);
    ZoneAStyle::drawCard (g, editorViewport.getBounds().expanded (cardPad, headerH), accent);
}

void LyricsPanel::resized()
{
    const int gap = ZoneAControlStyle::compactGap();
    const int labelH = ZoneAControlStyle::sectionHeaderHeight() - 2;
    const int rowH = ZoneAControlStyle::controlHeight();
    const int headerH = ZoneAStyle::compactHeaderHeight();
    const int groupHeaderH = ZoneAStyle::compactGroupHeaderHeight();
    itemList.setRowHeight (ZoneAStyle::compactRowHeight());

    auto area = getLocalBounds().reduced (gap + 4);
    area.removeFromTop (headerH);
    listHeader.setBounds (area.removeFromTop (groupHeaderH));
    auto buttonRow = area.removeFromTop (rowH);
    addItemButton.setBounds (buttonRow.removeFromLeft (54));
    buttonRow.removeFromLeft (gap);
    duplicateButton.setBounds (buttonRow.removeFromLeft (42));
    buttonRow.removeFromLeft (gap);
    deleteButton.setBounds (buttonRow.removeFromLeft (42));
    buttonRow.removeFromLeft (gap);
    pinButton.setBounds (buttonRow.removeFromLeft (42));
    area.removeFromTop (gap);
    itemList.setBounds (area.removeFromTop (126));
    area.removeFromTop (gap + 2);
    editorHeader.setBounds (area.removeFromTop (groupHeaderH));
    editorHint.setBounds (area.removeFromTop (groupHeaderH));
    area.removeFromTop (gap);
    editorViewport.setBounds (area);

    editorContent.setSize (juce::jmax (160, editorViewport.getWidth() - 12), 260);
    auto content = editorContent.getLocalBounds().reduced (4);
    textLabel.setBounds (content.removeFromTop (labelH));
    textEditor.setBounds (content.removeFromTop (92));
    content.removeFromTop (gap + 2);
    auto labels = content.removeFromTop (labelH);
    fontLabel.setBounds (labels.removeFromLeft (60));
    labels.removeFromLeft (gap + 2);
    sizeLabel.setBounds (labels.removeFromLeft (56));
    labels.removeFromLeft (gap + 2);
    colorLabel.setBounds (labels.removeFromLeft (56));
    auto controls = content.removeFromTop (rowH);
    fontBox.setBounds (controls.removeFromLeft (60));
    controls.removeFromLeft (gap + 2);
    sizeBox.setBounds (controls.removeFromLeft (56));
    controls.removeFromLeft (gap + 2);
    colorBox.setBounds (controls.removeFromLeft (56));
    content.removeFromTop (gap + 2);
    beatLabel.setBounds (content.removeFromTop (labelH));
    beatSlider.setBounds (content.removeFromTop (ZoneAControlStyle::controlBarHeight()));
    beatValue.setBounds (content.removeFromTop (labelH));
    content.removeFromTop (gap + 2);
    placementSummary.setBounds (content.removeFromTop (32));
}
void LyricsPanel::refreshFromModel()
{
    selectedIndex = juce::jmin (selectedIndex, static_cast<int> (lyrics().items.size()) - 1);
    refreshList();
    refreshEditor();
    repaint();
}

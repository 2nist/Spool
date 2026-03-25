#include "StructurePanel.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include "BinaryData.h"

#include "../../PluginProcessor.h"
#include "../../dsp/SongManager.h"

#include <array>

namespace
{
void paintRowSelection (juce::Graphics& g, bool selected)
{
    if (selected)
        g.fillAll (Theme::Colour::accent.withAlpha (0.2f));
}

void styleLabel (juce::Label& l)
{
    l.setJustificationType (juce::Justification::centredLeft);
    l.setColour (juce::Label::textColourId, Theme::Colour::inkMid);
    l.setFont (Theme::Font::micro());
}

void styleEditor (juce::TextEditor& e)
{
    e.setColour (juce::TextEditor::backgroundColourId, Theme::Colour::surface2);
    e.setColour (juce::TextEditor::textColourId, Theme::Colour::inkLight);
    e.setColour (juce::TextEditor::outlineColourId, Theme::Colour::surfaceEdge);
}

bool parseChordToken (juce::String token, int& rootMidi, int& chordType)
{
    token = token.trim();
    if (token.isEmpty())
        return false;

    bool minor = token.endsWithIgnoreCase ("m");
    if (minor)
        token = token.dropLastCharacters (1);

    juce::String rootName;
    rootName << juce::String::charToString (token[0]).toUpperCase();
    if (token.length() > 1 && (token[1] == '#' || token[1] == 'b'))
        rootName << juce::String::charToString (token[1]);

    int semitone = -1;
    if (rootName == "C") semitone = 0;
    else if (rootName == "C#" || rootName == "Db") semitone = 1;
    else if (rootName == "D") semitone = 2;
    else if (rootName == "D#" || rootName == "Eb") semitone = 3;
    else if (rootName == "E") semitone = 4;
    else if (rootName == "F") semitone = 5;
    else if (rootName == "F#" || rootName == "Gb") semitone = 6;
    else if (rootName == "G") semitone = 7;
    else if (rootName == "G#" || rootName == "Ab") semitone = 8;
    else if (rootName == "A") semitone = 9;
    else if (rootName == "A#" || rootName == "Bb") semitone = 10;
    else if (rootName == "B") semitone = 11;

    if (semitone < 0)
        return false;

    rootMidi = 60 + semitone;
    chordType = minor ? 1 : 0;
    return true;
}

int rootNameToPitchClass (const juce::String& root)
{
    const auto r = root.trim();
    if (r.equalsIgnoreCase ("C")) return 0;
    if (r.equalsIgnoreCase ("C#") || r.equalsIgnoreCase ("Db")) return 1;
    if (r.equalsIgnoreCase ("D")) return 2;
    if (r.equalsIgnoreCase ("D#") || r.equalsIgnoreCase ("Eb")) return 3;
    if (r.equalsIgnoreCase ("E")) return 4;
    if (r.equalsIgnoreCase ("F")) return 5;
    if (r.equalsIgnoreCase ("F#") || r.equalsIgnoreCase ("Gb")) return 6;
    if (r.equalsIgnoreCase ("G")) return 7;
    if (r.equalsIgnoreCase ("G#") || r.equalsIgnoreCase ("Ab")) return 8;
    if (r.equalsIgnoreCase ("A")) return 9;
    if (r.equalsIgnoreCase ("A#") || r.equalsIgnoreCase ("Bb")) return 10;
    if (r.equalsIgnoreCase ("B")) return 11;
    return 0;
}

juce::String pitchClassToRootName (int pitchClass)
{
    static constexpr std::array<const char*, 12> names = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const auto wrapped = (pitchClass % 12 + 12) % 12;
    return names[(size_t) wrapped];
}

std::array<int, 7> modeIntervals (juce::String mode)
{
    mode = mode.trim().toLowerCase();
    if (mode == "minor")
        return { 0, 2, 3, 5, 7, 8, 10 };
    if (mode == "dorian")
        return { 0, 2, 3, 5, 7, 9, 10 };
    if (mode == "mixolydian")
        return { 0, 2, 4, 5, 7, 9, 10 };

    // Default to major for this thin validation slice.
    return { 0, 2, 4, 5, 7, 9, 11 };
}

bool parseRomanToken (juce::String token, int& degreeOut, int& accidentalOut, juce::String& chordTypeOut)
{
    token = token.trim();
    if (token.isEmpty())
        return false;

    accidentalOut = 0;
    if (token.startsWithChar ('b'))
    {
        accidentalOut = -1;
        token = token.substring (1);
    }
    else if (token.startsWithChar ('#'))
    {
        accidentalOut = 1;
        token = token.substring (1);
    }

    bool diminished = false;
    if (token.endsWithChar (0x00B0) || token.endsWithIgnoreCase ("o"))
    {
        diminished = true;
        token = token.dropLastCharacters (1);
    }

    const auto lower = token.toLowerCase();
    if (lower == "i") degreeOut = 1;
    else if (lower == "ii") degreeOut = 2;
    else if (lower == "iii") degreeOut = 3;
    else if (lower == "iv") degreeOut = 4;
    else if (lower == "v") degreeOut = 5;
    else if (lower == "vi") degreeOut = 6;
    else if (lower == "vii") degreeOut = 7;
    else return false;

    if (diminished)
    {
        chordTypeOut = "dim";
        return true;
    }

    const bool majorQuality = (token == token.toUpperCase());
    chordTypeOut = majorQuality ? "maj" : "min";
    return true;
}

int chordTypeToLegacyInt (const juce::String& type)
{
    const auto t = type.toLowerCase();
    if (t == "min") return 1;
    if (t == "dim") return 2;
    return 0;
}
} // namespace

StructurePanel::StructurePanel (PluginProcessor& p) : processorRef (p)
{
    addAndMakeVisible (listTabs);
    listTabs.addTab ("Patterns", Theme::Colour::surface2, &patternsList, false);
    listTabs.addTab ("Sections", Theme::Colour::surface2, &sectionsList, false);
    listTabs.addTab ("Chords", Theme::Colour::surface2, &chordsList, false);

    // Keep the inner tabs text-only (remove small extra icons).
    // The outer `TabStrip` provides icon-only navigation for the left edge.

    addAndMakeVisible (addPatternButton);
    addAndMakeVisible (addSectionButton);
    addAndMakeVisible (addChordButton);
    addAndMakeVisible (deleteButton);
    addPatternButton.onClick = [this] { addPattern(); };
    addSectionButton.onClick = [this] { addSection(); };
    addChordButton.onClick = [this] { addChord(); };
    deleteButton.onClick = [this] { deleteSelected(); };

    addAndMakeVisible (loadButton);
    addAndMakeVisible (saveButton);
    loadButton.onClick = [this] { loadSong(); };
    saveButton.onClick = [this] { saveSong(); };

    addAndMakeVisible (intentTitle);
    addAndMakeVisible (intentSectionLabel);
    addAndMakeVisible (intentSectionEditor);
    addAndMakeVisible (intentRootLabel);
    addAndMakeVisible (intentRootBox);
    addAndMakeVisible (intentModeLabel);
    addAndMakeVisible (intentModeBox);
    addAndMakeVisible (intentProgLabel);
    addAndMakeVisible (intentProgressionEditor);
    addAndMakeVisible (intentBarsLabel);
    addAndMakeVisible (intentBarsEditor);
    addAndMakeVisible (intentApplyButton);
    addAndMakeVisible (intentApplyToRailsButton);
    addAndMakeVisible (intentFollowToggle);
    addAndMakeVisible (intentSelectedOnlyToggle);
    addAndMakeVisible (advancedToggle);
    addAndMakeVisible (intentFeedbackLabel);

    addAndMakeVisible (quickTitle);
    addAndMakeVisible (progressionLabel);
    addAndMakeVisible (progressionEditor);
    addAndMakeVisible (barsLabel);
    addAndMakeVisible (barsPerChordBox);
    addAndMakeVisible (targetPatternLabel);
    addAndMakeVisible (targetPatternBox);
    addAndMakeVisible (replaceExistingToggle);
    addAndMakeVisible (buildProgressionButton);

    addAndMakeVisible (detailsTitle);

    addAndMakeVisible (patternNameLabel);
    addAndMakeVisible (patternNameEditor);
    addAndMakeVisible (patternLengthLabel);
    addAndMakeVisible (patternLengthEditor);

    addAndMakeVisible (sectionPatternLabel);
    addAndMakeVisible (sectionPatternBox);
    addAndMakeVisible (sectionStartLabel);
    addAndMakeVisible (sectionStartEditor);
    addAndMakeVisible (sectionLengthLabel);
    addAndMakeVisible (sectionLengthEditor);
    addAndMakeVisible (sectionRepeatsLabel);
    addAndMakeVisible (sectionRepeatsEditor);
    addAndMakeVisible (sectionSlotsLabel);
    addAndMakeVisible (sectionSlotsEditor);
    addAndMakeVisible (sectionTransposeLabel);
    addAndMakeVisible (sectionTransposeEditor);

    addAndMakeVisible (chordStartLabel);
    addAndMakeVisible (chordStartEditor);
    addAndMakeVisible (chordDurationLabel);
    addAndMakeVisible (chordDurationEditor);
    addAndMakeVisible (chordRootLabel);
    addAndMakeVisible (chordRootEditor);
    addAndMakeVisible (chordTypeLabel);
    addAndMakeVisible (chordTypeEditor);

    addAndMakeVisible (applyButton);
    applyButton.onClick = [this] { applySelectionChanges(); };

    buildProgressionButton.onClick = [this] { buildProgression(); };
    progressionEditor.setText ("C Am F G", juce::dontSendNotification);

    intentApplyButton.onClick = [this] { applyIntentStructure(); };
    intentApplyToRailsButton.onClick = [this] { applyIntentToRails(); };
    intentSectionEditor.setText ("Verse", juce::dontSendNotification);
    intentProgressionEditor.setText ("I - V - vi - IV", juce::dontSendNotification);
    intentBarsEditor.setText ("8", juce::dontSendNotification);
    styleLabel (intentTitle);
    styleLabel (intentSectionLabel);
    styleLabel (intentRootLabel);
    styleLabel (intentModeLabel);
    styleLabel (intentProgLabel);
    styleLabel (intentBarsLabel);
    styleLabel (intentFeedbackLabel);
    styleEditor (intentSectionEditor);
    styleEditor (intentProgressionEditor);
    styleEditor (intentBarsEditor);
    for (const auto root : { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" })
        intentRootBox.addItem (root, intentRootBox.getNumItems() + 1);
    for (const auto mode : { "Major", "Minor", "Dorian", "Mixolydian", "Lydian", "Phrygian", "Locrian", "Custom" })
        intentModeBox.addItem (mode, intentModeBox.getNumItems() + 1);
    setIntentKeyMode (processorRef.getAppState().song.keyRoot, processorRef.getAppState().song.keyScale);
    advancedToggle.setToggleState (false, juce::dontSendNotification);
    advancedToggle.onClick = [this]
    {
        showAdvanced = advancedToggle.getToggleState();
        updateLegacyVisibility();
        resized();
    };
    intentFollowToggle.setToggleState (processorRef.isStructureFollowForTracksEnabled(), juce::dontSendNotification);
    intentFollowToggle.onClick = [this]
    {
        const bool enabled = intentFollowToggle.getToggleState();
        processorRef.setStructureFollowForTracks (enabled);
        if (onStructureFollowModeChanged)
            onStructureFollowModeChanged (enabled);
    };
    intentSelectedOnlyToggle.setToggleState (false, juce::dontSendNotification);
    intentFeedbackLabel.setText ("Enter Roman progression and apply.", juce::dontSendNotification);

    barsPerChordBox.addItem ("1", 1);
    barsPerChordBox.addItem ("2", 2);
    barsPerChordBox.addItem ("4", 4);
    barsPerChordBox.setSelectedId (1, juce::dontSendNotification);
    replaceExistingToggle.setToggleState (true, juce::dontSendNotification);

    styleLabel (quickTitle);
    styleLabel (progressionLabel);
    styleLabel (barsLabel);
    styleLabel (targetPatternLabel);

    styleLabel (patternNameLabel);
    styleLabel (patternLengthLabel);
    styleLabel (sectionPatternLabel);
    styleLabel (sectionStartLabel);
    styleLabel (sectionLengthLabel);
    styleLabel (sectionRepeatsLabel);
    styleLabel (sectionSlotsLabel);
    styleLabel (sectionTransposeLabel);
    styleLabel (chordStartLabel);
    styleLabel (chordDurationLabel);
    styleLabel (chordRootLabel);
    styleLabel (chordTypeLabel);

    detailsTitle.setFont (Theme::Font::label());
    detailsTitle.setColour (juce::Label::textColourId, Theme::Colour::inkMid);
    detailsTitle.setJustificationType (juce::Justification::centredLeft);

    styleEditor (progressionEditor);
    styleEditor (patternNameEditor);
    styleEditor (patternLengthEditor);
    styleEditor (sectionStartEditor);
    styleEditor (sectionLengthEditor);
    styleEditor (sectionRepeatsEditor);
    styleEditor (sectionSlotsEditor);
    styleEditor (sectionTransposeEditor);
    styleEditor (chordStartEditor);
    styleEditor (chordDurationEditor);
    styleEditor (chordRootEditor);
    styleEditor (chordTypeEditor);

    clearSelection();
    refreshLists();
    updateLegacyVisibility();
}

void StructurePanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    if (! showAdvanced)
        return;

    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.45f));
    g.drawRect (listTabs.getBounds().expanded (1, 1));

    const auto rightArea = detailsTitle.getBounds().withTop (quickTitle.getY() - 4).expanded (4, 4);
    g.drawRect (rightArea);
}

void StructurePanel::resized()
{
    auto bounds = getLocalBounds().reduced (8);
    auto intentArea = bounds.removeFromTop (118);
    intentTitle.setBounds (intentArea.removeFromTop (16));
    intentArea.removeFromTop (2);

    auto row1 = intentArea.removeFromTop (24);
    intentSectionLabel.setBounds (row1.removeFromLeft (48));
    intentSectionEditor.setBounds (row1.removeFromLeft (juce::jmax (80, row1.getWidth() / 3)));
    row1.removeFromLeft (6);
    intentRootLabel.setBounds (row1.removeFromLeft (34));
    intentRootBox.setBounds (row1.removeFromLeft (64));
    row1.removeFromLeft (6);
    intentModeLabel.setBounds (row1.removeFromLeft (38));
    intentModeBox.setBounds (row1.removeFromLeft (juce::jmax (96, row1.getWidth() - 110)));
    row1.removeFromLeft (6);
    advancedToggle.setBounds (row1);

    intentArea.removeFromTop (3);
    auto row2 = intentArea.removeFromTop (24);
    intentProgLabel.setBounds (row2.removeFromLeft (40));
    intentProgressionEditor.setBounds (row2.removeFromLeft (juce::jmax (100, row2.getWidth() - 180)));
    row2.removeFromLeft (6);
    intentBarsLabel.setBounds (row2.removeFromLeft (28));
    intentBarsEditor.setBounds (row2.removeFromLeft (40));
    row2.removeFromLeft (6);
    const int actionW = juce::jmax (96, (row2.getWidth() - 6) / 2);
    intentApplyButton.setBounds (row2.removeFromLeft (actionW));
    row2.removeFromLeft (6);
    intentApplyToRailsButton.setBounds (row2.removeFromLeft (actionW));

    intentArea.removeFromTop (3);
    auto row3 = intentArea.removeFromTop (20);
    intentFollowToggle.setBounds (row3.removeFromLeft (180));
    row3.removeFromLeft (10);
    intentSelectedOnlyToggle.setBounds (row3);

    intentArea.removeFromTop (2);
    intentFeedbackLabel.setBounds (intentArea.removeFromTop (18));

    bounds.removeFromTop (6);

    if (! showAdvanced)
        return;

    auto topRow = bounds.removeFromTop (28);
    loadButton.setBounds (topRow.removeFromLeft (64));
    saveButton.setBounds (topRow.removeFromRight (64));

    auto actionRow = topRow;
    addPatternButton.setBounds (actionRow.removeFromLeft (94));
    addSectionButton.setBounds (actionRow.removeFromLeft (94));
    addChordButton.setBounds (actionRow.removeFromLeft (86));
    deleteButton.setBounds (actionRow);

    bounds.removeFromTop (8);

    auto left = bounds.removeFromLeft (juce::roundToInt (bounds.getWidth() * 0.4f));
    bounds.removeFromLeft (8);
    auto right = bounds;

    listTabs.setBounds (left);

    auto quickArea = right.removeFromTop (104);
    quickTitle.setBounds (quickArea.removeFromTop (20));

    auto quickRow1 = quickArea.removeFromTop (24);
    progressionLabel.setBounds (quickRow1.removeFromLeft (74));
    progressionEditor.setBounds (quickRow1);

    quickArea.removeFromTop (4);

    auto quickRow2 = quickArea.removeFromTop (24);
    barsLabel.setBounds (quickRow2.removeFromLeft (74));
    barsPerChordBox.setBounds (quickRow2.removeFromLeft (64));
    quickRow2.removeFromLeft (8);
    targetPatternLabel.setBounds (quickRow2.removeFromLeft (56));
    targetPatternBox.setBounds (quickRow2.removeFromLeft (150));
    replaceExistingToggle.setBounds (quickRow2);

    quickArea.removeFromTop (4);
    buildProgressionButton.setBounds (quickArea.removeFromTop (24).withTrimmedLeft (74));

    right.removeFromTop (8);
    auto detailsArea = right;

    detailsTitle.setBounds (detailsArea.removeFromTop (20));
    detailsArea.removeFromTop (4);

    auto form = detailsArea;
    form.removeFromBottom (28);

    const int rowH = 22;
    const int rowGap = 3;

    auto layoutRow = [&] (juce::Label& label, juce::Component& field)
    {
        auto row = form.removeFromTop (rowH);
        label.setBounds (row.removeFromLeft (84));
        field.setBounds (row);
        form.removeFromTop (rowGap);
    };

    layoutRow (patternNameLabel, patternNameEditor);
    layoutRow (patternLengthLabel, patternLengthEditor);

    layoutRow (sectionPatternLabel, sectionPatternBox);
    layoutRow (sectionStartLabel, sectionStartEditor);
    layoutRow (sectionLengthLabel, sectionLengthEditor);
    layoutRow (sectionRepeatsLabel, sectionRepeatsEditor);
    layoutRow (sectionSlotsLabel, sectionSlotsEditor);
    layoutRow (sectionTransposeLabel, sectionTransposeEditor);

    layoutRow (chordStartLabel, chordStartEditor);
    layoutRow (chordDurationLabel, chordDurationEditor);
    layoutRow (chordRootLabel, chordRootEditor);
    layoutRow (chordTypeLabel, chordTypeEditor);

    applyButton.setBounds (detailsArea.removeFromBottom (24).withTrimmedLeft (90));
}

int StructurePanel::PatternsModel::getNumRows()
{
    return owner.processorRef.getSongManager().getPatterns().size();
}

void StructurePanel::PatternsModel::paintListBoxItem (int rowNumber,
                                                      juce::Graphics& g,
                                                      int width,
                                                      int height,
                                                      bool selected)
{
    const auto& patterns = owner.processorRef.getSongManager().getPatterns();
    if (rowNumber < 0 || rowNumber >= patterns.size())
        return;

    paintRowSelection (g, selected);

    const auto& pat = patterns.getReference (rowNumber);
    g.setColour (Theme::Colour::inkLight);
    g.drawText (pat.name, 4, 0, width - 8, height, juce::Justification::centredLeft);

    g.setColour (Theme::Colour::inkGhost);
    g.drawText (juce::String (pat.baseSteps.size()) + " steps, " + juce::String (pat.lengthBeats, 1) + "b",
                width - 120,
                0,
                116,
                height,
                juce::Justification::centredRight);
}

void StructurePanel::PatternsModel::selectedRowsChanged (int lastRowSelected)
{
    owner.setSelection (SelectionType::pattern, lastRowSelected);
}

int StructurePanel::SectionsModel::getNumRows()
{
    return static_cast<int> (owner.processorRef.getAppState().structure.sections.size());
}

void StructurePanel::SectionsModel::paintListBoxItem (int rowNumber,
                                                      juce::Graphics& g,
                                                      int width,
                                                      int height,
                                                      bool selected)
{
    const auto& sections = owner.processorRef.getAppState().structure.sections;
    if (rowNumber < 0 || rowNumber >= static_cast<int> (sections.size()))
        return;

    paintRowSelection (g, selected);

    const auto& sec = sections[(size_t) rowNumber];
    g.setColour (Theme::Colour::inkLight);
    g.drawText (sec.name.isNotEmpty() ? sec.name : ("Section " + juce::String (rowNumber + 1)),
                4,
                0,
                width - 8,
                height,
                juce::Justification::centredLeft);

    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("Bar " + juce::String (sec.startBar + 1) + "  (" + juce::String (sec.lengthBars) + " bars)",
                width - 160,
                0,
                156,
                height,
                juce::Justification::centredRight);
}

void StructurePanel::SectionsModel::selectedRowsChanged (int lastRowSelected)
{
    owner.setSelection (SelectionType::section, lastRowSelected);
}

int StructurePanel::ChordsModel::getNumRows()
{
    return static_cast<int> (owner.chordRowMap.size());
}

void StructurePanel::ChordsModel::paintListBoxItem (int rowNumber,
                                                    juce::Graphics& g,
                                                    int width,
                                                    int height,
                                                    bool selected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int> (owner.chordRowMap.size()))
        return;

    paintRowSelection (g, selected);

    const auto [sectionIndex, chordIndex] = owner.chordRowMap[(size_t) rowNumber];
    const auto& section = owner.processorRef.getAppState().structure.sections[(size_t) sectionIndex];
    const auto& chord = section.progression[(size_t) chordIndex];
    g.setColour (Theme::Colour::inkLight);
    g.drawText (chord.root + chord.type,
                4,
                0,
                width - 8,
                height,
                juce::Justification::centredLeft);

    g.setColour (Theme::Colour::inkGhost);
    g.drawText (section.name.isNotEmpty() ? section.name : ("Section " + juce::String (sectionIndex + 1)),
                width - 100,
                0,
                96,
                height,
                juce::Justification::centredRight);
}

void StructurePanel::ChordsModel::selectedRowsChanged (int lastRowSelected)
{
    owner.setSelection (SelectionType::chord, lastRowSelected);
}

void StructurePanel::setSelection (SelectionType type, int index)
{
    if (suppressSelectionCallbacks)
        return;

    if (index < 0)
    {
        clearSelection();
        return;
    }

    selectionType = type;
    selectedIndex = index;

    suppressSelectionCallbacks = true;
    if (type != SelectionType::pattern)
        patternsList.deselectAllRows();
    if (type != SelectionType::section)
        sectionsList.deselectAllRows();
    if (type != SelectionType::chord)
        chordsList.deselectAllRows();
    suppressSelectionCallbacks = false;

    populateEditorsForSelection();
}

void StructurePanel::clearSelection()
{
    selectionType = SelectionType::none;
    selectedIndex = -1;

    detailsTitle.setText ("Details", juce::dontSendNotification);
    patternNameEditor.clear();
    patternLengthEditor.clear();
    sectionPatternBox.clear();
    sectionStartEditor.clear();
    sectionLengthEditor.clear();
    sectionRepeatsEditor.clear();
    sectionSlotsEditor.clear();
    sectionTransposeEditor.clear();
    chordStartEditor.clear();
    chordDurationEditor.clear();
    chordRootEditor.clear();
    chordTypeEditor.clear();

    showPatternEditors (false);
    showSectionEditors (false);
    showChordEditors (false);
    applyButton.setVisible (false);
}

void StructurePanel::showPatternEditors (bool show)
{
    patternNameLabel.setVisible (show);
    patternNameEditor.setVisible (show);
    patternLengthLabel.setVisible (show);
    patternLengthEditor.setVisible (show);
}

void StructurePanel::showSectionEditors (bool show)
{
    sectionPatternLabel.setVisible (show);
    sectionPatternBox.setVisible (show);
    sectionStartLabel.setVisible (show);
    sectionStartEditor.setVisible (show);
    sectionLengthLabel.setVisible (show);
    sectionLengthEditor.setVisible (show);
    sectionRepeatsLabel.setVisible (show);
    sectionRepeatsEditor.setVisible (show);
    sectionSlotsLabel.setVisible (show);
    sectionSlotsEditor.setVisible (show);
    sectionTransposeLabel.setVisible (show);
    sectionTransposeEditor.setVisible (show);
}

void StructurePanel::showChordEditors (bool show)
{
    chordStartLabel.setVisible (show);
    chordStartEditor.setVisible (show);
    chordDurationLabel.setVisible (show);
    chordDurationEditor.setVisible (show);
    chordRootLabel.setVisible (show);
    chordRootEditor.setVisible (show);
    chordTypeLabel.setVisible (show);
    chordTypeEditor.setVisible (show);
}

void StructurePanel::populateEditorsForSelection()
{
    showPatternEditors (false);
    showSectionEditors (false);
    showChordEditors (false);
    applyButton.setVisible (showAdvanced && selectionType != SelectionType::none && selectedIndex >= 0);

    if (selectionType == SelectionType::pattern)
    {
        const auto& patterns = processorRef.getSongManager().getPatterns();
        if (selectedIndex < 0 || selectedIndex >= patterns.size())
            return;

        const auto& p = patterns.getReference (selectedIndex);
        detailsTitle.setText ("Pattern Details", juce::dontSendNotification);
        patternNameEditor.setText (p.name, juce::dontSendNotification);
        patternLengthEditor.setText (juce::String (p.lengthBeats, 2), juce::dontSendNotification);
        showPatternEditors (true);
        return;
    }

    if (selectionType == SelectionType::section)
    {
        const auto& sections = processorRef.getAppState().structure.sections;
        if (selectedIndex < 0 || selectedIndex >= static_cast<int> (sections.size()))
            return;

        const auto& s = sections[(size_t) selectedIndex];
        detailsTitle.setText ("Section Details", juce::dontSendNotification);

        suppressSelectionCallbacks = true;
        sectionPatternBox.clear (juce::dontSendNotification);
        for (int i = 0; i < static_cast<int> (sections.size()); ++i)
        {
            const auto& section = sections[(size_t) i];
            const auto name = section.name.isNotEmpty() ? section.name : ("Section " + juce::String (i + 1));
            sectionPatternBox.addItem (name, i + 1);
        }
        sectionPatternBox.setSelectedId (selectedIndex + 1, juce::dontSendNotification);
        suppressSelectionCallbacks = false;

        sectionStartEditor.setText (juce::String (s.startBar), juce::dontSendNotification);
        sectionLengthEditor.setText (juce::String (s.lengthBars), juce::dontSendNotification);
        sectionRepeatsEditor.setText (juce::String ((int) s.progression.size()), juce::dontSendNotification);
        sectionSlotsEditor.setText (s.name, juce::dontSendNotification);
        sectionTransposeEditor.setText ("0", juce::dontSendNotification);
        showSectionEditors (true);
        return;
    }

    if (selectionType == SelectionType::chord)
    {
        if (selectedIndex < 0 || selectedIndex >= static_cast<int> (chordRowMap.size()))
            return;

        const auto [sectionIndex, chordIndex] = chordRowMap[(size_t) selectedIndex];
        const auto& c = processorRef.getAppState().structure.sections[(size_t) sectionIndex].progression[(size_t) chordIndex];
        detailsTitle.setText ("Chord Details", juce::dontSendNotification);
        chordStartEditor.setText (juce::String (sectionIndex), juce::dontSendNotification);
        chordDurationEditor.setText (juce::String (chordIndex), juce::dontSendNotification);
        chordRootEditor.setText (c.root, juce::dontSendNotification);
        chordTypeEditor.setText (c.type, juce::dontSendNotification);
        showChordEditors (true);
    }
}

void StructurePanel::applySelectionChanges()
{
    if (selectedIndex < 0)
        return;

    juce::ScopedWriteLock structureWrite (processorRef.getStructureLock());

    if (selectionType == SelectionType::pattern)
    {
        DBG ("Pattern writes are disabled in AppState migration phase.");
        return;
    }

    if (selectionType == SelectionType::section)
    {
        auto& sections = processorRef.getAppState().structure.sections;
        if (selectedIndex >= static_cast<int> (sections.size()))
            return;

        auto& updated = sections[(size_t) selectedIndex];
        updated.startBar = juce::jmax (0, sectionStartEditor.getText().getIntValue());
        updated.lengthBars = juce::jmax (1, sectionLengthEditor.getText().getIntValue());
        updated.name = sectionSlotsEditor.getText().trim();
        processorRef.getStructureEngine().rebuild();
        refreshLists();
        if (onStructureApplied)
            onStructureApplied();
        return;
    }

    if (selectionType == SelectionType::chord)
    {
        if (selectedIndex >= static_cast<int> (chordRowMap.size()))
            return;

        const auto [sectionIndex, chordIndex] = chordRowMap[(size_t) selectedIndex];
        auto& chord = processorRef.getAppState().structure.sections[(size_t) sectionIndex].progression[(size_t) chordIndex];
        chord.root = chordRootEditor.getText().trim();
        chord.type = chordTypeEditor.getText().trim();
        refreshLists();
        if (onStructureApplied)
            onStructureApplied();
    }
}

void StructurePanel::addPattern()
{
    addSection();
}

void StructurePanel::addSection()
{
    juce::ScopedWriteLock structureWrite (processorRef.getStructureLock());

    auto& structure = processorRef.getAppState().structure;
    Section section;
    section.name = "Section " + juce::String (structure.sections.size() + 1);
    section.startBar = structure.totalBars;
    section.lengthBars = 4;
    section.progression.push_back ({ "C", "maj" });
    structure.sections.push_back (section);
    processorRef.getStructureEngine().rebuild();
    refreshLists();
    if (onStructureApplied)
        onStructureApplied();
}

void StructurePanel::addChord()
{
    juce::ScopedWriteLock structureWrite (processorRef.getStructureLock());

    auto& structure = processorRef.getAppState().structure;
    if (structure.sections.empty())
    {
        Section section;
        section.name = "Section 1";
        section.startBar = 0;
        section.lengthBars = 4;
        section.progression.push_back ({ "C", "maj" });
        structure.sections.push_back (section);
        processorRef.getStructureEngine().rebuild();
        refreshLists();
        if (onStructureApplied)
            onStructureApplied();
        return;
    }

    const int sectionIndex = juce::jlimit (0,
                                           static_cast<int> (structure.sections.size() - 1),
                                           sectionsList.getSelectedRow() >= 0 ? sectionsList.getSelectedRow() : 0);
    structure.sections[(size_t) sectionIndex].progression.push_back ({ "C", "maj" });
    refreshLists();
    if (onStructureApplied)
        onStructureApplied();
}

void StructurePanel::deleteSelected()
{
    juce::ScopedWriteLock structureWrite (processorRef.getStructureLock());

    if (selectionType == SelectionType::pattern && selectedIndex >= 0)
        DBG ("Pattern delete is disabled in AppState migration phase.");
    else if (selectionType == SelectionType::section && selectedIndex >= 0)
    {
        auto& sections = processorRef.getAppState().structure.sections;
        if (selectedIndex < static_cast<int> (sections.size()))
            sections.erase (sections.begin() + selectedIndex);
        processorRef.getStructureEngine().rebuild();
    }
    else if (selectionType == SelectionType::chord && selectedIndex >= 0)
    {
        if (selectedIndex < static_cast<int> (chordRowMap.size()))
        {
            const auto [sectionIndex, chordIndex] = chordRowMap[(size_t) selectedIndex];
            auto& progression = processorRef.getAppState().structure.sections[(size_t) sectionIndex].progression;
            if (chordIndex < static_cast<int> (progression.size()))
                progression.erase (progression.begin() + chordIndex);
        }
    }

    suppressSelectionCallbacks = true;
    patternsList.deselectAllRows();
    sectionsList.deselectAllRows();
    chordsList.deselectAllRows();
    suppressSelectionCallbacks = false;

    clearSelection();
    refreshLists();
    if (onStructureApplied)
        onStructureApplied();
}

void StructurePanel::buildProgression()
{
    juce::ScopedWriteLock structureWrite (processorRef.getStructureLock());

    const auto barsPerChord = juce::jmax (1, barsPerChordBox.getSelectedId());

    auto tokenArray = juce::StringArray::fromTokens (progressionEditor.getText(), " ,|", "");
    tokenArray.trim();
    tokenArray.removeEmptyStrings();
    if (tokenArray.isEmpty())
        return;

    auto& structure = processorRef.getAppState().structure;
    if (replaceExistingToggle.getToggleState())
        structure.sections.clear();

    Section section;
    section.name = "Progression";
    section.startBar = structure.totalBars;
    section.lengthBars = tokenArray.size() * barsPerChord;

    for (const auto& token : tokenArray)
    {
        int root = 60;
        int type = 0;
        if (! parseChordToken (token, root, type))
            continue;

        const auto rootName = juce::MidiMessage::getMidiNoteName (root, true, false, 4).upToFirstOccurrenceOf ("4", false, false);
        section.progression.push_back ({ rootName, type == 1 ? "min" : "maj" });
    }

    if (! section.progression.empty())
        structure.sections.push_back (section);

    processorRef.getStructureEngine().rebuild();

    refreshLists();
    if (onStructureApplied)
        onStructureApplied();
}

void StructurePanel::applyIntentStructure()
{
    juce::ScopedWriteLock structureWrite (processorRef.getStructureLock());

    const auto sectionName = intentSectionEditor.getText().trim().isNotEmpty()
                               ? intentSectionEditor.getText().trim()
                               : juce::String ("Section");
    const auto rootName = intentRootBox.getText().trim().isNotEmpty()
                            ? intentRootBox.getText().trim()
                            : juce::String ("C");
    const auto modeName = intentModeBox.getText().trim().isNotEmpty()
                            ? intentModeBox.getText().trim()
                            : juce::String ("Major");
    const int sectionBars = juce::jmax (1, intentBarsEditor.getText().getIntValue());

    juce::StringArray tokens = juce::StringArray::fromTokens (intentProgressionEditor.getText(), " -|,", "");
    tokens.trim();
    tokens.removeEmptyStrings();
    if (tokens.isEmpty())
    {
        intentFeedbackLabel.setText ("No progression entered.", juce::dontSendNotification);
        return;
    }

    const auto tonicPc = rootNameToPitchClass (rootName);
    const auto intervals = modeIntervals (modeName);

    Section section;
    section.name = sectionName;
    section.startBar = 0;
    section.lengthBars = sectionBars;

    juce::StringArray parsedSymbols;
    int ignored = 0;
    for (const auto& token : tokens)
    {
        int degree = 1;
        int accidental = 0;
        juce::String chordType;
        if (! parseRomanToken (token, degree, accidental, chordType))
        {
            ++ignored;
            continue;
        }

        const int pc = tonicPc + intervals[(size_t) juce::jlimit (0, 6, degree - 1)] + accidental;
        const auto root = pitchClassToRootName (pc);
        section.progression.push_back ({ root, chordType });
        parsedSymbols.add (token.trim());
    }

    if (section.progression.empty())
    {
        intentFeedbackLabel.setText ("No valid Roman tokens parsed.", juce::dontSendNotification);
        return;
    }

    auto& appState = processorRef.getAppState();
    appState.song.keyRoot = rootName;
    appState.song.keyScale = modeName;

    auto& structure = appState.structure;
    structure.sections.clear();
    structure.sections.push_back (section);
    processorRef.getStructureEngine().rebuild();

    // Thin-slice bridge: mirror intent structure into legacy SongManager timeline
    // so existing track sequencer playback has a section/pattern/chord source.
    auto& songMgr = processorRef.getSongManager();
    while (songMgr.getChords().size() > 0)
        songMgr.removeChord (songMgr.getChords().size() - 1);
    while (songMgr.getSections().size() > 0)
        songMgr.removeSection (songMgr.getSections().size() - 1);
    while (songMgr.getPatterns().size() > 0)
        songMgr.removePattern (songMgr.getPatterns().size() - 1);

    SongPattern pattern;
    pattern.name = section.name;
    pattern.lengthBeats = static_cast<double> (section.lengthBars * 4);
    const int total16thSteps = juce::jmax (1, static_cast<int> (pattern.lengthBeats * 4.0));
    for (int step = 0; step < total16thSteps; step += 4) // quarter-note pulse
        pattern.baseSteps.add (step);
    const int patternId = songMgr.addPattern (pattern);

    SongSection songSection;
    songSection.patternId = patternId;
    songSection.slotMask = 0xFF; // all slots; inactive slots are ignored downstream
    songSection.startBeat = 0.0;
    songSection.lengthBeats = pattern.lengthBeats;
    songSection.repeats = 1;
    songSection.transpose = 0;
    songSection.transition = 0.0;
    songMgr.addSection (songSection);

    const int chordCount = juce::jmax (1, static_cast<int> (section.progression.size()));
    const double chordDurationBeats = pattern.lengthBeats / static_cast<double> (chordCount);
    for (int i = 0; i < chordCount; ++i)
    {
        ChordEvent chordEvent;
        chordEvent.startBeat = chordDurationBeats * static_cast<double> (i);
        chordEvent.durationBeats = (i == chordCount - 1)
                                     ? (pattern.lengthBeats - chordEvent.startBeat)
                                     : chordDurationBeats;
        const auto& chord = section.progression[(size_t) i];
        chordEvent.root = 60 + rootNameToPitchClass (chord.root);
        chordEvent.type = chordTypeToLegacyInt (chord.type);
        songMgr.addChord (chordEvent);
    }

    DBG ("Structure intent applied: section=" << section.name
         << " root=" << rootName
         << " mode=" << modeName
         << " bars=" << sectionBars
         << " progression=" << parsedSymbols.joinIntoString (" "));

    intentFeedbackLabel.setText ("Parsed " + juce::String (parsedSymbols.size())
                                 + " chord(s), ignored " + juce::String (ignored) + ".",
                                 juce::dontSendNotification);

    applyIntentToRails();

    refreshLists();
    if (onStructureApplied)
        onStructureApplied();
}

void StructurePanel::setIntentKeyMode (const juce::String& keyRoot, const juce::String& mode)
{
    const auto normalizedRoot = keyRoot.trim().isNotEmpty() ? keyRoot.trim().toUpperCase() : juce::String ("C");
    const auto normalizedMode = mode.trim().isNotEmpty() ? mode.trim() : juce::String ("Major");

    for (int i = 0; i < intentRootBox.getNumItems(); ++i)
    {
        const auto item = intentRootBox.getItemText (i).toUpperCase();
        if (item == normalizedRoot)
        {
            intentRootBox.setSelectedItemIndex (i, juce::dontSendNotification);
            break;
        }
    }

    const auto desiredModeLower = normalizedMode.toLowerCase();
    for (int i = 0; i < intentModeBox.getNumItems(); ++i)
    {
        const auto itemLower = intentModeBox.getItemText (i).toLowerCase();
        if (itemLower == desiredModeLower)
        {
            intentModeBox.setSelectedItemIndex (i, juce::dontSendNotification);
            return;
        }
    }

    intentModeBox.setSelectedId (1, juce::dontSendNotification);
}

void StructurePanel::applyIntentToRails()
{
    juce::Array<int> targetSlots;
    const bool selectedOnly = intentSelectedOnlyToggle.getToggleState();

    if (selectedOnly)
    {
        int focused = processorRef.getFocusedSlot();
        if (focused < 0 || focused >= 8)
            focused = 0;
        targetSlots.add (focused);
        processorRef.setSlotActive (focused, true);
        processorRef.getAudioGraph().setSlotSynthType (focused, 0); // sine fallback
    }
    else
    {
        targetSlots.add (0); // guarantee one audible fallback lane
        processorRef.setSlotActive (0, true);
        processorRef.getAudioGraph().setSlotSynthType (0, 0); // sine fallback

        for (int slot = 0; slot < 8; ++slot)
        {
            if (processorRef.getAudioGraph().isSlotActive (slot))
                targetSlots.addIfNotAlreadyThere (slot);
        }
    }

    for (const auto slot : targetSlots)
    {
        processorRef.setStepCount (slot, 16);
        for (int s = 0; s < 64; ++s)
            processorRef.setStepActive (slot, s, false);
        for (const auto step : { 0, 4, 8, 12 })
            processorRef.setStepActive (slot, step, true);
    }

    processorRef.setStructureFollowForTracks (true);
    intentFollowToggle.setToggleState (true, juce::dontSendNotification);
    if (onStructureFollowModeChanged)
        onStructureFollowModeChanged (true);
    if (onRailsSeeded)
        onRailsSeeded (targetSlots);

    juce::StringArray slotNames;
    for (const auto slot : targetSlots)
        slotNames.add (juce::String (slot + 1));

    const auto modeLabel = selectedOnly ? "Selected lane" : "All active lanes";
    intentFeedbackLabel.setText ("Seeded rails on slot(s): " + slotNames.joinIntoString (", ")
                                 + " · " + modeLabel + " · Structure follow ON",
                                 juce::dontSendNotification);
}

void StructurePanel::refreshPatternChoices()
{
    const auto& patterns = processorRef.getSongManager().getPatterns();

    const int oldSectionId = sectionPatternBox.getSelectedId();
    const int oldTargetId = targetPatternBox.getSelectedId();

    sectionPatternBox.clear (juce::dontSendNotification);
    targetPatternBox.clear (juce::dontSendNotification);

    for (int i = 0; i < patterns.size(); ++i)
    {
        const auto item = juce::String (i) + ": " + patterns.getReference (i).name;
        sectionPatternBox.addItem (item, i + 1);
        targetPatternBox.addItem (item, i + 1);
    }

    if (patterns.isEmpty())
        return;

    sectionPatternBox.setSelectedId (juce::jlimit (1, patterns.size(), juce::jmax (1, oldSectionId)), juce::dontSendNotification);
    targetPatternBox.setSelectedId (juce::jlimit (1, patterns.size(), juce::jmax (1, oldTargetId)), juce::dontSendNotification);
}

void StructurePanel::rebuildChordRowMap()
{
    chordRowMap.clear();
    const auto& sections = processorRef.getAppState().structure.sections;
    for (int s = 0; s < static_cast<int> (sections.size()); ++s)
    {
        const auto& sec = sections[(size_t) s];
        for (int c = 0; c < static_cast<int> (sec.progression.size()); ++c)
            chordRowMap.emplace_back (s, c);
    }
}

void StructurePanel::loadSong()
{
    DBG ("Structure load via AppState binding not implemented in this phase.");
}

void StructurePanel::saveSong()
{
    DBG ("Structure save via AppState binding not implemented in this phase.");
}

void StructurePanel::updateLegacyVisibility()
{
    listTabs.setVisible (showAdvanced);
    addPatternButton.setVisible (showAdvanced);
    addSectionButton.setVisible (showAdvanced);
    addChordButton.setVisible (showAdvanced);
    deleteButton.setVisible (showAdvanced);
    loadButton.setVisible (showAdvanced);
    saveButton.setVisible (showAdvanced);

    quickTitle.setVisible (showAdvanced);
    progressionLabel.setVisible (showAdvanced);
    progressionEditor.setVisible (showAdvanced);
    barsLabel.setVisible (showAdvanced);
    barsPerChordBox.setVisible (showAdvanced);
    targetPatternLabel.setVisible (showAdvanced);
    targetPatternBox.setVisible (showAdvanced);
    replaceExistingToggle.setVisible (showAdvanced);
    buildProgressionButton.setVisible (showAdvanced);

    detailsTitle.setVisible (showAdvanced);
    patternNameLabel.setVisible (showAdvanced);
    patternNameEditor.setVisible (showAdvanced);
    patternLengthLabel.setVisible (showAdvanced);
    patternLengthEditor.setVisible (showAdvanced);

    sectionPatternLabel.setVisible (showAdvanced);
    sectionPatternBox.setVisible (showAdvanced);
    sectionStartLabel.setVisible (showAdvanced);
    sectionStartEditor.setVisible (showAdvanced);
    sectionLengthLabel.setVisible (showAdvanced);
    sectionLengthEditor.setVisible (showAdvanced);
    sectionRepeatsLabel.setVisible (showAdvanced);
    sectionRepeatsEditor.setVisible (showAdvanced);
    sectionSlotsLabel.setVisible (showAdvanced);
    sectionSlotsEditor.setVisible (showAdvanced);
    sectionTransposeLabel.setVisible (showAdvanced);
    sectionTransposeEditor.setVisible (showAdvanced);

    chordStartLabel.setVisible (showAdvanced);
    chordStartEditor.setVisible (showAdvanced);
    chordDurationLabel.setVisible (showAdvanced);
    chordDurationEditor.setVisible (showAdvanced);
    chordRootLabel.setVisible (showAdvanced);
    chordRootEditor.setVisible (showAdvanced);
    chordTypeLabel.setVisible (showAdvanced);
    chordTypeEditor.setVisible (showAdvanced);
    applyButton.setVisible (showAdvanced && selectionType != SelectionType::none && selectedIndex >= 0);
}

void StructurePanel::refreshLists()
{
    rebuildChordRowMap();
    patternsList.updateContent();
    sectionsList.updateContent();
    chordsList.updateContent();
    refreshPatternChoices();

    patternsList.repaint();
    sectionsList.repaint();
    chordsList.repaint();

    populateEditorsForSelection();
    updateLegacyVisibility();
}

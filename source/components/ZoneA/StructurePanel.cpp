#include "StructurePanel.h"
#include "../../PluginProcessor.h"
#include "../ZoneD/ClipData.h"
#include "ZoneAControlStyle.h"

namespace
{
constexpr const char* kRoots[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
constexpr const char* kModes[] = { "Major", "Minor", "Dorian", "Mixolydian", "Lydian", "Phrygian", "Locrian" };
constexpr const char* kDegrees[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
constexpr const char* kTypes[] = { "maj", "min", "maj7", "min7", "7", "sus4", "dim" };
constexpr const char* kTransitions[] = { "None", "Pickup", "Cadence", "Lift", "Drop", "Fill Launch", "Soft Resolve", "Dominant Push" };
enum Cmd { dupCell = 1, clearCell, splitCell, diatonicSub, secondaryDom, borrowedChord, extendChord, rootBase = 100, typeBase = 200 };
void styleTitle (juce::Label& l) { l.setFont (Theme::Font::micro()); l.setColour (juce::Label::textColourId, Theme::Zone::a); l.setJustificationType (juce::Justification::centredLeft); }
void styleHint (juce::Label& l) { l.setFont (Theme::Font::micro()); l.setColour (juce::Label::textColourId, Theme::Colour::inkMid); l.setJustificationType (juce::Justification::centredLeft); }
void styleSummary (juce::Label& l) { l.setFont (Theme::Font::label()); l.setColour (juce::Label::textColourId, Theme::Colour::inkLight); }
void styleCombo (juce::ComboBox& c) { c.setColour (juce::ComboBox::backgroundColourId, Theme::Colour::surface2); c.setColour (juce::ComboBox::textColourId, Theme::Colour::inkLight); c.setColour (juce::ComboBox::outlineColourId, Theme::Colour::surfaceEdge.withAlpha (0.55f)); }
void header (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& t) { g.setColour (Theme::Colour::surface1); g.fillRect (r); g.setColour (Theme::Zone::a.withAlpha (0.9f)); g.fillRect (r.removeFromLeft (3)); g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.45f)); g.fillRect (r.getX(), r.getBottom() - 1, r.getWidth(), 1); g.setColour (Theme::Zone::a); g.setFont (Theme::Font::micro()); g.drawText (t.toUpperCase(), r.reduced (8, 0), juce::Justification::centredLeft, false); }
void card (juce::Graphics& g, juce::Rectangle<int> r) { g.setColour (Theme::Colour::surface2); g.fillRoundedRectangle (r.toFloat(), 6.0f); g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.55f)); g.drawRoundedRectangle (r.toFloat(), 6.0f, 1.0f); }
int pc (juce::String root) { root = root.trim().toUpperCase(); if (root == "C") return 0; if (root == "C#" || root == "DB") return 1; if (root == "D") return 2; if (root == "D#" || root == "EB") return 3; if (root == "E") return 4; if (root == "F") return 5; if (root == "F#" || root == "GB") return 6; if (root == "G") return 7; if (root == "G#" || root == "AB") return 8; if (root == "A") return 9; if (root == "A#" || root == "BB") return 10; return 11; }
juce::String rootName (int pitchClass) { return kRoots[(pitchClass % 12 + 12) % 12]; }
int deg (juce::String roman) { roman = roman.trim().toUpperCase(); for (int i = 0; i < 7; ++i) if (roman == kDegrees[i]) return i; return 0; }
std::array<int, 7> intervals (juce::String mode) { mode = mode.trim().toLowerCase(); if (mode == "minor") return { 0, 2, 3, 5, 7, 8, 10 }; if (mode == "dorian") return { 0, 2, 3, 5, 7, 9, 10 }; if (mode == "mixolydian") return { 0, 2, 4, 5, 7, 9, 10 }; if (mode == "lydian") return { 0, 2, 4, 6, 7, 9, 11 }; if (mode == "phrygian") return { 0, 1, 3, 5, 7, 8, 10 }; if (mode == "locrian") return { 0, 1, 3, 5, 6, 8, 10 }; return { 0, 2, 4, 5, 7, 9, 11 }; }
std::array<juce::String, 7> qualities (juce::String mode) { mode = mode.trim().toLowerCase(); if (mode == "minor") return { "min", "dim", "maj", "min", "min", "maj", "maj" }; return { "maj", "min", "min", "maj", "maj", "min", "dim" }; }
Chord buildDegreeChord (const Section& s, int degree) { auto root = s.keyRoot.isNotEmpty() ? s.keyRoot : juce::String ("C"); auto mode = s.mode.isNotEmpty() ? s.mode : juce::String ("Major"); return { rootName (pc (root) + intervals (mode)[(size_t) juce::jlimit (0, 6, degree)]), qualities (mode)[(size_t) juce::jlimit (0, 6, degree)] }; }
juce::String durationText (double seconds) { const int s = juce::jmax (0, juce::roundToInt (seconds)); return juce::String (s / 60) + ":" + juce::String (s % 60).paddedLeft ('0', 2); }
juce::String chordLabel (const Chord& chord) { return chord.root.isNotEmpty() ? chord.root + chord.type : juce::String ("Rest"); }
}

StructurePanel::StructurePanel (PluginProcessor& p) : processorRef (p)
{
    addAndMakeVisible (viewport);
    viewport.setScrollBarsShown (true, false);
    viewport.setViewedComponent (&content, false);
    for (juce::Component* c : { static_cast<juce::Component*> (&structureFollowToggle), static_cast<juce::Component*> (&seedRailsButton), static_cast<juce::Component*> (&sectionListTitle), static_cast<juce::Component*> (&sectionList), static_cast<juce::Component*> (&addSectionButton), static_cast<juce::Component*> (&duplicateSectionButton), static_cast<juce::Component*> (&deleteSectionButton), static_cast<juce::Component*> (&moveSectionUpButton), static_cast<juce::Component*> (&moveSectionDownButton), static_cast<juce::Component*> (&arrangementTitle), static_cast<juce::Component*> (&arrangementList), static_cast<juce::Component*> (&addArrangementButton), static_cast<juce::Component*> (&duplicateArrangementButton), static_cast<juce::Component*> (&deleteArrangementButton), static_cast<juce::Component*> (&moveArrangementUpButton), static_cast<juce::Component*> (&moveArrangementDownButton), static_cast<juce::Component*> (&editorHeader), static_cast<juce::Component*> (&editorHint), static_cast<juce::Component*> (&nameLabel), static_cast<juce::Component*> (&barsLabel), static_cast<juce::Component*> (&repeatsLabel), static_cast<juce::Component*> (&transitionLabel), static_cast<juce::Component*> (&beatsLabel), static_cast<juce::Component*> (&keyLabel), static_cast<juce::Component*> (&modeLabel), static_cast<juce::Component*> (&centerLabel), static_cast<juce::Component*> (&nameEditor), static_cast<juce::Component*> (&barsSlider), static_cast<juce::Component*> (&repeatsSlider), static_cast<juce::Component*> (&beatsPerBarBox), static_cast<juce::Component*> (&transitionIntentBox), static_cast<juce::Component*> (&keyRootBox), static_cast<juce::Component*> (&modeBox), static_cast<juce::Component*> (&harmonicCenterBox), static_cast<juce::Component*> (&progressionHeader), static_cast<juce::Component*> (&progressionHint), static_cast<juce::Component*> (&chordRootLabel), static_cast<juce::Component*> (&chordTypeLabel), static_cast<juce::Component*> (&progressionStrip), static_cast<juce::Component*> (&addChordCellButton), static_cast<juce::Component*> (&removeChordCellButton), static_cast<juce::Component*> (&chordRootBox), static_cast<juce::Component*> (&chordTypeBox), static_cast<juce::Component*> (&summaryHeader), static_cast<juce::Component*> (&summaryTempo), static_cast<juce::Component*> (&summaryKey), static_cast<juce::Component*> (&summaryMode), static_cast<juce::Component*> (&summaryBars), static_cast<juce::Component*> (&summaryDuration) }) content.addAndMakeVisible (*c);
    for (auto* l : { &sectionListTitle, &arrangementTitle, &editorHeader, &progressionHeader, &summaryHeader }) styleTitle (*l); for (auto* l : { &editorHint, &progressionHint, &nameLabel, &barsLabel, &repeatsLabel, &transitionLabel, &beatsLabel, &keyLabel, &modeLabel, &centerLabel, &chordRootLabel, &chordTypeLabel }) styleHint (*l); for (auto* l : { &summaryTempo, &summaryKey, &summaryMode, &summaryBars, &summaryDuration }) styleSummary (*l);
    ZoneAControlStyle::styleTextEditor (nameEditor); for (auto* c : { &beatsPerBarBox, &transitionIntentBox, &keyRootBox, &modeBox, &harmonicCenterBox, &chordRootBox, &chordTypeBox }) styleCombo (*c);
    for (auto* b : { &addSectionButton, &duplicateSectionButton, &deleteSectionButton, &moveSectionUpButton, &moveSectionDownButton, &addArrangementButton, &duplicateArrangementButton, &deleteArrangementButton, &moveArrangementUpButton, &moveArrangementDownButton, &addChordCellButton, &removeChordCellButton, &seedRailsButton }) ZoneAControlStyle::styleTextButton (*b, Theme::Zone::a);
    ZoneAControlStyle::initBarSlider (barsSlider, "BARS"); barsSlider.setRange (1, 32, 1); ZoneAControlStyle::initBarSlider (repeatsSlider, "REPEATS"); repeatsSlider.setRange (1, 8, 1);
    sectionList.setColour (juce::ListBox::backgroundColourId, Theme::Colour::surface2); sectionList.setRowHeight (28); arrangementList.setColour (juce::ListBox::backgroundColourId, Theme::Colour::surface2); arrangementList.setRowHeight (26);
    for (auto r : kRoots) { keyRootBox.addItem (r, keyRootBox.getNumItems() + 1); chordRootBox.addItem (r, chordRootBox.getNumItems() + 1); } for (auto m : kModes) modeBox.addItem (m, modeBox.getNumItems() + 1); for (auto d : kDegrees) harmonicCenterBox.addItem (d, harmonicCenterBox.getNumItems() + 1); for (auto t : kTypes) chordTypeBox.addItem (t, chordTypeBox.getNumItems() + 1); for (auto i : kTransitions) transitionIntentBox.addItem (i, transitionIntentBox.getNumItems() + 1); for (auto beats : { 3, 4, 5, 6, 7 }) beatsPerBarBox.addItem (juce::String (beats) + "/4", beats);
    structureFollowToggle.setToggleState (processorRef.isStructureFollowForTracksEnabled(), juce::dontSendNotification); structureFollowToggle.onClick = [this] { const bool enabled = structureFollowToggle.getToggleState(); processorRef.setStructureFollowForTracks (enabled); if (onStructureFollowModeChanged) onStructureFollowModeChanged (enabled); };
    seedRailsButton.onClick = [this] { if (onRailsSeeded) { juce::Array<int> slots; slots.add (0); onRailsSeeded (slots); } };
    addSectionButton.onClick = [this] { addSection(); }; duplicateSectionButton.onClick = [this] { duplicateSelectedSection(); }; deleteSectionButton.onClick = [this] { deleteSelectedSection(); }; moveSectionUpButton.onClick = [this] { moveSelectedSection (-1); }; moveSectionDownButton.onClick = [this] { moveSelectedSection (1); };
    addArrangementButton.onClick = [this] { addArrangementBlock(); }; duplicateArrangementButton.onClick = [this] { duplicateSelectedArrangementBlock(); }; deleteArrangementButton.onClick = [this] { deleteSelectedArrangementBlock(); }; moveArrangementUpButton.onClick = [this] { moveSelectedArrangementBlock (-1); }; moveArrangementDownButton.onClick = [this] { moveSelectedArrangementBlock (1); };
    addChordCellButton.onClick = [this] { addChordCell(); }; removeChordCellButton.onClick = [this] { removeSelectedChordCell(); };
    nameEditor.onReturnKey = [this] { if (auto* s = getSelectedSection()) { s->name = nameEditor.getText().trim(); commitStructureChange (false); refreshLists(); } }; nameEditor.onFocusLost = nameEditor.onReturnKey;
    barsSlider.onValueChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { s->bars = juce::roundToInt (barsSlider.getValue()); commitStructureChange (false); refreshLists(); progressionStrip.repaint(); } }; repeatsSlider.onValueChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { s->repeats = juce::roundToInt (repeatsSlider.getValue()); commitStructureChange (false); refreshLists(); } };
    beatsPerBarBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { s->beatsPerBar = juce::jmax (1, beatsPerBarBox.getSelectedId()); commitStructureChange (false); refreshLists(); } }; transitionIntentBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { s->transitionIntent = transitionIntentBox.getText(); commitStructureChange (false); progressionStrip.repaint(); } };
    keyRootBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { s->keyRoot = keyRootBox.getText(); commitStructureChange (false); progressionStrip.repaint(); } }; modeBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { s->mode = modeBox.getText(); commitStructureChange (false); progressionStrip.repaint(); } }; harmonicCenterBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { s->harmonicCenter = harmonicCenterBox.getText(); commitStructureChange (false); } };
    chordRootBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* c = getSelectedChordCell()) { c->root = chordRootBox.getText(); commitStructureChange (false); progressionStrip.repaint(); } }; chordTypeBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* c = getSelectedChordCell()) { c->type = chordTypeBox.getText(); commitStructureChange (false); progressionStrip.repaint(); } };
    setIntentKeyMode (processorRef.getAppState().song.keyRoot, processorRef.getAppState().song.keyScale); refreshLists(); refreshEditor(); refreshSummary();
}

void StructurePanel::ContentComponent::paint (juce::Graphics& g) { owner.paintContent (g); }
void StructurePanel::paint (juce::Graphics& g) { g.fillAll (Theme::Zone::bgA); }

void StructurePanel::resized()
{
    viewport.setBounds (getLocalBounds());
    const int contentHeight = 920;
    content.setSize (juce::jmax (viewport.getWidth() - 10, 200), contentHeight);
    layoutContent (content.getLocalBounds());
}

void StructurePanel::paintContent (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);
    header (g, { 0, 0, content.getWidth(), 24 }, "Structure");
    card (g, sectionList.getBounds().expanded (6, 32));
    card (g, arrangementList.getBounds().expanded (6, 32));
    card (g, progressionStrip.getBounds().expanded (8, 38));
    card (g, summaryHeader.getBounds().getUnion (summaryDuration.getBounds()).expanded (8, 26));
}

void StructurePanel::layoutContent (juce::Rectangle<int> bounds)
{
    bounds = bounds.reduced (10);
    auto top = bounds.removeFromTop (28);
    seedRailsButton.setBounds (top.removeFromRight (96));
    top.removeFromRight (8);
    structureFollowToggle.setBounds (top.removeFromRight (190));

    bounds.removeFromTop (8);

    auto sectionCard = bounds.removeFromTop (164);
    sectionListTitle.setBounds (sectionCard.removeFromTop (16));
    auto sbtn = sectionCard.removeFromTop (22);
    addSectionButton.setBounds (sbtn.removeFromLeft (42));
    sbtn.removeFromLeft (4);
    duplicateSectionButton.setBounds (sbtn.removeFromLeft (72));
    sbtn.removeFromLeft (4);
    deleteSectionButton.setBounds (sbtn.removeFromLeft (54));
    sbtn.removeFromLeft (4);
    moveSectionUpButton.setBounds (sbtn.removeFromLeft (34));
    sbtn.removeFromLeft (4);
    moveSectionDownButton.setBounds (sbtn.removeFromLeft (44));
    sectionCard.removeFromTop (6);
    sectionList.setBounds (sectionCard);

    bounds.removeFromTop (10);

    auto arrangementCard = bounds.removeFromTop (138);
    arrangementTitle.setBounds (arrangementCard.removeFromTop (16));
    auto abtn = arrangementCard.removeFromTop (22);
    addArrangementButton.setBounds (abtn.removeFromLeft (42));
    abtn.removeFromLeft (4);
    duplicateArrangementButton.setBounds (abtn.removeFromLeft (72));
    abtn.removeFromLeft (4);
    deleteArrangementButton.setBounds (abtn.removeFromLeft (54));
    abtn.removeFromLeft (4);
    moveArrangementUpButton.setBounds (abtn.removeFromLeft (34));
    abtn.removeFromLeft (4);
    moveArrangementDownButton.setBounds (abtn.removeFromLeft (44));
    arrangementCard.removeFromTop (6);
    arrangementList.setBounds (arrangementCard);

    bounds.removeFromTop (12);

    editorHeader.setBounds (bounds.removeFromTop (16));
    editorHint.setBounds (bounds.removeFromTop (16));
    bounds.removeFromTop (8);

    nameLabel.setBounds (bounds.removeFromTop (14));
    nameEditor.setBounds (bounds.removeFromTop (24));
    bounds.removeFromTop (8);
    barsLabel.setBounds (bounds.removeFromTop (14));
    barsSlider.setBounds (bounds.removeFromTop (18));
    bounds.removeFromTop (8);
    repeatsLabel.setBounds (bounds.removeFromTop (14));
    repeatsSlider.setBounds (bounds.removeFromTop (18));
    bounds.removeFromTop (8);

    auto meterLabels = bounds.removeFromTop (14);
    transitionLabel.setBounds (meterLabels.removeFromLeft (juce::jmax (120, meterLabels.getWidth() / 2)));
    meterLabels.removeFromLeft (8);
    beatsLabel.setBounds (meterLabels);
    auto meterRow = bounds.removeFromTop (24);
    transitionIntentBox.setBounds (meterRow.removeFromLeft (juce::jmax (120, meterRow.getWidth() / 2)));
    meterRow.removeFromLeft (8);
    beatsPerBarBox.setBounds (meterRow);

    bounds.removeFromTop (10);

    keyLabel.setBounds (bounds.removeFromTop (14));
    auto tonalRow1 = bounds.removeFromTop (24);
    keyRootBox.setBounds (tonalRow1);
    bounds.removeFromTop (6);
    modeLabel.setBounds (bounds.removeFromTop (14));
    auto tonalRow2 = bounds.removeFromTop (24);
    modeBox.setBounds (tonalRow2);
    bounds.removeFromTop (6);
    centerLabel.setBounds (bounds.removeFromTop (14));
    auto tonalRow3 = bounds.removeFromTop (24);
    harmonicCenterBox.setBounds (tonalRow3);

    bounds.removeFromTop (12);

    progressionHeader.setBounds (bounds.removeFromTop (16));
    progressionHint.setBounds (bounds.removeFromTop (16));
    bounds.removeFromTop (6);

    auto prow = bounds.removeFromTop (24);
    addChordCellButton.setBounds (prow.removeFromLeft (68));
    prow.removeFromLeft (6);
    removeChordCellButton.setBounds (prow.removeFromLeft (68));
    juce::ignoreUnused (prow);

    bounds.removeFromTop (8);
    auto chordLabels = bounds.removeFromTop (14);
    chordRootLabel.setBounds (chordLabels.removeFromLeft (juce::jmax (88, chordLabels.getWidth() / 2 - 3)));
    chordLabels.removeFromLeft (6);
    chordTypeLabel.setBounds (chordLabels);
    auto chordRow = bounds.removeFromTop (24);
    chordRootBox.setBounds (chordRow.removeFromLeft (juce::jmax (88, chordRow.getWidth() / 2 - 3)));
    chordRow.removeFromLeft (6);
    chordTypeBox.setBounds (chordRow);

    bounds.removeFromTop (8);
    progressionStrip.setBounds (bounds.removeFromTop (118));

    bounds.removeFromTop (12);
    summaryHeader.setBounds (bounds.removeFromTop (16));
    summaryTempo.setBounds (bounds.removeFromTop (18));
    summaryKey.setBounds (bounds.removeFromTop (18));
    summaryMode.setBounds (bounds.removeFromTop (18));
    summaryBars.setBounds (bounds.removeFromTop (18));
    summaryDuration.setBounds (bounds.removeFromTop (18));
}

void StructurePanel::setIntentKeyMode (const juce::String& keyRoot, const juce::String& mode) { auto& s = processorRef.getAppState().structure; s.songKey = keyRoot.trim().isNotEmpty() ? keyRoot.trim() : juce::String ("C"); s.songMode = mode.trim().isNotEmpty() ? mode.trim() : juce::String ("Major"); refreshSummary(); }
int StructurePanel::SectionListModel::getNumRows() { return static_cast<int> (owner.processorRef.getAppState().structure.sections.size()); }
void StructurePanel::SectionListModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) { const auto& sections = owner.processorRef.getAppState().structure.sections; if (row < 0 || row >= static_cast<int> (sections.size())) return; const auto& s = sections[(size_t) row]; g.setColour (selected ? Theme::Colour::surface3 : Theme::Colour::surface2); g.fillRect (0, 0, w, h); if (selected) { g.setColour (Theme::Zone::a); g.fillRect (0, 0, 3, h); } g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.22f)); g.fillRect (0, h - 1, w, 1); g.setColour (Theme::Colour::inkLight); g.setFont (Theme::Font::label()); g.drawText (s.name, 8, 0, w - 100, h, juce::Justification::centredLeft, true); g.setColour (Theme::Colour::inkGhost); g.setFont (Theme::Font::micro()); g.drawText (juce::String (s.bars) + " bars x" + juce::String (s.repeats), w - 88, 0, 80, h, juce::Justification::centredRight, true); }
void StructurePanel::SectionListModel::selectedRowsChanged (int row) { owner.selectSection (row); }
int StructurePanel::ArrangementListModel::getNumRows() { return static_cast<int> (buildResolvedStructure (owner.processorRef.getAppState().structure).size()); }
void StructurePanel::ArrangementListModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) { const auto resolved = buildResolvedStructure (owner.processorRef.getAppState().structure); if (row < 0 || row >= static_cast<int> (resolved.size()) || resolved[(size_t) row].section == nullptr) return; const auto& r = resolved[(size_t) row]; g.setColour (selected ? sectionColourForIndex (row).withAlpha (0.72f) : Theme::Colour::surface2); g.fillRect (0, 0, w, h); g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.22f)); g.fillRect (0, h - 1, w, 1); g.setColour (selected ? Theme::Colour::inkDark : Theme::Colour::inkLight); g.setFont (Theme::Font::label()); g.drawText (r.section->name, 8, 0, w - 80, h, juce::Justification::centredLeft, true); g.setFont (Theme::Font::micro()); g.drawText (juce::String (r.barsPerRepeat) + "x" + juce::String (r.repeats), w - 58, 0, 50, h, juce::Justification::centredRight, true); }
void StructurePanel::ArrangementListModel::selectedRowsChanged (int row) { owner.selectArrangementBlock (row); }

void StructurePanel::ProgressionStrip::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface2);
    const auto* s = owner.getSelectedSection();
    if (s == nullptr) { g.setColour (Theme::Colour::inkGhost); g.setFont (Theme::Font::micro()); g.drawText ("Select a section to define looping harmonic cells.", getLocalBounds().reduced (8), juce::Justification::centred, true); return; }
    if (s->progression.empty()) { g.setColour (Theme::Colour::inkGhost); g.setFont (Theme::Font::micro()); g.drawText ("No chord cells yet. Add one to define the section loop.", getLocalBounds().reduced (8), juce::Justification::centred, true); return; }

    auto area = getLocalBounds().reduced (10, 10);
    auto info = area.removeFromBottom (16);
    const int bars = juce::jmax (1, s->bars);
    const int cells = static_cast<int> (s->progression.size());
    const int gap = 4;
    const int barWidth = juce::jmax (28, (area.getWidth() - gap * juce::jmax (0, bars - 1)) / bars);

    for (int bar = 0; bar < bars; ++bar)
    {
        auto slot = juce::Rectangle<int> (area.getX() + bar * (barWidth + gap), area.getY(), barWidth, area.getHeight());
        const int chordIndex = bar % cells;
        const bool selected = owner.selectedChordIndex == chordIndex;
        const auto tint = sectionColourForIndex (chordIndex);
        g.setColour (Theme::Colour::surface1.withAlpha (0.55f));
        g.fillRoundedRectangle (slot.toFloat(), 6.0f);
        g.setColour (selected ? tint.brighter (0.1f) : Theme::Colour::surfaceEdge.withAlpha (0.35f));
        g.drawRoundedRectangle (slot.toFloat(), 6.0f, selected ? 1.8f : 1.0f);
        auto cell = slot.reduced (4, 4);
        cell.removeFromBottom (12);
        g.setColour (selected ? tint : tint.withAlpha (0.55f));
        g.fillRoundedRectangle (cell.toFloat(), 5.0f);
        g.setColour (selected ? Theme::Colour::inkDark : Theme::Colour::inkLight);
        g.setFont (Theme::Font::heading());
        g.drawText (chordLabel (s->progression[(size_t) chordIndex]), cell.reduced (6, 4), juce::Justification::centredLeft, true);
        g.setFont (Theme::Font::micro());
        g.setColour (selected ? Theme::Colour::inkLight : Theme::Colour::inkGhost);
        g.drawText ("bar " + juce::String (bar + 1), slot.removeFromBottom (12), juce::Justification::centred, false);
        if (bar >= cells)
        {
            g.setColour (Theme::Colour::inkGhost.withAlpha (0.7f));
            g.drawText ("loop", slot.withTrimmedTop (4).removeFromRight (28), juce::Justification::topRight, false);
        }
    }

    g.setColour (Theme::Colour::inkGhost);
    g.setFont (Theme::Font::micro());
    g.drawText (juce::String (cells) + " cell" + (cells == 1 ? "" : "s") + " looping across " + juce::String (bars) + " bars", info, juce::Justification::centredLeft, true);
}

void StructurePanel::ProgressionStrip::mouseUp (const juce::MouseEvent& e)
{
    const auto* s = owner.getSelectedSection();
    if (s == nullptr || s->progression.empty()) return;
    auto area = getLocalBounds().reduced (10, 10); area.removeFromBottom (16);
    const int bars = juce::jmax (1, s->bars), cells = static_cast<int> (s->progression.size()), gap = 4;
    const int barWidth = juce::jmax (28, (area.getWidth() - gap * juce::jmax (0, bars - 1)) / bars);
    for (int bar = 0; bar < bars; ++bar)
        if (juce::Rectangle<int> (area.getX() + bar * (barWidth + gap), area.getY(), barWidth, area.getHeight()).contains (e.getPosition()))
        { owner.selectChordCell (bar % cells); if (e.mods.isRightButtonDown()) owner.showChordCellMenu(); return; }
}

void StructurePanel::ProgressionStrip::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto* s = owner.getSelectedSection();
    if (s == nullptr || s->progression.empty()) return;
    auto area = getLocalBounds().reduced (10, 10); area.removeFromBottom (16);
    const int bars = juce::jmax (1, s->bars), cells = static_cast<int> (s->progression.size()), gap = 4;
    const int barWidth = juce::jmax (28, (area.getWidth() - gap * juce::jmax (0, bars - 1)) / bars);
    for (int bar = 0; bar < bars; ++bar)
        if (juce::Rectangle<int> (area.getX() + bar * (barWidth + gap), area.getY(), barWidth, area.getHeight()).contains (e.getPosition()))
        { const int index = bar % cells; owner.selectChordCell (index); owner.showChordEditMenu (index); return; }
}

Section* StructurePanel::getSelectedSection() { auto& s = processorRef.getAppState().structure; if (selectionTarget == SelectionTarget::section && selectedSectionIndex >= 0 && selectedSectionIndex < static_cast<int> (s.sections.size())) return &s.sections[(size_t) selectedSectionIndex]; if (selectionTarget == SelectionTarget::arrangementBlock) { const auto resolved = buildResolvedStructure (s); if (selectedArrangementIndex >= 0 && selectedArrangementIndex < static_cast<int> (resolved.size())) return const_cast<Section*> (resolved[(size_t) selectedArrangementIndex].section); } return nullptr; }
const Section* StructurePanel::getSelectedSection() const { return const_cast<StructurePanel*> (this)->getSelectedSection(); }
ArrangementBlock* StructurePanel::getSelectedArrangementBlock() { auto resolved = buildResolvedStructure (processorRef.getAppState().structure); if (selectedArrangementIndex < 0 || selectedArrangementIndex >= static_cast<int> (resolved.size())) return nullptr; return const_cast<ArrangementBlock*> (resolved[(size_t) selectedArrangementIndex].block); }
const ArrangementBlock* StructurePanel::getSelectedArrangementBlock() const { return const_cast<StructurePanel*> (this)->getSelectedArrangementBlock(); }
Chord* StructurePanel::getSelectedChordCell() { auto* s = getSelectedSection(); if (s == nullptr || selectedChordIndex < 0 || selectedChordIndex >= static_cast<int> (s->progression.size())) return nullptr; return &s->progression[(size_t) selectedChordIndex]; }
const Chord* StructurePanel::getSelectedChordCell() const { return const_cast<StructurePanel*> (this)->getSelectedChordCell(); }

void StructurePanel::addSection() { auto& st = processorRef.getAppState().structure; Section s; s.id = makeStructureId(); s.name = "Section " + juce::String (st.sections.size() + 1); s.bars = 8; s.beatsPerBar = 4; s.repeats = 1; s.keyRoot = st.songKey; s.mode = st.songMode; s.harmonicCenter = "I"; s.transitionIntent = "None"; s.progression = { buildDegreeChord (s, deg (s.harmonicCenter)) }; st.sections.push_back (s); selectionTarget = SelectionTarget::section; selectedSectionIndex = static_cast<int> (st.sections.size() - 1); selectedArrangementIndex = -1; selectedChordIndex = 0; commitStructureChange (false); refreshLists(); refreshEditor(); }
void StructurePanel::duplicateSelectedSection() { const auto* src = getSelectedSection(); if (src == nullptr) return; auto copy = *src; copy.id = makeStructureId(); copy.name = src->name + " Copy"; auto& sections = processorRef.getAppState().structure.sections; sections.push_back (copy); selectionTarget = SelectionTarget::section; selectedSectionIndex = static_cast<int> (sections.size() - 1); selectedArrangementIndex = -1; selectedChordIndex = 0; commitStructureChange (false); refreshLists(); refreshEditor(); }
void StructurePanel::deleteSelectedSection() { auto& st = processorRef.getAppState().structure; if (selectedSectionIndex < 0 || selectedSectionIndex >= static_cast<int> (st.sections.size())) return; const auto removedId = st.sections[(size_t) selectedSectionIndex].id; st.sections.erase (st.sections.begin() + selectedSectionIndex); std::erase_if (st.arrangement, [&] (const ArrangementBlock& b) { return b.sectionId == removedId; }); resequenceArrangement(); clearSelection(); commitStructureChange (false); refreshLists(); refreshEditor(); }
void StructurePanel::moveSelectedSection (int delta) { auto& sections = processorRef.getAppState().structure.sections; if (selectedSectionIndex < 0 || selectedSectionIndex >= static_cast<int> (sections.size())) return; const int target = juce::jlimit (0, static_cast<int> (sections.size() - 1), selectedSectionIndex + delta); if (target == selectedSectionIndex) return; std::iter_swap (sections.begin() + selectedSectionIndex, sections.begin() + target); selectedSectionIndex = target; commitStructureChange (false); refreshLists(); }
void StructurePanel::addArrangementBlock() { auto& st = processorRef.getAppState().structure; if (st.sections.empty()) { addSection(); return; } const int source = juce::jlimit (0, static_cast<int> (st.sections.size() - 1), selectedSectionIndex >= 0 ? selectedSectionIndex : 0); ArrangementBlock b; b.id = makeStructureId(); b.sectionId = st.sections[(size_t) source].id; b.orderIndex = static_cast<int> (st.arrangement.size()); st.arrangement.push_back (b); selectionTarget = SelectionTarget::arrangementBlock; selectedArrangementIndex = static_cast<int> (st.arrangement.size() - 1); selectedChordIndex = 0; commitStructureChange (false); refreshLists(); refreshEditor(); }
void StructurePanel::duplicateSelectedArrangementBlock() { auto& a = processorRef.getAppState().structure.arrangement; if (selectedArrangementIndex < 0 || selectedArrangementIndex >= static_cast<int> (a.size())) return; auto copy = a[(size_t) selectedArrangementIndex]; copy.id = makeStructureId(); a.insert (a.begin() + selectedArrangementIndex + 1, copy); selectedArrangementIndex += 1; resequenceArrangement(); selectionTarget = SelectionTarget::arrangementBlock; commitStructureChange (false); refreshLists(); }
void StructurePanel::deleteSelectedArrangementBlock() { auto& a = processorRef.getAppState().structure.arrangement; if (selectedArrangementIndex < 0 || selectedArrangementIndex >= static_cast<int> (a.size())) return; a.erase (a.begin() + selectedArrangementIndex); resequenceArrangement(); clearSelection(); commitStructureChange (false); refreshLists(); refreshEditor(); }
void StructurePanel::moveSelectedArrangementBlock (int delta) { auto& a = processorRef.getAppState().structure.arrangement; if (selectedArrangementIndex < 0 || selectedArrangementIndex >= static_cast<int> (a.size())) return; const int target = juce::jlimit (0, static_cast<int> (a.size() - 1), selectedArrangementIndex + delta); if (target == selectedArrangementIndex) return; std::iter_swap (a.begin() + selectedArrangementIndex, a.begin() + target); selectedArrangementIndex = target; resequenceArrangement(); selectionTarget = SelectionTarget::arrangementBlock; commitStructureChange (false); refreshLists(); }
void StructurePanel::resequenceArrangement() { auto& a = processorRef.getAppState().structure.arrangement; for (int i = 0; i < static_cast<int> (a.size()); ++i) a[(size_t) i].orderIndex = i; }
void StructurePanel::addChordCell() { if (auto* s = getSelectedSection()) { s->progression.push_back (buildDegreeChord (*s, deg (s->harmonicCenter))); selectedChordIndex = static_cast<int> (s->progression.size() - 1); commitStructureChange (false); refreshEditor(); progressionStrip.repaint(); } }
void StructurePanel::removeSelectedChordCell() { if (auto* s = getSelectedSection()) { if (selectedChordIndex < 0 || selectedChordIndex >= static_cast<int> (s->progression.size())) return; s->progression.erase (s->progression.begin() + selectedChordIndex); selectedChordIndex = s->progression.empty() ? -1 : juce::jlimit (0, static_cast<int> (s->progression.size()) - 1, selectedChordIndex); commitStructureChange (false); refreshEditor(); progressionStrip.repaint(); } }
void StructurePanel::selectChordCell (int index) { selectedChordIndex = index; refreshEditor(); progressionStrip.repaint(); }
void StructurePanel::showChordCellMenu() { if (getSelectedChordCell() == nullptr) return; juce::PopupMenu m; m.addItem (dupCell, "Duplicate Cell"); m.addItem (clearCell, "Clear Cell"); m.addItem (splitCell, "Split Cell"); m.addSeparator(); m.addItem (diatonicSub, "Diatonic Substitute"); m.addItem (secondaryDom, "Secondary Dominant"); m.addItem (borrowedChord, "Borrowed Chord"); m.addItem (extendChord, "Extend Chord"); m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&progressionStrip), [this] (int result) { if (result != 0) applyTheoryAction (result); }); }
void StructurePanel::showChordEditMenu (int index) { if (index < 0) return; selectedChordIndex = index; juce::PopupMenu rootMenu, typeMenu, menu; for (int i = 0; i < 12; ++i) rootMenu.addItem (rootBase + i, kRoots[i]); for (int i = 0; i < 7; ++i) typeMenu.addItem (typeBase + i, kTypes[i]); menu.addSubMenu ("Root", rootMenu); menu.addSubMenu ("Quality", typeMenu); menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&progressionStrip), [this] (int result) { if (result != 0) applyTheoryAction (result); }); }
void StructurePanel::applyTheoryAction (int cmd) { auto* s = getSelectedSection(); auto* c = getSelectedChordCell(); if (s == nullptr || c == nullptr) return; if (cmd >= rootBase && cmd < rootBase + 12) c->root = kRoots[cmd - rootBase]; else if (cmd >= typeBase && cmd < typeBase + 7) c->type = kTypes[cmd - typeBase]; else if (cmd == dupCell || cmd == splitCell) { s->progression.insert (s->progression.begin() + selectedChordIndex + 1, *c); if (cmd == dupCell) selectedChordIndex += 1; } else if (cmd == clearCell) *c = buildDegreeChord (*s, deg (s->harmonicCenter)); else if (cmd == diatonicSub) *c = buildDegreeChord (*s, (deg (s->harmonicCenter) + selectedChordIndex + 1) % 7); else if (cmd == secondaryDom) { const int next = (selectedChordIndex + 1) % juce::jmax (1, static_cast<int> (s->progression.size())); c->root = rootName (pc (s->progression[(size_t) next].root) + 7); c->type = "7"; } else if (cmd == borrowedChord) c->type = c->type.startsWithIgnoreCase ("min") ? "maj" : "min"; else if (cmd == extendChord) { if (c->type == "maj") c->type = "maj7"; else if (c->type == "min") c->type = "min7"; else c->type = "7"; } commitStructureChange (false); refreshEditor(); progressionStrip.repaint(); }
void StructurePanel::selectSection (int index) { if (suppressListCallbacks) return; if (index < 0) { clearSelection(); return; } selectionTarget = SelectionTarget::section; selectedSectionIndex = index; selectedArrangementIndex = -1; selectedChordIndex = 0; suppressListCallbacks = true; arrangementList.deselectAllRows(); suppressListCallbacks = false; refreshEditor(); progressionStrip.repaint(); }
void StructurePanel::selectArrangementBlock (int index) { if (index < 0) { clearSelection(); return; } selectionTarget = SelectionTarget::arrangementBlock; selectedArrangementIndex = index; selectedSectionIndex = -1; selectedChordIndex = 0; suppressListCallbacks = true; sectionList.deselectAllRows(); suppressListCallbacks = false; refreshEditor(); progressionStrip.repaint(); }
void StructurePanel::clearSelection() { selectionTarget = SelectionTarget::none; selectedSectionIndex = -1; selectedArrangementIndex = -1; selectedChordIndex = -1; suppressListCallbacks = true; sectionList.deselectAllRows(); arrangementList.deselectAllRows(); suppressListCallbacks = false; refreshEditor(); progressionStrip.repaint(); }
juce::Rectangle<int> StructurePanel::chordCellBounds (int index) const { const auto* s = getSelectedSection(); if (s == nullptr || s->progression.empty()) return {}; auto r = progressionStrip.getLocalBounds().reduced (10, 12); const int count = static_cast<int> (s->progression.size()); const int gap = 6; const int w = juce::jmax (72, (r.getWidth() - gap * juce::jmax (0, count - 1)) / juce::jmax (1, count)); return { r.getX() + index * (w + gap), r.getY(), w, r.getHeight() }; }
void StructurePanel::refreshLists() { suppressListCallbacks = true; sectionList.updateContent(); arrangementList.updateContent(); if (selectionTarget == SelectionTarget::section && selectedSectionIndex >= 0) sectionList.selectRow (selectedSectionIndex, false, false); if (selectionTarget == SelectionTarget::arrangementBlock && selectedArrangementIndex >= 0) arrangementList.selectRow (selectedArrangementIndex, false, false); suppressListCallbacks = false; }
void StructurePanel::refreshEditor() { const auto* s = getSelectedSection(); const bool has = s != nullptr; editorHint.setText (has ? "Shape length, tonal center, repeating harmonic cells, and transitions like a Zone A module." : "Select a section or arrangement block to sculpt song form.", juce::dontSendNotification); suppressControlCallbacks = true; nameEditor.setEnabled (has); barsSlider.setEnabled (has); repeatsSlider.setEnabled (has); beatsPerBarBox.setEnabled (has); transitionIntentBox.setEnabled (has); keyRootBox.setEnabled (has); modeBox.setEnabled (has); harmonicCenterBox.setEnabled (has); addChordCellButton.setEnabled (has); if (! has) { nameEditor.clear(); barsSlider.setValue (1, juce::dontSendNotification); repeatsSlider.setValue (1, juce::dontSendNotification); beatsPerBarBox.setSelectedId (4, juce::dontSendNotification); transitionIntentBox.setSelectedId (1, juce::dontSendNotification); keyRootBox.setSelectedId (1, juce::dontSendNotification); modeBox.setSelectedId (1, juce::dontSendNotification); harmonicCenterBox.setSelectedId (1, juce::dontSendNotification); chordRootBox.setSelectedId (1, juce::dontSendNotification); chordTypeBox.setSelectedId (1, juce::dontSendNotification); suppressControlCallbacks = false; return; } nameEditor.setText (s->name, juce::dontSendNotification); barsSlider.setValue (s->bars, juce::dontSendNotification); repeatsSlider.setValue (s->repeats, juce::dontSendNotification); beatsPerBarBox.setSelectedId (s->beatsPerBar, juce::dontSendNotification); transitionIntentBox.setText (s->transitionIntent, juce::dontSendNotification); keyRootBox.setText (s->keyRoot.isNotEmpty() ? s->keyRoot : processorRef.getAppState().structure.songKey, juce::dontSendNotification); modeBox.setText (s->mode.isNotEmpty() ? s->mode : processorRef.getAppState().structure.songMode, juce::dontSendNotification); harmonicCenterBox.setText (s->harmonicCenter, juce::dontSendNotification); if (selectedChordIndex < 0 && ! s->progression.empty()) selectedChordIndex = 0; if (selectedChordIndex >= static_cast<int> (s->progression.size())) selectedChordIndex = static_cast<int> (s->progression.size()) - 1; const bool hasChord = getSelectedChordCell() != nullptr; removeChordCellButton.setEnabled (hasChord); chordRootBox.setEnabled (hasChord); chordTypeBox.setEnabled (hasChord); if (const auto* chord = getSelectedChordCell()) { chordRootBox.setText (chord->root, juce::dontSendNotification); chordTypeBox.setText (chord->type, juce::dontSendNotification); } suppressControlCallbacks = false; progressionStrip.repaint(); }
void StructurePanel::refreshSummary() { auto& s = processorRef.getAppState().structure; s.songTempo = processorRef.getAppState().song.bpm; if (s.songKey.isEmpty()) s.songKey = processorRef.getAppState().song.keyRoot; if (s.songMode.isEmpty()) s.songMode = processorRef.getAppState().song.keyScale; processorRef.getStructureEngine().rebuild(); summaryTempo.setText ("Tempo  " + juce::String (s.songTempo) + " BPM", juce::dontSendNotification); summaryKey.setText ("Key  " + s.songKey, juce::dontSendNotification); summaryMode.setText ("Mode  " + s.songMode, juce::dontSendNotification); summaryBars.setText ("Total Bars  " + juce::String (s.totalBars), juce::dontSendNotification); summaryDuration.setText ("Estimated Duration  " + durationText (s.estimatedDurationSeconds), juce::dontSendNotification); }
void StructurePanel::commitStructureChange (bool seedRails) { auto& s = processorRef.getAppState().structure; s.songTempo = processorRef.getAppState().song.bpm; s.songKey = processorRef.getAppState().song.keyRoot; s.songMode = processorRef.getAppState().song.keyScale; processorRef.getStructureEngine().rebuild(); refreshSummary(); if (seedRails && onRailsSeeded) { juce::Array<int> slots; slots.add (0); onRailsSeeded (slots); } if (onStructureApplied) onStructureApplied(); }

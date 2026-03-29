#include "StructurePanel.h"
#include "../../PluginProcessor.h"
#include "../../PluginEditor.h"
#include "../../theory/DiatonicHarmony.h"
#include "../ZoneD/ClipData.h"
#include "ZoneAControlStyle.h"

#include <cmath>

namespace
{
constexpr int kProgressionGridSteps = 64;
constexpr int kProgressionGridColumns = 8;
constexpr int kProgressionGridRows = kProgressionGridSteps / kProgressionGridColumns;
constexpr const char* kRoots[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
constexpr const char* kModes[] = { "Major", "Minor", "Dorian", "Mixolydian", "Lydian", "Phrygian", "Locrian" };
constexpr const char* kDegrees[] = { "I", "II", "III", "IV", "V", "VI", "VII" };
constexpr const char* kTypes[] = { "maj", "min", "maj7", "min7", "7", "sus4", "dim" };
constexpr const char* kTransitions[] = { "None", "Pickup", "Cadence", "Lift", "Drop", "Fill Launch", "Soft Resolve", "Dominant Push" };
enum Cmd { dupCell = 1, clearCell, splitCell, diatonicSub, secondaryDom, borrowedChord, extendChord, shortenBeat, lengthenBeat, flattenRoot, sharpenRoot, setMaj, setMin, setDim, setSus, rootBase = 100, typeBase = 200 };
void styleTitle (juce::Label& l) { l.setFont (Theme::Font::micro()); l.setColour (juce::Label::textColourId, Theme::Zone::a); l.setJustificationType (juce::Justification::centredLeft); }
void styleHint (juce::Label& l) { l.setFont (Theme::Font::micro()); l.setColour (juce::Label::textColourId, Theme::Colour::inkMid); l.setJustificationType (juce::Justification::centredLeft); }
void styleSummary (juce::Label& l) { l.setFont (Theme::Font::label()); l.setColour (juce::Label::textColourId, Theme::Colour::inkLight); }
void styleCombo (juce::ComboBox& c, juce::Colour accent) { ZoneAControlStyle::styleComboBox (c, accent); }
juce::String durationText (double seconds) { const int s = juce::jmax (0, juce::roundToInt (seconds)); return juce::String (s / 60) + ":" + juce::String (s % 60).paddedLeft ('0', 2); }
juce::String chordLabel (const Chord& chord) { return abbreviatedChordLabel (chord); }
int limitedProgressionBeats (const Section& section)
{
    return juce::jlimit (1, kProgressionGridSteps, computeSectionLoopBeats (section));
}
juce::Path hexagonPath (juce::Point<float> center, float radius)
{
    juce::Path path;
    constexpr float rotation = juce::MathConstants<float>::pi / 6.0f;
    for (int i = 0; i < 6; ++i)
    {
        const auto angle = juce::MathConstants<float>::twoPi * (0.1666667f * (float) i)
                         - juce::MathConstants<float>::halfPi
                         + rotation;
        const auto point = juce::Point<float> (center.x + std::cos (angle) * radius,
                                               center.y + std::sin (angle) * radius);
        if (i == 0)
            path.startNewSubPath (point);
        else
            path.lineTo (point);
    }
    path.closeSubPath();
    return path;
}

struct StructureUndoAction final : public juce::UndoableAction
{
    StructureUndoAction (PluginProcessor& processorIn,
                         const StructureState& beforeIn,
                         const StructureState& afterIn)
        : processor (processorIn), beforeState (beforeIn), afterState (afterIn) {}

    bool perform() override
    {
        processor.getSongManager().replaceStructureState (afterState);
        processor.getStructureEngine().rebuild();
        processor.syncAuthoredSongToRuntime();
        if (auto* editor = dynamic_cast<PluginEditor*> (processor.getActiveEditor()))
            editor->refreshFromAuthoredSong();
        return true;
    }

    bool undo() override
    {
        processor.getSongManager().replaceStructureState (beforeState);
        processor.getStructureEngine().rebuild();
        processor.syncAuthoredSongToRuntime();
        if (auto* editor = dynamic_cast<PluginEditor*> (processor.getActiveEditor()))
            editor->refreshFromAuthoredSong();
        return true;
    }

    int getSizeInUnits() override { return 1; }

    PluginProcessor& processor;
    StructureState beforeState;
    StructureState afterState;
};
}

StructurePanel::StructurePanel (PluginProcessor& p) : processorRef (p)
{
    const auto accent = ZoneAStyle::accentForTabId ("structure");
    addAndMakeVisible (viewport);
    viewport.setScrollBarsShown (true, false);
    viewport.setViewedComponent (&content, false);
    for (juce::Component* c : { static_cast<juce::Component*> (&structureFollowToggle), static_cast<juce::Component*> (&seedRailsButton), static_cast<juce::Component*> (&sectionListTitle), static_cast<juce::Component*> (&sectionListHint), static_cast<juce::Component*> (&sectionList), static_cast<juce::Component*> (&addSectionButton), static_cast<juce::Component*> (&duplicateSectionButton), static_cast<juce::Component*> (&deleteSectionButton), static_cast<juce::Component*> (&moveSectionUpButton), static_cast<juce::Component*> (&moveSectionDownButton), static_cast<juce::Component*> (&inlineSectionNameEditor), static_cast<juce::Component*> (&arrangementTitle), static_cast<juce::Component*> (&arrangementFlowHint), static_cast<juce::Component*> (&arrangementList), static_cast<juce::Component*> (&arrangementHint), static_cast<juce::Component*> (&addArrangementButton), static_cast<juce::Component*> (&duplicateArrangementButton), static_cast<juce::Component*> (&deleteArrangementButton), static_cast<juce::Component*> (&moveArrangementUpButton), static_cast<juce::Component*> (&moveArrangementDownButton), static_cast<juce::Component*> (&editorHeader), static_cast<juce::Component*> (&editorHint), static_cast<juce::Component*> (&nameLabel), static_cast<juce::Component*> (&barsLabel), static_cast<juce::Component*> (&repeatsLabel), static_cast<juce::Component*> (&transitionLabel), static_cast<juce::Component*> (&beatsLabel), static_cast<juce::Component*> (&keyLabel), static_cast<juce::Component*> (&modeLabel), static_cast<juce::Component*> (&centerLabel), static_cast<juce::Component*> (&nameEditor), static_cast<juce::Component*> (&barsSlider), static_cast<juce::Component*> (&repeatsSlider), static_cast<juce::Component*> (&beatsPerBarBox), static_cast<juce::Component*> (&transitionIntentBox), static_cast<juce::Component*> (&keyRootBox), static_cast<juce::Component*> (&modeBox), static_cast<juce::Component*> (&harmonicCenterBox), static_cast<juce::Component*> (&progressionHeader), static_cast<juce::Component*> (&progressionHint), static_cast<juce::Component*> (&progressionPaletteHint), static_cast<juce::Component*> (&diatonicPaletteStrip), static_cast<juce::Component*> (&chordRootLabel), static_cast<juce::Component*> (&chordTypeLabel), static_cast<juce::Component*> (&progressionStrip), static_cast<juce::Component*> (&addChordCellButton), static_cast<juce::Component*> (&progressionPresetButton), static_cast<juce::Component*> (&cadenceButton), static_cast<juce::Component*> (&removeChordCellButton), static_cast<juce::Component*> (&chordRootBox), static_cast<juce::Component*> (&chordTypeBox), static_cast<juce::Component*> (&summaryHeader), static_cast<juce::Component*> (&summaryTempo), static_cast<juce::Component*> (&summaryKey), static_cast<juce::Component*> (&summaryMode), static_cast<juce::Component*> (&summaryBars), static_cast<juce::Component*> (&summaryDuration) }) content.addAndMakeVisible (*c);
    for (auto* l : { &sectionListTitle, &arrangementTitle, &editorHeader, &progressionHeader, &summaryHeader }) { styleTitle (*l); l->setColour (juce::Label::textColourId, accent); } for (auto* l : { &sectionListHint, &arrangementFlowHint, &arrangementHint, &editorHint, &progressionHint, &progressionPaletteHint, &nameLabel, &barsLabel, &repeatsLabel, &transitionLabel, &beatsLabel, &keyLabel, &modeLabel, &centerLabel, &chordRootLabel, &chordTypeLabel }) styleHint (*l); for (auto* l : { &summaryTempo, &summaryKey, &summaryMode, &summaryBars, &summaryDuration }) styleSummary (*l);
    ZoneAControlStyle::styleTextEditor (nameEditor); ZoneAControlStyle::styleTextEditor (inlineSectionNameEditor); inlineSectionNameEditor.setVisible (false); arrangementHint.setInterceptsMouseClicks (false, false); for (auto* c : { &beatsPerBarBox, &transitionIntentBox, &keyRootBox, &modeBox, &harmonicCenterBox, &chordRootBox, &chordTypeBox }) styleCombo (*c, accent);
    for (auto* b : { &addSectionButton, &duplicateSectionButton, &moveSectionUpButton, &moveSectionDownButton, &addArrangementButton, &duplicateArrangementButton, &moveArrangementUpButton, &moveArrangementDownButton, &addChordCellButton, &progressionPresetButton, &cadenceButton, &removeChordCellButton, &seedRailsButton }) ZoneAControlStyle::styleTextButton (*b, accent);
    ZoneAControlStyle::initBarSlider (barsSlider, "BARS"); barsSlider.setRange (1, 32, 1); ZoneAControlStyle::tintBarSlider (barsSlider, accent); ZoneAControlStyle::initBarSlider (repeatsSlider, "REPEATS"); repeatsSlider.setRange (1, 8, 1); ZoneAControlStyle::tintBarSlider (repeatsSlider, accent);
    moveSectionUpButton.setVisible (true);
    moveSectionDownButton.setVisible (true);
    moveArrangementUpButton.setVisible (true);
    moveArrangementDownButton.setVisible (true);
    sectionList.setColour (juce::ListBox::backgroundColourId, Theme::Colour::surface2); sectionList.setRowHeight (22); arrangementList.setColour (juce::ListBox::backgroundColourId, Theme::Colour::surface2); arrangementList.setRowHeight (22);
    for (auto r : kRoots) { keyRootBox.addItem (r, keyRootBox.getNumItems() + 1); chordRootBox.addItem (r, chordRootBox.getNumItems() + 1); } for (auto m : kModes) modeBox.addItem (m, modeBox.getNumItems() + 1); for (auto d : kDegrees) harmonicCenterBox.addItem (d, harmonicCenterBox.getNumItems() + 1); for (auto t : kTypes) chordTypeBox.addItem (t, chordTypeBox.getNumItems() + 1); for (auto i : kTransitions) transitionIntentBox.addItem (i, transitionIntentBox.getNumItems() + 1); for (auto beats : { 3, 4, 5, 6, 7 }) beatsPerBarBox.addItem (juce::String (beats) + "/4", beats);
    ZoneAControlStyle::styleToggleButton (structureFollowToggle, accent);
    structureFollowToggle.setButtonText ("Follow");
    structureFollowToggle.setToggleState (processorRef.isStructureFollowForTracksEnabled(), juce::dontSendNotification); structureFollowToggle.onClick = [this] { const bool enabled = structureFollowToggle.getToggleState(); processorRef.setStructureFollowForTracks (enabled); if (onStructureFollowModeChanged) onStructureFollowModeChanged (enabled); };
    seedRailsButton.onClick = [this] { if (onRailsSeeded) { juce::Array<int> slots; slots.add (0); onRailsSeeded (slots); } };
    seedRailsButton.setVisible (false);
    sectionListHint.setText ("Reusable loops (double-click row to rename, drag to reorder)", juce::dontSendNotification);
    arrangementFlowHint.setText ("Song order (drag sections in, drag rows to reorder)", juce::dontSendNotification);
    arrangementHint.setText ("Drop a section here to start the song form", juce::dontSendNotification);
    addSectionButton.setButtonText ("New");
    addArrangementButton.setButtonText ("Place");
    progressionPresetButton.onClick = [this] { showProgressionPresetMenu(); };
    cadenceButton.onClick = [this] { showCadenceMenu(); };
    addSectionButton.onClick = [this] { addSection(); }; duplicateSectionButton.onClick = [this] { duplicateSelectedSection(); }; moveSectionUpButton.onClick = [this] { moveSelectedSection (-1); }; moveSectionDownButton.onClick = [this] { moveSelectedSection (1); };
    addArrangementButton.onClick = [this] { addArrangementBlock(); }; duplicateArrangementButton.onClick = [this] { duplicateSelectedArrangementBlock(); }; moveArrangementUpButton.onClick = [this] { moveSelectedArrangementBlock (-1); }; moveArrangementDownButton.onClick = [this] { moveSelectedArrangementBlock (1); };
    addChordCellButton.onClick = [this] { addChordCell(); };
    removeChordCellButton.onClick = [this] { removeSelectedChordCell(); };
    nameEditor.onReturnKey = [this] { if (auto* s = getSelectedSection()) { const auto beforeState = processorRef.getSongManager().getStructureState(); s->name = nameEditor.getText().trim(); commitStructureChange (beforeState, "Rename Section", false); refreshLists(); } }; nameEditor.onFocusLost = nameEditor.onReturnKey;
    inlineSectionNameEditor.onReturnKey = [this] { finishInlineSectionRename (true); };
    inlineSectionNameEditor.onEscapeKey = [this] { finishInlineSectionRename (false); };
    inlineSectionNameEditor.onFocusLost = [this] { finishInlineSectionRename (true); };
    barsSlider.onValueChange = [this] {};
    repeatsSlider.onValueChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { const auto beforeState = processorRef.getSongManager().getStructureState(); s->repeats = juce::roundToInt (repeatsSlider.getValue()); updateSectionDerivedLength (*s); commitStructureChange (beforeState, "Change Section Repeats", false); refreshEditor(); refreshLists(); } };
    beatsPerBarBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { const auto beforeState = processorRef.getSongManager().getStructureState(); s->beatsPerBar = juce::jmax (1, beatsPerBarBox.getSelectedId()); updateSectionDerivedLength (*s); commitStructureChange (beforeState, "Change Section Meter", false); refreshEditor(); refreshLists(); } }; transitionIntentBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { const auto beforeState = processorRef.getSongManager().getStructureState(); s->transitionIntent = transitionIntentBox.getText(); commitStructureChange (beforeState, "Change Transition Intent", false); progressionStrip.repaint(); } };
    keyRootBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { const auto beforeState = processorRef.getSongManager().getStructureState(); s->keyRoot = keyRootBox.getText(); commitStructureChange (beforeState, "Change Section Key", false); refreshEditor(); } }; modeBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { const auto beforeState = processorRef.getSongManager().getStructureState(); s->mode = modeBox.getText(); commitStructureChange (beforeState, "Change Section Mode", false); refreshEditor(); } }; harmonicCenterBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* s = getSelectedSection()) { const auto beforeState = processorRef.getSongManager().getStructureState(); s->harmonicCenter = harmonicCenterBox.getText(); commitStructureChange (beforeState, "Change Harmonic Center", false); refreshEditor(); } };
    chordRootBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* c = getSelectedChordCell()) { const auto beforeState = processorRef.getSongManager().getStructureState(); c->root = chordRootBox.getText(); commitStructureChange (beforeState, "Change Chord Root", false); progressionStrip.repaint(); } }; chordTypeBox.onChange = [this] { if (! suppressControlCallbacks) if (auto* c = getSelectedChordCell()) { const auto beforeState = processorRef.getSongManager().getStructureState(); c->type = chordTypeBox.getText(); commitStructureChange (beforeState, "Change Chord Quality", false); progressionStrip.repaint(); } };
    setIntentKeyMode (processorRef.getSongManager().getKeyRoot(), processorRef.getSongManager().getKeyScale()); refreshLists(); refreshEditor(); refreshSummary();
}

void StructurePanel::ContentComponent::paint (juce::Graphics& g) { owner.paintContent (g); }
void StructurePanel::paint (juce::Graphics& g) { g.fillAll (Theme::Zone::bgA); }
void StructurePanel::refreshFromModel() { refreshLists(); refreshEditor(); refreshSummary(); repaint(); }

StructurePanel::StructureListBox::StructureListBox (StructurePanel& ownerIn,
                                                    ListKind kindIn,
                                                    const juce::String& name,
                                                    juce::ListBoxModel* modelIn)
    : juce::ListBox (name, modelIn),
      owner (ownerIn),
      kind (kindIn)
{
}

void StructurePanel::StructureListBox::mouseDown (const juce::MouseEvent& event)
{
    owner.finishInlineSectionRename (true);
    juce::ListBox::mouseDown (event);
    int row = getSelectedRow();
    if (row < 0)
        row = getRowContainingPosition (event.getPosition().x, event.getPosition().y);
    const int totalRows = getListBoxModel() != nullptr ? getListBoxModel()->getNumRows() : 0;
    dragStartRow = event.mods.isLeftButtonDown() && row >= 0 && row < totalRows ? row : -1;
    dragStarted = false;
}

void StructurePanel::StructureListBox::mouseDrag (const juce::MouseEvent& event)
{
    if (dragStartRow < 0 || dragStarted)
    {
        juce::ListBox::mouseDrag (event);
        return;
    }

    if (event.getDistanceFromDragStart() < 4)
    {
        juce::ListBox::mouseDrag (event);
        return;
    }

    const auto description = kind == ListKind::sections
                                ? owner.dragDescriptionForSectionIndex (dragStartRow)
                                : owner.dragDescriptionForArrangementIndex (dragStartRow);
    if (description.isEmpty())
    {
        juce::ListBox::mouseDrag (event);
        return;
    }

    dragStarted = true;
    owner.startDragging (description, this);
    return;
}

void StructurePanel::StructureListBox::mouseUp (const juce::MouseEvent& event)
{
    juce::ListBox::mouseUp (event);
    dragStartRow = -1;
    dragStarted = false;
}

void StructurePanel::StructureListBox::paintOverChildren (juce::Graphics& g)
{
    juce::ListBox::paintOverChildren (g);

    if (dropHover)
    {
        const auto accent = ZoneAStyle::accentForTabId ("structure");
        g.setColour (accent.withAlpha (dropAcceptsExternalSource ? 0.12f : 0.07f));
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), Theme::Radius::sm);
    }

    if (dropRow >= 0)
    {
        const int rowY = juce::jlimit (0, getHeight(), dropRow * getRowHeight());
        const auto accent = ZoneAStyle::accentForTabId ("structure");
        g.setColour (accent.withAlpha (0.95f));
        g.fillRect (4.0f, (float) rowY - 1.0f, (float) juce::jmax (0, getWidth() - 8), 2.0f);
    }

    const int totalRows = getListBoxModel() != nullptr ? getListBoxModel()->getNumRows() : 0;
    if (dropHover && kind == ListKind::arrangement && dropAcceptsExternalSource && totalRows == 0)
    {
        g.setColour (Theme::Colour::inkLight.withAlpha (0.92f));
        g.setFont (Theme::Font::microMedium());
        g.drawText ("Release to place section in song order",
                    getLocalBounds().reduced (8),
                    juce::Justification::centred,
                    true);
    }
}

bool StructurePanel::StructureListBox::isInterestedInDragSource (const SourceDetails& dragSourceDetails)
{
    if (kind == ListKind::arrangement && owner.isDragDescriptionForKind (dragSourceDetails.description, ListKind::sections))
        return true;
    return owner.isDragDescriptionForKind (dragSourceDetails.description, kind);
}

void StructurePanel::StructureListBox::itemDragEnter (const SourceDetails& dragSourceDetails)
{
    dropHover = true;
    dropAcceptsExternalSource = kind == ListKind::arrangement
                             && owner.isDragDescriptionForKind (dragSourceDetails.description, ListKind::sections);
    itemDragMove (dragSourceDetails);
}

void StructurePanel::StructureListBox::itemDragMove (const SourceDetails& dragSourceDetails)
{
    dropHover = true;
    dropAcceptsExternalSource = kind == ListKind::arrangement
                             && owner.isDragDescriptionForKind (dragSourceDetails.description, ListKind::sections);
    const int totalRows = getListBoxModel() != nullptr ? getListBoxModel()->getNumRows() : 0;
    if (totalRows <= 0)
    {
        dropRow = 0;
        repaint();
        return;
    }

    const auto mousePos = getMouseXYRelative();
    const int rawY = mousePos.y;
    const int fallbackY = static_cast<int> (dragSourceDetails.localPosition.y);
    const int y = juce::jlimit (0, getHeight(), rawY >= 0 ? rawY : fallbackY);
    const float rowHeightValue = (float) juce::jmax (1, getRowHeight());
    dropRow = juce::jlimit (0, totalRows, static_cast<int> (std::floor ((float) y / rowHeightValue)));
    repaint();
}

void StructurePanel::StructureListBox::itemDragExit (const SourceDetails&)
{
    dropRow = -1;
    dropHover = false;
    dropAcceptsExternalSource = false;
    repaint();
}

void StructurePanel::StructureListBox::itemDropped (const SourceDetails& dragSourceDetails)
{
    const bool sectionToArrangement = kind == ListKind::arrangement
                                   && owner.isDragDescriptionForKind (dragSourceDetails.description, ListKind::sections);
    const int fromIndex = owner.dragIndexFromDescription (dragSourceDetails.description,
                                                          sectionToArrangement ? ListKind::sections : kind);
    const int totalRows = getListBoxModel() != nullptr ? getListBoxModel()->getNumRows() : 0;
    const int fallbackDropRow = juce::jlimit (0, totalRows, static_cast<int> (std::floor ((float) juce::jlimit (0, getHeight(), getMouseXYRelative().y)
                                                                                              / (float) juce::jmax (1, getRowHeight()))));
    const int targetRow = dropRow >= 0 ? dropRow : fallbackDropRow;
    dropRow = -1;
    dropHover = false;
    dropAcceptsExternalSource = false;
    repaint();

    if (fromIndex >= 0)
    {
        if (sectionToArrangement)
            owner.addArrangementBlockFromSection (fromIndex, targetRow);
        else
            owner.handleListDrop (kind, fromIndex, targetRow);
    }
}

void StructurePanel::TrashDropZone::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    const auto accent = ZoneAStyle::accentForTabId ("structure");
    const auto fill = dragHover ? ThemeManager::get().theme().controlOnBg
                                : ThemeManager::get().theme().controlBg;
    g.setColour (fill);
    g.fillRoundedRectangle (bounds, Theme::Radius::sm);
    g.setColour (accent.withAlpha (dragHover ? 0.9f : 0.4f));
    g.drawRoundedRectangle (bounds, Theme::Radius::sm, dragHover ? 1.4f : Theme::Stroke::normal);
    g.setFont (Theme::Font::microMedium());
    g.setColour (ThemeManager::get().theme().controlTextOn);
    g.drawText ("TRASH", getLocalBounds(), juce::Justification::centred, false);
}

void StructurePanel::TrashDropZone::mouseUp (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    if (kind == ListKind::sections)
        owner.deleteSelectedSection();
    else
        owner.deleteSelectedArrangementBlock();
}

bool StructurePanel::TrashDropZone::isInterestedInDragSource (const SourceDetails& dragSourceDetails)
{
    return owner.isDragDescriptionForKind (dragSourceDetails.description, kind);
}

void StructurePanel::TrashDropZone::itemDragEnter (const SourceDetails&)
{
    dragHover = true;
    repaint();
}

void StructurePanel::TrashDropZone::itemDragExit (const SourceDetails&)
{
    dragHover = false;
    repaint();
}

void StructurePanel::TrashDropZone::itemDropped (const SourceDetails& dragSourceDetails)
{
    dragHover = false;
    repaint();

    const int fromIndex = owner.dragIndexFromDescription (dragSourceDetails.description, kind);
    if (fromIndex >= 0)
        owner.handleTrashDrop (kind, fromIndex);
}

void StructurePanel::resized()
{
    viewport.setBounds (getLocalBounds());
    const int contentHeight = 1120;
    content.setSize (juce::jmax (viewport.getWidth() - 10, 200), contentHeight);
    layoutContent (content.getLocalBounds());
}

void StructurePanel::paintContent (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);
    const auto accent = ZoneAStyle::accentForTabId ("structure");
    const int headerH = ZoneAStyle::compactHeaderHeight();
    const int cardPad = ZoneAControlStyle::compactGap() + 2;
    ZoneAStyle::drawHeader (g, { 0, 0, content.getWidth(), headerH }, "STRUCTURE", accent);
    const auto sectionCard = sectionListTitle.getBounds()
                               .getUnion (sectionList.getBounds())
                               .getUnion (addSectionButton.getBounds())
                               .getUnion (moveSectionDownButton.getBounds());
    const auto arrangementCard = arrangementTitle.getBounds()
                                   .getUnion (arrangementList.getBounds())
                                   .getUnion (addArrangementButton.getBounds())
                                   .getUnion (moveArrangementDownButton.getBounds());
    ZoneAStyle::drawCard (g, sectionCard.expanded (cardPad, headerH + 6), accent);
    ZoneAStyle::drawCard (g, arrangementCard.expanded (cardPad, headerH + 6), Theme::Colour::accentWarm);
    ZoneAStyle::drawCard (g, editorHeader.getBounds().getUnion (harmonicCenterBox.getBounds()).expanded (cardPad, headerH), accent);
    ZoneAStyle::drawCard (g, progressionHeader.getBounds().getUnion (progressionStrip.getBounds()).expanded (cardPad, headerH + 8), Theme::Colour::accentWarm);
    ZoneAStyle::drawCard (g, summaryHeader.getBounds().getUnion (summaryDuration.getBounds()).expanded (cardPad, headerH), accent);
}

void StructurePanel::layoutContent (juce::Rectangle<int> bounds)
{
    const int gap = ZoneAControlStyle::compactGap();
    const int labelH = ZoneAControlStyle::sectionHeaderHeight() - 2;
    const int rowH = ZoneAControlStyle::controlHeight();
    juce::ignoreUnused (ZoneAControlStyle::controlBarHeight());
    const int headerH = ZoneAStyle::compactHeaderHeight();
    const int groupHeaderH = ZoneAStyle::compactGroupHeaderHeight();

    sectionList.setRowHeight (ZoneAStyle::compactRowHeight());
    arrangementList.setRowHeight (ZoneAStyle::compactRowHeight());

    bounds = bounds.reduced (gap + 4).withTrimmedTop (headerH);
    auto top = bounds.removeFromTop (rowH);
    seedRailsButton.setBounds ({});
    structureFollowToggle.setBounds (top.removeFromRight (88));

    bounds.removeFromTop (gap + 1);

    const auto layoutThreeButtons = [gap] (juce::Rectangle<int> area,
                                           juce::Component& b1,
                                           juce::Component& b2,
                                           juce::Component& b3)
    {
        const int usableW = area.getWidth() - gap * 2;
        const int btnW = juce::jmax (20, usableW / 3);
        b1.setBounds (area.removeFromLeft (btnW));
        area.removeFromLeft (gap);
        b2.setBounds (area.removeFromLeft (btnW));
        area.removeFromLeft (gap);
        b3.setBounds (area.removeFromLeft (btnW));
    };

    const auto layoutTwoButtons = [gap] (juce::Rectangle<int> area,
                                         juce::Component& b1,
                                         juce::Component& b2)
    {
        const int usableW = area.getWidth() - gap;
        const int btnW = juce::jmax (20, usableW / 2);
        b1.setBounds (area.removeFromLeft (btnW));
        area.removeFromLeft (gap);
        b2.setBounds (area.removeFromLeft (btnW));
    };

    const auto layoutLibraryCard = [&] (juce::Rectangle<int> card,
                                        juce::Label& titleLabel,
                                        juce::Label& hintLabel,
                                        StructureListBox& list,
                                        juce::Component* emptyHint,
                                        juce::Component& addButton,
                                        juce::Component& duplicateButton,
                                        juce::Component& deleteButton,
                                        juce::Component& moveUpButton,
                                        juce::Component& moveDownButton)
    {
        titleLabel.setBounds (card.removeFromTop (groupHeaderH));
        hintLabel.setBounds (card.removeFromTop (labelH));
        card.removeFromTop (juce::jmax (1, gap / 2));

        auto topButtons = card.removeFromTop (rowH);
        layoutThreeButtons (topButtons, addButton, duplicateButton, deleteButton);

        card.removeFromTop (juce::jmax (1, gap / 2));

        auto lowerButtons = card.removeFromTop (rowH);
        layoutTwoButtons (lowerButtons, moveUpButton, moveDownButton);

        card.removeFromTop (gap);
        list.setBounds (card);

        if (emptyHint != nullptr)
            emptyHint->setBounds (list.getBounds().reduced (10, 8));
    };

    const int libraryCardH = 196;
    const bool stackLibraryCards = bounds.getWidth() < 560;

    if (stackLibraryCards)
    {
        auto sectionCard = bounds.removeFromTop (libraryCardH);
        layoutLibraryCard (sectionCard,
                           sectionListTitle,
                           sectionListHint,
                           sectionList,
                           nullptr,
                           addSectionButton,
                           duplicateSectionButton,
                           deleteSectionButton,
                           moveSectionUpButton,
                           moveSectionDownButton);

        bounds.removeFromTop (gap + 2);

        auto arrangementCard = bounds.removeFromTop (libraryCardH);
        layoutLibraryCard (arrangementCard,
                           arrangementTitle,
                           arrangementFlowHint,
                           arrangementList,
                           &arrangementHint,
                           addArrangementButton,
                           duplicateArrangementButton,
                           deleteArrangementButton,
                           moveArrangementUpButton,
                           moveArrangementDownButton);
    }
    else
    {
        auto libraryBand = bounds.removeFromTop (libraryCardH);
        const int leftW = juce::jmax (180, (libraryBand.getWidth() - gap) / 2);
        auto sectionCard = libraryBand.removeFromLeft (leftW);
        libraryBand.removeFromLeft (gap);
        auto arrangementCard = libraryBand;

        layoutLibraryCard (sectionCard,
                           sectionListTitle,
                           sectionListHint,
                           sectionList,
                           nullptr,
                           addSectionButton,
                           duplicateSectionButton,
                           deleteSectionButton,
                           moveSectionUpButton,
                           moveSectionDownButton);

        layoutLibraryCard (arrangementCard,
                           arrangementTitle,
                           arrangementFlowHint,
                           arrangementList,
                           &arrangementHint,
                           addArrangementButton,
                           duplicateArrangementButton,
                           deleteArrangementButton,
                           moveArrangementUpButton,
                           moveArrangementDownButton);
    }

    if (inlineRenameRow >= 0)
    {
        auto rowBounds = sectionList.getRowPosition (inlineRenameRow, true).reduced (2, 1);
        rowBounds = rowBounds.translated (sectionList.getX(), sectionList.getY());
        inlineSectionNameEditor.setBounds (rowBounds.constrainedWithin (content.getLocalBounds().reduced (4)));
        inlineSectionNameEditor.toFront (false);
    }

    bounds.removeFromTop (gap + 3);

    editorHeader.setBounds (bounds.removeFromTop (groupHeaderH));
    editorHint.setBounds (bounds.removeFromTop (labelH));
    auto nameRow = bounds.removeFromTop (rowH);
    nameLabel.setText ("Name", juce::dontSendNotification);
    nameLabel.setBounds (nameRow.removeFromLeft (42));
    nameEditor.setBounds (nameRow);
    barsLabel.setBounds ({});
    barsSlider.setBounds ({});
    bounds.removeFromTop (gap);

    auto timingLabels = bounds.removeFromTop (labelH);
    const int timingGap = gap + 2;
    const int timingW = juce::jmax (58, (timingLabels.getWidth() - timingGap * 2) / 3);
    repeatsLabel.setBounds (timingLabels.removeFromLeft (timingW));
    timingLabels.removeFromLeft (timingGap);
    transitionLabel.setBounds (timingLabels.removeFromLeft (timingW));
    timingLabels.removeFromLeft (timingGap);
    beatsLabel.setBounds (timingLabels);

    auto timingRow = bounds.removeFromTop (rowH);
    repeatsSlider.setBounds (timingRow.removeFromLeft (timingW));
    timingRow.removeFromLeft (timingGap);
    transitionIntentBox.setBounds (timingRow.removeFromLeft (timingW));
    timingRow.removeFromLeft (timingGap);
    beatsPerBarBox.setBounds (timingRow);

    bounds.removeFromTop (gap);

    auto tonalLabels1 = bounds.removeFromTop (labelH);
    const int tonalGap = gap + 2;
    const int tonalW = juce::jmax (72, (tonalLabels1.getWidth() - tonalGap * 2) / 3);
    modeLabel.setText ("Mode", juce::dontSendNotification);
    keyLabel.setText ("Key", juce::dontSendNotification);
    centerLabel.setText ("Root", juce::dontSendNotification);
    modeLabel.setBounds (tonalLabels1.removeFromLeft (tonalW));
    tonalLabels1.removeFromLeft (tonalGap);
    keyLabel.setBounds (tonalLabels1.removeFromLeft (tonalW));
    tonalLabels1.removeFromLeft (tonalGap);
    centerLabel.setBounds (tonalLabels1);
    auto tonalRow1 = bounds.removeFromTop (rowH);
    modeBox.setBounds (tonalRow1.removeFromLeft (tonalW));
    tonalRow1.removeFromLeft (tonalGap);
    keyRootBox.setBounds (tonalRow1.removeFromLeft (tonalW));
    tonalRow1.removeFromLeft (tonalGap);
    harmonicCenterBox.setBounds (tonalRow1);

    bounds.removeFromTop (gap + 4);

    progressionHeader.setBounds (bounds.removeFromTop (groupHeaderH));
    progressionHint.setBounds (bounds.removeFromTop (labelH));
    progressionPaletteHint.setBounds (bounds.removeFromTop (labelH * 2));
    bounds.removeFromTop (gap);
    auto paletteArea = bounds.removeFromTop (152);
    diatonicPaletteStrip.setBounds (paletteArea);
    bounds.removeFromTop (gap + 2);
    auto gridArea = bounds.removeFromTop (210);
    progressionStrip.setBounds (gridArea);
    bounds.removeFromTop (gap + 2);

    auto actionRow = bounds.removeFromTop (rowH);
    const int actionGap = gap;
    const int actionW = juce::jmax (42, (actionRow.getWidth() - actionGap * 3) / 4);
    addChordCellButton.setBounds (actionRow.removeFromLeft (actionW));
    actionRow.removeFromLeft (actionGap);
    removeChordCellButton.setBounds (actionRow.removeFromLeft (actionW));
    actionRow.removeFromLeft (actionGap);
    progressionPresetButton.setBounds (actionRow.removeFromLeft (actionW));
    actionRow.removeFromLeft (actionGap);
    cadenceButton.setBounds (actionRow.removeFromLeft (actionW));

    bounds.removeFromTop (gap);
    auto chordLabelsRow = bounds.removeFromTop (labelH);
    chordRootLabel.setBounds (chordLabelsRow.removeFromLeft (juce::jmax (56, chordLabelsRow.getWidth() / 2 - gap / 2)));
    chordLabelsRow.removeFromLeft (gap);
    chordTypeLabel.setBounds (chordLabelsRow);
    auto chordRow = bounds.removeFromTop (rowH);
    chordRootBox.setBounds (chordRow.removeFromLeft (juce::jmax (56, chordRow.getWidth() / 2 - gap / 2)));
    chordRow.removeFromLeft (gap);
    chordTypeBox.setBounds (chordRow);

    bounds.removeFromTop (gap + 4);
    summaryHeader.setBounds (bounds.removeFromTop (groupHeaderH));
    summaryTempo.setBounds (bounds.removeFromTop (groupHeaderH));
    summaryKey.setBounds (bounds.removeFromTop (groupHeaderH));
    summaryMode.setBounds (bounds.removeFromTop (groupHeaderH));
    summaryBars.setBounds (bounds.removeFromTop (groupHeaderH));
    summaryDuration.setBounds (bounds.removeFromTop (groupHeaderH));
}

void StructurePanel::setIntentKeyMode (const juce::String& keyRoot, const juce::String& mode) { processorRef.getSongManager().setKeyRoot (keyRoot); processorRef.getSongManager().setKeyScale (mode); processorRef.syncAuthoredSongToRuntime(); refreshSummary(); }
int StructurePanel::SectionListModel::getNumRows() { return static_cast<int> (owner.processorRef.getSongManager().getStructureState().sections.size()); }
void StructurePanel::SectionListModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) { const auto& sections = owner.processorRef.getSongManager().getStructureState().sections; if (row < 0 || row >= static_cast<int> (sections.size())) return; const auto& s = sections[(size_t) row]; const auto accent = sectionColourForIndex (row); ZoneAStyle::drawRowBackground (g, { 0, 0, w, h }, false, selected, accent); auto area = juce::Rectangle<int> (0, 0, w, h).reduced (8, 3); auto badge = area.removeFromLeft (24).reduced (0, 3); ZoneAStyle::drawBadge (g, badge, "S" + juce::String (row + 1), accent); area.removeFromLeft (6); auto stats = area.removeFromRight (66); g.setColour (Theme::Colour::inkLight); g.setFont (Theme::Font::label()); g.drawText (s.name, area.removeFromTop (15), juce::Justification::centredLeft, true); g.setColour (Theme::Colour::inkGhost.withAlpha (0.92f)); g.setFont (Theme::Font::micro()); g.drawText (juce::String (juce::jmax (1, s.bars)) + " bars  |  " + juce::String ((int) s.progression.size()) + " cells", area, juce::Justification::centredLeft, true); g.setColour (accent.withAlpha (0.9f)); g.setFont (Theme::Font::micro()); g.drawText ("drag", stats.removeFromTop (10), juce::Justification::centredRight, false); g.setColour (Theme::Colour::inkGhost); g.drawText (s.keyRoot + " " + s.mode, stats, juce::Justification::centredRight, true); }
void StructurePanel::SectionListModel::selectedRowsChanged (int row) { owner.selectSection (row); }
void StructurePanel::SectionListModel::listBoxItemDoubleClicked (int row, const juce::MouseEvent&) { owner.beginInlineSectionRename (row); }
int StructurePanel::ArrangementListModel::getNumRows() { return static_cast<int> (buildResolvedStructure (owner.processorRef.getSongManager().getStructureState()).size()); }
void StructurePanel::ArrangementListModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) { const auto resolved = buildResolvedStructure (owner.processorRef.getSongManager().getStructureState()); if (row < 0 || row >= static_cast<int> (resolved.size()) || resolved[(size_t) row].section == nullptr) return; const auto& r = resolved[(size_t) row]; const auto accent = sectionColourForIndex (row); ZoneAStyle::drawRowBackground (g, { 0, 0, w, h }, false, selected, accent); auto area = juce::Rectangle<int> (0, 0, w, h).reduced (8, 3); auto orderBox = area.removeFromLeft (30).reduced (0, 3); g.setColour (accent.withAlpha (selected ? 0.92f : 0.72f)); g.fillRoundedRectangle (orderBox.toFloat(), Theme::Radius::sm); g.setColour (Theme::Helper::inkFor (accent)); g.setFont (Theme::Font::microMedium()); g.drawText (juce::String (row + 1), orderBox, juce::Justification::centred, false); area.removeFromLeft (6); auto rightMeta = area.removeFromRight (86); g.setColour (Theme::Colour::inkLight); g.setFont (Theme::Font::label()); g.drawText (r.section->name, area.removeFromTop (15), juce::Justification::centredLeft, true); g.setColour (Theme::Colour::inkGhost.withAlpha (0.92f)); g.setFont (Theme::Font::micro()); g.drawText ("instance  |  " + juce::String (juce::jmax (1, r.totalBars)) + " bars", area, juce::Justification::centredLeft, true); g.setColour (accent.withAlpha (0.95f)); g.drawText ("from " + r.section->name.substring (0, juce::jmin (4, r.section->name.length())).toUpperCase(), rightMeta.removeFromTop (10), juce::Justification::centredRight, true); g.setColour (Theme::Colour::inkGhost); g.drawText (r.transitionIntent.isNotEmpty() ? r.transitionIntent : "None", rightMeta, juce::Justification::centredRight, true); }
void StructurePanel::ArrangementListModel::selectedRowsChanged (int row) { owner.selectArrangementBlock (row); }

void StructurePanel::ProgressionStrip::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1.darker (0.08f));
    const auto* s = owner.getSelectedSection();
    if (s == nullptr) { g.setColour (Theme::Colour::inkGhost); g.setFont (Theme::Font::micro()); g.drawText ("Select a section to define looping harmonic cells.", getLocalBounds().reduced (8), juce::Justification::centred, true); return; }

    const auto grid = owner.progressionGridBounds();
    g.setColour (Theme::Colour::surface1.withAlpha (0.98f));
    g.fillRoundedRectangle (grid, Theme::Radius::lg);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.45f));
    g.drawRoundedRectangle (grid, Theme::Radius::lg, Theme::Stroke::normal);

    const int loopBeats = limitedProgressionBeats (*s);
    const int totalSectionBeats = loopBeats * juce::jmax (1, s->repeats);
    const int beatsPerBar = juce::jmax (1, s->beatsPerBar);

    if (s->progression.empty())
    {
        g.setColour (Theme::Colour::inkGhost);
        g.setFont (Theme::Font::micro());
        g.drawText ("No chord cells yet. Add one beat, then drag right to sustain it.", grid.toNearestInt().reduced (18), juce::Justification::centred, true);
        return;
    }

    for (int beat = 0; beat < kProgressionGridSteps; ++beat)
    {
        auto cell = owner.progressionChipBounds (beat);
        const int chordIndex = beat < loopBeats ? owner.chordIndexForBeat (beat) : -1;
        const bool filled = chordIndex >= 0;
        const bool selected = filled && chordIndex == owner.selectedChordIndex;
        const bool inLoop = beat < loopBeats;
        const bool barLine = inLoop && ((beat % beatsPerBar) == 0);
        const auto tint = filled ? sectionColourForIndex (chordIndex) : Theme::Colour::surface2;
        const auto chipFill = filled
                                ? Theme::Colour::surface2.interpolatedWith (tint, selected ? 0.52f : 0.28f)
                                : (inLoop ? Theme::Colour::surface2.brighter (0.04f)
                                          : Theme::Colour::surface1.withAlpha (0.28f));

        g.setColour (chipFill);
        g.fillRoundedRectangle (cell, Theme::Radius::sm);

        g.setColour (selected ? tint.withAlpha (0.95f)
                              : (barLine ? Theme::Colour::surfaceEdge.withAlpha (0.85f)
                                         : Theme::Colour::surfaceEdge.withAlpha (0.42f)));
        g.drawRoundedRectangle (cell, Theme::Radius::sm, selected ? 1.8f : (barLine ? 1.25f : Theme::Stroke::subtle));

        if (! filled || owner.chordStartBeat (chordIndex) != beat)
            continue;

        auto text = cell.toNearestInt().reduced (3, 3);
        g.setColour (selected ? Theme::Colour::inkLight : Theme::Colour::inkLight);
        g.setFont (Theme::Font::microMedium());
        g.drawFittedText (chordLabel (s->progression[(size_t) chordIndex]), text.removeFromTop (12), juce::Justification::centred, 1);
        g.setFont (Theme::Font::micro());
        g.setColour (selected ? Theme::Colour::inkLight.withAlpha (0.8f) : Theme::Colour::inkGhost);
        g.drawText (juce::String (juce::jmax (1, s->progression[(size_t) chordIndex].durationBeats)) + "b", text, juce::Justification::centred, false);
    }

    g.setColour (Theme::Colour::inkLight);
    g.setFont (Theme::Font::label());
    g.drawText ("8 x 8 beat grid", grid.toNearestInt().removeFromTop (18).translated (0, -24), juce::Justification::centred, false);
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("Loop ends at beat " + juce::String (loopBeats) + "  |  " + juce::String (s->progression.size()) + " chord cells  |  section " + juce::String (totalSectionBeats) + " beats",
                grid.toNearestInt().removeFromBottom (20).translated (0, 24),
                juce::Justification::centred,
                true);
}

void StructurePanel::ProgressionStrip::mouseDown (const juce::MouseEvent& e)
{
    const auto* s = owner.getSelectedSection();
    dragCellIndex = -1;
    resizingDuration = false;
    dragStartDuration = 0;

    if (s == nullptr || s->progression.empty())
        return;

    const int beat = owner.progressionBeatAtPoint (e.getPosition());
    const int chordIndex = owner.chordIndexForBeat (beat);
    if (e.mods.isPopupMenu())
    {
        if (chordIndex >= 0)
        {
            owner.selectChordCell (chordIndex);
            owner.showChordCellMenu (chordIndex, e.getPosition());
        }
        return;
    }

    if (chordIndex < 0)
        return;

    owner.selectChordCell (chordIndex);
    dragCellIndex = chordIndex;
    dragStartDuration = s->progression[(size_t) chordIndex].durationBeats;
    dragBeforeState = owner.processorRef.getSongManager().getStructureState();
    resizingDuration = e.mods.isLeftButtonDown();
}

void StructurePanel::ProgressionStrip::mouseDrag (const juce::MouseEvent& e)
{
    auto* s = owner.getSelectedSection();
    if (! resizingDuration || s == nullptr || dragCellIndex < 0 || dragCellIndex >= static_cast<int> (s->progression.size()))
        return;

    const int beat = owner.progressionBeatAtPoint (e.getPosition());
    if (beat < 0)
        return;

    const int startBeat = owner.chordStartBeat (dragCellIndex);
    const int newDuration = juce::jlimit (1, kProgressionGridSteps - startBeat, beat - startBeat + 1);
    auto& chord = s->progression[(size_t) dragCellIndex];
    if (chord.durationBeats == newDuration)
        return;

    chord.durationBeats = newDuration;
    owner.updateSectionDerivedLength (*s);
    owner.selectChordCell (dragCellIndex);
    owner.refreshSummary();
    repaint();
}

void StructurePanel::ProgressionStrip::mouseUp (const juce::MouseEvent& e)
{
    auto* s = owner.getSelectedSection();
    const int beat = owner.progressionBeatAtPoint (e.getPosition());
    const int chordIndex = owner.chordIndexForBeat (beat);
    const bool changedDuration = resizingDuration && s != nullptr && dragCellIndex >= 0
                              && dragCellIndex < static_cast<int> (s->progression.size())
                              && s->progression[(size_t) dragCellIndex].durationBeats != dragStartDuration;

    if (chordIndex >= 0)
        owner.selectChordCell (chordIndex);

    if (changedDuration)
    {
        owner.commitStructureChange (dragBeforeState, "Resize Chord Cell", false);
        owner.refreshEditor();
    }

    dragCellIndex = -1;
    resizingDuration = false;
    dragStartDuration = 0;
}

void StructurePanel::ProgressionStrip::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto* s = owner.getSelectedSection();
    if (s == nullptr || s->progression.empty())
        return;

    const int beat = owner.progressionBeatAtPoint (e.getPosition());
    const int chordIndex = owner.chordIndexForBeat (beat);
    if (chordIndex >= 0)
    {
        owner.selectChordCell (chordIndex);
        owner.showChordEditMenu (chordIndex, e.getPosition());
    }
}

void StructurePanel::DiatonicPaletteStrip::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1.darker (0.10f));

    const auto* section = owner.getSelectedSection();
    if (section == nullptr)
        return;

    const auto keyRoot = section->keyRoot.isNotEmpty() ? section->keyRoot : juce::String ("C");
    const auto mode = section->mode.isNotEmpty() ? section->mode : juce::String ("Major");
    const auto palette = DiatonicHarmony::buildPalette (keyRoot, mode);
    if (palette.empty())
        return;

    auto area = getLocalBounds().reduced (8, 6).toFloat();
    const float radius = juce::jmin (28.0f, juce::jmax (18.0f, juce::jmin (area.getWidth() / 6.2f, area.getHeight() / 4.1f)));
    const float stepX = radius * 1.52f;
    const float stepY = radius * 0.88f;
    const auto center = juce::Point<float> (area.getCentreX(), area.getCentreY());
    const std::array<juce::Point<float>, 7> centers =
    {{
        center,
        { center.x, center.y - radius * 1.74f },
        { center.x + stepX, center.y - stepY },
        { center.x + stepX, center.y + stepY },
        { center.x, center.y + radius * 1.74f },
        { center.x - stepX, center.y + stepY },
        { center.x - stepX, center.y - stepY }
    }};
    const auto selectedChord = owner.getSelectedChordCell();

    for (int i = 0; i < 7; ++i)
    {
        const auto& chord = palette[(size_t) i];
        const bool matchesSelected = selectedChord != nullptr
                                  && selectedChord->root == chord.root
                                  && selectedChord->type == chord.type;
        const auto tint = sectionColourForIndex (i);
        const auto path = hexagonPath (centers[(size_t) i], radius);
        const auto baseFill = Theme::Colour::surface2.interpolatedWith (tint, matchesSelected ? 0.34f : 0.14f);
        const auto fill = baseFill.withAlpha (0.96f);
        const auto edge = matchesSelected ? tint.withAlpha (0.95f)
                                          : Theme::Colour::surfaceEdge.interpolatedWith (tint, 0.35f).withAlpha (0.9f);

        auto shadow = path;
        shadow.applyTransform (juce::AffineTransform::translation (0.0f, 2.0f));
        g.setColour (juce::Colours::black.withAlpha (0.16f));
        g.fillPath (shadow);

        g.setColour (fill);
        g.fillPath (path);
        g.setColour (edge);
        g.strokePath (path, juce::PathStrokeType (matchesSelected ? 2.2f : 1.35f));

        auto text = path.getBounds().toNearestInt().reduced (4, 6);
        g.setColour (Theme::Colour::inkLight.withAlpha (0.98f));
        g.setFont (Theme::Font::microMedium());
        g.drawFittedText (abbreviatedChordLabel (chord.root, chord.type), text.removeFromTop (16), juce::Justification::centred, 1);
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.95f));
        g.drawText (chord.roman, text.removeFromTop (12), juce::Justification::centred, false);
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.72f));
        g.drawFittedText (i == 0 ? "Key" : chord.function, text, juce::Justification::centred, 1);
    }
}

void StructurePanel::DiatonicPaletteStrip::mouseUp (const juce::MouseEvent& event)
{
    const auto* section = owner.getSelectedSection();
    if (section == nullptr)
        return;

    auto area = getLocalBounds().reduced (8, 6).toFloat();
    const float radius = juce::jmin (28.0f, juce::jmax (18.0f, juce::jmin (area.getWidth() / 6.2f, area.getHeight() / 4.1f)));
    const float stepX = radius * 1.52f;
    const float stepY = radius * 0.88f;
    const auto center = juce::Point<float> (area.getCentreX(), area.getCentreY());
    const std::array<juce::Point<float>, 7> centers =
    {{
        center,
        { center.x, center.y - radius * 1.74f },
        { center.x + stepX, center.y - stepY },
        { center.x + stepX, center.y + stepY },
        { center.x, center.y + radius * 1.74f },
        { center.x - stepX, center.y + stepY },
        { center.x - stepX, center.y - stepY }
    }};
    for (int i = 0; i < 7; ++i)
    {
        if (hexagonPath (centers[(size_t) i], radius).contains ((float) event.x, (float) event.y))
        {
            owner.applyDiatonicPaletteChord (i, event.mods.isPopupMenu());
            return;
        }
    }
}

Section* StructurePanel::getSelectedSection() { auto& s = processorRef.getSongManager().getStructureStateForEdit(); if (selectionTarget == SelectionTarget::section && selectedSectionIndex >= 0 && selectedSectionIndex < static_cast<int> (s.sections.size())) return &s.sections[(size_t) selectedSectionIndex]; if (selectionTarget == SelectionTarget::arrangementBlock) { const auto resolved = buildResolvedStructure (s); if (selectedArrangementIndex >= 0 && selectedArrangementIndex < static_cast<int> (resolved.size())) return const_cast<Section*> (resolved[(size_t) selectedArrangementIndex].section); } return nullptr; }
const Section* StructurePanel::getSelectedSection() const { return const_cast<StructurePanel*> (this)->getSelectedSection(); }
ArrangementBlock* StructurePanel::getSelectedArrangementBlock() { auto resolved = buildResolvedStructure (processorRef.getSongManager().getStructureState()); if (selectedArrangementIndex < 0 || selectedArrangementIndex >= static_cast<int> (resolved.size())) return nullptr; return const_cast<ArrangementBlock*> (resolved[(size_t) selectedArrangementIndex].block); }
const ArrangementBlock* StructurePanel::getSelectedArrangementBlock() const { return const_cast<StructurePanel*> (this)->getSelectedArrangementBlock(); }
Chord* StructurePanel::getSelectedChordCell() { auto* s = getSelectedSection(); if (s == nullptr || selectedChordIndex < 0 || selectedChordIndex >= static_cast<int> (s->progression.size())) return nullptr; return &s->progression[(size_t) selectedChordIndex]; }
const Chord* StructurePanel::getSelectedChordCell() const { return const_cast<StructurePanel*> (this)->getSelectedChordCell(); }

void StructurePanel::addSection() { const auto beforeState = processorRef.getSongManager().getStructureState(); auto& st = processorRef.getSongManager().getStructureStateForEdit(); Section s; s.id = makeStructureId(); s.name = "Section " + juce::String (st.sections.size() + 1); s.bars = 1; s.beatsPerBar = 4; s.repeats = 1; s.keyRoot = st.songKey; s.mode = st.songMode; s.harmonicCenter = "I"; s.transitionIntent = "None"; s.progression = { DiatonicHarmony::buildChordForCenter (s.keyRoot, s.mode, s.harmonicCenter) }; updateSectionDerivedLength (s); st.sections.push_back (s); selectionTarget = SelectionTarget::section; selectedSectionIndex = static_cast<int> (st.sections.size() - 1); selectedArrangementIndex = -1; selectedChordIndex = 0; commitStructureChange (beforeState, "Add Section", false); refreshLists(); refreshEditor(); }
void StructurePanel::duplicateSelectedSection() { const auto* src = getSelectedSection(); if (src == nullptr) return; const auto beforeState = processorRef.getSongManager().getStructureState(); auto copy = *src; copy.id = makeStructureId(); copy.name = src->name + " Copy"; auto& sections = processorRef.getSongManager().getStructureStateForEdit().sections; sections.push_back (copy); selectionTarget = SelectionTarget::section; selectedSectionIndex = static_cast<int> (sections.size() - 1); selectedArrangementIndex = -1; selectedChordIndex = 0; commitStructureChange (beforeState, "Duplicate Section", false); refreshLists(); refreshEditor(); }
void StructurePanel::deleteSelectedSection() { const auto beforeState = processorRef.getSongManager().getStructureState(); auto& st = processorRef.getSongManager().getStructureStateForEdit(); if (selectedSectionIndex < 0 || selectedSectionIndex >= static_cast<int> (st.sections.size())) return; const auto removedId = st.sections[(size_t) selectedSectionIndex].id; st.sections.erase (st.sections.begin() + selectedSectionIndex); std::erase_if (st.arrangement, [&] (const ArrangementBlock& b) { return b.sectionId == removedId; }); resequenceArrangement(); clearSelection(); commitStructureChange (beforeState, "Delete Section", false); refreshLists(); refreshEditor(); }
void StructurePanel::moveSelectedSection (int delta) { const auto beforeState = processorRef.getSongManager().getStructureState(); auto& sections = processorRef.getSongManager().getStructureStateForEdit().sections; if (selectedSectionIndex < 0 || selectedSectionIndex >= static_cast<int> (sections.size())) return; const int target = juce::jlimit (0, static_cast<int> (sections.size() - 1), selectedSectionIndex + delta); if (target == selectedSectionIndex) return; std::iter_swap (sections.begin() + selectedSectionIndex, sections.begin() + target); selectedSectionIndex = target; commitStructureChange (beforeState, "Reorder Section", false); refreshLists(); }
void StructurePanel::moveSectionTo (int fromIndex, int toIndex)
{
    const auto beforeState = processorRef.getSongManager().getStructureState();
    auto& sections = processorRef.getSongManager().getStructureStateForEdit().sections;
    if (fromIndex < 0 || fromIndex >= static_cast<int> (sections.size()))
        return;

    toIndex = juce::jlimit (0, static_cast<int> (sections.size()), toIndex);
    if (toIndex > fromIndex)
        --toIndex;

    if (toIndex == fromIndex)
        return;

    auto moved = sections[(size_t) fromIndex];
    sections.erase (sections.begin() + fromIndex);
    sections.insert (sections.begin() + toIndex, moved);
    selectionTarget = SelectionTarget::section;
    selectedSectionIndex = toIndex;
    selectedArrangementIndex = -1;
    commitStructureChange (beforeState, "Reorder Section", false);
    refreshLists();
    refreshEditor();
}

void StructurePanel::deleteSectionAt (int index)
{
    selectionTarget = SelectionTarget::section;
    selectedSectionIndex = index;
    deleteSelectedSection();
}
void StructurePanel::addArrangementBlock() { const auto beforeState = processorRef.getSongManager().getStructureState(); auto& st = processorRef.getSongManager().getStructureStateForEdit(); if (st.sections.empty()) { addSection(); return; } const int source = juce::jlimit (0, static_cast<int> (st.sections.size() - 1), selectedSectionIndex >= 0 ? selectedSectionIndex : 0); ArrangementBlock b; b.id = makeStructureId(); b.sectionId = st.sections[(size_t) source].id; b.orderIndex = static_cast<int> (st.arrangement.size()); st.arrangement.push_back (b); selectionTarget = SelectionTarget::arrangementBlock; selectedArrangementIndex = static_cast<int> (st.arrangement.size() - 1); selectedChordIndex = 0; commitStructureChange (beforeState, "Add Arrangement Block", false); refreshLists(); refreshEditor(); }
void StructurePanel::duplicateSelectedArrangementBlock() { const auto beforeState = processorRef.getSongManager().getStructureState(); auto& a = processorRef.getSongManager().getStructureStateForEdit().arrangement; if (selectedArrangementIndex < 0 || selectedArrangementIndex >= static_cast<int> (a.size())) return; auto copy = a[(size_t) selectedArrangementIndex]; copy.id = makeStructureId(); a.insert (a.begin() + selectedArrangementIndex + 1, copy); selectedArrangementIndex += 1; resequenceArrangement(); selectionTarget = SelectionTarget::arrangementBlock; commitStructureChange (beforeState, "Duplicate Arrangement Block", false); refreshLists(); }
void StructurePanel::deleteSelectedArrangementBlock() { const auto beforeState = processorRef.getSongManager().getStructureState(); auto& a = processorRef.getSongManager().getStructureStateForEdit().arrangement; if (selectedArrangementIndex < 0 || selectedArrangementIndex >= static_cast<int> (a.size())) return; a.erase (a.begin() + selectedArrangementIndex); resequenceArrangement(); clearSelection(); commitStructureChange (beforeState, "Delete Arrangement Block", false); refreshLists(); refreshEditor(); }
void StructurePanel::moveSelectedArrangementBlock (int delta) { const auto beforeState = processorRef.getSongManager().getStructureState(); auto& a = processorRef.getSongManager().getStructureStateForEdit().arrangement; if (selectedArrangementIndex < 0 || selectedArrangementIndex >= static_cast<int> (a.size())) return; const int target = juce::jlimit (0, static_cast<int> (a.size() - 1), selectedArrangementIndex + delta); if (target == selectedArrangementIndex) return; std::iter_swap (a.begin() + selectedArrangementIndex, a.begin() + target); selectedArrangementIndex = target; resequenceArrangement(); selectionTarget = SelectionTarget::arrangementBlock; commitStructureChange (beforeState, "Reorder Arrangement Block", false); refreshLists(); }
void StructurePanel::moveArrangementBlockTo (int fromIndex, int toIndex)
{
    const auto beforeState = processorRef.getSongManager().getStructureState();
    auto& arrangement = processorRef.getSongManager().getStructureStateForEdit().arrangement;
    if (fromIndex < 0 || fromIndex >= static_cast<int> (arrangement.size()))
        return;

    toIndex = juce::jlimit (0, static_cast<int> (arrangement.size()), toIndex);
    if (toIndex > fromIndex)
        --toIndex;

    if (toIndex == fromIndex)
        return;

    auto moved = arrangement[(size_t) fromIndex];
    arrangement.erase (arrangement.begin() + fromIndex);
    arrangement.insert (arrangement.begin() + toIndex, moved);
    resequenceArrangement();
    selectionTarget = SelectionTarget::arrangementBlock;
    selectedArrangementIndex = toIndex;
    selectedSectionIndex = -1;
    commitStructureChange (beforeState, "Reorder Arrangement Block", false);
    refreshLists();
    refreshEditor();
}

void StructurePanel::addArrangementBlockFromSection (int sectionIndex, int targetIndex)
{
    const auto beforeState = processorRef.getSongManager().getStructureState();
    auto& state = processorRef.getSongManager().getStructureStateForEdit();
    if (sectionIndex < 0 || sectionIndex >= static_cast<int> (state.sections.size()))
        return;

    ArrangementBlock block;
    block.id = makeStructureId();
    block.sectionId = state.sections[(size_t) sectionIndex].id;

    targetIndex = juce::jlimit (0, static_cast<int> (state.arrangement.size()), targetIndex);
    state.arrangement.insert (state.arrangement.begin() + targetIndex, block);
    resequenceArrangement();
    selectionTarget = SelectionTarget::arrangementBlock;
    selectedArrangementIndex = targetIndex;
    selectedSectionIndex = -1;
    commitStructureChange (beforeState, "Add Arrangement Block", false);
    refreshLists();
    refreshEditor();
}

void StructurePanel::deleteArrangementBlockAt (int index)
{
    selectionTarget = SelectionTarget::arrangementBlock;
    selectedArrangementIndex = index;
    deleteSelectedArrangementBlock();
}
void StructurePanel::resequenceArrangement() { auto& a = processorRef.getSongManager().getStructureStateForEdit().arrangement; for (int i = 0; i < static_cast<int> (a.size()); ++i) a[(size_t) i].orderIndex = i; }
juce::String StructurePanel::dragDescriptionForSectionIndex (int index) const
{
    const auto& sections = processorRef.getSongManager().getStructureState().sections;
    if (index < 0 || index >= static_cast<int> (sections.size()))
        return {};

    return "section:" + sections[(size_t) index].id + ":" + juce::String (index);
}

juce::String StructurePanel::dragDescriptionForArrangementIndex (int index) const
{
    const auto resolved = buildResolvedStructure (processorRef.getSongManager().getStructureState());
    if (index < 0 || index >= static_cast<int> (resolved.size()) || resolved[(size_t) index].block == nullptr)
        return {};

    return "arrangement:" + resolved[(size_t) index].block->id + ":" + juce::String (index);
}

bool StructurePanel::isDragDescriptionForKind (const juce::var& description, ListKind kind) const
{
    const auto text = description.toString();
    return kind == ListKind::sections ? text.startsWith ("section:") : text.startsWith ("arrangement:");
}

int StructurePanel::dragIndexFromDescription (const juce::var& description, ListKind kind) const
{
    if (! isDragDescriptionForKind (description, kind))
        return -1;

    const auto text = description.toString();
    const auto token = text.fromFirstOccurrenceOf (":", false, false);
    const auto idToken = token.upToFirstOccurrenceOf (":", false, false);
    const bool hasIndexSuffix = token.containsChar (':');
    const int fallbackIndex = hasIndexSuffix ? token.fromFirstOccurrenceOf (":", false, false).getIntValue() : -1;

    if (kind == ListKind::sections)
    {
        const auto& sections = processorRef.getSongManager().getStructureState().sections;
        for (int i = 0; i < static_cast<int> (sections.size()); ++i)
            if (sections[(size_t) i].id == idToken)
                return i;
        return juce::isPositiveAndBelow (fallbackIndex, static_cast<int> (sections.size())) ? fallbackIndex : -1;
    }

    const auto resolved = buildResolvedStructure (processorRef.getSongManager().getStructureState());
    for (int i = 0; i < static_cast<int> (resolved.size()); ++i)
        if (resolved[(size_t) i].block != nullptr && resolved[(size_t) i].block->id == idToken)
            return i;

    return juce::isPositiveAndBelow (fallbackIndex, static_cast<int> (resolved.size())) ? fallbackIndex : -1;
}

void StructurePanel::handleListDrop (ListKind kind, int fromIndex, int toIndex)
{
    if (kind == ListKind::sections)
        moveSectionTo (fromIndex, toIndex);
    else
        moveArrangementBlockTo (fromIndex, toIndex);
}

void StructurePanel::handleTrashDrop (ListKind kind, int fromIndex)
{
    if (kind == ListKind::sections)
        deleteSectionAt (fromIndex);
    else
        deleteArrangementBlockAt (fromIndex);
}
void StructurePanel::addChordCell() { const auto beforeState = processorRef.getSongManager().getStructureState(); if (auto* s = getSelectedSection()) { if (computeSectionLoopBeats (*s) >= kProgressionGridSteps) return; auto chord = DiatonicHarmony::buildChordForCenter (s->keyRoot, s->mode, s->harmonicCenter); chord.durationBeats = 1; s->progression.push_back (chord); updateSectionDerivedLength (*s); selectedChordIndex = static_cast<int> (s->progression.size() - 1); commitStructureChange (beforeState, "Add Chord Cell", false); refreshEditor(); progressionStrip.repaint(); } }
void StructurePanel::removeSelectedChordCell() { const auto beforeState = processorRef.getSongManager().getStructureState(); if (auto* s = getSelectedSection()) { if (selectedChordIndex < 0 || selectedChordIndex >= static_cast<int> (s->progression.size())) return; s->progression.erase (s->progression.begin() + selectedChordIndex); updateSectionDerivedLength (*s); selectedChordIndex = s->progression.empty() ? -1 : juce::jlimit (0, static_cast<int> (s->progression.size()) - 1, selectedChordIndex); commitStructureChange (beforeState, "Remove Chord Cell", false); refreshEditor(); progressionStrip.repaint(); } }
void StructurePanel::selectChordCell (int index) { selectedChordIndex = index; refreshEditor(); progressionStrip.repaint(); }
void StructurePanel::showChordCellMenu (int index, juce::Point<int> anchorInProgressionStrip)
{
    if (index >= 0)
        selectedChordIndex = index;
    if (getSelectedChordCell() == nullptr)
        return;

    juce::PopupMenu rootMenu, typeMenu, menu;
    for (int i = 0; i < 12; ++i)
        rootMenu.addItem (rootBase + i, kRoots[i]);
    for (int i = 0; i < 7; ++i)
        typeMenu.addItem (typeBase + i, kTypes[i]);

    menu.addSubMenu ("Set Root", rootMenu);
    menu.addSubMenu ("Set Quality", typeMenu);
    menu.addSeparator();
    menu.addItem (dupCell, "Duplicate Cell");
    menu.addItem (clearCell, "Clear Cell");
    menu.addItem (splitCell, "Split Cell");
    menu.addSeparator();
    menu.addItem (shortenBeat, "Shorten by Beat");
    menu.addItem (lengthenBeat, "Lengthen by Beat");
    menu.addSeparator();
    menu.addItem (diatonicSub, "Diatonic Substitute");
    menu.addItem (secondaryDom, "Secondary Dominant");
    menu.addItem (borrowedChord, "Borrowed Chord");
    menu.addItem (extendChord, "Extend Chord");
    menu.addSeparator();
    menu.addItem (flattenRoot, "Flatten Root");
    menu.addItem (sharpenRoot, "Sharpen Root");
    menu.addItem (setMaj, "Set Major");
    menu.addItem (setMin, "Set Minor");
    menu.addItem (setDim, "Set Diminished");
    menu.addItem (setSus, "Set Sus4");

    auto options = juce::PopupMenu::Options().withTargetComponent (&progressionStrip);
    if (anchorInProgressionStrip != juce::Point<int>())
    {
        const auto screenAnchor = progressionStrip.localPointToGlobal (anchorInProgressionStrip);
        options = options.withTargetScreenArea (juce::Rectangle<int> (screenAnchor.x, screenAnchor.y, 1, 1));
    }

    menu.showMenuAsync (options,
                        [this] (int result)
                        {
                            if (result != 0)
                                applyTheoryAction (result);
                        });
}

void StructurePanel::showChordEditMenu (int index, juce::Point<int> anchorInProgressionStrip)
{
    if (index < 0)
        return;
    selectedChordIndex = index;

    juce::PopupMenu rootMenu, typeMenu, menu;
    for (int i = 0; i < 12; ++i)
        rootMenu.addItem (rootBase + i, kRoots[i]);
    for (int i = 0; i < 7; ++i)
        typeMenu.addItem (typeBase + i, kTypes[i]);
    menu.addSubMenu ("Root", rootMenu);
    menu.addSubMenu ("Quality", typeMenu);

    auto options = juce::PopupMenu::Options().withTargetComponent (&progressionStrip);
    if (anchorInProgressionStrip != juce::Point<int>())
    {
        const auto screenAnchor = progressionStrip.localPointToGlobal (anchorInProgressionStrip);
        options = options.withTargetScreenArea (juce::Rectangle<int> (screenAnchor.x, screenAnchor.y, 1, 1));
    }

    menu.showMenuAsync (options,
                        [this] (int result)
                        {
                            if (result != 0)
                                applyTheoryAction (result);
                        });
}
void StructurePanel::showProgressionPresetMenu()
{
    if (getSelectedSection() == nullptr)
        return;

    juce::PopupMenu menu;
    int commandId = 1000;
    for (const auto& preset : DiatonicHarmony::progressionPresets())
        menu.addItem (commandId++, preset.name + "  " + preset.summary);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&progressionPresetButton),
                        [this] (int result)
                        {
                            if (result == 0)
                                return;

                            auto* section = getSelectedSection();
                            if (section == nullptr)
                                return;

                            const auto index = result - 1000;
                            const auto& presets = DiatonicHarmony::progressionPresets();
                            if (index < 0 || index >= static_cast<int> (presets.size()))
                                return;

                            const auto beforeState = processorRef.getSongManager().getStructureState();
                            section->progression = DiatonicHarmony::buildProgressionFromTemplate (section->keyRoot, section->mode, presets[(size_t) index]);
                            updateSectionDerivedLength (*section);
                            selectedChordIndex = section->progression.empty() ? -1 : 0;
                            commitStructureChange (beforeState, "Apply Progression Preset", false);
                            refreshEditor();
                            progressionStrip.repaint();
                        });
}

void StructurePanel::showCadenceMenu()
{
    if (getSelectedSection() == nullptr)
        return;

    juce::PopupMenu menu;
    int commandId = 1100;
    for (const auto& cadence : DiatonicHarmony::cadencePresets())
        menu.addItem (commandId++, cadence.name + "  " + cadence.summary);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&cadenceButton),
                        [this] (int result)
                        {
                            if (result == 0)
                                return;

                            auto* section = getSelectedSection();
                            if (section == nullptr)
                                return;

                            const auto index = result - 1100;
                            const auto& cadences = DiatonicHarmony::cadencePresets();
                            if (index < 0 || index >= static_cast<int> (cadences.size()))
                                return;

                            const auto beforeState = processorRef.getSongManager().getStructureState();
                            const auto cadence = DiatonicHarmony::buildProgressionFromTemplate (section->keyRoot, section->mode, cadences[(size_t) index]);
                            if (section->progression.empty())
                                section->progression = cadence;
                            else
                                section->progression.insert (section->progression.end(), cadence.begin(), cadence.end());
                            updateSectionDerivedLength (*section);
                            selectedChordIndex = section->progression.empty() ? -1 : static_cast<int> (section->progression.size() - 1);
                            commitStructureChange (beforeState, "Append Cadence", false);
                            refreshEditor();
                            progressionStrip.repaint();
                        });
}

void StructurePanel::applyDiatonicPaletteChord (int degreeIndex, bool append)
{
    auto* section = getSelectedSection();
    if (section == nullptr)
        return;

    const auto beforeState = processorRef.getSongManager().getStructureState();
    const auto chord = DiatonicHarmony::buildChordForDegree (section->keyRoot, section->mode, degreeIndex);

    if (append || getSelectedChordCell() == nullptr)
    {
        const int insertIndex = selectedChordIndex >= 0 ? selectedChordIndex + 1
                                                        : static_cast<int> (section->progression.size());
        section->progression.insert (section->progression.begin() + juce::jlimit (0, static_cast<int> (section->progression.size()), insertIndex),
                                     chord);
        selectedChordIndex = juce::jlimit (0, static_cast<int> (section->progression.size() - 1), insertIndex);
        updateSectionDerivedLength (*section);
        commitStructureChange (beforeState, append ? "Append Palette Chord" : "Insert Palette Chord", false);
    }
    else if (auto* selected = getSelectedChordCell())
    {
        *selected = chord;
        updateSectionDerivedLength (*section);
        commitStructureChange (beforeState, "Set Palette Chord", false);
    }

    refreshEditor();
    progressionStrip.repaint();
    diatonicPaletteStrip.repaint();
}

void StructurePanel::applyTheoryAction (int cmd)
{
    const auto beforeState = processorRef.getSongManager().getStructureState();
    auto* s = getSelectedSection();
    auto* c = getSelectedChordCell();
    if (s == nullptr || c == nullptr)
        return;

    if (cmd >= rootBase && cmd < rootBase + 12)
        c->root = kRoots[cmd - rootBase];
    else if (cmd >= typeBase && cmd < typeBase + 7)
        c->type = kTypes[cmd - typeBase];
    else if (cmd == dupCell || cmd == splitCell)
    {
        s->progression.insert (s->progression.begin() + selectedChordIndex + 1, *c);
        if (cmd == dupCell)
            selectedChordIndex += 1;
    }
    else if (cmd == clearCell)
        *c = DiatonicHarmony::buildChordForCenter (s->keyRoot, s->mode, s->harmonicCenter);
    else if (cmd == diatonicSub)
        *c = DiatonicHarmony::buildChordForDegree (s->keyRoot, s->mode, (DiatonicHarmony::degreeIndexForRoman (s->harmonicCenter) + selectedChordIndex + 1) % 7);
    else if (cmd == secondaryDom)
    {
        const int next = (selectedChordIndex + 1) % juce::jmax (1, static_cast<int> (s->progression.size()));
        c->root = DiatonicHarmony::rootNameForPitchClass (DiatonicHarmony::pitchClassForRoot (s->progression[(size_t) next].root) + 7);
        c->type = "7";
    }
    else if (cmd == borrowedChord)
        c->type = c->type.startsWithIgnoreCase ("min") ? "maj" : "min";
    else if (cmd == extendChord)
    {
        if (c->type == "maj") c->type = "maj7";
        else if (c->type == "min") c->type = "min7";
        else c->type = "7";
    }
    else if (cmd == shortenBeat)
        c->durationBeats = juce::jmax (1, c->durationBeats - 1);
    else if (cmd == lengthenBeat)
        c->durationBeats = juce::jmin (16, c->durationBeats + 1);
    else if (cmd == flattenRoot)
        c->root = DiatonicHarmony::rootNameForPitchClass (DiatonicHarmony::pitchClassForRoot (c->root) - 1);
    else if (cmd == sharpenRoot)
        c->root = DiatonicHarmony::rootNameForPitchClass (DiatonicHarmony::pitchClassForRoot (c->root) + 1);
    else if (cmd == setMaj)
        c->type = "maj";
    else if (cmd == setMin)
        c->type = "min";
    else if (cmd == setDim)
        c->type = "dim";
    else if (cmd == setSus)
        c->type = "sus4";

    updateSectionDerivedLength (*s);
    commitStructureChange (beforeState, "Edit Progression Cell", false);
    refreshEditor();
    progressionStrip.repaint();
}
void StructurePanel::selectSection (int index) { if (suppressListCallbacks) return; finishInlineSectionRename (true); if (index < 0) { clearSelection(); return; } selectionTarget = SelectionTarget::section; selectedSectionIndex = index; selectedArrangementIndex = -1; selectedChordIndex = 0; suppressListCallbacks = true; arrangementList.deselectAllRows(); suppressListCallbacks = false; refreshEditor(); progressionStrip.repaint(); }
void StructurePanel::selectArrangementBlock (int index) { finishInlineSectionRename (true); if (index < 0) { clearSelection(); return; } selectionTarget = SelectionTarget::arrangementBlock; selectedArrangementIndex = index; selectedSectionIndex = -1; selectedChordIndex = 0; suppressListCallbacks = true; sectionList.deselectAllRows(); suppressListCallbacks = false; refreshEditor(); progressionStrip.repaint(); }
void StructurePanel::clearSelection() { finishInlineSectionRename (true); selectionTarget = SelectionTarget::none; selectedSectionIndex = -1; selectedArrangementIndex = -1; selectedChordIndex = -1; suppressListCallbacks = true; sectionList.deselectAllRows(); arrangementList.deselectAllRows(); suppressListCallbacks = false; refreshEditor(); progressionStrip.repaint(); }

void StructurePanel::beginInlineSectionRename (int index)
{
    if (index < 0)
        return;

    finishInlineSectionRename (true);

    auto& sections = processorRef.getSongManager().getStructureStateForEdit().sections;
    if (index >= static_cast<int> (sections.size()))
        return;

    sectionList.selectRow (index, false, true);
    inlineRenameRow = index;
    inlineSectionNameEditor.setText (sections[(size_t) index].name, juce::dontSendNotification);
    auto rowBounds = sectionList.getRowPosition (index, true).reduced (2, 1);
    rowBounds = rowBounds.translated (sectionList.getX(), sectionList.getY());
    inlineSectionNameEditor.setBounds (rowBounds.constrainedWithin (content.getLocalBounds().reduced (4)));
    inlineSectionNameEditor.setVisible (true);
    inlineSectionNameEditor.toFront (false);
    inlineSectionNameEditor.grabKeyboardFocus();
    inlineSectionNameEditor.selectAll();
}

void StructurePanel::finishInlineSectionRename (bool commit)
{
    if (inlineRenameRow < 0)
        return;

    const int row = inlineRenameRow;
    inlineRenameRow = -1;

    if (commit)
    {
        auto& sections = processorRef.getSongManager().getStructureStateForEdit().sections;
        if (row >= 0 && row < static_cast<int> (sections.size()))
        {
            const auto updatedName = inlineSectionNameEditor.getText().trim();
            if (updatedName.isNotEmpty() && updatedName != sections[(size_t) row].name)
            {
                const auto beforeState = processorRef.getSongManager().getStructureState();
                sections[(size_t) row].name = updatedName;
                commitStructureChange (beforeState, "Rename Section", false);
                refreshLists();
                refreshEditor();
            }
        }
    }

    inlineSectionNameEditor.setVisible (false);
}

void StructurePanel::updateSectionDerivedLength (Section& section)
{
    section.bars = computeSectionBarsFromProgression (section);
}

int StructurePanel::chordStartBeat (int index) const { const auto* s = getSelectedSection(); if (s == nullptr || index < 0 || index >= static_cast<int> (s->progression.size())) return -1; int beat = 0; for (int i = 0; i < index; ++i) beat += juce::jmax (1, s->progression[(size_t) i].durationBeats); return beat; }
int StructurePanel::chordIndexForBeat (int beat) const { const auto* s = getSelectedSection(); if (s == nullptr || beat < 0) return -1; int runningBeat = 0; for (int i = 0; i < static_cast<int> (s->progression.size()); ++i) { const int duration = juce::jmax (1, s->progression[(size_t) i].durationBeats); if (beat >= runningBeat && beat < runningBeat + duration) return i; runningBeat += duration; if (runningBeat > kProgressionGridSteps) break; } return -1; }
int StructurePanel::progressionBeatAtPoint (juce::Point<int> position) const { const auto grid = progressionGridBounds(); if (! grid.toNearestInt().contains (position)) return -1; const float cellW = grid.getWidth() / (float) kProgressionGridColumns; const float cellH = grid.getHeight() / (float) kProgressionGridRows; const int column = juce::jlimit (0, kProgressionGridColumns - 1, (int) ((position.x - grid.getX()) / cellW)); const int row = juce::jlimit (0, kProgressionGridRows - 1, (int) ((position.y - grid.getY()) / cellH)); return row * kProgressionGridColumns + column; }
juce::Rectangle<float> StructurePanel::progressionGridBounds() const { auto area = progressionStrip.getLocalBounds().reduced (18, 18).toFloat(); const float side = juce::jmax (120.0f, juce::jmin (area.getWidth(), area.getHeight())); return juce::Rectangle<float> (area.getCentreX() - side * 0.5f, area.getCentreY() - side * 0.5f, side, side); }
juce::Rectangle<int> StructurePanel::chordCellBounds (int index) const { return progressionChipBounds (index).toNearestInt(); }
juce::Rectangle<float> StructurePanel::progressionChipBounds (int index) const { if (index < 0 || index >= kProgressionGridSteps) return {}; const auto grid = progressionGridBounds(); const float cellW = grid.getWidth() / (float) kProgressionGridColumns; const float cellH = grid.getHeight() / (float) kProgressionGridRows; const int row = index / kProgressionGridColumns; const int column = index % kProgressionGridColumns; return { grid.getX() + cellW * (float) column + 1.5f, grid.getY() + cellH * (float) row + 1.5f, cellW - 3.0f, cellH - 3.0f }; }
void StructurePanel::refreshLists() { suppressListCallbacks = true; sectionList.updateContent(); arrangementList.updateContent(); if (selectionTarget == SelectionTarget::section && selectedSectionIndex >= 0) sectionList.selectRow (selectedSectionIndex, false, false); if (selectionTarget == SelectionTarget::arrangementBlock && selectedArrangementIndex >= 0) arrangementList.selectRow (selectedArrangementIndex, false, false); suppressListCallbacks = false; arrangementHint.setVisible (arrangementListModel.getNumRows() == 0); arrangementHint.toFront (false); arrangementFlowHint.setText (arrangementListModel.getNumRows() == 0 ? "Song order: drop sections here" : "Song order built from section instances (drag rows to reorder)", juce::dontSendNotification); }
void StructurePanel::refreshEditor()
{
    const auto* s = getSelectedSection();
    const bool has = s != nullptr;
    const bool arrangementSelection = selectionTarget == SelectionTarget::arrangementBlock;

    editorHeader.setText (arrangementSelection ? "Arrangement-Sourced Section" : "Section Authoring",
                          juce::dontSendNotification);
    editorHint.setText (has
                            ? (arrangementSelection
                                   ? "Editing the reusable section behind this arrangement instance."
                                   : "Shape the reusable section, then place it into the arrangement.")
                            : "Select a section or arrangement block to sculpt song form.",
                        juce::dontSendNotification);
    progressionHeader.setText ("Progression Loop", juce::dontSendNotification);
    progressionHint.setText (has
                                 ? "Each cell is a chord event on the beat grid. The filled loop repeats across the section length."
                                 : "Build the progression loop here once a section is selected.",
                             juce::dontSendNotification);

    suppressControlCallbacks = true;
    nameEditor.setEnabled (has);
    barsSlider.setEnabled (false);
    repeatsSlider.setEnabled (has);
    beatsPerBarBox.setEnabled (has);
    transitionIntentBox.setEnabled (has);
    keyRootBox.setEnabled (has);
    modeBox.setEnabled (has);
    harmonicCenterBox.setEnabled (has);
    addChordCellButton.setEnabled (has);
    progressionPresetButton.setEnabled (has);
    cadenceButton.setEnabled (has);

    if (! has)
    {
        nameEditor.clear();
        barsSlider.setValue (1, juce::dontSendNotification);
        repeatsSlider.setValue (1, juce::dontSendNotification);
        beatsPerBarBox.setSelectedId (4, juce::dontSendNotification);
        transitionIntentBox.setSelectedId (1, juce::dontSendNotification);
        keyRootBox.setSelectedId (1, juce::dontSendNotification);
        modeBox.setSelectedId (1, juce::dontSendNotification);
        harmonicCenterBox.setSelectedId (1, juce::dontSendNotification);
        chordRootBox.setSelectedId (1, juce::dontSendNotification);
        chordTypeBox.setSelectedId (1, juce::dontSendNotification);
        progressionPaletteHint.setText ("", juce::dontSendNotification);
        suppressControlCallbacks = false;
        diatonicPaletteStrip.repaint();
        return;
    }

    nameEditor.setText (s->name, juce::dontSendNotification);
    barsSlider.setValue (s->bars, juce::dontSendNotification);
    repeatsSlider.setValue (s->repeats, juce::dontSendNotification);
    beatsPerBarBox.setSelectedId (s->beatsPerBar, juce::dontSendNotification);
    transitionIntentBox.setText (s->transitionIntent, juce::dontSendNotification);
    keyRootBox.setText (s->keyRoot.isNotEmpty() ? s->keyRoot : processorRef.getSongManager().getStructureState().songKey,
                        juce::dontSendNotification);
    modeBox.setText (s->mode.isNotEmpty() ? s->mode : processorRef.getSongManager().getStructureState().songMode,
                     juce::dontSendNotification);
    harmonicCenterBox.setText (s->harmonicCenter, juce::dontSendNotification);
    progressionPaletteHint.setText ("Tap a chord hex to set the selected cell. Right-click a hex to append a new chord.",
                                    juce::dontSendNotification);

    if (selectedChordIndex < 0 && ! s->progression.empty())
        selectedChordIndex = 0;
    if (selectedChordIndex >= static_cast<int> (s->progression.size()))
        selectedChordIndex = static_cast<int> (s->progression.size()) - 1;

    const bool hasSelectedChord = selectedChordIndex >= 0
                               && selectedChordIndex < static_cast<int> (s->progression.size());
    removeChordCellButton.setEnabled (hasSelectedChord);
    chordRootBox.setEnabled (hasSelectedChord);
    chordTypeBox.setEnabled (hasSelectedChord);
    if (hasSelectedChord)
    {
        const auto& selected = s->progression[(size_t) selectedChordIndex];
        chordRootBox.setText (selected.root, juce::dontSendNotification);
        chordTypeBox.setText (selected.type, juce::dontSendNotification);
    }

    suppressControlCallbacks = false;
    progressionStrip.repaint();
    diatonicPaletteStrip.repaint();
}
void StructurePanel::refreshSummary() { const auto& s = processorRef.getSongManager().getStructureState(); summaryTempo.setText ("Tempo  " + juce::String (s.songTempo) + " BPM", juce::dontSendNotification); summaryKey.setText ("Key  " + s.songKey, juce::dontSendNotification); summaryMode.setText ("Mode  " + s.songMode, juce::dontSendNotification); summaryBars.setText ("Total Bars  " + juce::String (s.totalBars), juce::dontSendNotification); summaryDuration.setText ("Estimated Duration  " + durationText (s.estimatedDurationSeconds), juce::dontSendNotification); }
void StructurePanel::commitStructureChange (const StructureState& beforeState, const juce::String& actionName, bool seedRails) { processorRef.getSongManager().commitAuthoredState(); const auto afterState = processorRef.getSongManager().getStructureState(); processorRef.getUndoManager().beginNewTransaction (actionName); processorRef.getUndoManager().perform (new StructureUndoAction (processorRef, beforeState, afterState)); refreshSummary(); if (seedRails && onRailsSeeded) { juce::Array<int> slots; slots.add (0); onRailsSeeded (slots); } if (onStructureApplied) onStructureApplied(); }

#include "SongInfoPanel.h"

namespace
{
void styleSummaryLabel (juce::Label& label)
{
    label.setFont (Theme::Font::micro());
    label.setColour (juce::Label::textColourId, Theme::Colour::inkMid);
    label.setJustificationType (juce::Justification::centredLeft);
}

void styleSectionLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (Theme::Font::micro());
    label.setColour (juce::Label::textColourId, Theme::Colour::inkGhost);
    label.setJustificationType (juce::Justification::centredLeft);
}
}

const juce::StringArray SongInfoPanel::kKeys =
    { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

const juce::StringArray SongInfoPanel::kScales =
    { "MAJOR", "MINOR", "DORIAN", "MIXOLYDIAN", "LYDIAN", "PHRYGIAN", "LOCRIAN" };

//==============================================================================
SongInfoPanel::SongInfoPanel()
{
    ZoneAControlStyle::styleTextEditor (m_titleEditor);
    m_titleEditor.setFont (Theme::Font::heading());
    m_titleEditor.setText ("Untitled");
    m_titleEditor.onReturnKey = [this] { if (onTitleChanged) onTitleChanged (m_titleEditor.getText()); };
    m_titleEditor.onFocusLost = [this] { if (onTitleChanged) onTitleChanged (m_titleEditor.getText()); };
    styleSectionLabel (m_titleLabel, "TITLE");
    addAndMakeVisible (m_titleLabel);
    addAndMakeVisible (m_titleEditor);

    const auto accent = ZoneAStyle::accentForTabId ("song");
    styleSectionLabel (m_defaultsLabel, "HARMONIC DEFAULTS");
    addAndMakeVisible (m_defaultsLabel);

    // Key combo
    for (const auto& k : kKeys)
        m_keyCombo.addItem (k, m_keyCombo.getNumItems() + 1);
    m_keyCombo.setSelectedId (1, juce::dontSendNotification);
    m_keyCombo.onChange = [this]
    {
        if (onKeyChanged)
            onKeyChanged (m_keyCombo.getText());
    };
    ZoneAControlStyle::styleComboBox (m_keyCombo, accent);
    addAndMakeVisible (m_keyCombo);

    // Scale combo
    for (const auto& s : kScales)
        m_scaleCombo.addItem (s, m_scaleCombo.getNumItems() + 1);
    m_scaleCombo.setSelectedId (1, juce::dontSendNotification);
    m_scaleCombo.onChange = [this]
    {
        if (onScaleChanged)
            onScaleChanged (m_scaleCombo.getText());
    };
    ZoneAControlStyle::styleComboBox (m_scaleCombo, accent);
    addAndMakeVisible (m_scaleCombo);

    // BPM label (drag-to-change)
    m_bpmLabel.setFont (Theme::Font::value());
    m_bpmLabel.setColour (juce::Label::textColourId, Theme::Colour::inkLight);
    m_bpmLabel.setText ("120", juce::dontSendNotification);
    addAndMakeVisible (m_bpmLabel);

    // Time sig (static label for now)
    m_timeSigLabel.setFont (Theme::Font::value());
    m_timeSigLabel.setColour (juce::Label::textColourId, Theme::Colour::inkMid);
    m_timeSigLabel.setText ("4/4", juce::dontSendNotification);
    addAndMakeVisible (m_timeSigLabel);

    // Notes editor
    m_notesEditor.setMultiLine (true);
    m_notesEditor.setReturnKeyStartsNewLine (true);
    ZoneAControlStyle::styleTextEditor (m_notesEditor);
    m_notesEditor.setFont (Theme::Font::micro());
    m_notesEditor.setColour (juce::TextEditor::textColourId, Theme::Colour::inkMid);
    styleSectionLabel (m_notesLabel, "NOTES");
    addAndMakeVisible (m_notesLabel);
    addAndMakeVisible (m_notesEditor);

    m_summaryTitle.setFont (Theme::Font::label());
    m_summaryTitle.setColour (juce::Label::textColourId, Theme::Colour::inkLight);
    m_summaryTitle.setText ("STRUCTURE SUMMARY", juce::dontSendNotification);
    addAndMakeVisible (m_summaryTitle);

    for (auto* label : { &m_summaryTempo, &m_summaryKey, &m_summaryMode, &m_summaryBars, &m_summaryDuration, &m_summarySections, &m_summaryBlocks })
    {
        styleSummaryLabel (*label);
        addAndMakeVisible (*label);
    }

    setStructureSummary (StructureState {});
}

void SongInfoPanel::setBpm (float bpm)
{
    m_bpm = bpm;
    m_bpmLabel.setText (juce::String (juce::roundToInt (bpm)), juce::dontSendNotification);
}

void SongInfoPanel::setTitle (const juce::String& title)
{
    m_titleEditor.setText (title, juce::dontSendNotification);
}

void SongInfoPanel::setKey (const juce::String& key)
{
    const auto desired = key.trim().toUpperCase();
    for (int i = 0; i < m_keyCombo.getNumItems(); ++i)
    {
        if (m_keyCombo.getItemText (i).toUpperCase() == desired)
        {
            m_keyCombo.setSelectedItemIndex (i, juce::dontSendNotification);
            return;
        }
    }
}

void SongInfoPanel::setScale (const juce::String& scale)
{
    const auto desired = scale.trim().toUpperCase();
    for (int i = 0; i < m_scaleCombo.getNumItems(); ++i)
    {
        if (m_scaleCombo.getItemText (i).toUpperCase() == desired)
        {
            m_scaleCombo.setSelectedItemIndex (i, juce::dontSendNotification);
            return;
        }
    }
}

void SongInfoPanel::setStructureSummary (const StructureState& structure)
{
    const auto duration = [&]
    {
        const auto totalSeconds = juce::jmax (0, juce::roundToInt (structure.estimatedDurationSeconds));
        return juce::String (totalSeconds / 60) + ":" + juce::String (totalSeconds % 60).paddedLeft ('0', 2);
    }();

    const auto timeSig = (! structure.sections.empty() && structure.sections.front().beatsPerBar > 0)
        ? juce::String (structure.sections.front().beatsPerBar) + "/4"
        : juce::String ("4/4");
    m_timeSigLabel.setText (timeSig, juce::dontSendNotification);

    m_summaryTempo.setText ("Tempo  " + juce::String (structure.songTempo) + " BPM", juce::dontSendNotification);
    m_summaryKey.setText ("Key  " + (structure.songKey.isNotEmpty() ? structure.songKey : juce::String ("-")), juce::dontSendNotification);
    m_summaryMode.setText ("Mode  " + (structure.songMode.isNotEmpty() ? structure.songMode : juce::String ("-")), juce::dontSendNotification);
    m_summaryBars.setText ("Total Bars  " + juce::String (structure.totalBars), juce::dontSendNotification);
    m_summaryDuration.setText ("Est. Duration  " + duration, juce::dontSendNotification);
    m_summarySections.setText ("Sections  " + juce::String ((int) structure.sections.size()), juce::dontSendNotification);
    m_summaryBlocks.setText ("Arrangement Blocks  " + juce::String ((int) structure.arrangement.size()), juce::dontSendNotification);
}

juce::Rectangle<int> SongInfoPanel::bpmRect() const noexcept
{
    const int gap = ZoneAControlStyle::compactGap() + 2;
    const int rowH = ZoneAControlStyle::controlHeight();
    const int labelH = ZoneAControlStyle::sectionHeaderHeight() - 2;
    return { gap, gap + 5 * (labelH + rowH + gap), 72, rowH };
}

//==============================================================================
void SongInfoPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);
    const auto accent = ZoneAStyle::accentForTabId ("song");
    const int headerH = ZoneAStyle::compactHeaderHeight();
    const int cardPad = ZoneAControlStyle::compactGap() + 3;
    ZoneAStyle::drawHeader (g, { 0, 0, getWidth(), headerH }, "SONG", accent);
    ZoneAStyle::drawCard (g, m_titleEditor.getBounds().expanded (cardPad, headerH), accent);
    ZoneAStyle::drawCard (g, m_keyCombo.getBounds().getUnion (m_bpmLabel.getBounds()).expanded (cardPad, headerH), accent);
    ZoneAStyle::drawCard (g, m_notesEditor.getBounds().expanded (cardPad, headerH), accent);
    ZoneAStyle::drawCard (g, m_summaryTitle.getBounds().getUnion (m_summaryBlocks.getBounds()).expanded (cardPad, headerH), accent);

    g.setColour (accent);
    g.drawText ("↕", bpmRect().withTrimmedLeft (84), juce::Justification::centredLeft, false);
}

void SongInfoPanel::resized()
{
    const int gap = ZoneAControlStyle::compactGap() + 2;
    const int labelH = ZoneAControlStyle::sectionHeaderHeight() - 2;
    const int rowH = ZoneAControlStyle::controlHeight();
    const int headerH = ZoneAStyle::compactHeaderHeight();
    const int summaryRowH = ZoneAStyle::compactGroupHeaderHeight() - 1;

    auto bounds = getLocalBounds().reduced (gap + 2).withTrimmedTop (headerH + 2);

    auto titleArea = bounds.removeFromTop (labelH + gap + rowH);
    m_titleLabel.setBounds (titleArea.removeFromTop (labelH));
    titleArea.removeFromTop (gap);
    m_titleEditor.setBounds (titleArea.removeFromTop (rowH));

    bounds.removeFromTop (gap + 1);

    auto defaults = bounds.removeFromTop (labelH + gap + rowH + gap + rowH);
    m_defaultsLabel.setBounds (defaults.removeFromTop (labelH));
    defaults.removeFromTop (gap);
    auto row1 = defaults.removeFromTop (rowH);
    m_keyCombo.setBounds (row1.removeFromLeft (juce::jmax (72, row1.getWidth() / 3)));
    row1.removeFromLeft (gap);
    m_scaleCombo.setBounds (row1.removeFromLeft (juce::jmax (98, row1.getWidth() / 2)));
    row1.removeFromLeft (gap);
    m_timeSigLabel.setBounds (row1);
    defaults.removeFromTop (gap + 1);
    auto row2 = defaults.removeFromTop (rowH);
    m_bpmLabel.setBounds (row2.removeFromLeft (72));

    bounds.removeFromTop (gap + 1);

    const int summaryH = labelH + gap + summaryRowH * 7;
    const int notesH = juce::jmax (68, bounds.getHeight() - summaryH - gap - 1);
    m_notesLabel.setBounds (bounds.removeFromTop (labelH));
    bounds.removeFromTop (gap);
    m_notesEditor.setBounds (bounds.removeFromTop (notesH));

    bounds.removeFromTop (gap + 1);
    auto summary = bounds.removeFromTop (summaryH);
    m_summaryTitle.setBounds (summary.removeFromTop (labelH + 2).reduced (gap + 1, 0));
    m_summaryTempo.setBounds (summary.removeFromTop (summaryRowH).reduced (gap + 1, 0));
    m_summaryKey.setBounds (summary.removeFromTop (summaryRowH).reduced (gap + 1, 0));
    m_summaryMode.setBounds (summary.removeFromTop (summaryRowH).reduced (gap + 1, 0));
    m_summaryBars.setBounds (summary.removeFromTop (summaryRowH).reduced (gap + 1, 0));
    m_summaryDuration.setBounds (summary.removeFromTop (summaryRowH).reduced (gap + 1, 0));
    m_summarySections.setBounds (summary.removeFromTop (summaryRowH).reduced (gap + 1, 0));
    m_summaryBlocks.setBounds (summary.removeFromTop (summaryRowH).reduced (gap + 1, 0));
}

void SongInfoPanel::mouseDown (const juce::MouseEvent& e)
{
    if (bpmRect().contains (e.getPosition()))
    {
        m_draggingBpm  = true;
        m_dragStartY   = static_cast<float> (e.getPosition().getY());
        m_dragStartBpm = m_bpm;
    }
}

void SongInfoPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (!m_draggingBpm) return;

    const float delta = m_dragStartY - static_cast<float> (e.getPosition().getY());
    m_bpm = juce::jlimit (40.0f, 240.0f, m_dragStartBpm + delta * 0.5f);
    m_bpmLabel.setText (juce::String (juce::roundToInt (m_bpm)), juce::dontSendNotification);

    if (onBpmChanged) onBpmChanged (m_bpm);
}

void SongInfoPanel::mouseUp (const juce::MouseEvent&)
{
    m_draggingBpm = false;
}

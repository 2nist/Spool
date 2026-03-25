#include "SongInfoPanel.h"

const juce::StringArray SongInfoPanel::kKeys =
    { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

const juce::StringArray SongInfoPanel::kScales =
    { "MAJOR", "MINOR", "DORIAN", "MIXOLYDIAN", "LYDIAN", "PHRYGIAN", "LOCRIAN" };

//==============================================================================
SongInfoPanel::SongInfoPanel()
{
    // Title editor
    m_titleEditor.setFont (Theme::Font::heading());
    m_titleEditor.setColour (juce::TextEditor::backgroundColourId, Theme::Colour::surface2);
    m_titleEditor.setColour (juce::TextEditor::textColourId, Theme::Colour::inkLight);
    m_titleEditor.setColour (juce::TextEditor::outlineColourId, Theme::Colour::surfaceEdge);
    m_titleEditor.setText ("Untitled");
    m_titleEditor.onReturnKey = [this] { if (onTitleChanged) onTitleChanged (m_titleEditor.getText()); };
    m_titleEditor.onFocusLost = [this] { if (onTitleChanged) onTitleChanged (m_titleEditor.getText()); };
    addAndMakeVisible (m_titleEditor);

    // Key combo
    for (const auto& k : kKeys)
        m_keyCombo.addItem (k, m_keyCombo.getNumItems() + 1);
    m_keyCombo.setSelectedId (1, juce::dontSendNotification);
    m_keyCombo.onChange = [this]
    {
        if (onKeyChanged)
            onKeyChanged (m_keyCombo.getText());
    };
    m_keyCombo.setColour (juce::ComboBox::backgroundColourId, Theme::Colour::surface2);
    m_keyCombo.setColour (juce::ComboBox::textColourId, Theme::Colour::inkMid);
    m_keyCombo.setColour (juce::ComboBox::outlineColourId, Theme::Colour::surfaceEdge);
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
    m_scaleCombo.setColour (juce::ComboBox::backgroundColourId, Theme::Colour::surface2);
    m_scaleCombo.setColour (juce::ComboBox::textColourId, Theme::Colour::inkMid);
    m_scaleCombo.setColour (juce::ComboBox::outlineColourId, Theme::Colour::surfaceEdge);
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
    m_notesEditor.setFont (Theme::Font::micro());
    m_notesEditor.setColour (juce::TextEditor::backgroundColourId, Theme::Colour::surface2);
    m_notesEditor.setColour (juce::TextEditor::textColourId, Theme::Colour::inkMid);
    m_notesEditor.setColour (juce::TextEditor::outlineColourId, Theme::Colour::surfaceEdge);
    addAndMakeVisible (m_notesEditor);
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

juce::Rectangle<int> SongInfoPanel::bpmRect() const noexcept
{
    return { kPad, kPad + 5 * (kLabelH + kRowH + kPad), 80, kRowH };
}

//==============================================================================
void SongInfoPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    int y = kPad;
    auto drawLabel = [&] (const juce::String& text, int rowY)
    {
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText (text, kPad, rowY, getWidth() - kPad * 2, kLabelH,
                    juce::Justification::centredLeft, false);
    };

    drawLabel ("TITLE",    y);    y += kLabelH + kRowH + kPad;
    drawLabel ("KEY",      y);    y += kLabelH + kRowH + kPad;
    drawLabel ("SCALE",    y);    y += kLabelH + kRowH + kPad;
    drawLabel ("TIME SIG", y);    y += kLabelH + kRowH + kPad;
    drawLabel ("BPM  (drag to change)", y);

    // BPM drag hint arrow
    g.setColour (Theme::Zone::d);
    g.drawText ("↕", bpmRect().withTrimmedLeft (84), juce::Justification::centredLeft, false);
}

void SongInfoPanel::resized()
{
    int y = kPad + kLabelH;

    m_titleEditor.setBounds (kPad, y, getWidth() - kPad * 2, kRowH);
    y += kRowH + kPad + kLabelH;

    m_keyCombo.setBounds (kPad, y, getWidth() / 2 - kPad, kRowH);
    y += kRowH + kPad + kLabelH;

    m_scaleCombo.setBounds (kPad, y, getWidth() - kPad * 2, kRowH);
    y += kRowH + kPad + kLabelH;

    m_timeSigLabel.setBounds (kPad, y, 60, kRowH);
    y += kRowH + kPad + kLabelH;

    m_bpmLabel.setBounds (bpmRect());
    y += kRowH + kPad * 2;

    const int notesH = juce::jmax (60, getHeight() - y - kPad);
    m_notesEditor.setBounds (kPad, y, getWidth() - kPad * 2, notesH);
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

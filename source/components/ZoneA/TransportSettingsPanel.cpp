#include "TransportSettingsPanel.h"

//==============================================================================
TransportSettingsPanel::TransportSettingsPanel()
{
    m_metronomeSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    m_metronomeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    m_metronomeSlider.setRange (0.0, 1.0, 0.01);
    m_metronomeSlider.setValue (m_metronomeVolume);
    m_metronomeSlider.onValueChange = [this]
    {
        m_metronomeVolume = static_cast<float> (m_metronomeSlider.getValue());
        if (onMetronomeChanged) onMetronomeChanged (m_metronomeEnabled, m_metronomeVolume);
    };
    addAndMakeVisible (m_metronomeSlider);

    m_countInCombo.addItem ("None (0 bars)",  1);
    m_countInCombo.addItem ("1 bar",          2);
    m_countInCombo.addItem ("2 bars",         3);
    m_countInCombo.addItem ("4 bars",         4);
    m_countInCombo.setSelectedId (1, juce::dontSendNotification);
    m_countInCombo.onChange = [this]
    {
        m_countInBars = m_countInCombo.getSelectedId() - 1;
        if (onCountInChanged) onCountInChanged (m_countInBars);
    };
    m_countInCombo.setColour (juce::ComboBox::backgroundColourId, Theme::Colour::surface2);
    m_countInCombo.setColour (juce::ComboBox::textColourId,       Theme::Colour::inkMid);
    m_countInCombo.setColour (juce::ComboBox::outlineColourId,    Theme::Colour::surfaceEdge);
    addAndMakeVisible (m_countInCombo);
}

//==============================================================================
juce::Rectangle<int> TransportSettingsPanel::toggleRowRect (int index) const noexcept
{
    const int baseY = kPad + (kLblH + kRowH + kPad) * index;
    return { kPad, baseY, getWidth() - kPad * 2, kRowH };
}

void TransportSettingsPanel::paintToggleRow (juce::Graphics& g,
                                              int index,
                                              const juce::String& label,
                                              bool enabled) const
{
    const auto r = toggleRowRect (index);

    // Toggle pill
    const int pillW = 32, pillH = 16;
    const auto pill = juce::Rectangle<int> (r.getRight() - pillW,
                                             r.getCentreY() - pillH / 2,
                                             pillW, pillH);

    g.setColour (enabled ? Theme::Zone::a : Theme::Colour::surface3);
    g.fillRoundedRectangle (pill.toFloat(), static_cast<float> (pillH) * 0.5f);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRoundedRectangle (pill.toFloat(), static_cast<float> (pillH) * 0.5f, 0.5f);

    // Thumb
    const int thumbD = pillH - 4;
    const int thumbX = enabled ? pill.getRight() - thumbD - 2 : pill.getX() + 2;
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.fillEllipse (static_cast<float> (thumbX), static_cast<float> (pill.getCentreY() - thumbD / 2),
                   static_cast<float> (thumbD), static_cast<float> (thumbD));

    // Label
    g.setFont (Theme::Font::micro());
    g.setColour (enabled ? Theme::Colour::inkMid : Theme::Colour::inkGhost);
    g.drawText (label, r.withTrimmedRight (pillW + 8), juce::Justification::centredLeft, false);
}

void TransportSettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    // Header
    g.setFont (Theme::Font::heading());
    g.setColour (Theme::Colour::inkLight);
    g.drawText ("TRANSPORT SETTINGS",
                juce::Rectangle<int> (kPad, kPad, getWidth() - kPad * 2, 24),
                juce::Justification::centredLeft, false);

    g.setColour (Theme::Colour::surfaceEdge);
    g.fillRect (kPad, kPad + 28, getWidth() - kPad * 2, 1);

    const int baseY = kPad + 36;

    // Metronome section
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("METRONOME", kPad, baseY, getWidth() - kPad * 2, kLblH,
                juce::Justification::centredLeft, false);

    paintToggleRow (g, 0, "Metronome on", m_metronomeEnabled);

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    const int volLblY = toggleRowRect (0).getBottom() + 4;
    g.drawText ("Volume", kPad, volLblY, 50, kLblH, juce::Justification::centredLeft, false);

    // Count-in label
    const int cntLblY = toggleRowRect (1).getY() - kLblH - 2;
    g.drawText ("COUNT-IN", kPad, cntLblY, getWidth() - kPad * 2, kLblH,
                juce::Justification::centredLeft, false);

    // Toggle rows
    paintToggleRow (g, 2, "MIDI Clock out",  m_midiClockEnabled);
    paintToggleRow (g, 3, "Ableton Link",    m_linkEnabled);
    paintToggleRow (g, 4, "Tempo master",    m_tempoMaster);
}

void TransportSettingsPanel::resized()
{
    const int baseY = kPad + 36;

    // Adjust toggle row 0 y manually to sit below header
    const int row0Y    = baseY + kLblH;
    const int sliderY  = row0Y + kRowH + 2;

    m_metronomeSlider.setBounds (kPad + 56, sliderY, getWidth() - kPad * 2 - 56, 20);

    const int row1Y = sliderY + 24 + kLblH;
    m_countInCombo.setBounds (kPad, row1Y, getWidth() - kPad * 2, kRowH);
}

void TransportSettingsPanel::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    auto hit = [&] (int rowIdx) { return toggleRowRect (rowIdx).contains (pos); };

    if (hit (0))
    {
        m_metronomeEnabled = !m_metronomeEnabled;
        if (onMetronomeChanged) onMetronomeChanged (m_metronomeEnabled, m_metronomeVolume);
        repaint();
        return;
    }
    if (hit (2))
    {
        m_midiClockEnabled = !m_midiClockEnabled;
        if (onMidiClockChanged) onMidiClockChanged (m_midiClockEnabled);
        repaint();
        return;
    }
    if (hit (3))
    {
        m_linkEnabled = !m_linkEnabled;
        if (onLinkToggled) onLinkToggled (m_linkEnabled);
        repaint();
        return;
    }
    if (hit (4))
    {
        m_tempoMaster = !m_tempoMaster;
        if (onTempoMasterChanged) onTempoMasterChanged (m_tempoMaster);
        repaint();
    }
}

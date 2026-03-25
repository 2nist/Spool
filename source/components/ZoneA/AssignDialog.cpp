#include "AssignDialog.h"

AssignDialog::AssignDialog (int slotIndex)
    : m_slotIndex (slotIndex)
{
    // Populate with all known panels
    m_entries.add ({ "structure", "STRUCTURE" });
    m_entries.add ({ "patterns",  "PATTERNS"  });
    m_entries.add ({ "samples",   "SAMPLES"   });
    m_entries.add ({ "settings",  "SETTINGS"  });
    m_entries.add ({ "vst",       "VST BROWSER" });
    m_entries.add ({ "mixer",     "MIXER"     });
    m_entries.add ({ "fx",        "FX BROWSER" });
    m_entries.add ({ "routing",   "ROUTING"   });

    m_model.entries = &m_entries;
    m_list.setModel (&m_model);
    m_list.setRowHeight (static_cast<int> (Theme::Space::lg + Theme::Space::sm));
    m_list.setColour (juce::ListBox::backgroundColourId, Theme::Colour::surface2);
    addAndMakeVisible (m_list);

    m_assignBtn.setColour (juce::TextButton::buttonColourId,   Theme::Colour::accent);
    m_assignBtn.setColour (juce::TextButton::textColourOffId,  Theme::Colour::inkDark);
    m_cancelBtn.setColour (juce::TextButton::buttonColourId,   Theme::Colour::surface4);
    m_cancelBtn.setColour (juce::TextButton::textColourOffId,  Theme::Colour::inkMuted);
    addAndMakeVisible (m_assignBtn);
    addAndMakeVisible (m_cancelBtn);

    m_assignBtn.onClick = [this]
    {
        const int row = m_list.getSelectedRow();
        if (row >= 0 && row < m_entries.size())
        {
            if (onAssign)
                onAssign (m_slotIndex, m_entries[row].id);
        }
    };

    m_cancelBtn.onClick = [this]
    {
        if (onCancel)
            onCancel();
    };
}

void AssignDialog::paint (juce::Graphics& g)
{
    // Drop-shadow-like dark overlay behind the card
    g.fillAll (Theme::Colour::surface0.withAlpha (0.85f));

    // Card
    const auto card = getLocalBounds()
                          .reduced (static_cast<int> (Theme::Space::xl))
                          .toFloat();
    g.setColour (Theme::Colour::surface3);
    g.fillRoundedRectangle (card, Theme::Radius::md);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRoundedRectangle (card, Theme::Radius::md, Theme::Stroke::normal);

    // Title
    g.setFont   (Theme::Font::heading());
    g.setColour (Theme::Zone::a);
    g.drawText  ("ASSIGN BUTTON " + juce::String (m_slotIndex + 1),
                 card.withHeight (Theme::Space::xxl),
                 juce::Justification::centred, false);
}

void AssignDialog::resized()
{
    const int margin = static_cast<int> (Theme::Space::xl);
    auto card = getLocalBounds().reduced (margin);

    // Title area
    card.removeFromTop (static_cast<int> (Theme::Space::xxl));

    // Button row at bottom
    const int btnH = static_cast<int> (Theme::Space::xl + Theme::Space::sm);
    auto btnRow = card.removeFromBottom (btnH);
    card.removeFromBottom (static_cast<int> (Theme::Space::sm)); // gap
    const int btnW = btnRow.getWidth() / 2 - static_cast<int> (Theme::Space::xs);
    m_cancelBtn.setBounds (btnRow.removeFromLeft (btnW));
    btnRow.removeFromLeft (static_cast<int> (Theme::Space::xs * 2));
    m_assignBtn.setBounds (btnRow);

    m_list.setBounds (card);
}

#include "ModuleLoadDialog.h"
#include "../ZoneA/ZoneAControlStyle.h"

ModuleLoadDialog::ModuleLoadDialog (int slotIndex)
    : m_slotIndex (slotIndex)
{
    auto styleBtn = [](juce::TextButton& btn, const juce::Colour& accent)
    {
        ZoneAControlStyle::styleTextButton (btn, accent.withAlpha (0.8f));
    };

    styleBtn (m_synthBtn,   Theme::Signal::audio);
    styleBtn (m_samplerBtn, Theme::Signal::audio);
    styleBtn (m_seqBtn,     Theme::Signal::midi);
    styleBtn (m_vst3Btn,    Theme::Zone::menu);
    styleBtn (m_drumBtn,    Theme::Signal::midi);
    styleBtn (m_reelBtn,    Theme::Zone::a);
    ZoneAControlStyle::styleTextButton (m_cancelBtn, Theme::Colour::surfaceEdge.withAlpha (0.6f));

    addAndMakeVisible (m_synthBtn);
    addAndMakeVisible (m_samplerBtn);
    addAndMakeVisible (m_seqBtn);
    addAndMakeVisible (m_vst3Btn);
    addAndMakeVisible (m_drumBtn);
    addAndMakeVisible (m_reelBtn);
    addAndMakeVisible (m_cancelBtn);

    m_synthBtn.onClick   = [this] { if (onTypeChosen) onTypeChosen (m_slotIndex, ModuleSlot::ModuleType::Synth);       };
    m_samplerBtn.onClick = [this] { if (onTypeChosen) onTypeChosen (m_slotIndex, ModuleSlot::ModuleType::Sampler);     };
    m_seqBtn.onClick     = [this] { if (onTypeChosen) onTypeChosen (m_slotIndex, ModuleSlot::ModuleType::Sequencer);   };
    m_vst3Btn.onClick    = [this] { if (onTypeChosen) onTypeChosen (m_slotIndex, ModuleSlot::ModuleType::Vst3);        };
    m_drumBtn.onClick    = [this] { if (onTypeChosen) onTypeChosen (m_slotIndex, ModuleSlot::ModuleType::DrumMachine); };
    m_reelBtn.onClick    = [this] { if (onTypeChosen) onTypeChosen (m_slotIndex, ModuleSlot::ModuleType::Reel);        };
    m_cancelBtn.onClick  = [this] { if (onCancel)     onCancel();                                                      };
}

void ModuleLoadDialog::paint (juce::Graphics& g)
{
    // Card background
    g.setColour (Theme::Colour::surface2);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), Theme::Radius::md);

    // Border
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRoundedRectangle (getLocalBounds().toFloat(), Theme::Radius::md, Theme::Stroke::normal);

    // Title
    const int titleH = static_cast<int> (Theme::Space::xxl);
    g.setFont   (Theme::Font::heading());
    g.setColour (Theme::Colour::inkLight);
    g.drawText  ("LOAD MODULE \xe2\x80\x94 SLOT " + juce::String (m_slotIndex + 1),
                 juce::Rectangle<int> (0, static_cast<int> (Theme::Space::sm), getWidth(), titleH),
                 juce::Justification::centred, false);
}

void ModuleLoadDialog::resized()
{
    const int pad    = static_cast<int> (Theme::Space::md);
    const int titleH = static_cast<int> (Theme::Space::xxl);
    const int btnH   = 32;
    const int btnGap = static_cast<int> (Theme::Space::xs);

    int y = titleH + pad;
    for (auto* btn : { &m_synthBtn, &m_samplerBtn, &m_seqBtn, &m_vst3Btn, &m_drumBtn, &m_reelBtn, &m_cancelBtn })
    {
        btn->setBounds (pad, y, getWidth() - pad * 2, btnH);
        y += btnH + btnGap;
    }
}

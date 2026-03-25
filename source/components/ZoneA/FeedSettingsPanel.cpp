#include "FeedSettingsPanel.h"

//==============================================================================
FeedSettingsPanel::FeedSettingsPanel()
{
    m_items.add ({ "MIDI In activity",       true  });
    m_items.add ({ "MIDI Out activity",      true  });
    m_items.add ({ "System info",            true  });
    m_items.add ({ "Transport events",       true  });
    m_items.add ({ "Link status",            true  });
    m_items.add ({ "Warnings",               true  });

    m_speedSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    m_speedSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    m_speedSlider.setRange (0.0, 1.0, 0.01);
    m_speedSlider.setValue (0.5);
    addAndMakeVisible (m_speedSlider);

    m_clearBtn.onClick = [this] { if (onClearFeed) onClearFeed(); };
    addAndMakeVisible (m_clearBtn);
}

//==============================================================================
juce::Rectangle<int> FeedSettingsPanel::itemRect (int index) const noexcept
{
    const int topY = kPad + 28 + kPad;   // header height
    return { kPad, topY + index * kRowH, getWidth() - kPad * 2, kRowH };
}

void FeedSettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    // Header
    g.setFont (Theme::Font::heading());
    g.setColour (Theme::Colour::inkLight);
    g.drawText ("SYSTEM FEED",
                juce::Rectangle<int> (kPad, kPad, getWidth() - kPad * 2, 24),
                juce::Justification::centredLeft, false);

    // Divider below header
    g.setColour (Theme::Colour::surfaceEdge);
    g.fillRect (kPad, kPad + 28, getWidth() - kPad * 2, 1);

    // Toggle list
    for (int i = 0; i < m_items.size(); ++i)
    {
        const auto& item = m_items.getReference (i);
        const auto r     = itemRect (i);

        // Checkbox
        const int cbSz = 12;
        const auto cb  = juce::Rectangle<int> (r.getX(), r.getCentreY() - cbSz / 2, cbSz, cbSz);

        g.setColour (Theme::Colour::surface3);
        g.fillRoundedRectangle (cb.toFloat(), 2.0f);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRoundedRectangle (cb.toFloat(), 2.0f, 0.5f);

        if (item.enabled)
        {
            g.setColour (Theme::Zone::a);
            g.fillRoundedRectangle (cb.reduced (3).toFloat(), 1.0f);
        }

        // Label
        g.setFont (Theme::Font::micro());
        g.setColour (item.enabled ? Theme::Colour::inkMid : Theme::Colour::inkGhost);
        g.drawText (item.name, r.withTrimmedLeft (cbSz + 6), juce::Justification::centredLeft, false);
    }

    // Scroll speed label
    const int speedY = itemRect (m_items.size()).getY() + 8;
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("SCROLL SPEED", kPad, speedY, getWidth() - kPad * 2, 14,
                juce::Justification::centredLeft, false);
}

void FeedSettingsPanel::resized()
{
    const int speedY = itemRect (m_items.size()).getY() + 8;

    m_speedSlider.setBounds (kPad, speedY + 16,
                             getWidth() - kPad * 2, 20);

    m_clearBtn.setBounds (kPad, m_speedSlider.getBottom() + 12,
                          getWidth() - kPad * 2, 24);
}

void FeedSettingsPanel::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();
    for (int i = 0; i < m_items.size(); ++i)
    {
        if (itemRect (i).contains (pos))
        {
            onItemClicked (i, pos);
            return;
        }
    }
}

void FeedSettingsPanel::onItemClicked (int index, juce::Point<int>)
{
    auto& item   = m_items.getReference (index);
    item.enabled = !item.enabled;

    if (onFeedToggleChanged)
        onFeedToggleChanged (item.name, item.enabled);

    repaint();
}

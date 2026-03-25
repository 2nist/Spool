#include "RoutingPanel.h"

RoutingPanel::RoutingPanel()
{
    // Create toggle buttons for a simple routing matrix
    m_buttons.clear();
    for (int r = 0; r < kSources; ++r)
    {
        for (int c = 0; c < kOutputs; ++c)
        {
            auto btn = std::make_unique<juce::ToggleButton>();
            btn->setToggleState (false, juce::dontSendNotification);
            addAndMakeVisible (*btn);
            // capture index for callback
            const int idx = static_cast<int> (m_buttons.size());
            btn->onClick = [this, idx]
            {
                if (onMatrixChanged)
                {
                    std::vector<uint8_t> vals;
                    vals.reserve (kSources * kOutputs);
                    for (size_t i = 0; i < m_buttons.size(); ++i)
                        vals.push_back (m_buttons[i] ? (m_buttons[i]->getToggleState() ? 1 : 0) : 0);
                    onMatrixChanged (vals);
                }
            };
            m_buttons.push_back (std::move (btn));
        }
    }
}

void RoutingPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    g.setFont (Theme::Font::heading());
    g.setColour (Theme::Colour::inkLight);
    g.drawText ("ROUTING", headerRect(), juce::Justification::centredLeft, false);

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("Main Output: Stereo", getLocalBounds().withTrimmedTop (40).withHeight (20), juce::Justification::centredLeft, false);

    // Draw simple matrix labels
    const int left = 12;
    const int top  = 80;
    const int rowH = 22;
    for (int r = 0; r < m_sourceNames.size(); ++r)
    {
        g.setColour (Theme::Colour::inkMuted);
        g.setFont (Theme::Font::micro());
        g.drawText (m_sourceNames[r], left, top + r * rowH, 80, rowH, juce::Justification::centredLeft, false);
    }
    // Output headers
    for (int c = 0; c < m_outputNames.size(); ++c)
    {
        const int x = getWidth() - (m_outputNames.size() - c) * 48;
        g.setColour (Theme::Colour::inkMuted);
        g.drawText (m_outputNames[c], x, top - 20, 40, 16, juce::Justification::centred, false);
    }
}

void RoutingPanel::resized()
{
    const int top  = 80;
    const int rowH = 22;
    const int cellW = 40;
    // Position toggle buttons in grid order r*c
    int idx = 0;
    for (int r = 0; r < kSources; ++r)
    {
        for (int c = 0; c < kOutputs; ++c)
        {
            const int x = getWidth() - (kOutputs - c) * 48;
            const int y = top + r * rowH;
            if (idx < (int) m_buttons.size() && m_buttons[idx])
                m_buttons[idx]->setBounds (x, y, cellW, rowH - 4);
            ++idx;
        }
    }
}

void RoutingPanel::setMatrix (const std::vector<uint8_t>& vals)
{
    const int want = kSources * kOutputs;
    int count = juce::jmin (want, (int) vals.size());
    for (int i = 0; i < count && i < (int) m_buttons.size(); ++i)
    {
        if (m_buttons[i])
            m_buttons[i]->setToggleState (vals[i] != 0, juce::dontSendNotification);
    }
    // notify listeners
    if (onMatrixChanged)
        onMatrixChanged (vals);
}

juce::Rectangle<int> RoutingPanel::headerRect() const noexcept
{
    return getLocalBounds().withHeight (40).withTrimmedLeft (12);
}

juce::String RoutingPanel::getSummary() const
{
    if (m_buttons.empty()) return {};

    // Determine per-output whether any source is routed
    std::vector<bool> outActive (kOutputs, false);
    int idx = 0;
    for (int r = 0; r < kSources; ++r)
    {
        for (int c = 0; c < kOutputs; ++c)
        {
            if (idx < (int) m_buttons.size() && m_buttons[idx])
            {
                if (m_buttons[idx]->getToggleState())
                    outActive[c] = true;
            }
            ++idx;
        }
    }

    if (outActive[0] && outActive[1])
        return "Stereo";
    if (outActive[0] && !outActive[1])
        return "L only";
    if (!outActive[0] && outActive[1])
        return "R only";
    return "No output";
}

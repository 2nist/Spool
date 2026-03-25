#include "PanelContentView.h"

PanelContentView::PanelContentView()
{
}

void PanelContentView::setPanel (const juce::String& fullName,
                                  const juce::String& panelId,
                                  const juce::Colour& colour)
{
    m_fullName = fullName;
    m_panelId  = panelId;
    m_colour   = colour;

    // Route to ThemeEditorPanel
    if (panelId == "theme")
    {
        if (m_themePanel == nullptr)
        {
            m_themePanel = std::make_unique<ThemeEditorPanel>();
            addAndMakeVisible (*m_themePanel);
            if (getWidth() > 0)
                m_themePanel->setBounds (getLocalBounds());
        }
        m_themePanel->setVisible (true);
    }
    else
    {
        if (m_themePanel != nullptr)
            m_themePanel->setVisible (false);
    }

    // Route to RoutingPanel
    if (panelId == "routing")
    {
        if (m_routingPanel == nullptr)
        {
            m_routingPanel = std::make_unique<RoutingPanel>();
            addAndMakeVisible (*m_routingPanel);
            // forward UI changes up via onRoutingChanged
            m_routingPanel->onMatrixChanged = [this] (const std::vector<uint8_t>& vals)
            {
                std::array<uint8_t, 8> a { 0 };
                for (size_t i = 0; i < vals.size() && i < a.size(); ++i) a[i] = vals[i];
                if (onRoutingChanged) onRoutingChanged (a);
            };
            if (getWidth() > 0)
                m_routingPanel->setBounds (getLocalBounds());
        }
        m_routingPanel->setVisible (true);
    }
    else
    {
        if (m_routingPanel != nullptr)
            m_routingPanel->setVisible (false);
    }

    repaint();
}

void PanelContentView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface2);

    // Show stub content only when no themed panel is active
    if (m_panelId == "theme") return;

    const auto bounds = getLocalBounds().toFloat();
    g.setFont  (Theme::Font::heading());
    g.setColour (m_colour);
    g.drawText (m_fullName,
                bounds.withHeight (bounds.getHeight() / 2.0f),
                juce::Justification::centred,
                false);

    g.setFont  (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("[ panel content coming soon ]",
                bounds.withTrimmedTop (bounds.getHeight() / 2.0f),
                juce::Justification::centred,
                false);
}

void PanelContentView::resized()
{
    if (m_themePanel != nullptr)
        m_themePanel->setBounds (getLocalBounds());
    if (m_routingPanel != nullptr)
        m_routingPanel->setBounds (getLocalBounds());
}

juce::String PanelContentView::getRoutingSummary() const
{
    if (m_routingPanel == nullptr)
        return {};
    return m_routingPanel->getSummary();
}

void PanelContentView::setRoutingMatrix (const std::array<uint8_t, 8>& m)
{
    if (m_routingPanel != nullptr)
    {
        std::vector<uint8_t> vals;
        vals.reserve (m.size());
        for (auto b : m) vals.push_back (b ? 1 : 0);
        m_routingPanel->setMatrix (vals);
    }
}


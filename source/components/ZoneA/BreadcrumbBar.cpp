#include "BreadcrumbBar.h"

BreadcrumbBar::BreadcrumbBar()
{
}

void BreadcrumbBar::setCrumbs (const juce::StringArray& labels,
                                const juce::Colour&      activeColour)
{
    m_labels      = labels;
    m_activeColour = activeColour;
    repaint();
}

void BreadcrumbBar::setRoutingSummary (const juce::String& s)
{
    m_routingSummary = s;
    repaint();
}

void BreadcrumbBar::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface0);

    m_crumbRects.clear();

    if (m_labels.isEmpty())
        return;

    const auto microFont = Theme::Font::micro();
    const float h        = static_cast<float> (getHeight());
    const float sepW     = juce::GlyphArrangement::getStringWidth (microFont, " › ");
    float x              = Theme::Space::sm;

    g.setFont (microFont);

    for (int i = 0; i < m_labels.size(); ++i)
    {
        const bool   isCurrent = (i == m_labels.size() - 1);
        const auto&  label     = m_labels[i];
        const float  labelW    = juce::GlyphArrangement::getStringWidth (microFont, label);

        g.setColour (isCurrent ? m_activeColour : Theme::Colour::inkGhost);
        g.drawText (label, juce::Rectangle<float> (x, 0.0f, labelW, h),
                    juce::Justification::centredLeft, false);

        m_crumbRects.add ({ x, 0.0f, labelW, h });
        x += labelW;

        if (!isCurrent)
        {
            g.setColour (Theme::Colour::inkGhost);
            g.drawText (" › ", juce::Rectangle<float> (x, 0.0f, sepW, h),
                        juce::Justification::centredLeft, false);
            x += sepW;
        }
    }

    // Routing summary on the right
    if (m_routingSummary.isNotEmpty())
    {
        g.setColour (Theme::Colour::inkGhost);
        g.setFont (Theme::Font::micro());
        g.drawText (m_routingSummary, getLocalBounds().reduced (4).withLeft (getWidth() - 120), juce::Justification::centredRight, false);
    }
}

void BreadcrumbBar::mouseUp (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition().toFloat();
    for (int i = 0; i < m_crumbRects.size(); ++i)
    {
        if (m_crumbRects[i].contains (pos))
        {
            if (onCrumbClicked)
                onCrumbClicked (i);
            return;
        }
    }
}

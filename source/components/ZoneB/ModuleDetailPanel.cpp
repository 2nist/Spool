#include "ModuleDetailPanel.h"

//==============================================================================
ModuleDetailPanel::ModuleDetailPanel()
{
    m_editor.onParamChanged = [this] (int idx, float val)
    {
        if (onParamChanged) onParamChanged (idx, val);
    };
    addAndMakeVisible (m_editor);
}

//==============================================================================
void ModuleDetailPanel::setSlotInfo (const juce::Array<InlineEditor::Param>& params,
                                     const juce::StringArray&                inPorts,
                                     const juce::StringArray&                outPorts,
                                     juce::Colour                            signalColour,
                                     const juce::String&                     typeName,
                                     const juce::String&                     groupName,
                                     juce::Colour                            groupColour)
{
    m_groupColour = groupColour;
    m_groupName   = groupName;
    m_typeName    = typeName;
    m_hasSlot     = true;

    m_editor.setParams (params, signalColour, typeName, inPorts, outPorts);
    resized();
    repaint();
}

void ModuleDetailPanel::clearSlot()
{
    m_hasSlot   = false;
    m_typeName  = {};
    m_groupName = {};
    m_editor.setParams ({}, juce::Colours::transparentBlack, {}, {}, {});
    repaint();
}

//==============================================================================
void ModuleDetailPanel::paint (juce::Graphics& g)
{
    // Panel background
    g.fillAll (Theme::Colour::surface0);

    // Header bar
    g.setColour (Theme::Colour::surface1);
    g.fillRect (0, 0, getWidth(), kHeaderH);

    // Group colour swatch (4px left stripe)
    g.setColour (m_groupColour);
    g.fillRect (0, 0, 4, kHeaderH);

    if (m_hasSlot)
    {
        // Type badge
        constexpr int kBadgeW = 40;
        constexpr int kBadgeH = 14;
        const int badgeY = (kHeaderH - kBadgeH) / 2;

        g.setColour (m_groupColour.withAlpha (0.25f));
        g.fillRoundedRectangle (juce::Rectangle<int> (8, badgeY, kBadgeW, kBadgeH).toFloat(), Theme::Radius::chip);

        g.setFont (Theme::Font::micro());
        g.setColour (m_groupColour.brighter (0.3f));
        g.drawText (m_typeName,
                    juce::Rectangle<int> (8, badgeY, kBadgeW, kBadgeH),
                    juce::Justification::centred, false);

        // Group name
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText (m_groupName,
                    56, 0, getWidth() - 64, kHeaderH,
                    juce::Justification::centredLeft, false);
    }
    else
    {
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText ("SELECT A MODULE TO EDIT PARAMETERS",
                    8, 0, getWidth() - 8, kHeaderH,
                    juce::Justification::centredLeft, false);
    }

    // Bottom divider
    g.setColour (Theme::Colour::surfaceEdge);
    g.fillRect (0, getHeight() - 1, getWidth(), 1);
}

void ModuleDetailPanel::resized()
{
    m_editor.setBounds (0, kHeaderH, getWidth(), juce::jmax (0, getHeight() - kHeaderH));
}

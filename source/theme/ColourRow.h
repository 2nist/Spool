#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "ThemeManager.h"
#include "../Theme.h"

//==============================================================================
/**
    ColourRow — a single-row colour editor for ThemeEditorPanel.

    Shows a label on the left, a clickable colour swatch in the middle,
    and a hex text field on the right. Clicking the swatch opens a
    juce::ColourSelector in a popup CallOutBox.

    Usage:
        auto row = std::make_unique<ColourRow> ("Surface 0", &ThemeData::surface0);
        addAndMakeVisible (*row);
*/
class ColourRow final : public juce::Component,
                        public juce::ChangeListener,
                        private juce::TextEditor::Listener
{
public:
    explicit ColourRow (const juce::String& label,
                        juce::Colour ThemeData::* memberPtr)
        : m_label (label),
          m_ptr (memberPtr)
    {
        addAndMakeVisible (m_hexField);
        m_hexField.setFont (Theme::Font::micro());
        m_hexField.setJustification (juce::Justification::centred);
        m_hexField.setInputRestrictions (8, "0123456789abcdefABCDEF");
        m_hexField.addListener (this);
        refresh();
    }

    ~ColourRow() override
    {
        // Deregister from any still-open ColourSelector to prevent dangling listener
        if (m_selectorPtr != nullptr)
            m_selectorPtr->removeChangeListener (this);
    }

    /** Call to sync displayed value from ThemeManager (e.g. on themeChanged). */
    void refresh()
    {
        const auto col = ThemeManager::get().theme().*m_ptr;
        const auto hex = col.toString().substring (2).toUpperCase();
        m_hexField.setText (hex, juce::dontSendNotification);
        repaint();
    }

    static constexpr int rowHeight() { return 26; }

    void resized() override
    {
        auto r = getLocalBounds().reduced (2, 2);
        const int swatchW = 32;
        const int hexW    = 72;

        m_swatchBounds = r.removeFromRight (swatchW).reduced (2, 0);
        r.removeFromRight (4);
        m_hexField.setBounds (r.removeFromRight (hexW));
        m_labelBounds = r;
    }

    void paint (juce::Graphics& g) override
    {
        // Row background
        g.setColour (Theme::Colour::surface3.withAlpha (0.6f));
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.f), 3.f);

        // Label
        g.setColour (Theme::Colour::inkMuted);
        g.setFont (Theme::Font::micro());
        g.drawText (m_label, m_labelBounds.reduced (4, 0),
                    juce::Justification::centredLeft, true);

        // Swatch — chequerboard for transparency, then colour on top
        const auto sf  = m_swatchBounds.toFloat();
        paintChequerboard (g, m_swatchBounds);
        const auto col = ThemeManager::get().theme().*m_ptr;
        g.setColour (col);
        g.fillRoundedRectangle (sf, 3.f);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRoundedRectangle (sf.reduced (0.5f), 3.f, 1.f);

        if (m_swatchHovered)
        {
            g.setColour (juce::Colours::white.withAlpha (0.15f));
            g.fillRoundedRectangle (sf, 3.f);
        }
    }

    void mouseEnter (const juce::MouseEvent& e) override
    {
        m_swatchHovered = m_swatchBounds.contains (e.getPosition());
        repaint();
    }
    void mouseExit  (const juce::MouseEvent&) override { m_swatchHovered = false; repaint(); }
    void mouseMove  (const juce::MouseEvent& e) override
    {
        const bool h = m_swatchBounds.contains (e.getPosition());
        if (h != m_swatchHovered) { m_swatchHovered = h; repaint(); }
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (m_swatchBounds.contains (e.getPosition()) && e.getDistanceFromDragStart() < 5)
            openSelector();
    }

    // juce::ChangeListener — fires when ColourSelector value changes
    void changeListenerCallback (juce::ChangeBroadcaster* source) override
    {
        if (auto* cs = dynamic_cast<juce::ColourSelector*> (source))
        {
            ThemeManager::get().setColour (m_ptr, cs->getCurrentColour());
            refresh();
        }
    }

private:
    void openSelector()
    {
        // Deregister from any previous selector still alive
        if (m_selectorPtr != nullptr)
            m_selectorPtr->removeChangeListener (this);

        auto cs = std::make_unique<juce::ColourSelector> (
            juce::ColourSelector::showColourAtTop |
            juce::ColourSelector::showSliders     |
            juce::ColourSelector::showColourspace);

        cs->setCurrentColour (ThemeManager::get().theme().*m_ptr);
        cs->setSize (300, 280);
        cs->addChangeListener (this);
        m_selectorPtr = cs.get();

        juce::CallOutBox::launchAsynchronously (
            std::move (cs),
            getScreenBounds(),
            nullptr);
        // CallOutBox now owns the selector; m_selectorPtr is our weak ref for cleanup
    }

    void textEditorReturnKeyPressed (juce::TextEditor& te) override { applyHex (te.getText()); }
    void textEditorFocusLost        (juce::TextEditor& te) override { applyHex (te.getText()); }

    void applyHex (const juce::String& hex)
    {
        if (hex.length() < 6) return;
        const auto padded = "FF" + hex.paddedLeft ('0', 8).substring (0, 6).toUpperCase();
        const auto col    = juce::Colour::fromString (padded);
        ThemeManager::get().setColour (m_ptr, col);
        refresh();
    }

    static void paintChequerboard (juce::Graphics& g, juce::Rectangle<int> r)
    {
        const int sz = 4;
        g.saveState();
        g.reduceClipRegion (r);
        for (int y = r.getY(); y < r.getBottom(); y += sz)
            for (int x = r.getX(); x < r.getRight(); x += sz)
            {
                const bool light = (((x - r.getX()) / sz) + ((y - r.getY()) / sz)) % 2 == 0;
                g.setColour (light ? juce::Colours::lightgrey : juce::Colours::white);
                g.fillRect (x, y, sz, sz);
            }
        g.restoreState();
    }

    juce::String              m_label;
    juce::Colour ThemeData::* m_ptr;

    juce::TextEditor          m_hexField;
    juce::Rectangle<int>      m_swatchBounds;
    juce::Rectangle<int>      m_labelBounds;
    bool                      m_swatchHovered = false;

    juce::ColourSelector*     m_selectorPtr = nullptr;  // raw weak ref, NOT owned

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ColourRow)
};


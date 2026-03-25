#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ThemeManager.h"
#include "../Theme.h"

//==============================================================================
/**
    FloatRow — a single-row float editor for ThemeEditorPanel.

    Shows a label, a Slider (linear bar), and a numeric readout.
    On change, calls ThemeManager::get().setFloat(ptr, value).

    Usage:
        auto row = std::make_unique<FloatRow> ("Label Size", &ThemeData::sizeLabel, 8.f, 20.f);
        addAndMakeVisible (*row);
*/
class FloatRow final : public juce::Component,
                       private juce::Slider::Listener
{
public:
    FloatRow (const juce::String& label,
              float ThemeData::* memberPtr,
              float minVal,
              float maxVal,
              float stepVal = 0.5f)
        : m_label (label),
          m_ptr (memberPtr)
    {
        addAndMakeVisible (m_slider);
        m_slider.setSliderStyle (juce::Slider::LinearBar);
        m_slider.setRange (static_cast<double>(minVal),
                           static_cast<double>(maxVal),
                           static_cast<double>(stepVal));
        m_slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 18);
        m_slider.addListener (this);
        refresh();
    }

    /** Call to sync displayed value from ThemeManager (e.g. on themeChanged). */
    void refresh()
    {
        const float v = ThemeManager::get().theme().*m_ptr;
        m_slider.setValue (static_cast<double>(v), juce::dontSendNotification);
    }

    static constexpr int rowHeight() { return 26; }

    void resized() override
    {
        auto r = getLocalBounds().reduced (2, 2);
        const int labelW = 100;
        m_labelBounds = r.removeFromLeft (labelW);
        r.removeFromLeft (4);
        m_slider.setBounds (r);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Theme::Colour::surface3.withAlpha (0.6f));
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.f), 3.f);

        g.setColour (Theme::Colour::inkMuted);
        g.setFont (Theme::Font::micro());
        g.drawText (m_label, m_labelBounds.reduced (4, 0),
                    juce::Justification::centredLeft, true);
    }

private:
    void sliderValueChanged (juce::Slider* s) override
    {
        if (s == &m_slider)
            ThemeManager::get().setFloat (m_ptr, static_cast<float> (m_slider.getValue()));
    }

    juce::String              m_label;
    float ThemeData::*        m_ptr;

    juce::Slider              m_slider;
    juce::Rectangle<int>      m_labelBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FloatRow)
};

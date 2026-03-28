#include "VerticalFader.h"

//==============================================================================
// Constructor / Destructor
//==============================================================================

VerticalFader::VerticalFader()
{
    ThemeManager::get().addListener (this);

    m_slider.setSliderStyle (juce::Slider::LinearVertical);
    m_slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    m_slider.setRange (0.0, 1.0, 0.001);
    m_slider.setValue (m_value, juce::dontSendNotification);
    m_slider.setDoubleClickReturnValue (true, m_defaultValue);
    m_slider.setPopupDisplayEnabled (true, true, this);
    m_slider.getProperties().set ("tint", m_paramColor.toString());

    m_slider.textFromValueFunction = [this] (double value)
    {
        return formatValue ((float) value);
    };

    m_slider.valueFromTextFunction = [] (const juce::String& text)
    {
        return text.getDoubleValue();
    };

    m_slider.onValueChange = [this]
    {
        m_value = (float) m_slider.getValue();
        notifyValueChanged();
    };

    addAndMakeVisible (m_slider);
    m_slider.addMouseListener (this, false);
}

VerticalFader::~VerticalFader()
{
    stopTimer();
    ThemeManager::get().removeListener (this);
}

//==============================================================================
// Value
//==============================================================================

void VerticalFader::setValue (float normalized, bool notify)
{
    m_value = juce::jlimit (0.0f, 1.0f, normalized);
    m_slider.setValue (m_value, notify ? juce::sendNotificationSync : juce::dontSendNotification);
    repaint();
}

void VerticalFader::setValueFromProcessor (float normalized)
{
    m_value = juce::jlimit (0.0f, 1.0f, normalized);
    m_slider.setValue (m_value, juce::dontSendNotification);
    repaint();
}

void VerticalFader::notifyValueChanged()
{
    if (onValueChanged)
        onValueChanged (m_value);
}

//==============================================================================
// Status
//==============================================================================

void VerticalFader::setStatus (Status s)
{
    m_status = s;

    if (m_status == Status::midiLearn)
    {
        m_blinkCounter = 0;
        m_blinkOn = true;
        startTimerHz (10);
    }
    else if (isTimerRunning())
    {
        stopTimer();
    }

    repaint();
}

//==============================================================================
// Sizing
//==============================================================================

int VerticalFader::getPreferredWidth() const
{
    return 18;
}

//==============================================================================
// Layout / Paint
//==============================================================================

void VerticalFader::resized()
{
    m_slider.setBounds (getLocalBounds());
}

void VerticalFader::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);
}

//==============================================================================
// Mouse
//==============================================================================

void VerticalFader::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
        showContextMenu();
}

void VerticalFader::mouseDrag (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
}

void VerticalFader::mouseUp (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
}

void VerticalFader::mouseEnter (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
}

void VerticalFader::mouseExit (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
}

void VerticalFader::mouseDoubleClick (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    setValue (m_defaultValue, true);
}

void VerticalFader::mouseWheelMove (const juce::MouseEvent& e,
                                    const juce::MouseWheelDetails& d)
{
    juce::ignoreUnused (e, d);
}

//==============================================================================
// Timer
//==============================================================================

void VerticalFader::timerCallback()
{
    ++m_blinkCounter;
    if (m_blinkCounter >= 6)
        m_blinkCounter = 0;

    m_blinkOn = (m_blinkCounter < 5);
    repaint();
}

//==============================================================================
// Context menu
//==============================================================================

void VerticalFader::showContextMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("VALUE");
    menu.addItem (1, "Enter value...");
    menu.addItem (2, "Reset to default");

    if (onBuildExtraMenuItems)
    {
        menu.addSeparator();
        onBuildExtraMenuItems (menu);
    }

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
        [this] (int result)
        {
            if (result == 1)
            {
                showValueEditor();
            }
            else if (result == 2)
            {
                setValue (m_defaultValue, true);
            }
            else if (result >= kExtraMenuBase && onExtraMenuItemSelected)
            {
                onExtraMenuItemSelected (result);
            }
        });
}

void VerticalFader::showValueEditor()
{
    m_textEditor = std::make_unique<juce::TextEditor>();
    m_textEditor->setText (formatValue (m_value), false);
    m_textEditor->setSelectAllWhenFocused (true);
    m_textEditor->setBounds (0, 0, juce::jmax (44, getWidth()), 16);

    addAndMakeVisible (*m_textEditor);
    m_textEditor->grabKeyboardFocus();

    m_textEditor->onReturnKey = [this]
    {
        const float v = m_textEditor->getText().getFloatValue();
        setValue (juce::jlimit (0.0f, 1.0f, v), true);
        m_textEditor.reset();
    };

    m_textEditor->onEscapeKey = [this]
    {
        m_textEditor.reset();
    };

    m_textEditor->onFocusLost = [this]
    {
        m_textEditor.reset();
    };
}

juce::String VerticalFader::formatValue (float norm) const
{
    if (m_formatter)
        return m_formatter (norm);

    return juce::String (norm, 2);
}

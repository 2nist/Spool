#include "VerticalFader.h"

void VerticalFader::SlotSlider::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        owner.showContextMenu();
        return;
    }

    juce::Slider::mouseDown (e);
}

VerticalFader::VerticalFader()
{
    m_slider.setSliderStyle (juce::Slider::LinearBarVertical);
    m_slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    m_slider.setRange (0.0, 1.0, 0.0);
    m_slider.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    m_slider.onValueChange = [this]
    {
        if (m_internalSetValue)
            return;

        m_value = static_cast<float> (m_slider.getValue());
        updateTooltip();
        notifyValueChanged();
        repaint();
    };

    addAndMakeVisible (m_slider);
    setDefaultValue (m_defaultValue);
    setDisplayName (m_displayName);
    setParamColor (m_paramColor);
    setValue (m_value, false);
}

VerticalFader::~VerticalFader() = default;

void VerticalFader::setDisplayName (const juce::String& name)
{
    m_displayName = name;
    m_slider.setName (m_displayName);
    updateTooltip();
    repaint();
}

void VerticalFader::setParamColor (juce::Colour c)
{
    m_paramColor = c;
    m_slider.getProperties().set ("tint", c.toString());
    m_slider.repaint();
}

void VerticalFader::setDefaultValue (float normalizedDefault)
{
    m_defaultValue = juce::jlimit (0.0f, 1.0f, normalizedDefault);
    m_slider.setDoubleClickReturnValue (true, m_defaultValue);
}

void VerticalFader::setValue (float normalized, bool notify)
{
    m_value = juce::jlimit (0.0f, 1.0f, normalized);
    {
        const juce::ScopedValueSetter<bool> setting (m_internalSetValue, true);
        m_slider.setValue (m_value, juce::dontSendNotification);
    }

    updateTooltip();
    if (notify)
        notifyValueChanged();
}

void VerticalFader::setValueFromProcessor (float normalized)
{
    setValue (normalized, false);
}

void VerticalFader::setStatus (Status s)
{
    m_status = s;
    m_slider.setAlpha (m_status == Status::bypassed ? Theme::Alpha::disabled : 1.0f);
    repaint();
}

int VerticalFader::getPreferredWidth() const
{
    return 24;
}

void VerticalFader::paint (juce::Graphics& g)
{
    if (m_status == Status::none)
        return;

    juce::Colour dot = Theme::Colour::inkGhost;
    switch (m_status)
    {
        case Status::macroMapped:  dot = juce::Colour (0xFF4aee8a); break;
        case Status::midiLearn:    dot = juce::Colour (0xFFee4a4a); break;
        case Status::midiAssigned: dot = juce::Colour (0xFF4a9eff); break;
        case Status::modulated:    dot = juce::Colour (0xFFeec44a); break;
        case Status::automated:    dot = juce::Colour (0xFFee7c4a); break;
        case Status::follower:     dot = juce::Colour (0xFF6a5a8a); break;
        case Status::bypassed:     dot = juce::Colour (0xFF3a2e1e); break;
        case Status::none: break;
    }

    const auto r = getLocalBounds().removeFromBottom (8).removeFromRight (8).toFloat();
    g.setColour (dot);
    g.fillEllipse (r);
}

void VerticalFader::resized()
{
    m_slider.setBounds (getLocalBounds());
}

void VerticalFader::notifyValueChanged ()
{
    if (onValueChanged)
        onValueChanged (m_value);
}

void VerticalFader::showContextMenu()
{
    juce::PopupMenu menu;

    menu.addSectionHeader ("VALUE");
    menu.addItem (1, "Enter value...");
    menu.addItem (2, "Reset to default");

    menu.addSeparator();
    menu.addSectionHeader ("MACRO");
    {
        juce::PopupMenu macroSub;
        for (int k = 1; k <= 8; ++k)
            macroSub.addItem (100 + k, "K" + juce::String (k));
        menu.addSubMenu ("Assign to macro knob", macroSub);
        if (m_status == Status::macroMapped)
            menu.addItem (10, "Clear macro assignment");
    }

    menu.addSeparator();
    menu.addSectionHeader ("MIDI");
    menu.addItem (20, "MIDI learn");
    if (m_status == Status::midiAssigned)
        menu.addItem (21, "Clear MIDI assignment");

    menu.addSeparator();
    menu.addSectionHeader ("MODULATION");
    {
        juce::PopupMenu modSub;
        modSub.addItem (200, "LFO 1");
        modSub.addItem (201, "LFO 2");
        modSub.addItem (202, "Filter Envelope");
        modSub.addItem (203, "Amp Envelope");
        menu.addSubMenu ("Modulate with...", modSub);
        if (m_status == Status::modulated)
            menu.addItem (30, "Clear modulation");
    }

    menu.addSeparator();
    menu.addItem (40, m_status == Status::bypassed ? "Un-bypass parameter"
                                                    : "Bypass parameter");

    if (onBuildExtraMenuItems)
    {
        menu.addSeparator();
        onBuildExtraMenuItems (menu);
    }

    auto safeThis = juce::Component::SafePointer<VerticalFader> (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                        [safeThis] (int result)
                        {
                            if (safeThis == nullptr)
                                return;
                            safeThis->handleMenuResult (result);
                        });
}

void VerticalFader::handleMenuResult (int result)
{
    switch (result)
    {
        case 1:  showValueEditor();                             break;
        case 2:  setValue (m_defaultValue, true);               break;
        case 10: setStatus (Status::none);                      break;
        case 20: setStatus (Status::midiLearn);                 break;
        case 21: setStatus (Status::none);                      break;
        case 30: setStatus (Status::none);                      break;
        case 40:
            setStatus (m_status == Status::bypassed ? Status::none : Status::bypassed);
            break;
        default:
            if (result >= 101 && result <= 108)
            {
                setStatus (Status::macroMapped);
                setStatusLabel ("K" + juce::String (result - 100));
            }
            else if (result >= 200 && result <= 203)
            {
                const char* names[] = { "LFO 1", "LFO 2", "Filter Envelope", "Amp Envelope" };
                setStatus (Status::modulated);
                setStatusLabel (names[result - 200]);
            }
            else if (result >= kExtraMenuBase && onExtraMenuItemSelected)
            {
                onExtraMenuItemSelected (result);
            }
            break;
    }
}

void VerticalFader::showValueEditor ()
{
    m_textEditor = std::make_unique<juce::TextEditor>();
    m_textEditor->setFont (Theme::Font::micro());
    m_textEditor->setColour (juce::TextEditor::backgroundColourId, Theme::Colour::surface1);
    m_textEditor->setColour (juce::TextEditor::textColourId, Theme::Colour::inkLight);
    m_textEditor->setColour (juce::TextEditor::outlineColourId, m_paramColor);
    m_textEditor->setText (juce::String (m_value, 3), false);
    m_textEditor->setSelectAllWhenFocused (true);

    auto editorBounds = getLocalBounds().removeFromTop (14).reduced (1, 0);
    editorBounds = editorBounds.withWidth (juce::jmax (24, editorBounds.getWidth()));
    m_textEditor->setBounds (editorBounds);

    addAndMakeVisible (*m_textEditor);
    m_textEditor->grabKeyboardFocus();

    m_textEditor->onReturnKey = [this]
    {
        const float entered = m_textEditor->getText().getFloatValue();
        setValue (juce::jlimit (0.0f, 1.0f, entered), true);
        m_textEditor.reset();
        repaint();
    };

    m_textEditor->onEscapeKey = [this]
    {
        m_textEditor.reset();
        repaint();
    };

    m_textEditor->onFocusLost = [this]
    {
        m_textEditor.reset();
        repaint();
    };
}

void VerticalFader::updateTooltip ()
{
    auto tip = m_displayName;
    if (tip.isNotEmpty())
        tip << ": ";
    tip << formatValue (m_value);
    m_slider.setTooltip (tip);
}

juce::String VerticalFader::formatValue (float norm) const
{
    if (m_formatter)
        return m_formatter (norm);

    return juce::String (norm, 2);
}

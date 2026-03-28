#include "VerticalFader.h"
#include "../ZoneA/ZoneAControlStyle.h"

//==============================================================================
// Constructor / Destructor
//==============================================================================

VerticalFader::VerticalFader()
{
    ThemeManager::get().addListener (this);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

VerticalFader::~VerticalFader()
{
    stopTimer();  // CRITICAL — always stop before destruction
    ThemeManager::get().removeListener (this);
}

//==============================================================================
// Value
//==============================================================================

void VerticalFader::setValue (float normalized, bool notify)
{
    m_value = juce::jlimit (0.0f, 1.0f, normalized);
    if (notify) notifyValueChanged();
    repaint();
}

void VerticalFader::setValueFromProcessor (float normalized)
{
    // Does NOT call onValueChanged — prevents processor → UI → processor feedback
    m_value = juce::jlimit (0.0f, 1.0f, normalized);
    repaint();
}

void VerticalFader::notifyValueChanged()
{
    if (onValueChanged) onValueChanged (m_value);
}

//==============================================================================
// Status
//==============================================================================

void VerticalFader::setStatus (Status s)
{
    // MIDI learn always wins immediately — override anything
    if (s == Status::midiLearn)
    {
        m_status = s;
        m_blinkCounter = 0;
        m_blinkOn = true;
        startTimerHz (10);
        repaint();
        return;
    }

    // Do not let lower priority replace an active MIDI learn
    if (m_status == Status::midiLearn && s != Status::midiLearn)
        return;

    // Stop MIDI blink if leaving midiLearn (and no flash pending)
    if (m_status == Status::midiLearn && m_flashAlpha <= 1.0f)
        stopTimer();

    m_status = s;
    repaint();
}

//==============================================================================
// Sizing
//==============================================================================

int VerticalFader::getPreferredWidth() const
{
    // Match Zone A compact vertical bar footprint.
    return 18;
}

//==============================================================================
// Layout
//==============================================================================

void VerticalFader::resized() {}

juce::Rectangle<int> VerticalFader::trackBounds() const noexcept
{
    const int trackX = (getWidth() - TRACK_W) / 2;
    return { trackX, VALUE_AREA_H, TRACK_W, TRACK_H };
}

juce::Rectangle<int> VerticalFader::thumbBounds() const noexcept
{
    const int thumbCY = valueToY (m_value);
    return { 0, thumbCY - THUMB_SIZE / 2, THUMB_SIZE, THUMB_SIZE };
}

juce::Rectangle<int> VerticalFader::circleBounds() const noexcept
{
    return thumbBounds().withSizeKeepingCentre (CIRCLE_SIZE, CIRCLE_SIZE);
}

//==============================================================================
// Paint
//==============================================================================

void VerticalFader::paint (juce::Graphics& g)
{
    paintTrack  (g);
    paintFill   (g);
    paintThumb  (g);
    paintCircle (g);
    paintValue  (g);
    paintLabel  (g);
}

void VerticalFader::paintTrack (juce::Graphics& g)
{
    const auto& theme = ThemeManager::get().theme();
    const float a  = (m_status == Status::bypassed) ? Theme::Alpha::disabled : 1.0f;
    const auto  tb = trackBounds().toFloat();

    // Background
    g.setColour (theme.controlBg.withMultipliedAlpha (a).withAlpha (0.96f));
    g.fillRoundedRectangle (tb, 4.0f);

    // Border — brightens on hover
    const juce::Colour borderCol = m_hovered ? theme.surfaceEdge.brighter (0.12f)
                                             : theme.surfaceEdge;
    g.setColour (borderCol.withMultipliedAlpha (a));
    g.drawRoundedRectangle (tb, 4.0f, 1.0f);

    // Bipolar centre mark — always visible (structural, not decorative)
    if (m_bipolar)
    {
        const float cy = static_cast<float> (trackBounds().getCentreY());
        g.setColour (theme.surfaceEdge.withMultipliedAlpha (a).withAlpha (0.55f));
        g.drawLine (tb.getX(), cy, tb.getRight(), cy, 0.5f);
    }
}

void VerticalFader::paintFill (juce::Graphics& g)
{
    const auto& theme = ThemeManager::get().theme();
    const float a  = (m_status == Status::bypassed) ? Theme::Alpha::disabled : 1.0f;
    const auto  tb = trackBounds();
    const auto fillCol = theme.sliderTrack.interpolatedWith (m_paramColor, 0.4f);

    if (!m_bipolar)
    {
        auto fillR = tb;
        fillR.setTop (valueToY (m_value));
        g.setColour (fillCol.withAlpha (0.55f * a));
        g.fillRoundedRectangle (fillR.toFloat(), 3.0f);
    }
    else
    {
        const int cy = tb.getCentreY();
        const int ty = valueToY (m_value);
        const juce::Rectangle<int> fillR =
            (ty < cy) ? juce::Rectangle<int> (tb.getX(), ty,  tb.getWidth(), cy - ty)
                      : juce::Rectangle<int> (tb.getX(), cy,  tb.getWidth(), ty - cy);
        g.setColour (fillCol.withAlpha (0.45f * a));
        g.fillRect (fillR);
    }
}

void VerticalFader::paintLine (juce::Graphics& g)
{
    const auto& theme = ThemeManager::get().theme();
    if (m_status == Status::bypassed) return;
    const auto  tb = trackBounds();
    const float y  = static_cast<float> (valueToY (m_value));
    g.setColour (theme.sliderThumb.interpolatedWith (m_paramColor, 0.35f).withAlpha (0.92f));
    g.drawLine (static_cast<float> (tb.getX()), y,
                static_cast<float> (tb.getRight()), y, 1.0f);
}

void VerticalFader::paintThumb (juce::Graphics& g)
{
    const auto& theme = ThemeManager::get().theme();
    const float a      = (m_status == Status::bypassed) ? Theme::Alpha::disabled : 1.0f;
    const float bright = m_flashAlpha;  // >1.0 briefly after double-click reset
    const auto  tb     = thumbBounds().toFloat();
    const auto thumbBase = theme.sliderThumb.interpolatedWith (m_paramColor, 0.2f);

    g.setColour (thumbBase.withMultipliedAlpha (a).withMultipliedBrightness (bright));
    g.fillEllipse (tb);
    g.setColour (theme.surfaceEdge.withMultipliedAlpha (a * 0.8f));
    g.drawEllipse (tb, 1.0f);
}

void VerticalFader::paintCircle (juce::Graphics& g)
{
    const auto& theme = ThemeManager::get().theme();
    const float a  = (m_status == Status::bypassed) ? Theme::Alpha::disabled : 1.0f;
    const auto  cb = circleBounds().toFloat();

    juce::Colour fill;
    switch (m_status)
    {
        case Status::none:         fill = juce::Colour (0xFF080604); break;
        case Status::macroMapped:  fill = juce::Colour (0xFF4aee8a); break;
        case Status::midiLearn:
            fill = m_blinkOn ? juce::Colour (0xFFee4a4a)
                             : juce::Colour (0xFF080604);
            break;
        case Status::midiAssigned: fill = juce::Colour (0xFF4a9eff); break;
        case Status::modulated:    fill = juce::Colour (0xFFeec44a); break;
        case Status::automated:    fill = juce::Colour (0xFFee7c4a); break;
        case Status::follower:     fill = juce::Colour (0xFF6a5a8a); break;
        case Status::bypassed:     fill = juce::Colour (0xFF3a2e1e); break;
    }

    // Small status marker at thumb centre (not a second large control).
    g.setColour (fill.withMultipliedAlpha (a));
    g.fillEllipse (cb.withSizeKeepingCentre (3.0f, 3.0f));
}

void VerticalFader::paintValue (juce::Graphics& g)
{
    const auto& theme = ThemeManager::get().theme();
    const juce::String text = formatValue (m_value);
    const auto         font = Theme::Font::micro();
    const float textW = juce::GlyphArrangement::getStringWidth (font, text);

    const float thumbCY = static_cast<float> (valueToY (m_value));
    const float textX   = static_cast<float> (thumbBounds().getCentreX()) - textW * 0.5f;
    // Float above thumb — clamp so it never goes above component top
    const float textY   = juce::jmax (1.0f, thumbCY - 14.0f);

    const juce::Rectangle<float> bg (textX - 2.0f, textY - 1.0f, textW + 4.0f, 12.0f);

    // Background block masks the track behind the value text
    g.setColour (theme.controlBg);
    g.fillRoundedRectangle (bg, 2.0f);

    // Value text — param colour while dragging, light otherwise
    g.setFont (font);
    g.setColour (m_dragging ? m_paramColor : theme.controlTextOn);
    g.drawText  (text, bg.withTrimmedLeft (2.0f).withTrimmedRight (2.0f),
                 juce::Justification::centred, false);
}

void VerticalFader::paintLabel (juce::Graphics& g)
{
    const auto& theme = ThemeManager::get().theme();
    if (m_displayName.isEmpty()) return;

    const auto tb = trackBounds().toFloat();
    const auto fillCol = theme.sliderTrack.interpolatedWith (m_paramColor, 0.4f);
    const float proportion = juce::jlimit (0.0f, 1.0f, m_value);
    const bool overFill = proportion >= 0.58f;
    const auto labelBg = overFill ? fillCol : theme.controlBg;
    const auto textColour = Theme::Helper::inkFor (labelBg).withAlpha (0.97f);
    const auto shadowColour = textColour.getPerceivedBrightness() > 0.5f
                                ? Theme::Colour::inkDark.withAlpha (0.24f)
                                : Theme::Colour::inkLight.withAlpha (0.16f);

    g.setFont (Theme::Font::microMedium());
    juce::Graphics::ScopedSaveState state (g);
    const auto centre = tb.getCentre();
    g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, centre.x, centre.y));

    auto textBounds = tb.toNearestInt().reduced (0, 5);
    g.setColour (shadowColour);
    g.drawText (m_displayName, textBounds.translated (1, 1), juce::Justification::centred, false);
    g.setColour (textColour);
    g.drawText (m_displayName, textBounds, juce::Justification::centred, false);
}

//==============================================================================
// Mouse
//==============================================================================

void VerticalFader::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        showContextMenu();
        return;
    }

    m_dragging     = true;
    m_dragStartY   = static_cast<float> (e.getScreenY());
    m_dragStartVal = m_value;
    repaint();
}

void VerticalFader::mouseDrag (const juce::MouseEvent& e)
{
    if (!m_dragging) return;

    const float deltaY      = m_dragStartY - static_cast<float> (e.getScreenY());
    const float sensitivity = (e.mods.isShiftDown() ? 0.1f : 1.0f)
                              / static_cast<float> (TRACK_H);

    if (e.mods.isCtrlDown())
    {
        m_value = m_defaultValue;
    }
    else
    {
        m_value = juce::jlimit (0.0f, 1.0f,
                                m_dragStartVal + deltaY * sensitivity);
    }

    notifyValueChanged();
    repaint();
}

void VerticalFader::mouseUp (const juce::MouseEvent&)
{
    m_dragging = false;
    repaint();
}

void VerticalFader::mouseEnter (const juce::MouseEvent&)
{
    m_hovered = true;
    repaint();
}

void VerticalFader::mouseExit (const juce::MouseEvent&)
{
    m_hovered = false;
    repaint();
}

void VerticalFader::mouseDoubleClick (const juce::MouseEvent&)
{
    m_value = m_defaultValue;
    notifyValueChanged();

    // Brief brightness flash on thumb to confirm reset
    m_flashAlpha = 1.6f;
    if (!isTimerRunning())
        startTimerHz (10);
    repaint();
}

void VerticalFader::mouseWheelMove (const juce::MouseEvent&,
                                    const juce::MouseWheelDetails& d)
{
    m_value = juce::jlimit (0.0f, 1.0f, m_value + d.deltaY * 0.05f);
    notifyValueChanged();
    repaint();
}

//==============================================================================
// Timer — MIDI learn blink + double-click flash
//==============================================================================

void VerticalFader::timerCallback()
{
    if (m_status == Status::midiLearn)
    {
        // Asymmetric pulse: 5 ticks ON (500ms) then 1 tick OFF (100ms) at 10Hz
        ++m_blinkCounter;
        if (m_blinkCounter < 5)
        {
            m_blinkOn = true;
        }
        else
        {
            m_blinkOn = false;
            if (m_blinkCounter >= 5)
                m_blinkCounter = 0;
        }
        repaint();
    }

    if (m_flashAlpha > 1.0f)
    {
        m_flashAlpha = juce::jmax (1.0f, m_flashAlpha - 0.25f);
        repaint();

        // Stop timer if flash is done AND MIDI learn is not running
        if (m_flashAlpha <= 1.0f && m_status != Status::midiLearn)
            stopTimer();
    }
}

//==============================================================================
// Context menu
//==============================================================================

void VerticalFader::showContextMenu()
{
    juce::PopupMenu menu;

    // VALUE
    menu.addSectionHeader ("VALUE");
    menu.addItem (1, "Enter value\xe2\x80\xa6");
    menu.addItem (2, "Reset to default");

    // MACRO
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

    // MIDI
    menu.addSeparator();
    menu.addSectionHeader ("MIDI");
    menu.addItem (20, "MIDI learn");
    if (m_status == Status::midiAssigned)
        menu.addItem (21, "Clear MIDI assignment");

    // MODULATION
    menu.addSeparator();
    menu.addSectionHeader ("MODULATION");
    {
        juce::PopupMenu modSub;
        modSub.addItem (200, "LFO 1");
        modSub.addItem (201, "LFO 2");
        modSub.addItem (202, "Filter Envelope");
        modSub.addItem (203, "Amp Envelope");
        menu.addSubMenu ("Modulate with\xe2\x80\xa6", modSub);
        if (m_status == Status::modulated)
            menu.addItem (30, "Clear modulation");
    }

    // OTHER
    menu.addSeparator();
    menu.addItem (40, m_status == Status::bypassed ? "Un-bypass parameter"
                                                   : "Bypass parameter");

    // Extra items injected by the parent (e.g. FaceplatePanel slot assignment)
    if (onBuildExtraMenuItems)
    {
        menu.addSeparator();
        onBuildExtraMenuItems (menu);
    }

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
        [this] (int result)
        {
            switch (result)
            {
                case 1:  showValueEditor();                             break;
                case 2:  setValue (m_defaultValue, true);              break;
                case 10: setStatus (Status::none);                     break;
                case 20: setStatus (Status::midiLearn);                break;
                case 21: setStatus (Status::none);                     break;
                case 30: setStatus (Status::none);                     break;
                case 40:
                    setStatus (m_status == Status::bypassed
                               ? Status::none : Status::bypassed);
                    break;
                default:
                    if (result >= 101 && result <= 108)
                    {
                        setStatus (Status::macroMapped);
                        setStatusLabel ("K" + juce::String (result - 100));
                    }
                    else if (result >= 200 && result <= 203)
                    {
                        const char* names[] = { "LFO 1", "LFO 2",
                                                "Filter Envelope", "Amp Envelope" };
                        setStatus (Status::modulated);
                        setStatusLabel (names[result - 200]);
                    }
                    else if (result >= kExtraMenuBase && onExtraMenuItemSelected)
                    {
                        onExtraMenuItemSelected (result);
                    }
                    break;
            }
        });
}

//==============================================================================
// Inline value editor
//==============================================================================

void VerticalFader::showValueEditor()
{
    m_textEditor = std::make_unique<juce::TextEditor>();
    m_textEditor->setFont (Theme::Font::micro());
    ZoneAControlStyle::styleTextEditor (*m_textEditor);
    m_textEditor->setColour (juce::TextEditor::focusedOutlineColourId, m_paramColor);
    m_textEditor->setText (formatValue (m_value), false);
    m_textEditor->setSelectAllWhenFocused (true);

    const int edW = 44, edH = 14;
    const int thumbCY = valueToY (m_value);
    m_textEditor->setBounds (0, juce::jmax (0, thumbCY - edH - 2), edW, edH);

    addAndMakeVisible (*m_textEditor);
    m_textEditor->grabKeyboardFocus();

    m_textEditor->onReturnKey = [this]
    {
        const float v = m_textEditor->getText().getFloatValue();
        setValue (juce::jlimit (0.0f, 1.0f, v), true);
        m_textEditor.reset();
        repaint();
    };

    m_textEditor->onFocusLost = [this]
    {
        m_textEditor.reset();
        repaint();
    };
}

//==============================================================================
// Value formatting
//==============================================================================

juce::String VerticalFader::formatValue (float norm) const
{
    if (m_formatter)
        return m_formatter (norm);

    // Default: show to 2 decimal places
    return juce::String (norm, 2);
}

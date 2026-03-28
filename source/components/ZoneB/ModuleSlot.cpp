#include "ModuleSlot.h"

namespace
{
struct ModuleLayoutProfile
{
    int topTextRows = 0;                    // number of compact rows reserved for status text
    bool showCompactControls = true;        // show slider rows in module face
    const char* topStatusText = "";         // optional first-row status text
    bool emphasizeTopStatus = false;        // heading style vs micro style
    bool showOpenEditorLink = false;        // draw/use "OPEN EDITOR ↗" hotspot
    int openEditorHotZoneHeight = 12;       // clickable area at bottom of face
};

ModuleLayoutProfile layoutProfileForType (ModuleSlot::ModuleType type)
{
    switch (type)
    {
        case ModuleSlot::ModuleType::Sampler:
            return { 1, true, "no sample loaded", false, false, 12 };
        case ModuleSlot::ModuleType::Vst3:
            return { 1, true, "\xE2\x80\x94 empty \xE2\x80\x94", true, true, 12 };
        case ModuleSlot::ModuleType::DrumMachine:
            return { 0, false, "", false, false, 12 };
        default:
            return { 0, true, "", false, false, 12 };
    }
}
}

//==============================================================================
// Construction
//==============================================================================

ModuleSlot::ModuleSlot (int index)
    : m_index (index)
{
    for (int i = 0; i < kCompactRows; ++i)
    {
        auto& slider = m_compactSliders[(size_t) i];
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRange (0.0, 1.0, 0.001);
        slider.setScrollWheelEnabled (false);
        slider.setPopupDisplayEnabled (true, true, this);
        slider.onValueChange = [this, i]
        {
            const int paramIdx = m_compactParamIndex[(size_t) i];
            if (paramIdx < 0 || paramIdx >= m_allParams.size())
                return;

            auto& p = m_allParams.getReference (paramIdx);
            p.value = (float) m_compactSliders[(size_t) i].getValue();
            m_compactValues[(size_t) i].setText (juce::String (p.value, 2), juce::dontSendNotification);

            m_inlineEditor.setParams (m_allParams, getSignalColour(),
                                      getModuleTypeName(), m_inPorts, m_outPorts);
        };
        addAndMakeVisible (slider);

        auto& label = m_compactLabels[(size_t) i];
        label.setJustificationType (juce::Justification::centredLeft);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);

        auto& value = m_compactValues[(size_t) i];
        value.setJustificationType (juce::Justification::centredRight);
        value.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (value);
    }

    // InlineEditor is added LAST so it paints above everything else
    addAndMakeVisible (m_inlineEditor);
    m_inlineEditor.setVisible (false);

    refreshCompactControls();
}

//==============================================================================
// State setters
//==============================================================================

void ModuleSlot::setModuleType (ModuleType type)
{
    m_type = type;
    initParams();

    // Create / destroy drum machine data
    if (type == ModuleType::DrumMachine)
    {
        m_drumData = std::make_unique<DrumMachineData> (DrumMachineData::makeDefault());
    }
    else
    {
        m_drumData.reset();
    }

    m_inlineEditor.setParams (m_allParams, getSignalColour(),
                               getModuleTypeName(), m_inPorts, m_outPorts);
    m_inlineEditor.setVisible (false);  // never auto-show in group layout
    refreshCompactControls();
    resized();
    repaint();
}

void ModuleSlot::setSelected (bool selected)
{
    m_selected = selected;
    m_inlineEditor.setVisible (false);  // InlineEditor not used in group layout
    repaint();
}

void ModuleSlot::setFocused (bool focused)
{
    m_focused = focused;
    repaint();
}

void ModuleSlot::setGroupColor (juce::Colour c)
{
    m_groupColor = c;
    repaint();
}

void ModuleSlot::setMuted (bool muted)
{
    m_muted = muted;
    refreshCompactControls();
    repaint();
}

//==============================================================================
// Identity helpers
//==============================================================================

juce::String ModuleSlot::getModuleTypeName() const
{
    switch (m_type)
    {
        case ModuleType::Synth:        return "SYNTH";
        case ModuleType::Sampler:      return "SAMPLER";
        case ModuleType::Sequencer:    return "SEQ";
        case ModuleType::Vst3:         return "VST3";
        case ModuleType::DrumMachine:  return "DRUM MACHINE";
        case ModuleType::Output:       return "OUTPUT";
        default:                       return "";
    }
}

juce::Colour ModuleSlot::getSignalColour() const
{
    switch (m_type)
    {
        case ModuleType::Synth:
        case ModuleType::Sampler:      return Theme::Signal::audio;
        case ModuleType::Sequencer:    return Theme::Signal::midi;
        case ModuleType::Vst3:         return Theme::Zone::menu;
        case ModuleType::DrumMachine:  return Theme::Signal::midi;
        case ModuleType::Output:       return Theme::Colour::error;
        default:                       return Theme::Colour::surfaceEdge;
    }
}

//==============================================================================
// Param initialisation
//==============================================================================

void ModuleSlot::initParams()
{
    m_allParams.clear();
    m_inPorts.clear();
    m_outPorts.clear();

    using P = InlineEditor::Param;

    switch (m_type)
    {
        case ModuleType::Synth:
            m_allParams.add ({ "FREQ",   20.0f,  2000.0f, 440.0f });
            m_allParams.add ({ "DETUNE", -50.0f,   50.0f,   0.0f });
            m_allParams.add ({ "GAIN",    0.0f,    1.0f,   0.8f  });
            m_allParams.add ({ "PHASE",   0.0f,  360.0f,   0.0f  });
            m_allParams.add ({ "PW",      0.0f,    1.0f,   0.5f  });
            m_inPorts  = {};
            m_outPorts = { "AUDIO" };
            break;

        case ModuleType::Sampler:
            m_allParams.add ({ "PITCH",  -24.0f,  24.0f,  0.0f  });
            m_allParams.add ({ "LEVEL",    0.0f,   1.0f,  1.0f  });
            m_allParams.add ({ "START",    0.0f,   1.0f,  0.0f  });
            m_allParams.add ({ "END",      0.0f,   1.0f,  1.0f  });
            m_allParams.add ({ "LOOP",     0.0f,   1.0f,  0.0f  });
            m_inPorts  = {};
            m_outPorts = { "AUDIO" };
            break;

        case ModuleType::Vst3:
            m_allParams.add ({ "MIX",    0.0f,  1.0f, 1.0f });
            m_allParams.add ({ "GAIN",   0.0f,  2.0f, 0.8f });
            m_allParams.add ({ "BYPASS", 0.0f,  1.0f, 0.0f });
            m_inPorts  = { "AUDIO" };
            m_outPorts = { "AUDIO" };
            break;

        case ModuleType::Sequencer:
            m_pattern.setStepCount (32);
            m_allParams.add ({ "GATE",  0.0f,  1.0f,  0.5f });
            m_allParams.add ({ "VEL",   0.0f,  1.0f,  1.0f });
            m_allParams.add ({ "OCT",  -2.0f,  2.0f,  0.0f });
            m_inPorts  = {};
            m_outPorts = { "MIDI" };
            break;

        case ModuleType::DrumMachine:
            // No compact params — face shows mini pattern preview
            m_inPorts  = {};
            m_outPorts = { "AUDIO", "MIDI" };
            break;

        case ModuleType::Output:
            m_allParams.add ({ "LEVEL",  0.0f,  1.0f,  1.0f });
            m_allParams.add ({ "PAN",   -1.0f,  1.0f,  0.0f });
            m_inPorts  = { "AUDIO" };
            m_outPorts = {};
            break;

        default:
            break;
    }
}

//==============================================================================
// Region helpers
//==============================================================================

juce::Rectangle<int> ModuleSlot::regionStripe() const noexcept
{ return { 0, 0, getWidth(), kStripeH }; }

juce::Rectangle<int> ModuleSlot::regionHeader() const noexcept
{ return { 0, kStripeH, getWidth(), kHeaderH }; }

juce::Rectangle<int> ModuleSlot::regionFace() const noexcept
{ return { 0, kStripeH + kHeaderH, getWidth(), kFaceH }; }

juce::Rectangle<int> ModuleSlot::regionPort() const noexcept
{ return { 0, kStripeH + kHeaderH + kFaceH, getWidth(), kPortH }; }

//==============================================================================
// Layout
//==============================================================================

void ModuleSlot::resized()
{
    m_inlineEditor.setBounds (0, kBaseH, getWidth(), kEditorH);
    refreshCompactControls();
}

//==============================================================================
// Paint
//==============================================================================

void ModuleSlot::paint (juce::Graphics& g)
{
    const float alpha = m_muted ? Theme::Alpha::disabled : 1.0f;

    // Slot background
    g.setColour (Theme::Colour::surface3.withAlpha (alpha));
    g.fillRoundedRectangle (getLocalBounds().toFloat(), Theme::Radius::xs);

    // Focused tint (used in group layout instead of m_selected expansion)
    if ((m_selected || m_focused) && !isEmpty())
    {
        const juce::Colour tintCol = m_groupColor.isTransparent() ? getSignalColour() : m_groupColor;
        g.setColour (tintCol.withAlpha (0.08f));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), Theme::Radius::xs);
    }

    // Outer border
    {
        const bool highlight = m_selected || m_focused;
        const juce::Colour borderCol = highlight
                                       ? (m_groupColor.isTransparent() ? getSignalColour() : m_groupColor)
                                       : Theme::Colour::surfaceEdge;
        const float width = highlight ? Theme::Stroke::accent : Theme::Stroke::subtle;
        g.setColour (borderCol.withAlpha (alpha));
        g.drawRoundedRectangle (getLocalBounds().toFloat(), Theme::Radius::xs, width);
    }

    paintStripe (g);
    paintHeader (g);

    if (isEmpty())
        paintFaceEmpty (g);
    else if (m_type == ModuleType::DrumMachine)
        paintFaceDrumMachine (g);
    else
        paintFaceLoaded (g);

    if (!isEmpty())
        paintPortRow (g);
}

void ModuleSlot::paintStripe (juce::Graphics& g) const
{
    const float alpha = m_muted ? Theme::Alpha::disabled : 1.0f;
    // Use group color for stripe if set; output slot always uses its signal color
    const bool useGroupColor = !m_groupColor.isTransparent()
                               && m_type != ModuleType::Output;
    const auto  stripeCol = m_muted ? Theme::Colour::inactive
                                    : (useGroupColor ? m_groupColor : getSignalColour());
    g.setColour (stripeCol.withAlpha (alpha));
    g.fillRect  (regionStripe());
}

void ModuleSlot::paintHeader (juce::Graphics& g) const
{
    const float alpha = m_muted ? Theme::Alpha::disabled : 1.0f;
    g.setColour (Theme::Colour::surface0.withAlpha (alpha));
    g.fillRect  (regionHeader());

    const auto headR   = regionHeader();
    const int  padLeft = static_cast<int> (Theme::Space::xs);

    // Slot number "01"
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost.withAlpha (alpha));
    g.drawText  (juce::String (m_index + 1).paddedLeft ('0', 2),
                 headR.withTrimmedLeft (padLeft).withWidth (16),
                 juce::Justification::centredLeft, false);

    // Module type badge (center)
    if (!isEmpty())
    {
        g.setFont   (Theme::Font::heading());
        g.setColour (Theme::Colour::inkLight.withAlpha (alpha));
        g.drawText  (getModuleTypeName(), headR, juce::Justification::centred, false);
    }

    // Right: slot name or "EMPTY" / "MUTE" badge
    juce::String rightText = isEmpty() ? "EMPTY" : "Spool";
    if (m_muted && !isEmpty())
    {
        // Draw MUTE badge
        const int bw = 28, bh = 10;
        const juce::Rectangle<int> br { headR.getRight() - bw - padLeft,
                                         headR.getCentreY() - bh / 2, bw, bh };
        g.setColour (Theme::Colour::inactive.withAlpha (alpha));
        g.fillRoundedRectangle (br.toFloat(), Theme::Radius::xs);
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMuted.withAlpha (alpha));
        g.drawText  ("MUTE", br, juce::Justification::centred, false);
    }
    else
    {
        g.setFont   (Theme::Font::micro());
        g.setColour ((isEmpty() ? Theme::Colour::inkGhost : Theme::Colour::inkMuted).withAlpha (alpha));
        g.drawText  (rightText,
                     headR.withTrimmedRight (padLeft),
                     juce::Justification::centredRight, false);
    }

    // Bottom divider
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (alpha));
    g.drawLine  (static_cast<float> (headR.getX()),
                 static_cast<float> (headR.getBottom()),
                 static_cast<float> (headR.getRight()),
                 static_cast<float> (headR.getBottom()),
                 Theme::Stroke::subtle);
}

void ModuleSlot::paintFaceEmpty (juce::Graphics& g) const
{
    g.setColour (Theme::Colour::surface0);
    g.fillRect  (regionFace());

    // Large "+" centered
    g.setFont   (Theme::Font::heading());
    g.setColour (Theme::Colour::inkMid);
    g.drawText  ("+", regionFace(), juce::Justification::centred, false);

    // "click to load" sub-label
    const auto subR = regionFace().withTrimmedTop (regionFace().getHeight() / 2);
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inactive);
    g.drawText  ("click to load", subR, juce::Justification::centredTop, false);
}

void ModuleSlot::paintFaceLoaded (juce::Graphics& g) const
{
    const float alpha = m_muted ? Theme::Alpha::disabled : 1.0f;
    const auto  faceR = regionFace();
    const auto  profile = layoutProfileForType (m_type);
    g.setColour (Theme::Colour::surface3.withAlpha (alpha));
    g.fillRect  (faceR);

    if (profile.topTextRows > 0 && juce::String (profile.topStatusText).isNotEmpty())
    {
        const auto textRowR = juce::Rectangle<int> (
            faceR.getX() + static_cast<int> (Theme::Space::xs),
            faceR.getY() + kPadV,
            faceR.getWidth() - static_cast<int> (Theme::Space::xs) * 2,
            kRowH);
        g.setFont   (profile.emphasizeTopStatus ? Theme::Font::label() : Theme::Font::micro());
        g.setColour ((profile.emphasizeTopStatus ? Theme::Colour::inkLight
                                                 : Theme::Colour::inkMuted).withAlpha (alpha));
        g.drawText  (profile.topStatusText, textRowR, juce::Justification::centredLeft, true);

        // Optional "OPEN EDITOR ↗" link region for plugin-backed modules
        if (profile.showOpenEditorLink)
        {
            g.setFont   (Theme::Font::micro());
            g.setColour (Theme::Colour::inkGhost.withAlpha (alpha));
            g.drawText  ("OPEN EDITOR \xE2\x86\x97",
                         faceR.withTrimmedRight (static_cast<int> (Theme::Space::xs)),
                         juce::Justification::bottomRight, false);
        }
    }
    else if (profile.showOpenEditorLink)
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (alpha));
        g.drawText  ("OPEN EDITOR \xE2\x86\x97",
                     faceR.withTrimmedRight (static_cast<int> (Theme::Space::xs)),
                     juce::Justification::bottomRight, false);
    }
}

void ModuleSlot::paintFaceDrumMachine (juce::Graphics& g) const
{
    const float alpha = m_muted ? Theme::Alpha::disabled : 1.0f;
    const auto  faceR = regionFace();

    g.setColour (Theme::Colour::surface3.withAlpha (alpha));
    g.fillRect  (faceR);

    if (m_drumData == nullptr || m_drumData->voices.empty())
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (alpha));
        g.drawText  ("DRUM MACHINE", faceR, juce::Justification::centred, false);
        return;
    }

    // Mini pattern grid: one 1px-tall color bar per voice
    const int numVoices = static_cast<int> (m_drumData->voices.size());
    const int gridX     = faceR.getX() + 2;
    const int gridW     = faceR.getWidth() - 4;
    const int barH      = juce::jmax (1, (faceR.getHeight() - 4) / numVoices);

    for (int vi = 0; vi < numVoices; ++vi)
    {
        const auto& voice    = m_drumData->voices[static_cast<size_t> (vi)];
        const int   nSteps   = voice.stepCount;
        const float stepW    = nSteps > 0 ? static_cast<float> (gridW) / nSteps : 0.0f;
        const int   barY     = faceR.getY() + 2 + vi * barH;

        for (int s = 0; s < nSteps; ++s)
        {
            const float sx = static_cast<float> (gridX) + s * stepW;
            if (voice.stepActive (s))
            {
                g.setColour (juce::Colour (voice.colorArgb).withAlpha (alpha));
                g.fillRect (juce::Rectangle<float> (sx + 0.5f, static_cast<float> (barY),
                                                    juce::jmax (1.0f, stepW - 1.0f),
                                                    static_cast<float> (barH)));
            }
            else
            {
                g.setColour (Theme::Colour::surface0.withAlpha (alpha));
                g.fillRect (juce::Rectangle<float> (sx + 0.5f, static_cast<float> (barY),
                                                    juce::jmax (1.0f, stepW - 1.0f),
                                                    static_cast<float> (barH)));
            }
        }
    }
}

void ModuleSlot::refreshCompactControls()
{
    const auto profile = layoutProfileForType (m_type);
    const bool canShow = !isEmpty() && profile.showCompactControls;
    const int firstParam = profile.topTextRows;
    const int rows = canShow ? juce::jmin (m_allParams.size() - firstParam, kCompactRows) : 0;
    const auto faceR = regionFace();
    const int pad = static_cast<int> (Theme::Space::xs);
    const auto tint = getSignalColour();
    const float alpha = m_muted ? Theme::Alpha::disabled : 1.0f;

    for (int i = 0; i < kCompactRows; ++i)
    {
        auto& slider = m_compactSliders[(size_t) i];
        auto& label  = m_compactLabels[(size_t) i];
        auto& value  = m_compactValues[(size_t) i];

        if (i >= rows)
        {
            m_compactParamIndex[(size_t) i] = -1;
            slider.setVisible (false);
            label.setVisible (false);
            value.setVisible (false);
            continue;
        }

        const int paramIdx = firstParam + i;
        m_compactParamIndex[(size_t) i] = paramIdx;
        const auto& p = m_allParams[paramIdx];

        const int rowY = faceR.getY() + kPadV + (firstParam > 0 ? kRowH : 0) + i * kRowH;
        const auto rowR = juce::Rectangle<int> (faceR.getX(), rowY, faceR.getWidth(), kRowH);

        label.setText (p.label, juce::dontSendNotification);
        label.setFont (Theme::Font::micro());
        label.setColour (juce::Label::textColourId, Theme::Colour::inkMid.withAlpha (alpha));
        label.setBounds (rowR.getX() + pad, rowR.getY(), kLabelW, rowR.getHeight());
        label.setVisible (true);

        slider.getProperties().set ("tint", tint.toString());
        slider.setRange (p.min, p.max, (p.max - p.min) / 1000.0f);
        slider.setValue (p.value, juce::dontSendNotification);
        slider.setAlpha (alpha);
        slider.setBounds (rowR.getX() + kLabelW + pad, rowR.getY() + 1,
                          rowR.getWidth() - kLabelW - kValueW - pad * 2, rowR.getHeight() - 2);
        slider.setVisible (true);

        value.setText (juce::String (p.value, 2), juce::dontSendNotification);
        value.setFont (Theme::Font::value());
        value.setColour (juce::Label::textColourId, Theme::Colour::inkLight.withAlpha (alpha));
        value.setBounds (rowR.getRight() - kValueW - pad, rowR.getY(), kValueW, rowR.getHeight());
        value.setVisible (true);
    }
}

void ModuleSlot::paintPortRow (juce::Graphics& g) const
{
    const float alpha = m_muted ? Theme::Alpha::disabled : 1.0f;
    const auto  portR = regionPort();
    g.setColour (Theme::Colour::surface0.withAlpha (alpha));
    g.fillRect  (portR);

    const int dotD  = static_cast<int> (Theme::Space::portDotSize);
    const int dotCY = portR.getCentreY();
    const int pad   = static_cast<int> (Theme::Space::sm);
    const int gapX  = dotD + 2;

    // Input dots — left side
    int lx = portR.getX() + pad;
    for (const auto& port : m_inPorts)
    {
        const auto col = Theme::Signal::forType (port.toLowerCase());
        g.setColour (col.withAlpha (alpha));
        g.fillEllipse (static_cast<float> (lx),
                       static_cast<float> (dotCY - dotD / 2),
                       dotD, dotD);
        g.setFont   (Theme::Font::micro());
        g.setColour (col.withAlpha (alpha * 0.8f));
        g.drawText  (port,
                     juce::Rectangle<int> (lx + dotD + 2, dotCY - 6, 28, 12),
                     juce::Justification::centredLeft, false);
        lx += gapX + 32;
    }

    // Output dots — right side
    int rx = portR.getRight() - pad;
    for (int i = m_outPorts.size() - 1; i >= 0; --i)
    {
        const auto col = Theme::Signal::forType (m_outPorts[i].toLowerCase());
        rx -= dotD;
        g.setColour (col.withAlpha (alpha));
        g.fillEllipse (static_cast<float> (rx),
                       static_cast<float> (dotCY - dotD / 2),
                       dotD, dotD);
        g.setFont   (Theme::Font::micro());
        g.setColour (col.withAlpha (alpha * 0.8f));
        g.drawText  (m_outPorts[i],
                     juce::Rectangle<int> (rx - 28, dotCY - 6, 28, 12),
                     juce::Justification::centredRight, false);
        rx -= (gapX + 28);
    }

    // OUTPUT slot: ROUTING → link
    if (m_type == ModuleType::Output)
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (alpha));
        g.drawText  ("ROUTING \xe2\x86\x92",
                     portR.withTrimmedRight (pad),
                     juce::Justification::centredRight, false);
    }
}

//==============================================================================
// Mouse
//==============================================================================

void ModuleSlot::mouseDown (const juce::MouseEvent& e)
{
    if (isEmpty())
        return;  // handled in mouseUp

    if (e.mods.isRightButtonDown())
        return;  // handled in mouseUp
}

void ModuleSlot::mouseDrag (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
}

void ModuleSlot::mouseUp (const juce::MouseEvent& e)
{
    const auto profile = layoutProfileForType (m_type);

    // Right-click on header of loaded slot → context menu
    if (e.mods.isRightButtonDown() && !isEmpty() && regionHeader().contains (e.getPosition()))
    {
        if (onRightClickHeader) onRightClickHeader (m_index);
        return;
    }

    if (e.mods.isRightButtonDown()) return;

    // Left-click on empty slot
    if (isEmpty())
    {
        if (onEmptyClicked) onEmptyClicked (m_index);
        return;
    }

    // Left-click on loaded slot
    if (onSlotClicked) onSlotClicked (m_index);

    // OUTPUT routing link in port row
    if (m_type == ModuleType::Output && regionPort().contains (e.getPosition()))
    {
        const int rx = regionPort().getRight() - static_cast<int> (Theme::Space::sm);
        if (e.x > rx - 60)
        {
            DBG ("routing stub clicked");
        }
    }

    // Module-specific "OPEN EDITOR" hotspot in face (bottom-right)
    if (profile.showOpenEditorLink && regionFace().contains (e.getPosition()))
    {
        if (e.y > regionFace().getBottom() - profile.openEditorHotZoneHeight)
        {
            DBG ("open vst editor slot " + juce::String (m_index));
        }
    }
}

juce::MouseCursor ModuleSlot::getMouseCursor()
{
    return juce::MouseCursor (juce::MouseCursor::NormalCursor);
}

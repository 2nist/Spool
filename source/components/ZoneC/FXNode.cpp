#include "FXNode.h"

namespace
{
juce::String formatParamValue (const EffectParamDef& p, float value)
{
    if (p.type == EffectParamDef::Type::Bool)
        return value >= 0.5f ? "ON" : "OFF";
    if (p.type == EffectParamDef::Type::Enum)
    {
        const int idx = juce::jlimit (0, p.enumValues.size() - 1, (int) (value + 0.5f));
        return idx < p.enumValues.size() ? p.enumValues[idx] : juce::String (idx);
    }

    if (p.maxVal >= 1000.0f)
        return juce::String ((int) juce::roundToInt (value));

    return juce::String (value, 1);
}
}

//==============================================================================

FXNode::FXNode (int nodeIndex, const EffectNode& data)
    : m_nodeIndex (nodeIndex), m_data (data)
{
    setSize (184, kHeight);
    setRepaintsOnMouseActivity (true);

    for (int i = 0; i < kMaxVisibleRows; ++i)
    {
        auto& label = m_rowLabels[(size_t) i];
        label.setJustificationType (juce::Justification::centredLeft);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);

        auto& valueLabel = m_rowValueLabels[(size_t) i];
        valueLabel.setJustificationType (juce::Justification::centredRight);
        valueLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (valueLabel);

        auto& slider = m_rowSliders[(size_t) i];
        slider.setSliderStyle (juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setScrollWheelEnabled (false);
        slider.setPopupDisplayEnabled (true, true, this);
        slider.onValueChange = [this, i]
        {
            auto* d = def();
            if (d == nullptr || i >= d->params.size())
                return;

            const auto& p = d->params.getReference (i);
            while (m_data.params.size() <= i)
                m_data.params.add (p.defaultVal);

            const float newValue = (float) m_rowSliders[(size_t) i].getValue();
            m_data.params.set (i, newValue);
            m_rowValueLabels[(size_t) i].setText (formatParamValue (p, newValue), juce::dontSendNotification);

            if (onParamChanged)
                onParamChanged (m_nodeIndex, i, newValue);
        };
        addAndMakeVisible (slider);

        auto& button = m_rowButtons[(size_t) i];
        button.setClickingTogglesState (true);
        button.onClick = [this, i]
        {
            auto* d = def();
            if (d == nullptr || i >= d->params.size())
                return;

            const float newValue = m_rowButtons[(size_t) i].getToggleState() ? 1.0f : 0.0f;
            while (m_data.params.size() <= i)
                m_data.params.add (d->params[i].defaultVal);
            m_data.params.set (i, newValue);
            m_rowButtons[(size_t) i].setButtonText (newValue >= 0.5f ? "ON" : "OFF");
            if (onParamChanged)
                onParamChanged (m_nodeIndex, i, newValue);
        };
        addAndMakeVisible (button);

        auto& combo = m_rowCombos[(size_t) i];
        combo.onChange = [this, i]
        {
            auto* d = def();
            if (d == nullptr || i >= d->params.size())
                return;
            const float newValue = (float) juce::jmax (0, m_rowCombos[(size_t) i].getSelectedItemIndex());
            while (m_data.params.size() <= i)
                m_data.params.add (d->params[i].defaultVal);
            m_data.params.set (i, newValue);
            if (onParamChanged)
                onParamChanged (m_nodeIndex, i, newValue);
        };
        addAndMakeVisible (combo);
    }

    refreshRowControls();
}

void FXNode::setData (int nodeIndex, const EffectNode& data)
{
    m_nodeIndex = nodeIndex;
    m_data      = data;
    m_showConfirm = false;
    refreshRowControls();
    repaint();
}

void FXNode::setDragging (bool isDragging)
{
    m_isDragging = isDragging;
    repaint();
}

void FXNode::setFocused (bool isFocused)
{
    m_isFocused = isFocused;
    repaint();
}

void FXNode::showRemoveConfirmation (bool show)
{
    m_showConfirm = show;
    refreshRowControls();
    repaint();
}

//==============================================================================
// Regions
//==============================================================================

juce::Rectangle<int> FXNode::headerRect() const noexcept
{
    return getLocalBounds().removeFromTop (kHeaderH);
}

juce::Rectangle<int> FXNode::paramAreaRect() const noexcept
{
    return getLocalBounds().withTrimmedTop (kHeaderH);
}

juce::Rectangle<int> FXNode::bypassRect() const noexcept
{
    const int x = getWidth() - 6 - 8 - 4 - 14;
    return { x, (kHeaderH - 14) / 2, 14, 14 };
}

juce::Rectangle<int> FXNode::removeRect() const noexcept
{
    const int x = getWidth() - 6 - 14;
    return { x, (kHeaderH - 14) / 2, 14, 14 };
}

juce::Rectangle<int> FXNode::dragHandleRect() const noexcept
{
    const int x = getWidth() - 6 - 8 - 4 - 14 - 4 - 8;
    return { x, 0, 8, kHeaderH };
}

juce::Rectangle<int> FXNode::rowRect (int rowIdx) const noexcept
{
    const int y = kHeaderH + kPadTop + rowIdx * kRowH;
    return { 0, y, getWidth(), kRowH };
}

//==============================================================================
// Paint
//==============================================================================

void FXNode::paint (juce::Graphics& g)
{
    if (m_isDragging)
    {
        const auto shadowR = getLocalBounds().translated (3, 3);
        g.setColour (Theme::Colour::surface0.withAlpha (0.6f));
        g.fillRoundedRectangle (shadowR.toFloat(), Theme::Radius::sm);

        g.setColour (Theme::Zone::c);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f),
                                Theme::Radius::sm, Theme::Stroke::accent);
    }
    else
    {
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f),
                                Theme::Radius::sm, Theme::Stroke::subtle);
    }

    if (m_isFocused)
    {
        g.setColour (Theme::Zone::c.withAlpha (0.9f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.5f),
                                Theme::Radius::sm, Theme::Stroke::accent);
    }

    paintHeader (g);

    if (m_showConfirm)
        paintConfirm (g);
    else
        paintParams (g);
}

void FXNode::paintHeader (juce::Graphics& g) const
{
    auto* d = def();
    const juce::Colour effectCol = d ? d->colour : Theme::Colour::accent;
    const auto hdrR = headerRect();

    g.setColour (Theme::Colour::surface0);
    {
        juce::Path p;
        const auto paddedR = hdrR.toFloat().withBottom (hdrR.toFloat().getBottom() + Theme::Radius::sm);
        p.addRoundedRectangle (paddedR, Theme::Radius::sm);
        g.fillPath (p);
    }

    const int dotD = 6;
    const int dotY = (kHeaderH - dotD) / 2;
    g.setColour (effectCol);
    g.fillEllipse (8.0f, (float) dotY, (float) dotD, (float) dotD);

    g.setFont (Theme::Font::label());
    g.setColour (Theme::Colour::inkLight);
    g.drawText (d ? d->displayName : m_data.effectType.toUpperCase(),
                8 + dotD + 4, 0, bypassRect().getX() - (8 + dotD + 4) - 4, kHeaderH,
                juce::Justification::centredLeft, false);

    if (d != nullptr)
    {
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        const int tagX = 8 + dotD + 4 + 32;
        g.drawText (d->tag, tagX, 0, dragHandleRect().getX() - tagX, kHeaderH,
                    juce::Justification::centredLeft, false);
    }

    {
        const auto dh = dragHandleRect();
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        for (int i = 0; i < 3; ++i)
        {
            const float dy = (float) (dh.getY() + (kHeaderH / 2) - 3 + i * 3);
            g.fillEllipse ((float) (dh.getX() + 3), dy, 2.0f, 2.0f);
        }
    }

    {
        const auto br = bypassRect();
        if (m_data.bypassed)
        {
            g.setColour (Theme::Colour::accentWarm.withAlpha (0.2f));
            g.fillRoundedRectangle (br.toFloat(), Theme::Radius::xs);
            g.setColour (Theme::Colour::accentWarm);
            g.drawRoundedRectangle (br.toFloat(), Theme::Radius::xs, Theme::Stroke::subtle);
            g.setFont (Theme::Font::micro());
            g.drawText ("B", br, juce::Justification::centred, false);
        }
        else
        {
            g.setColour (Theme::Colour::surface3);
            g.fillRoundedRectangle (br.toFloat(), Theme::Radius::xs);
            g.setColour (Theme::Colour::surfaceEdge);
            g.drawRoundedRectangle (br.toFloat(), Theme::Radius::xs, Theme::Stroke::subtle);
            g.setFont (Theme::Font::micro());
            g.setColour (Theme::Colour::inkGhost);
            g.drawText ("B", br, juce::Justification::centred, false);
        }
    }

    {
        const auto rr = removeRect();
        g.setFont (Theme::Font::micro());
        g.setColour (m_hoverRemove ? Theme::Colour::inkMuted : Theme::Colour::inkGhost);
        g.drawText ("\xC3\x97", rr, juce::Justification::centred, false);
    }
}

void FXNode::paintParams (juce::Graphics& g) const
{
    auto* d = def();
    if (d == nullptr)
        return;

    g.setColour (Theme::Colour::surface3);
    g.fillRoundedRectangle (paramAreaRect().toFloat(), Theme::Radius::sm);

    const int count = juce::jmin (kMaxVisibleRows, d->params.size());
    for (int i = 1; i < count; ++i)
    {
        const auto rowR = rowRect (i);
        g.setColour (Theme::Colour::surface0);
        g.drawLine ((float) rowR.getX(), (float) rowR.getY(),
                    (float) rowR.getRight(), (float) rowR.getY(), 1.0f);
    }
}

void FXNode::paintConfirm (juce::Graphics& g) const
{
    const auto pa = paramAreaRect();

    g.setColour (Theme::Colour::surface2);
    g.fillRoundedRectangle (pa.toFloat(), Theme::Radius::sm);

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkMuted);
    g.drawText ("remove last effect?", pa.getX(), pa.getY() + 6, pa.getWidth(), 12,
                juce::Justification::centred, false);

    const int btnW = 40;
    const int btnH = 12;
    const int gap  = 8;
    const int btnY = pa.getY() + 24;
    const int totalW = btnW * 2 + gap;
    const int btnX0 = pa.getX() + (pa.getWidth() - totalW) / 2;

    g.setColour (Theme::Colour::accentWarm);
    g.fillRoundedRectangle ((float) btnX0, (float) btnY, (float) btnW, (float) btnH, Theme::Radius::xs);
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkDark);
    g.drawText ("YES", btnX0, btnY, btnW, btnH, juce::Justification::centred, false);

    const int btnX1 = btnX0 + btnW + gap;
    g.setColour (Theme::Colour::surface3);
    g.fillRoundedRectangle ((float) btnX1, (float) btnY, (float) btnW, (float) btnH, Theme::Radius::xs);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRoundedRectangle ((float) btnX1, (float) btnY, (float) btnW, (float) btnH, Theme::Radius::xs, Theme::Stroke::subtle);
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("NO", btnX1, btnY, btnW, btnH, juce::Justification::centred, false);
}

void FXNode::refreshRowControls()
{
    auto* d = def();
    const bool controlsEnabled = (d != nullptr && !m_showConfirm);
    const int count = d == nullptr ? 0 : juce::jmin (kMaxVisibleRows, d->params.size());
    const juce::Colour effectCol = d != nullptr ? d->colour : Theme::Colour::accent;

    for (int i = 0; i < kMaxVisibleRows; ++i)
    {
        auto& label = m_rowLabels[(size_t) i];
        auto& valueLabel = m_rowValueLabels[(size_t) i];
        auto& slider = m_rowSliders[(size_t) i];
        auto& button = m_rowButtons[(size_t) i];
        auto& combo = m_rowCombos[(size_t) i];

        const bool rowVisible = controlsEnabled && (i < count);
        label.setVisible (rowVisible);
        valueLabel.setVisible (false);
        slider.setVisible (false);
        button.setVisible (false);
        combo.setVisible (false);

        if (!rowVisible)
            continue;

        const auto& p = d->params.getReference (i);
        const float value = i < m_data.params.size() ? m_data.params[i] : p.defaultVal;
        const auto rowR = rowRect (i);

        label.setText (p.name, juce::dontSendNotification);
        label.setFont (Theme::Font::label());
        label.setColour (juce::Label::textColourId, Theme::Colour::inkMuted);
        label.setBounds (rowR.getX() + 2, rowR.getY(), kLabelW - 4, kRowH);

        const auto controlR = juce::Rectangle<int> (rowR.getX() + kLabelW + 2, rowR.getY() + 1,
                                                    rowR.getWidth() - kLabelW - kValueW - 6, kRowH - 2);
        const auto valueR = juce::Rectangle<int> (rowR.getRight() - kValueW - 2, rowR.getY(), kValueW, kRowH);

        if (p.type == EffectParamDef::Type::Continuous)
        {
            slider.getProperties().set ("tint", effectCol.toString());
            slider.setRange (p.minVal, p.maxVal, (p.maxVal - p.minVal) / 1000.0f);
            slider.setValue (value, juce::dontSendNotification);
            slider.setBounds (controlR);
            slider.setVisible (true);

            valueLabel.setFont (Theme::Font::label());
            valueLabel.setColour (juce::Label::textColourId, Theme::Colour::inkLight);
            valueLabel.setText (formatParamValue (p, value), juce::dontSendNotification);
            valueLabel.setBounds (valueR);
            valueLabel.setVisible (true);
        }
        else if (p.type == EffectParamDef::Type::Bool)
        {
            button.setButtonText (value >= 0.5f ? "ON" : "OFF");
            button.setToggleState (value >= 0.5f, juce::dontSendNotification);
            button.setBounds (controlR);
            button.setVisible (true);
        }
        else
        {
            combo.clear (juce::dontSendNotification);
            for (int item = 0; item < p.enumValues.size(); ++item)
                combo.addItem (p.enumValues[item], item + 1);

            const int idx = juce::jlimit (0, p.enumValues.size() - 1, (int) (value + 0.5f));
            combo.setSelectedItemIndex (idx, juce::dontSendNotification);
            combo.setBounds (controlR);
            combo.setVisible (true);
        }
    }
}

//==============================================================================
// resized
//==============================================================================

void FXNode::resized()
{
    refreshRowControls();
}

//==============================================================================
// Mouse
//==============================================================================

void FXNode::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    if (onFocused)
        onFocused (m_nodeIndex);

    if (dragHandleRect().contains (pos))
    {
        if (onDragStarted)
            onDragStarted (m_nodeIndex, e);
        return;
    }

    if (bypassRect().contains (pos))
    {
        m_data.bypassed = !m_data.bypassed;
        if (onBypassToggled)
            onBypassToggled (m_nodeIndex);
        refreshRowControls();
        repaint();
        return;
    }

    if (removeRect().contains (pos))
    {
        if (onRemoveClicked)
            onRemoveClicked (m_nodeIndex);
        return;
    }

    if (m_showConfirm)
    {
        const auto pa = paramAreaRect();
        const int btnW = 40, btnH = 12, gap = 8, btnY = pa.getY() + 24;
        const int totalW = btnW * 2 + gap;
        const int btnX0 = pa.getX() + (pa.getWidth() - totalW) / 2;

        if (juce::Rectangle<int> (btnX0, btnY, btnW, btnH).contains (pos))
        {
            if (onRemoveClicked)
                onRemoveClicked (m_nodeIndex);
            return;
        }
        if (juce::Rectangle<int> (btnX0 + btnW + gap, btnY, btnW, btnH).contains (pos))
        {
            showRemoveConfirmation (false);
            return;
        }
    }
}

void FXNode::mouseDrag (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
}

void FXNode::mouseUp (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
}

void FXNode::mouseMove (const juce::MouseEvent& e)
{
    const bool over = removeRect().contains (e.getPosition());
    if (over != m_hoverRemove)
    {
        m_hoverRemove = over;
        repaint();
    }
}

void FXNode::mouseExit (const juce::MouseEvent&)
{
    if (m_hoverRemove)
    {
        m_hoverRemove = false;
        repaint();
    }
}

juce::MouseCursor FXNode::getMouseCursor()
{
    if (dragHandleRect().contains (getMouseXYRelative()))
        return juce::MouseCursor::UpDownResizeCursor;
    return juce::MouseCursor::NormalCursor;
}

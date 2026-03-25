#include "FXNode.h"

//==============================================================================

FXNode::FXNode (int nodeIndex, const EffectNode& data)
    : m_nodeIndex (nodeIndex), m_data (data)
{
    setSize (184, kHeight);
    setRepaintsOnMouseActivity (true);
}

void FXNode::setData (int nodeIndex, const EffectNode& data)
{
    m_nodeIndex = nodeIndex;
    m_data      = data;
    m_showConfirm = false;
    repaint();
}

void FXNode::setDragging (bool isDragging)
{
    m_isDragging = isDragging;
    repaint();
}

void FXNode::showRemoveConfirmation (bool show)
{
    m_showConfirm = show;
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
    const int x = getWidth() - 6 - 8 - 4 - 14;  // from right: inset + handle + gap + bypass
    return { x, (kHeaderH - 14) / 2, 14, 14 };
}

juce::Rectangle<int> FXNode::removeRect() const noexcept
{
    const int x = getWidth() - 6 - 14;  // rightmost button
    return { x, (kHeaderH - 14) / 2, 14, 14 };
}

juce::Rectangle<int> FXNode::dragHandleRect() const noexcept
{
    // Between bypass and remove: 8px wide strip
    const int x = getWidth() - 6 - 8 - 4 - 14 - 4 - 8;
    return { x, 0, 8, kHeaderH };
}

juce::Rectangle<int> FXNode::rowRect (int rowIdx) const noexcept
{
    const int y = kHeaderH + kPadTop + rowIdx * kRowH;
    return { 0, y, getWidth(), kRowH };
}

juce::Rectangle<int> FXNode::trackRect (juce::Rectangle<int> rowR) const noexcept
{
    // rowR: full row bounds
    // Track occupies space between label and value display
    const int x     = kLabelW;
    const int w     = rowR.getWidth() - kLabelW - kValueW;
    const int trackH = 2;
    const int trackY = rowR.getY() + (kRowH - trackH) / 2;
    return { x, trackY, w, trackH };
}

//==============================================================================
// Paint
//==============================================================================

void FXNode::paint (juce::Graphics& g)
{
    auto* d = def();
    const juce::Colour effectCol = d ? d->colour : Theme::Colour::accent;

    // Lifted drag state
    if (m_isDragging)
    {
        // Shadow
        const auto shadowR = getLocalBounds().translated (3, 3);
        g.setColour (Theme::Colour::surface0.withAlpha (0.6f));
        g.fillRoundedRectangle (shadowR.toFloat(), Theme::Radius::sm);

        // Border in Zone::c
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

    paintHeader (g);

    if (m_showConfirm)
        paintConfirm (g);
    else
        paintParams (g);

    juce::ignoreUnused (effectCol);
}

void FXNode::paintHeader (juce::Graphics& g) const
{
    auto* d = def();
    const juce::Colour effectCol = d ? d->colour : Theme::Colour::accent;
    const auto hdrR = headerRect();

    // Header background (top corners rounded only)
    g.setColour (Theme::Colour::surface0);
    {
        // Extend rect down by radius so bottom corners are hidden behind content
        juce::Path p;
        const auto paddedR = hdrR.toFloat().withBottom (hdrR.toFloat().getBottom() + Theme::Radius::sm);
        p.addRoundedRectangle (paddedR, Theme::Radius::sm);
        g.fillPath (p);
    }

    // Colour dot
    const int dotD = 6;
    const int dotY = (kHeaderH - dotD) / 2;
    g.setColour (effectCol);
    g.fillEllipse (8.0f, static_cast<float> (dotY), static_cast<float> (dotD), static_cast<float> (dotD));

    // Effect name
    g.setFont (Theme::Font::label());
    g.setColour (Theme::Colour::inkLight);
    g.drawText (d ? d->displayName : m_data.effectType.toUpperCase(),
                8 + dotD + 4, 0, bypassRect().getX() - (8 + dotD + 4) - 4, kHeaderH,
                juce::Justification::centredLeft, false);

    // Subtype tag — centered in remaining space
    if (d)
    {
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        const int tagX = 8 + dotD + 4 + 32;  // after name approx
        g.drawText (d->tag, tagX, 0, dragHandleRect().getX() - tagX, kHeaderH,
                    juce::Justification::centredLeft, false);
    }

    // Drag handle — three dots
    {
        const auto dh = dragHandleRect();
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        const int dotSpacing = 3;
        for (int i = 0; i < 3; ++i)
        {
            const float dy = static_cast<float> (dh.getY() + (kHeaderH / 2) - dotSpacing + i * dotSpacing);
            g.fillEllipse (static_cast<float> (dh.getX() + 3), dy, 2.0f, 2.0f);
        }
    }

    // Bypass button
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

    // Remove button
    {
        const auto rr = removeRect();
        g.setFont (Theme::Font::micro());
        g.setColour (m_hoverRemove ? Theme::Colour::inkMuted : Theme::Colour::inkGhost);
        g.drawText ("\xc3\x97", rr, juce::Justification::centred, false);
    }
}

void FXNode::paintParams (juce::Graphics& g) const
{
    auto* d = def();
    if (!d) return;

    // Param area background
    g.setColour (Theme::Colour::surface3);
    g.fillRoundedRectangle (paramAreaRect().toFloat(), Theme::Radius::sm);

    const int count = juce::jmin (3, d->params.size());
    for (int i = 0; i < count; ++i)
        paintRow (g, i);
}

void FXNode::paintRow (juce::Graphics& g, int rowIdx) const
{
    auto* d = def();
    if (!d || rowIdx >= d->params.size()) return;

    const auto& p   = d->params.getReference (rowIdx);
    const float val = rowIdx < m_data.params.size() ? m_data.params[rowIdx] : p.defaultVal;
    const auto rowR = rowRect (rowIdx);

    // 1px separator above rows (except first)
    if (rowIdx > 0)
    {
        g.setColour (Theme::Colour::surface0);
        g.drawLine (static_cast<float> (rowR.getX()),
                    static_cast<float> (rowR.getY()),
                    static_cast<float> (rowR.getRight()),
                    static_cast<float> (rowR.getY()), 1.0f);
    }

    // Label
    g.setFont (Theme::Font::label());
    g.setColour (Theme::Colour::inkMuted);
    g.drawText (p.name, rowR.getX() + 2, rowR.getY(), kLabelW - 2, kRowH,
                juce::Justification::centredLeft, false);

    const juce::Colour effectCol = d->colour;

    if (p.type == EffectParamDef::Type::Continuous)
    {
        // Slider track
        auto tr = trackRect (rowR);

        // Track bg
        g.setColour (Theme::Colour::surface0);
        g.fillRect (tr);

        // Track fill
        const float norm = (p.maxVal > p.minVal)
                           ? juce::jlimit (0.0f, 1.0f, (val - p.minVal) / (p.maxVal - p.minVal))
                           : 0.0f;
        g.setColour (effectCol);
        g.fillRect (tr.withWidth (static_cast<int> (tr.getWidth() * norm)));

        // Thumb
        const int thumbD = 8;
        const float thumbX = static_cast<float> (tr.getX()) + norm * static_cast<float> (tr.getWidth()) - thumbD * 0.5f;
        const float thumbY = static_cast<float> (rowR.getY()) + (kRowH - thumbD) * 0.5f;
        g.setColour (effectCol);
        g.fillEllipse (thumbX, thumbY, static_cast<float> (thumbD), static_cast<float> (thumbD));

        // Value text
        g.setFont (Theme::Font::label());
        g.setColour (Theme::Colour::inkLight);
        const juce::String valStr = juce::String (val, 1);
        g.drawText (valStr,
                    rowR.getRight() - kValueW - 2, rowR.getY(), kValueW, kRowH,
                    juce::Justification::centredRight, false);
    }
    else if (p.type == EffectParamDef::Type::Bool)
    {
        const bool isOn = val >= 0.5f;
        const int centerX = kLabelW + (getWidth() - kLabelW - kArrowW * 2 - kValueW) / 2;

        // ‹ arrow
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMuted);
        g.drawText ("\xe2\x80\xb9",
                    rowR.getX() + kLabelW, rowR.getY(), kArrowW, kRowH,
                    juce::Justification::centred, false);

        // Value text
        g.setColour (isOn ? effectCol : Theme::Colour::inkGhost);
        g.drawText (isOn ? "ON" : "OFF",
                    rowR.getX() + kLabelW + kArrowW, rowR.getY(),
                    getWidth() - kLabelW - kArrowW * 2, kRowH,
                    juce::Justification::centred, false);

        // › arrow
        g.setColour (Theme::Colour::inkMuted);
        g.drawText ("\xe2\x80\xba",
                    rowR.getRight() - kArrowW, rowR.getY(), kArrowW, kRowH,
                    juce::Justification::centred, false);

        juce::ignoreUnused (centerX);
    }
    else if (p.type == EffectParamDef::Type::Enum)
    {
        const int idx = juce::jlimit (0, p.enumValues.size() - 1,
                                      static_cast<int> (val + 0.5f));
        const juce::String enumVal = idx < p.enumValues.size()
                                     ? p.enumValues[idx] : juce::String (idx);

        // ‹ arrow
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMuted);
        g.drawText ("\xe2\x80\xb9",
                    rowR.getX() + kLabelW, rowR.getY(), kArrowW, kRowH,
                    juce::Justification::centred, false);

        // Value text
        g.setColour (effectCol);
        g.drawText (enumVal,
                    rowR.getX() + kLabelW + kArrowW, rowR.getY(),
                    getWidth() - kLabelW - kArrowW * 2, kRowH,
                    juce::Justification::centred, false);

        // › arrow
        g.setColour (Theme::Colour::inkMuted);
        g.drawText ("\xe2\x80\xba",
                    rowR.getRight() - kArrowW, rowR.getY(), kArrowW, kRowH,
                    juce::Justification::centred, false);
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

    // YES button
    g.setColour (Theme::Colour::accentWarm);
    g.fillRoundedRectangle (static_cast<float> (btnX0), static_cast<float> (btnY),
                            static_cast<float> (btnW), static_cast<float> (btnH),
                            Theme::Radius::xs);
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkDark);
    g.drawText ("YES", btnX0, btnY, btnW, btnH, juce::Justification::centred, false);

    // NO button
    const int btnX1 = btnX0 + btnW + gap;
    g.setColour (Theme::Colour::surface3);
    g.fillRoundedRectangle (static_cast<float> (btnX1), static_cast<float> (btnY),
                            static_cast<float> (btnW), static_cast<float> (btnH),
                            Theme::Radius::xs);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRoundedRectangle (static_cast<float> (btnX1), static_cast<float> (btnY),
                            static_cast<float> (btnW), static_cast<float> (btnH),
                            Theme::Radius::xs, Theme::Stroke::subtle);
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("NO", btnX1, btnY, btnW, btnH, juce::Justification::centred, false);
}

//==============================================================================
// resized — nothing to layout, children placed via regions in paint
//==============================================================================

void FXNode::resized() {}

//==============================================================================
// Mouse
//==============================================================================

int FXNode::hitTestParam (juce::Point<int> pos) const noexcept
{
    auto* d = def();
    if (!d) return -1;
    const int count = juce::jmin (3, d->params.size());
    for (int i = 0; i < count; ++i)
    {
        if (rowRect (i).contains (pos) &&
            d->params[i].type == EffectParamDef::Type::Continuous)
        {
            auto tr = trackRect (rowRect (i));
            // Expand hit area to full row height
            const auto hitR = juce::Rectangle<int> (tr.getX(), rowRect (i).getY(),
                                                    tr.getWidth(), kRowH);
            if (hitR.contains (pos))
                return i;
        }
    }
    return -1;
}

int FXNode::hitTestArrow (juce::Point<int> pos, int rowIdx) const noexcept
{
    const auto rowR = rowRect (rowIdx);
    // left arrow
    if (juce::Rectangle<int> (rowR.getX() + kLabelW, rowR.getY(), kArrowW, kRowH).contains (pos))
        return -1;
    // right arrow
    if (juce::Rectangle<int> (rowR.getRight() - kArrowW, rowR.getY(), kArrowW, kRowH).contains (pos))
        return +1;
    return 0;
}

void FXNode::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // Drag handle
    if (dragHandleRect().contains (pos))
    {
        if (onDragStarted)
            onDragStarted (m_nodeIndex, e);
        return;
    }

    // Bypass
    if (bypassRect().contains (pos))
    {
        m_data.bypassed = !m_data.bypassed;
        if (onBypassToggled)
            onBypassToggled (m_nodeIndex);
        repaint();
        return;
    }

    // Remove
    if (removeRect().contains (pos))
    {
        if (onRemoveClicked)
            onRemoveClicked (m_nodeIndex);
        return;
    }

    // Confirm YES / NO
    if (m_showConfirm)
    {
        const auto pa = paramAreaRect();
        const int btnW = 40, btnH = 12, gap = 8, btnY = pa.getY() + 24;
        const int totalW = btnW * 2 + gap;
        const int btnX0 = pa.getX() + (pa.getWidth() - totalW) / 2;

        if (juce::Rectangle<int> (btnX0, btnY, btnW, btnH).contains (pos))
        {
            // YES — fire remove (parent handles it)
            if (onRemoveClicked)
                onRemoveClicked (m_nodeIndex);
            return;
        }
        if (juce::Rectangle<int> (btnX0 + btnW + gap, btnY, btnW, btnH).contains (pos))
        {
            // NO — dismiss confirm
            showRemoveConfirmation (false);
            return;
        }
        return;
    }

    // Continuous param slider
    auto* d = def();
    if (!d) return;
    const int count = juce::jmin (3, d->params.size());
    for (int i = 0; i < count; ++i)
    {
        const auto& p = d->params.getReference (i);
        if (p.type == EffectParamDef::Type::Continuous)
        {
            const auto tr = trackRect (rowRect (i));
            const auto hitR = juce::Rectangle<int> (tr.getX(), rowRect (i).getY(),
                                                    tr.getWidth(), kRowH);
            if (hitR.contains (pos))
            {
                m_draggingParam = i;
                m_dragStartVal  = i < m_data.params.size() ? m_data.params[i] : p.defaultVal;
                m_dragStartX    = e.x;
                setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
                return;
            }
        }
        else // Bool or Enum — handled on mouseUp via arrow hit-test
        {
        }
    }
}

void FXNode::mouseDrag (const juce::MouseEvent& e)
{
    if (m_draggingParam < 0) return;
    auto* d = def();
    if (!d || m_draggingParam >= d->params.size()) return;

    const auto& p  = d->params.getReference (m_draggingParam);
    const auto  tr = trackRect (rowRect (m_draggingParam));
    const float delta = static_cast<float> (e.x - m_dragStartX) / static_cast<float> (tr.getWidth());
    const float newVal = juce::jlimit (p.minVal, p.maxVal,
                                       m_dragStartVal + delta * (p.maxVal - p.minVal));

    while (m_data.params.size() <= m_draggingParam)
        m_data.params.add (p.defaultVal);
    m_data.params.set (m_draggingParam, newVal);

    if (onParamChanged)
        onParamChanged (m_nodeIndex, m_draggingParam, newVal);

    repaint();
}

void FXNode::mouseUp (const juce::MouseEvent& e)
{
    if (m_draggingParam >= 0)
    {
        m_draggingParam = -1;
        setMouseCursor (juce::MouseCursor::NormalCursor);
        return;
    }

    // Bool / Enum arrow clicks
    auto* d = def();
    if (!d) return;
    const int count = juce::jmin (3, d->params.size());
    for (int i = 0; i < count; ++i)
    {
        const auto& p = d->params.getReference (i);
        if (p.type == EffectParamDef::Type::Continuous) continue;

        if (rowRect (i).contains (e.getPosition()))
        {
            const int arrow = hitTestArrow (e.getPosition(), i);
            if (arrow == 0) continue;

            while (m_data.params.size() <= i)
                m_data.params.add (p.defaultVal);

            float cur = m_data.params[i];

            if (p.type == EffectParamDef::Type::Bool)
            {
                cur = (cur >= 0.5f) ? 0.0f : 1.0f;
            }
            else // Enum
            {
                const int numVals = p.enumValues.size();
                int idx = juce::jlimit (0, numVals - 1, static_cast<int> (cur + 0.5f));
                idx = (idx + arrow + numVals) % numVals;
                cur = static_cast<float> (idx);
            }

            m_data.params.set (i, cur);
            if (onParamChanged) onParamChanged (m_nodeIndex, i, cur);
            repaint();
            return;
        }
    }
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
    const auto pos = getMouseXYRelative();
    if (dragHandleRect().contains (pos))
        return juce::MouseCursor::UpDownResizeCursor;
    if (hitTestParam (pos) >= 0)
        return juce::MouseCursor::LeftRightResizeCursor;
    return juce::MouseCursor::NormalCursor;
}

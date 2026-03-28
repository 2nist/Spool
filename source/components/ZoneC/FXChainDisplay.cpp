#include "FXChainDisplay.h"

//==============================================================================

FXChainDisplay::FXChainDisplay()
{
    setInterceptsMouseClicks (true, true);
}

//==============================================================================

void FXChainDisplay::refresh (const ChainState* chain)
{
    m_chain = chain;
    if (m_chain != nullptr)
        m_focusedNodeIndex = juce::jlimit (0, juce::jmax (0, m_chain->nodes.size() - 1), m_focusedNodeIndex);
    buildNodes();

    const int n = chain ? chain->nodes.size() : 0;
    const int totalH = 144 + n * kNodeStep;
    setSize (getWidth() > 0 ? getWidth() : 184, totalH);

    // Recalculate add button Y
    // Layout: kPad + kInOutH + kConnH + n*(kNodeH+kConnH) + kAddBtnH ...
    m_addButtonY = firstNodeY() + n * kNodeStep;

    resized();
    repaint();
}

void FXChainDisplay::setFocusedNode (int nodeIndex)
{
    m_focusedNodeIndex = juce::jmax (0, nodeIndex);
    buildNodes();
    repaint();
}

void FXChainDisplay::buildNodes()
{
    m_nodes.clear();
    if (!m_chain) return;

    for (int i = 0; i < m_chain->nodes.size(); ++i)
    {
        auto* node = m_nodes.add (new FXNode (i, m_chain->nodes.getReference (i)));

        node->onBypassToggled = [this] (int idx)
        {
            if (onBypassChanged && m_chain)
                onBypassChanged (idx, m_chain->nodes[idx].bypassed);
        };

        node->onRemoveClicked = [this] (int idx)
        {
            if (!m_chain) return;
            if (m_chain->nodes.size() > 1)
            {
                if (onNodeRemoved) onNodeRemoved (idx);
            }
            else
            {
                // Show inline confirmation
                m_confirmNodeIndex = idx;
                if (idx < m_nodes.size())
                    m_nodes[idx]->showRemoveConfirmation (true);
                // Second press (YES) calls onRemoveClicked again and lands here again --
                // but by then nodes.size() == 1 and showConfirm is already true,
                // so the FXNode handles YES→fires onRemoveClicked→this branch fires onNodeRemoved.
                // Actually FXNode fires onRemoveClicked again from the YES button path.
                // We detect: if showConfirm is already true, this is the YES confirmation.
                // This is handled in FXNode mouseDown directly; if node fires onRemoveClicked
                // and it's the last node and confirm is not yet shown we show it.
                // If confirm IS shown and YES is pressed, FXNode fires onRemoveClicked again
                // — so here we just always fire onNodeRemoved if confirm was showing.
                if (m_confirmNodeIndex == idx)
                {
                    if (onNodeRemoved) onNodeRemoved (idx);
                    m_confirmNodeIndex = -1;
                }
            }
        };

        node->onDragStarted = [this] (int idx, const juce::MouseEvent& e)
        {
            startDrag (idx, e);
        };

        node->onParamChanged = [this] (int idx, int paramIdx, float val)
        {
            if (onParamChanged) onParamChanged (idx, paramIdx, val);
        };

        node->onFocused = [this] (int idx)
        {
            m_focusedNodeIndex = idx;
            for (int n = 0; n < m_nodes.size(); ++n)
                m_nodes[n]->setFocused (n == idx);
            if (onNodeFocused)
                onNodeFocused (idx);
        };

        node->setFocused (i == m_focusedNodeIndex);

        addAndMakeVisible (*node);
    }
}

//==============================================================================
// resized
//==============================================================================

void FXChainDisplay::resized()
{
    const int nodeCount = m_nodes.size();
    const int w = getWidth() > 0 ? getWidth() : 184;

    for (int i = 0; i < nodeCount; ++i)
        m_nodes[i]->setBounds (0, nodeY (i), w, kNodeH);
}

//==============================================================================
// Paint
//==============================================================================

void FXChainDisplay::paint (juce::Graphics& g)
{
    if (!m_chain || m_chain->slotIndex < 0)
    {
        // Empty state
        g.setFont (Theme::Font::label());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText ("select a module",
                    0, getHeight() / 2 - 16, getWidth(), 14,
                    juce::Justification::centred, false);
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inactive);
        g.drawText ("to view its chain",
                    0, getHeight() / 2, getWidth(), 12,
                    juce::Justification::centred, false);
        return;
    }

    const int nodeCount = m_chain->nodes.size();

    // IN node
    paintInOutNode (g, false, inNodeY());

    // Connector IN → first node (or OUT if empty)
    paintConnector (g, inNodeY() + kInOutH);

    for (int i = 0; i < nodeCount; ++i)
    {
        // Connector below each effect node
        paintConnector (g, nodeY (i) + kNodeH);
    }

    // + ADD EFFECT button
    paintAddButton (g);

    // Connector below + button
    paintConnector (g, m_addButtonY + kAddBtnH);

    // OUT node
    const int outY = m_addButtonY + kAddBtnH + kConnH;
    paintInOutNode (g, true, outY);

    // Drag overlay
    if (m_isDragging)
        paintDragOverlay (g);
}

void FXChainDisplay::paintConnector (juce::Graphics& g, int y) const
{
    const juce::Colour lineCol = Theme::Signal::audio.withAlpha (0.4f);
    const int cx = getWidth() / 2;

    g.setColour (lineCol);
    g.drawLine (static_cast<float> (cx),
                static_cast<float> (y),
                static_cast<float> (cx),
                static_cast<float> (y + kConnH), 1.0f);

    // ▼ arrow at midpoint
    g.setFont (Theme::Font::micro());
    g.drawText ("\xe2\x96\xbc",
                cx - 4, y + kConnH / 2 - 5, 8, 10,
                juce::Justification::centred, false);
}

void FXChainDisplay::paintInOutNode (juce::Graphics& g, bool isOut, int y) const
{
    const auto r = juce::Rectangle<int> (0, y, getWidth(), kInOutH);

    // Background
    g.setColour (Theme::Colour::surface2);
    g.fillRoundedRectangle (r.toFloat(), Theme::Radius::sm);

    // Border Zone::c
    g.setColour (Theme::Zone::c);
    g.drawRoundedRectangle (r.toFloat().reduced (0.5f), Theme::Radius::sm, Theme::Stroke::subtle);

    // Label
    g.setFont (Theme::Font::label());
    g.setColour (Theme::Zone::c);
    g.drawText (isOut ? "OUT" : "IN",
                r.withTrimmedLeft (8), juce::Justification::centredLeft, false);

    // AUDIO badge (right side)
    const juce::String badgeTxt = "AUDIO";
    const int badgeW = 32, badgeH = 12;
    const auto badgeR = juce::Rectangle<int> (r.getRight() - badgeW - 6,
                                               r.getCentreY() - badgeH / 2,
                                               badgeW, badgeH);
    g.setColour (Theme::Signal::audio.withAlpha (0.15f));
    g.fillRoundedRectangle (badgeR.toFloat(), Theme::Radius::xs);
    g.setColour (Theme::Signal::audio);
    g.drawRoundedRectangle (badgeR.toFloat(), Theme::Radius::xs, Theme::Stroke::subtle);
    g.setFont (Theme::Font::micro());
    g.drawText (badgeTxt, badgeR, juce::Justification::centred, false);
}

void FXChainDisplay::paintAddButton (juce::Graphics& g) const
{
    const int xPad = static_cast<int> (Theme::Space::sm);
    const auto r = juce::Rectangle<int> (xPad, m_addButtonY,
                                          getWidth() - xPad * 2, kAddBtnH);

    // Background
    g.setColour (Theme::Colour::surface0);
    g.fillRoundedRectangle (r.toFloat(), Theme::Radius::xs);

    // Dashed border — alternate 4px painted / 4px gap
    {
        const juce::Colour borderCol = m_addHover ? Theme::Colour::surfaceEdge
                                                   : Theme::Colour::surface4;
        const float fw = static_cast<float> (r.getWidth());
        const float fh = static_cast<float> (r.getHeight());
        const float fx = static_cast<float> (r.getX());
        const float fy = static_cast<float> (r.getY());
        g.setColour (borderCol);

        // Top and bottom edges
        for (float x = fx; x < fx + fw; x += 8.0f)
        {
            g.drawLine (x, fy, juce::jmin (x + 4.0f, fx + fw), fy, 0.5f);
            g.drawLine (x, fy + fh, juce::jmin (x + 4.0f, fx + fw), fy + fh, 0.5f);
        }
        // Left and right edges
        for (float y = fy; y < fy + fh; y += 8.0f)
        {
            g.drawLine (fx, y, fx, juce::jmin (y + 4.0f, fy + fh), 0.5f);
            g.drawLine (fx + fw, y, fx + fw, juce::jmin (y + 4.0f, fy + fh), 0.5f);
        }
    }

    // Text
    g.setFont (Theme::Font::micro());
    g.setColour (m_addHover ? Theme::Colour::inkMuted : Theme::Colour::inkGhost);
    g.drawText ("+ ADD EFFECT", r, juce::Justification::centred, false);
}

void FXChainDisplay::paintDragOverlay (juce::Graphics& g) const
{
    if (m_dropTargetIndex < 0 || !m_chain) return;

    // Insertion indicator
    const int insertY = nodeY (m_dropTargetIndex) - 2;
    g.setColour (Theme::Zone::c);
    g.fillRect (0, insertY, getWidth(), 2);
}

//==============================================================================
// Drag reorder
//==============================================================================

void FXChainDisplay::startDrag (int nodeIndex, const juce::MouseEvent& e)
{
    m_dragNodeIndex   = nodeIndex;
    m_dropTargetIndex = nodeIndex;
    m_dragStartY      = e.getScreenY();
    m_isDragging      = true;

    if (nodeIndex < m_nodes.size())
        m_nodes[nodeIndex]->setDragging (true);

    repaint();
}

void FXChainDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (!m_isDragging || !m_chain) return;

    const int deltaY   = e.getScreenY() - m_dragStartY;
    const int rawIndex = m_dragNodeIndex + (deltaY / kNodeStep);
    m_dropTargetIndex  = juce::jlimit (0, m_chain->nodes.size() - 1, rawIndex);

    repaint();
}

void FXChainDisplay::mouseUp (const juce::MouseEvent&)
{
    if (!m_isDragging) return;

    if (m_dragNodeIndex >= 0 && m_dragNodeIndex < m_nodes.size())
        m_nodes[m_dragNodeIndex]->setDragging (false);

    if (m_isDragging && m_dropTargetIndex != m_dragNodeIndex && m_dropTargetIndex >= 0)
    {
        // TODO: viewport edge auto-scroll
        if (onNodeMoved)
            onNodeMoved (m_dragNodeIndex, m_dropTargetIndex);
    }

    m_isDragging      = false;
    m_dragNodeIndex   = -1;
    m_dropTargetIndex = -1;
    repaint();
}

#include "ZoneCComponent.h"

//==============================================================================
ZoneCComponent::ZoneCComponent()
{
    // Ensure our content never bleeds outside our bounds
    setPaintingIsUnclipped (false);

    // Viewport setup
    m_chainViewport.setScrollBarsShown (false, false);
    m_chainViewport.setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::never);
    m_chainViewport.setViewedComponent (&m_chainDisplay, false);

    addAndMakeVisible (m_chainViewport);
    addAndMakeVisible (m_macroStrip);

    // Wire FXChainDisplay callbacks
    m_chainDisplay.onAddEffectClicked = [this]()
    {
        showEffectPicker();
    };

    m_chainDisplay.onBypassChanged = [this] (int nodeIndex, bool bypassed)
    {
        updateNodeBypass (nodeIndex, bypassed);
    };

    m_chainDisplay.onNodeRemoved = [this] (int nodeIndex)
    {
        removeNode (nodeIndex);
    };

    m_chainDisplay.onParamChanged = [this] (int nodeIndex, int paramIdx, float value)
    {
        updateNodeParam (nodeIndex, paramIdx, value);
    };

    m_chainDisplay.onNodeMoved = [this] (int fromIndex, int toIndex)
    {
        moveNode (fromIndex, toIndex);
    };

    m_chainDisplay.onNodeFocused = [this] (int nodeIndex)
    {
        setFocusedNode (nodeIndex);
    };

    // Show slot 0 chain by default so nodes are visible on launch
    applyDefaultChain (0);
    m_currentSlotIndex = 0;
    refreshDisplay();
}

//==============================================================================
void ZoneCComponent::showChainForSlot (int slotIndex)
{
    if (m_isPinned)
        return;

    m_currentSlotIndex = slotIndex;

    if (slotIndex >= 0 && slotIndex < 8 && !m_chains[slotIndex].initialised)
        applyDefaultChain (slotIndex);

    refreshDisplay();
    resized();
    repaint();
}

void ZoneCComponent::showDefault()
{
    if (m_isPinned)
        return;

    m_currentSlotIndex = -1;
    refreshDisplay();
    resized();
    repaint();
}

void ZoneCComponent::setCurrentModuleTypeName (const juce::String& name)
{
    m_moduleTypeName = name;
    repaint();
}

int ZoneCComponent::currentWidth() const
{
    return m_isCollapsed ? kCollapsedW : m_expandedWidth;
}

void ZoneCComponent::setMacroStripVisible (bool shouldShow)
{
    if (m_showMacroStrip == shouldShow)
        return;

    m_showMacroStrip = shouldShow;
    m_macroStrip.setVisible (shouldShow);
    resized();
    repaint();
}

//==============================================================================
void ZoneCComponent::paint (juce::Graphics& g)
{
    if (m_isCollapsed)
    {
        paintCollapsed (g);
        return;
    }

    g.setColour (Theme::Colour::surface0.withAlpha (0.92f));
    g.fillRect  (getLocalBounds());

    // Zone stripe at top
    Theme::Helper::drawZoneStripe (g, getLocalBounds(), Theme::Zone::c);

    paintHeader (g);

    if (m_isPinned)
        paintPinBanner (g);

}

void ZoneCComponent::resized()
{
    if (m_isCollapsed)
        return;

    const int w = getWidth();
    const int h = getHeight();

    const int stripeH = static_cast<int> (Theme::Space::zoneStripeHeight);
    const int pinH    = pinBannerHeight();
    const int tabsH   = tabsHeight();

    // Macro strip at the bottom
    const int macroH = macroStripHeight();
    m_macroStrip.setVisible (macroH > 0);
    if (macroH > 0)
        m_macroStrip.setBounds (0, h - macroH, w, macroH);
    else
        m_macroStrip.setBounds (0, h, 0, 0);

    // Viewport fills the remaining area between tabs and macro strip
    const auto vr = chainRect();
    m_chainViewport.setBounds (vr);

    // Keep FXChainDisplay width in sync
    m_chainDisplay.setSize (vr.getWidth(), m_chainDisplay.getHeight());

    // Position picker overlay if visible
    if (m_picker != nullptr && m_picker->isVisible())
    {
        const int addBtnAbsY = vr.getY()
                               + m_chainDisplay.addButtonY()
                               - m_chainViewport.getViewPosition().getY();
        const int overlayY   = juce::jmin (addBtnAbsY,
                                           h - EffectPickerOverlay::kHeight - macroH);
        m_picker->setBounds (0, overlayY, w, EffectPickerOverlay::kHeight);
    }

    juce::ignoreUnused (stripeH, pinH, tabsH);
}

void ZoneCComponent::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // Collapse button
    if (m_collapseButtonRect.contains (pos))
    {
        m_isCollapsed = !m_isCollapsed;
        if (onCollapseChanged)
            onCollapseChanged (m_isCollapsed);
        resized();
        repaint();
        return;
    }

    // Pin button (only when expanded)
    if (!m_isCollapsed && m_pinButtonRect.contains (pos))
    {
        setPinned (!m_isPinned);
        return;
    }

    // Unpin link in banner
    if (!m_isCollapsed && m_isPinned && m_unpinLinkRect.contains (pos))
    {
        setPinned (false);
        return;
    }

}

//==============================================================================
// Private helpers
//==============================================================================

int ZoneCComponent::pinBannerHeight() const
{
    return m_isPinned ? kPinBannerH : 0;
}

int ZoneCComponent::tabsHeight() const
{
    return kTabsH;
}

juce::Rectangle<int> ZoneCComponent::chainRect() const
{
    const int w      = getWidth();
    const int h      = getHeight();
    const int top    = static_cast<int> (Theme::Space::zoneStripeHeight)
                       + kHeaderH
                       + pinBannerHeight()
                       + kTabsH;
    const int bottom = h - macroStripHeight();
    return { 0, top, w, juce::jmax (0, bottom - top) };
}

ChainState* ZoneCComponent::currentChainPtr()
{
    const int idx = m_isPinned ? m_pinnedSlotIndex : m_currentSlotIndex;
    if (idx >= 0 && idx < 8)
        return &m_chains[idx];
    return nullptr;
}

void ZoneCComponent::applyDefaultChain (int slotIndex)
{
    auto& c      = m_chains[slotIndex];
    c.slotIndex  = slotIndex;
    c.nodes.clear();
    c.initialised = true;

    // Default chain assignments per slot type:
    // SYNTH (0)  → FILTER + REVERB
    // SAMPLER (1)→ EQ + COMPRESSOR
    // VST3 (2)   → empty
    // OUTPUT (7) → EQ + COMPRESSOR
    // Others     → empty
    if (slotIndex == 0)
    {
        c.nodes.add (EffectRegistry::makeDefault ("filter"));
        c.nodes.add (EffectRegistry::makeDefault ("reverb"));
    }
    else if (slotIndex == 1)
    {
        c.nodes.add (EffectRegistry::makeDefault ("eq"));
        c.nodes.add (EffectRegistry::makeDefault ("compressor"));
    }
    else if (slotIndex == 7)
    {
        c.nodes.add (EffectRegistry::makeDefault ("eq"));
        c.nodes.add (EffectRegistry::makeDefault ("compressor"));
    }
    // else: empty chain
}

void ZoneCComponent::refreshDisplay()
{
    m_chainDisplay.refresh (currentChainPtr());
    m_chainDisplay.setFocusedNode (m_focusedNodeIndex);
}

void ZoneCComponent::setPinned (bool pin)
{
    m_isPinned = pin;
    if (pin)
        m_pinnedSlotIndex = m_currentSlotIndex;
    else
        m_pinnedSlotIndex = -1;

    refreshDisplay();
    resized();
    repaint();
}

void ZoneCComponent::setFocusedNode (int nodeIndex)
{
    auto* chain = currentChainPtr();
    if (chain == nullptr || chain->nodes.isEmpty())
    {
        m_focusedNodeIndex = 0;
        repaint();
        return;
    }

    m_focusedNodeIndex = juce::jlimit (0, chain->nodes.size() - 1, nodeIndex);
    m_chainDisplay.setFocusedNode (m_focusedNodeIndex);
    repaint();
}

void ZoneCComponent::showEffectPicker()
{
    if (m_currentSlotIndex < 0 && !m_isPinned)
        return;

    if (m_picker == nullptr)
        m_picker = std::make_unique<EffectPickerOverlay>();

    m_picker->onEffectChosen = [this] (const juce::String& effectId)
    {
        addEffect (effectId);
        dismissPicker();
    };

    m_picker->onCancel = [this]()
    {
        dismissPicker();
    };

    addAndMakeVisible (*m_picker);
    m_picker->toFront (false);

    // Position — do a resized() pass to compute bounds
    resized();
}

void ZoneCComponent::dismissPicker()
{
    if (m_picker != nullptr)
    {
        removeChildComponent (m_picker.get());
        m_picker.reset();
    }
}

void ZoneCComponent::addEffect (const juce::String& effectId)
{
    const int idx = m_isPinned ? m_pinnedSlotIndex : m_currentSlotIndex;
    if (idx < 0 || idx >= 8)
        return;

    if (!m_chains[idx].initialised)
        applyDefaultChain (idx);

    m_chains[idx].nodes.add (EffectRegistry::makeDefault (effectId));
    m_focusedNodeIndex = juce::jmax (0, m_chains[idx].nodes.size() - 1);
    refreshDisplay();
    resized();
    if (onChainRebuilt) onChainRebuilt (idx, m_chains[idx]);
}

void ZoneCComponent::removeNode (int nodeIndex)
{
    const int idx = m_isPinned ? m_pinnedSlotIndex : m_currentSlotIndex;
    if (idx < 0 || idx >= 8)
        return;

    auto& nodes = m_chains[idx].nodes;
    if (nodeIndex >= 0 && nodeIndex < nodes.size())
    {
        nodes.remove (nodeIndex);
        m_focusedNodeIndex = juce::jlimit (0, juce::jmax (0, nodes.size() - 1), m_focusedNodeIndex);
        refreshDisplay();
        resized();
        if (onChainRebuilt) onChainRebuilt (idx, m_chains[idx]);
    }
}

void ZoneCComponent::moveNode (int fromIndex, int toIndex)
{
    const int idx = m_isPinned ? m_pinnedSlotIndex : m_currentSlotIndex;
    if (idx < 0 || idx >= 8)
        return;

    auto& nodes = m_chains[idx].nodes;
    if (fromIndex < 0 || fromIndex >= nodes.size())
        return;
    if (toIndex < 0 || toIndex >= nodes.size())
        return;
    if (fromIndex == toIndex)
        return;

    const EffectNode node = nodes[fromIndex];
    nodes.remove (fromIndex);
    nodes.insert (toIndex, node);
    refreshDisplay();
    resized();
    if (onChainRebuilt) onChainRebuilt (idx, m_chains[idx]);
}

void ZoneCComponent::updateNodeBypass (int nodeIndex, bool bypassed)
{
    const int idx = m_isPinned ? m_pinnedSlotIndex : m_currentSlotIndex;
    if (idx < 0 || idx >= 8)
        return;

    auto& nodes = m_chains[idx].nodes;
    if (nodeIndex >= 0 && nodeIndex < nodes.size())
    {
        nodes.getReference (nodeIndex).bypassed = bypassed;
        repaint();
        if (onChainRebuilt) onChainRebuilt (idx, m_chains[idx]);
    }
}

void ZoneCComponent::updateNodeParam (int nodeIndex, int paramIndex, float value)
{
    const int idx = m_isPinned ? m_pinnedSlotIndex : m_currentSlotIndex;
    if (idx < 0 || idx >= 8)
        return;

    auto& nodes = m_chains[idx].nodes;
    if (nodeIndex >= 0 && nodeIndex < nodes.size())
    {
        auto& params = nodes.getReference (nodeIndex).params;
        if (paramIndex >= 0 && paramIndex < params.size())
        {
            params.set (paramIndex, value);
            repaint();

            // Fire DSP callback — PluginEditor wires this to the processor
            if (onDspParamChanged)
                onDspParamChanged (idx, nodeIndex, paramIndex, value);
        }
    }
}

//==============================================================================
// Paint helpers
//==============================================================================

void ZoneCComponent::paintCollapsed (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgC);

    // Vertical zone stripe
    g.setColour (Theme::Zone::c);
    g.fillRect (0, 0, static_cast<int> (Theme::Space::zoneStripeHeight), getHeight());

    // Vertical "ZONE C" label
    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (
        -juce::MathConstants<float>::halfPi,
        static_cast<float> (getWidth()) * 0.5f,
        static_cast<float> (getHeight()) * 0.5f));
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Zone::c);
    g.drawText ("ZONE C", getLocalBounds(), juce::Justification::centred, false);
    g.restoreState();

    // Collapse toggle arrow (›) — collapsed means expand arrow points left
    const int btnH = 20;
    const int btnY = kHeaderH - btnH;
    m_collapseButtonRect = { 0, btnY, getWidth(), btnH };
    g.setFont (Theme::Font::label());
    g.setColour (Theme::Colour::inkMid);
    g.drawText (juce::CharPointer_UTF8 ("\xc2\xab"),
                m_collapseButtonRect, juce::Justification::centred, false);
}

void ZoneCComponent::paintHeader (juce::Graphics& g)
{
    const int w = getWidth();
    const int stripeH = static_cast<int> (Theme::Space::zoneStripeHeight);
    const juce::Rectangle<int> headerR { 0, stripeH, w, kHeaderH };

    g.setColour (Theme::Colour::surface0);
    g.fillRect (headerR);

    // "ZONE C" label left
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Zone::c);
    g.drawText ("ZONE C",
                headerR.withTrimmedLeft (static_cast<int> (Theme::Space::sm))
                        .withWidth (44),
                juce::Justification::centredLeft, false);

    // Module name centred
    if (m_currentSlotIndex >= 0 || m_isPinned)
    {
        auto titleArea = headerR.withTrimmedLeft (56).withTrimmedRight (40);
        g.setFont (Theme::Font::label());
        g.setColour (Theme::Colour::inkMuted);
        const juce::String label = m_moduleTypeName.isNotEmpty()
                                   ? m_moduleTypeName : "SLOT " + juce::String (m_currentSlotIndex + 1);
        g.drawText (label,
                    titleArea.removeFromTop (14),
                    juce::Justification::centred, false);

        if (auto* chain = currentChainPtr(); chain != nullptr && ! chain->nodes.isEmpty())
        {
            const int currentIndex = juce::jlimit (0, chain->nodes.size() - 1, m_focusedNodeIndex);
            const auto& current = chain->nodes.getReference (currentIndex);
            const juce::String previous = currentIndex > 0 ? chain->nodes[currentIndex - 1].effectType.toUpperCase() : "INPUT";
            const juce::String next = currentIndex + 1 < chain->nodes.size() ? chain->nodes[currentIndex + 1].effectType.toUpperCase() : "ADD";
            g.setFont (Theme::Font::micro());
            g.setColour (Theme::Colour::inkGhost);
            g.drawText ("IN " + previous + "  |  CUR " + current.effectType.toUpperCase()
                            + " " + current.effectDomain.toUpperCase() + "  |  OUT " + next,
                        titleArea,
                        juce::Justification::centred,
                        true);
        }
    }

    // Pin button (16px, right area) — computed directly to avoid mutating const headerR
    m_pinButtonRect = juce::Rectangle<int> (w - 36, stripeH, 36, kHeaderH).reduced (4, 4);
    const juce::String pinChar = m_isPinned
                                 ? juce::CharPointer_UTF8 ("\xe2\x97\x86")
                                 : juce::CharPointer_UTF8 ("\xe2\x97\x87");
    g.setFont (Theme::Font::body());
    g.setColour (m_isPinned ? Theme::Zone::c : Theme::Colour::inkMuted);
    g.drawText (pinChar, m_pinButtonRect, juce::Justification::centred, false);

    // Collapse button (›) — expanded means collapse arrow points right
    m_collapseButtonRect = { w - 16, stripeH, 16, kHeaderH };
    g.setFont (Theme::Font::label());
    g.setColour (Theme::Colour::inkMid);
    g.drawText (juce::CharPointer_UTF8 ("\xc2\xbb"),
                m_collapseButtonRect, juce::Justification::centred, false);
}

void ZoneCComponent::paintPinBanner (juce::Graphics& g)
{
    const int w = getWidth();
    const int stripeH = static_cast<int> (Theme::Space::zoneStripeHeight);
    const juce::Rectangle<int> bannerR { 0, stripeH + kHeaderH, w, kPinBannerH };

    // Background
    g.setColour (Theme::Zone::d.withAlpha (0.15f));
    g.fillRect (bannerR);

    // Warning + text
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Zone::d);
    const juce::String txt = juce::String (juce::CharPointer_UTF8 ("\xe2\x9a\xa0")) +
                             juce::String (" PINNED TO SLOT ") +
                             juce::String (m_pinnedSlotIndex + 1);
    g.drawText (txt,
                bannerR.withTrimmedLeft (static_cast<int> (Theme::Space::sm)),
                juce::Justification::centredLeft, false);

    // Unpin link at right — computed directly to avoid mutating const bannerR
    m_unpinLinkRect = juce::Rectangle<int> (w - 44, stripeH + kHeaderH, 44, kPinBannerH).reduced (2, 2);
    g.setColour (Theme::Zone::c);
    g.drawText ("UNPIN", m_unpinLinkRect, juce::Justification::centred, false);
}


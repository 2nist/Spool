#include "ZoneCComponent.h"

namespace
{
using ChainContext = ZoneCComponent::ChainContext;

ChainContext contextFromIndex (int index)
{
    switch (index)
    {
        case 0:  return ChainContext::insert;
        case 1:  return ChainContext::busA;
        case 2:  return ChainContext::busB;
        case 3:  return ChainContext::busC;
        case 4:  return ChainContext::busD;
        default: return ChainContext::master;
    }
}

juce::String contextTitle (ChainContext context)
{
    switch (context)
    {
        case ChainContext::insert: return "INSERT";
        case ChainContext::busA:   return "BUS A";
        case ChainContext::busB:   return "BUS B";
        case ChainContext::busC:   return "BUS C";
        case ChainContext::busD:   return "BUS D";
        case ChainContext::master: return "MASTER";
    }
    return "INSERT";
}
}

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
    m_chainContext = ChainContext::insert;
    refreshDisplay();
}

//==============================================================================
void ZoneCComponent::showChainForSlot (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= 8)
        return;

    if (isInsertContext() && m_isPinned)
        return;

    m_currentSlotIndex = slotIndex;

    if (!m_chains[slotIndex].initialised)
        applyDefaultChain (slotIndex);

    if (isInsertContext())
        refreshDisplay();

    repaint();
}

void ZoneCComponent::showDefault()
{
    if (isInsertContext() && m_isPinned)
        return;

    m_currentSlotIndex = -1;

    if (isInsertContext())
        refreshDisplay();

    repaint();
}

void ZoneCComponent::setChainContext (ChainContext context)
{
    if (m_chainContext == context)
        return;

    m_chainContext = context;

    // Pinning only applies to INSERT chain.
    if (! isInsertContext())
    {
        m_isPinned = false;
        m_pinnedSlotIndex = -1;
    }

    ensureCurrentChainInitialised();
    refreshDisplay();
    resized();
    repaint();
    notifyContextChanged();
}

void ZoneCComponent::showBusContext (int busIndex)
{
    const int clamped = juce::jlimit (0, kNumBusChains - 1, busIndex);
    setChainContext (contextFromIndex (clamped + 1));
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

ZoneCComponent::ChainBank ZoneCComponent::getChainBank() const
{
    ChainBank bank;
    for (int i = 0; i < 8; ++i)
        bank.inserts[(size_t) i] = m_chains[i];
    for (int i = 0; i < kNumBusChains; ++i)
        bank.buses[(size_t) i] = m_busChains[i];
    bank.master = m_masterChain;
    return bank;
}

void ZoneCComponent::setChainBank (const ChainBank& bank, bool pushToDspCallbacks)
{
    for (int i = 0; i < 8; ++i)
    {
        m_chains[i] = bank.inserts[(size_t) i];
        if (m_chains[i].slotIndex < 0)
            m_chains[i].slotIndex = i;
    }

    for (int i = 0; i < kNumBusChains; ++i)
    {
        m_busChains[i] = bank.buses[(size_t) i];
        if (m_busChains[i].slotIndex < 0)
            m_busChains[i].slotIndex = 100 + i;
    }

    m_masterChain = bank.master;
    if (m_masterChain.slotIndex < 0)
        m_masterChain.slotIndex = 200;

    refreshDisplay();
    resized();
    repaint();

    if (! pushToDspCallbacks)
        return;

    for (int slot = 0; slot < 8; ++slot)
    {
        if (onChainRebuilt)
            onChainRebuilt (slot, m_chains[slot]);
        if (onContextChainRebuilt)
            onContextChainRebuilt (ChainContext::insert, slot, m_chains[slot]);
    }

    for (int bus = 0; bus < kNumBusChains; ++bus)
    {
        if (onContextChainRebuilt)
            onContextChainRebuilt (static_cast<ChainContext> (static_cast<int> (ChainContext::busA) + bus),
                                   bus,
                                   m_busChains[bus]);
    }

    if (onContextChainRebuilt)
        onContextChainRebuilt (ChainContext::master, -1, m_masterChain);
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

    if (m_isPinned && isInsertContext())
        paintPinBanner (g);

    paintContextTabs (g);
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

    // Context tabs
    if (!m_isCollapsed)
    {
        for (int i = 0; i < static_cast<int> (m_contextTabRects.size()); ++i)
        {
            if (m_contextTabRects[(size_t) i].contains (pos))
            {
                setChainContext (contextFromIndex (i));
                return;
            }
        }
    }

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
    if (!m_isCollapsed && isInsertContext() && m_pinButtonRect.contains (pos))
    {
        setPinned (!m_isPinned);
        return;
    }

    // Unpin link in banner
    if (!m_isCollapsed && isInsertContext() && m_isPinned && m_unpinLinkRect.contains (pos))
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
    return (m_isPinned && isInsertContext()) ? kPinBannerH : 0;
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

bool ZoneCComponent::isInsertContext() const noexcept
{
    return m_chainContext == ChainContext::insert;
}

int ZoneCComponent::currentContextIndex() const noexcept
{
    if (isInsertContext())
        return m_isPinned ? m_pinnedSlotIndex : m_currentSlotIndex;

    if (m_chainContext == ChainContext::master)
        return -1;

    return static_cast<int> (m_chainContext) - 1;
}

ChainState* ZoneCComponent::chainForContext (ChainContext context)
{
    if (context == ChainContext::insert)
    {
        const int idx = m_isPinned ? m_pinnedSlotIndex : m_currentSlotIndex;
        if (idx >= 0 && idx < 8)
            return &m_chains[idx];
        return nullptr;
    }

    if (context == ChainContext::master)
        return &m_masterChain;

    const int busIndex = static_cast<int> (context) - 1;
    if (busIndex >= 0 && busIndex < kNumBusChains)
        return &m_busChains[busIndex];

    return nullptr;
}

ChainState* ZoneCComponent::currentChainPtr()
{
    return chainForContext (m_chainContext);
}

void ZoneCComponent::ensureCurrentChainInitialised()
{
    if (m_chainContext == ChainContext::insert)
    {
        const int idx = m_isPinned ? m_pinnedSlotIndex : m_currentSlotIndex;
        if (idx >= 0 && idx < 8 && !m_chains[idx].initialised)
            applyDefaultChain (idx);
        return;
    }

    if (m_chainContext == ChainContext::master)
    {
        if (!m_masterChain.initialised)
            applyDefaultMasterChain();
        return;
    }

    const int busIndex = static_cast<int> (m_chainContext) - 1;
    if (busIndex >= 0 && busIndex < kNumBusChains && !m_busChains[busIndex].initialised)
        applyDefaultBusChain (busIndex);
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

void ZoneCComponent::applyDefaultBusChain (int busIndex)
{
    if (busIndex < 0 || busIndex >= kNumBusChains)
        return;

    auto& c = m_busChains[busIndex];
    c.slotIndex = 100 + busIndex; // non-slot sentinel
    c.nodes.clear();
    c.initialised = true;
}

void ZoneCComponent::applyDefaultMasterChain()
{
    m_masterChain.slotIndex = 200; // non-slot sentinel
    m_masterChain.nodes.clear();
    m_masterChain.initialised = true;
}

void ZoneCComponent::refreshDisplay()
{
    ensureCurrentChainInitialised();
    if (auto* chain = currentChainPtr())
    {
        if (chain->nodes.isEmpty())
            m_focusedNodeIndex = 0;
        else
            m_focusedNodeIndex = juce::jlimit (0, chain->nodes.size() - 1, m_focusedNodeIndex);
    }

    m_chainDisplay.refresh (currentChainPtr());
    m_chainDisplay.setFocusedNode (m_focusedNodeIndex);
}

void ZoneCComponent::notifyContextChanged()
{
    if (onContextChanged)
        onContextChanged (m_chainContext, currentContextIndex());
}

void ZoneCComponent::notifyChainRebuiltForCurrentContext()
{
    auto* chain = currentChainPtr();
    if (chain == nullptr)
        return;

    const int contextIndex = currentContextIndex();

    // Legacy insert-only callback path
    if (isInsertContext() && contextIndex >= 0 && contextIndex < 8 && onChainRebuilt)
        onChainRebuilt (contextIndex, m_chains[contextIndex]);

    if (onContextChainRebuilt)
        onContextChainRebuilt (m_chainContext, contextIndex, *chain);
}

void ZoneCComponent::notifyParamChangedForCurrentContext (int nodeIndex, int paramIndex, float value)
{
    const int contextIndex = currentContextIndex();

    // Legacy insert-only callback path
    if (isInsertContext() && contextIndex >= 0 && contextIndex < 8 && onDspParamChanged)
        onDspParamChanged (contextIndex, nodeIndex, paramIndex, value);

    if (onContextDspParamChanged)
        onContextDspParamChanged (m_chainContext, contextIndex, nodeIndex, paramIndex, value);
}

void ZoneCComponent::setPinned (bool pin)
{
    if (! isInsertContext())
        return;

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
    if (currentChainPtr() == nullptr)
        return;

    ensureCurrentChainInitialised();

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
    auto* chain = currentChainPtr();
    if (chain == nullptr)
        return;

    ensureCurrentChainInitialised();

    chain->nodes.add (EffectRegistry::makeDefault (effectId));
    m_focusedNodeIndex = juce::jmax (0, chain->nodes.size() - 1);
    refreshDisplay();
    resized();
    notifyChainRebuiltForCurrentContext();
}

void ZoneCComponent::removeNode (int nodeIndex)
{
    auto* chain = currentChainPtr();
    if (chain == nullptr)
        return;

    auto& nodes = chain->nodes;
    if (nodeIndex >= 0 && nodeIndex < nodes.size())
    {
        nodes.remove (nodeIndex);
        m_focusedNodeIndex = juce::jlimit (0, juce::jmax (0, nodes.size() - 1), m_focusedNodeIndex);
        refreshDisplay();
        resized();
        notifyChainRebuiltForCurrentContext();
    }
}

void ZoneCComponent::moveNode (int fromIndex, int toIndex)
{
    auto* chain = currentChainPtr();
    if (chain == nullptr)
        return;

    auto& nodes = chain->nodes;
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
    notifyChainRebuiltForCurrentContext();
}

void ZoneCComponent::updateNodeBypass (int nodeIndex, bool bypassed)
{
    auto* chain = currentChainPtr();
    if (chain == nullptr)
        return;

    auto& nodes = chain->nodes;
    if (nodeIndex >= 0 && nodeIndex < nodes.size())
    {
        nodes.getReference (nodeIndex).bypassed = bypassed;
        repaint();
        notifyChainRebuiltForCurrentContext();
    }
}

void ZoneCComponent::updateNodeParam (int nodeIndex, int paramIndex, float value)
{
    auto* chain = currentChainPtr();
    if (chain == nullptr)
        return;

    auto& nodes = chain->nodes;
    if (nodeIndex >= 0 && nodeIndex < nodes.size())
    {
        auto& params = nodes.getReference (nodeIndex).params;
        if (paramIndex >= 0 && paramIndex < params.size())
        {
            params.set (paramIndex, value);
            repaint();
            notifyParamChangedForCurrentContext (nodeIndex, paramIndex, value);
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

    // Context title centred
    {
        auto titleArea = headerR.withTrimmedLeft (56).withTrimmedRight (40);
        g.setFont (Theme::Font::label());
        g.setColour (Theme::Colour::inkMuted);

        juce::String label = contextTitle (m_chainContext);
        if (isInsertContext())
        {
            const int slotIndex = currentContextIndex();
            if (m_moduleTypeName.isNotEmpty())
                label = m_moduleTypeName;
            else if (slotIndex >= 0)
                label = "SLOT " + juce::String (slotIndex + 1);
            else
                label = "INSERT";
        }

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

    // Pin button (16px, right area) — only for INSERT chain
    if (isInsertContext())
    {
        m_pinButtonRect = juce::Rectangle<int> (w - 36, stripeH, 36, kHeaderH).reduced (4, 4);
        const juce::String pinChar = m_isPinned
                                     ? juce::CharPointer_UTF8 ("\xe2\x97\x86")
                                     : juce::CharPointer_UTF8 ("\xe2\x97\x87");
        g.setFont (Theme::Font::body());
        g.setColour (m_isPinned ? Theme::Zone::c : Theme::Colour::inkMuted);
        g.drawText (pinChar, m_pinButtonRect, juce::Justification::centred, false);
    }
    else
    {
        m_pinButtonRect = {};
    }

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

void ZoneCComponent::paintContextTabs (juce::Graphics& g)
{
    if (m_isCollapsed || tabsHeight() <= 0)
        return;

    const int stripeH = static_cast<int> (Theme::Space::zoneStripeHeight);
    auto tabsArea = juce::Rectangle<int> (0, stripeH + kHeaderH + pinBannerHeight(), getWidth(), tabsHeight());

    g.setColour (Theme::Colour::surface1.withAlpha (0.95f));
    g.fillRect (tabsArea);

    constexpr const char* labels[kNumContexts] = { "INSERT", "BUS A", "BUS B", "BUS C", "BUS D", "MASTER" };
    auto remaining = tabsArea;
    for (int i = 0; i < kNumContexts; ++i)
    {
        const int remainingTabs = kNumContexts - i;
        auto tab = remaining.removeFromLeft (remaining.getWidth() / remainingTabs).reduced (1, 2);
        m_contextTabRects[(size_t) i] = tab;

        const auto tabContext = contextFromIndex (i);
        const bool active = (m_chainContext == tabContext);
        bool emptyContext = false;

        if (tabContext == ChainContext::master)
            emptyContext = m_masterChain.nodes.isEmpty();
        else if (tabContext != ChainContext::insert)
            emptyContext = m_busChains[(size_t) (i - 1)].nodes.isEmpty();

        juce::Colour fill = Theme::Colour::surface2.withAlpha (active ? 0.92f : 0.55f);
        if (!active && emptyContext)
            fill = Theme::Colour::surface2.withAlpha (0.28f);
        g.setColour (fill);
        g.fillRoundedRectangle (tab.toFloat(), 3.0f);

        juce::Colour edge = active ? Theme::Zone::c.withAlpha (0.9f)
                                   : Theme::Colour::surfaceEdge.withAlpha (0.45f);
        g.setColour (edge);
        g.drawRoundedRectangle (tab.toFloat(), 3.0f, 1.0f);

        g.setFont (Theme::Font::micro());
        juce::Colour textCol = active ? juce::Colour (Theme::Colour::inkLight)
                                      : juce::Colour (Theme::Colour::inkGhost);
        if (!active && emptyContext)
            textCol = Theme::Colour::inkGhost.withAlpha (0.55f);
        g.setColour (textCol);
        g.drawText (labels[i], tab, juce::Justification::centred, false);
    }
}


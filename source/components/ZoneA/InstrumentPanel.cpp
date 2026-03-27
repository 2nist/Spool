#include "InstrumentPanel.h"
#include "PolySynthEditorComponent.h"
#include "DrumEditorComponent.h"
#include "../../PluginProcessor.h"
#include "../../preset/PresetManager.h"

//==============================================================================
InstrumentPanel::InstrumentPanel (PluginProcessor& p)
    : processorRef (p)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    m_editorViewport.setScrollBarsShown (true, false);
    addChildComponent (m_editorViewport);
    addChildComponent (m_presetBar);
}

//==============================================================================
// Static helpers
//==============================================================================

juce::Colour InstrumentPanel::typeColor (const juce::String& moduleType)
{
    if (moduleType == "SYNTH")  return juce::Colour (Theme::Zone::a);
    if (moduleType == "DRUMS")  return juce::Colour (Theme::Zone::b);
    if (moduleType == "REEL")   return juce::Colour (Theme::Zone::d);
    if (moduleType == "OUTPUT") return juce::Colour (Theme::Colour::error);
    return juce::Colour (Theme::Colour::inkGhost);
}

juce::String InstrumentPanel::typeShort (const juce::String& moduleType)
{
    if (moduleType == "SYNTH")  return "POLY";
    if (moduleType == "DRUMS")  return "DRUM";
    if (moduleType == "REEL")   return "REEL";
    if (moduleType == "OUTPUT") return "OUT";
    if (moduleType.isNotEmpty()) return moduleType.substring (0, 4);
    return "";
}

//==============================================================================
// Data population
//==============================================================================

void InstrumentPanel::setSlots (const juce::Array<SlotEntry>& slots)
{
    m_slots = slots;
    rebuildRowCache();
    repaint();
}

void InstrumentPanel::rebuildRowCache()
{
    m_rowYCache.clear();

    int y = kBrowserHeaderH;
    juce::String lastGroup;

    for (const auto& entry : m_slots)
    {
        if (entry.groupName != lastGroup)
        {
            lastGroup = entry.groupName;
            y += kGroupHeaderH;
        }
        m_rowYCache.add ({ entry.slotIndex, y });
        y += kSlotRowH;
    }
}

int InstrumentPanel::slotAtBrowserY (int posY) const noexcept
{
    for (const auto& pair : m_rowYCache)
        if (posY >= pair.second && posY < pair.second + kSlotRowH)
            return pair.first;
    return -1;
}

//==============================================================================
// Navigation
//==============================================================================

void InstrumentPanel::showSlot (int slotIndex)
{
    m_activeSlot = slotIndex;
    m_state      = State::editor;

    const SlotEntry* entry = getActiveEntry();
    if (entry != nullptr)
        mountEditor (*entry);

    m_editorViewport.setVisible (true);
    resized();
    repaint();
}

void InstrumentPanel::showBrowser()
{
    unmountEditor();
    m_editorViewport.setVisible (false);
    m_state      = State::browser;
    m_activeSlot = -1;
    resized();
    repaint();
}

//==============================================================================
// Editor host
//==============================================================================

void InstrumentPanel::mountEditor (const SlotEntry& entry)
{
    // Detach any previous content without deleting owned components
    m_editorViewport.setViewedComponent (nullptr, false);

    if (entry.moduleType == "SYNTH")
    {
        if (!m_polySynthEditor)
            m_polySynthEditor = std::make_unique<PolySynthEditorComponent>();

        auto& synth = processorRef.getAudioGraph().getPolySynth (entry.slotIndex);
        m_polySynthEditor->setProcessor (&synth);
        m_editorViewport.setViewedComponent (m_polySynthEditor.get(), false);
    }
    else if (entry.moduleType == "DRUMS")
    {
        if (!m_drumEditor)
            m_drumEditor = std::make_unique<DrumEditorComponent>();

        m_drumEditor->setState (onRequestDrumState != nullptr ? onRequestDrumState (entry.slotIndex) : nullptr);
        m_drumEditor->onStateChanged = [this, slot = entry.slotIndex]
        {
            if (onDrumStateChanged != nullptr)
                if (auto* state = onRequestDrumState != nullptr ? onRequestDrumState (slot) : nullptr)
                    onDrumStateChanged (slot, *state);
        };
        m_drumEditor->onPreviewVoice = [this, slot = entry.slotIndex] (int voiceIndex, float velocity)
        {
            if (onPreviewDrumVoice != nullptr)
                onPreviewDrumVoice (slot, voiceIndex, velocity);
        };
        m_editorViewport.setViewedComponent (m_drumEditor.get(), false);
    }
    // REEL, OUTPUT, unknown: leave viewport empty — module picks its own Zone A panel

    wirePresetBar (entry);
}

void InstrumentPanel::unmountEditor()
{
    m_editorViewport.setViewedComponent (nullptr, false);

    if (m_polySynthEditor)
        m_polySynthEditor->setProcessor (nullptr);

    if (m_drumEditor)
    {
        m_drumEditor->setState (nullptr);
        m_drumEditor->onStateChanged = nullptr;
        m_drumEditor->onPreviewVoice = nullptr;
    }

    m_presetBar.onRequestState = nullptr;
    m_presetBar.onLoadPreset   = nullptr;
    m_presetBar.setVisible (false);
}

//==============================================================================
// Entry helper
//==============================================================================

const InstrumentPanel::SlotEntry* InstrumentPanel::getActiveEntry() const noexcept
{
    for (const auto& s : m_slots)
        if (s.slotIndex == m_activeSlot)
            return &s;
    return nullptr;
}

//==============================================================================
// Preset bar wiring
//==============================================================================

void InstrumentPanel::wirePresetBar (const SlotEntry& entry)
{
    const int slot = entry.slotIndex;

    if (entry.moduleType == "SYNTH")
    {
        m_presetBar.setModuleType (PresetManager::ModuleType::Synth);

        m_presetBar.onRequestState = [this, slot]() -> juce::var
        {
            return PresetManager::captureSynthPreset (
                processorRef.getAudioGraph().getPolySynth (slot),
                m_presetBar.getCurrentName());
        };

        m_presetBar.onLoadPreset = [this, slot] (const juce::var& data)
        {
            PresetManager::applySynthPreset (
                data,
                processorRef.getAudioGraph().getPolySynth (slot));
        };

        m_presetBar.setVisible (true);
    }
    else if (entry.moduleType == "DRUMS")
    {
        m_presetBar.setModuleType (PresetManager::ModuleType::Drum);

        m_presetBar.onRequestState = [this, slot]() -> juce::var
        {
            if (auto* state = onRequestDrumState != nullptr ? onRequestDrumState (slot) : nullptr)
                return PresetManager::captureDrumPreset (*state, m_presetBar.getCurrentName());
            return {};
        };

        m_presetBar.onLoadPreset = [this, slot] (const juce::var& data)
        {
            if (auto* state = onRequestDrumState != nullptr ? onRequestDrumState (slot) : nullptr)
            {
                PresetManager::applyDrumPreset (data, *state);
                if (m_drumEditor != nullptr)
                    m_drumEditor->setState (state);
                if (onDrumStateChanged != nullptr)
                    onDrumStateChanged (slot, *state);
            }
        };

        m_presetBar.setVisible (true);
    }
    else
    {
        m_presetBar.onRequestState = nullptr;
        m_presetBar.onLoadPreset   = nullptr;
        m_presetBar.setVisible (false);
    }
}

//==============================================================================
// Paint
//==============================================================================

void InstrumentPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);

    if (m_state == State::browser)
        paintBrowser (g);
    else
        paintEditorHeader (g);
}

void InstrumentPanel::paintBrowser (juce::Graphics& g) const
{
    const int w = getWidth();

    // Header
    g.setColour (Theme::Colour::surface1);
    g.fillRect  (0, 0, w, kBrowserHeaderH);
    g.setColour (Theme::Zone::a.withAlpha (0.85f));
    g.fillRect (0, 0, 3, kBrowserHeaderH);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect  (0, kBrowserHeaderH - 1, w, 1);
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Zone::a);
    g.drawText  ("INSTRUMENT",
                 juce::Rectangle<int> (0, 0, w, kBrowserHeaderH),
                 juce::Justification::centred, false);

    if (m_slots.isEmpty())
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("No modules loaded.\nAdd modules in Zone B.",
                     getLocalBounds().withTrimmedTop (kBrowserHeaderH).reduced (8),
                     juce::Justification::centredTop, true);
        return;
    }

    // Slot rows
    juce::String lastGroup;

    for (const auto& entry : m_slots)
    {
        // Group header
        if (entry.groupName != lastGroup)
        {
            lastGroup = entry.groupName;

            int groupY = kBrowserHeaderH;
            for (const auto& pair : m_rowYCache)
                if (pair.first == entry.slotIndex)
                    groupY = pair.second - kGroupHeaderH;

            const juce::Rectangle<int> grpR (0, groupY, w, kGroupHeaderH);
            g.setColour (entry.groupColor.withAlpha (0.12f));
            g.fillRect  (grpR);
            g.setColour (entry.groupColor);
            g.fillRect  (0, groupY, 2, kGroupHeaderH);
            g.setFont   (Theme::Font::micro());
            g.setColour (entry.groupColor);
            g.drawText  (entry.groupName,
                         juce::Rectangle<int> (6, groupY, w - 54, kGroupHeaderH),
                         juce::Justification::centredLeft, false);

            int groupSlotCount = 0;
            for (const auto& s : m_slots)
                if (s.groupName == entry.groupName)
                    ++groupSlotCount;
            g.setColour (Theme::Colour::inkGhost);
            g.drawText  (juce::String (groupSlotCount) + " slots",
                         juce::Rectangle<int> (w - 48, groupY, 44, kGroupHeaderH),
                         juce::Justification::centredRight, false);
        }

        int rowY = -1;
        for (const auto& pair : m_rowYCache)
            if (pair.first == entry.slotIndex)
                rowY = pair.second;

        if (rowY < 0) continue;

        paintBrowserSlot (g, entry,
                          juce::Rectangle<int> (0, rowY, w, kSlotRowH),
                          entry.slotIndex == m_hoveredSlot);
    }
}

void InstrumentPanel::paintBrowserSlot (juce::Graphics& g,
                                         const SlotEntry& entry,
                                         juce::Rectangle<int> r,
                                         bool hovered) const
{
    const int w = r.getWidth();

    g.setColour (hovered ? Theme::Colour::surface3 : Theme::Colour::surface2);
    g.fillRect  (r);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.3f));
    g.fillRect  (r.getX(), r.getBottom() - 1, w, 1);

    if (entry.moduleType.isEmpty())
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.4f));
        g.drawText  ("+ EMPTY",
                     r.withTrimmedLeft (kPad + 8),
                     juce::Justification::centredLeft, false);
        return;
    }

    // Color dot
    g.setColour (entry.groupColor);
    g.fillEllipse (static_cast<float> (r.getX() + kPad),
                   static_cast<float> (r.getCentreY() - 3),
                   6.0f, 6.0f);

    // Slot number
    const juce::String numStr = juce::String (entry.slotIndex + 1).paddedLeft ('0', 2);
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  (numStr,
                 juce::Rectangle<int> (r.getX() + kPad + 10, r.getY(), 18, r.getHeight()),
                 juce::Justification::centredLeft, false);

    // Module name
    g.setFont   (Theme::Font::label());
    g.setColour (Theme::Colour::inkLight);
    g.drawText  (entry.displayName,
                 juce::Rectangle<int> (r.getX() + kPad + 30, r.getY(),
                                       w - kPad - 80, r.getHeight()),
                 juce::Justification::centredLeft, true);

    // Type badge
    const juce::String badge    = typeShort (entry.moduleType);
    const juce::Colour badgeCol = typeColor (entry.moduleType);
    const int badgeW = 36, badgeH = 12;
    const int badgeX = r.getX() + w - badgeW - kPad;
    const int badgeY = r.getCentreY() - badgeH / 2;

    g.setColour (badgeCol.withAlpha (0.15f));
    g.fillRoundedRectangle (juce::Rectangle<int> (badgeX, badgeY, badgeW, badgeH).toFloat(), 2.5f);
    g.setColour (badgeCol.withAlpha (0.5f));
    g.drawRoundedRectangle (juce::Rectangle<int> (badgeX, badgeY, badgeW, badgeH).toFloat(), 2.5f, 0.5f);
    g.setFont   (Theme::Font::micro());
    g.setColour (badgeCol);
    g.drawText  (badge,
                 juce::Rectangle<int> (badgeX, badgeY, badgeW, badgeH),
                 juce::Justification::centred, false);
}

//==============================================================================
// Editor header (back button, slot name, type badge)
//==============================================================================

void InstrumentPanel::paintEditorHeader (juce::Graphics& g) const
{
    const int w     = getWidth();
    const auto* entry = getActiveEntry();

    g.setColour (Theme::Colour::surface1);
    g.fillRect  (0, 0, w, kEditorHeaderH);
    g.setColour (Theme::Zone::a.withAlpha (0.85f));
    g.fillRect (0, 0, 3, kEditorHeaderH);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect  (0, kEditorHeaderH - 1, w, 1);

    const auto backR = backBtnRect();
    g.setFont   (Theme::Font::label());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  ("\xe2\x86\x90", backR, juce::Justification::centred, false);  // ←

    if (entry != nullptr)
    {
        g.setFont   (Theme::Font::label());
        g.setColour (Theme::Colour::inkLight);
        g.drawText  (entry->displayName,
                     juce::Rectangle<int> (backR.getRight() + 4, 0,
                                           w - backR.getRight() - kPad - 40, kEditorHeaderH),
                     juce::Justification::centredLeft, true);

        const juce::String badge    = typeShort (entry->moduleType);
        const juce::Colour badgeCol = typeColor (entry->moduleType);
        const int badgeW = 36, badgeH = 12;
        const int badgeX = w - badgeW - kPad;
        const int badgeY = (kEditorHeaderH - badgeH) / 2;

        g.setColour (badgeCol.withAlpha (0.15f));
        g.fillRoundedRectangle (juce::Rectangle<int> (badgeX, badgeY, badgeW, badgeH).toFloat(), 2.5f);
        g.setFont   (Theme::Font::micro());
        g.setColour (badgeCol);
        g.drawText  (badge,
                     juce::Rectangle<int> (badgeX, badgeY, badgeW, badgeH),
                     juce::Justification::centred, false);

        // Stub message for types without a real editor
        if (entry->moduleType != "SYNTH" && entry->moduleType != "DRUMS")
        {
            g.setFont   (Theme::Font::micro());
            g.setColour (Theme::Colour::inkGhost);
            g.drawText  (entry->moduleType + " editor coming soon",
                         getLocalBounds().withTrimmedTop (kEditorHeaderH).reduced (12),
                         juce::Justification::centredTop, true);
        }
    }
    else
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("No module",
                     getLocalBounds().withTrimmedTop (kEditorHeaderH),
                     juce::Justification::centred, false);
    }
}

juce::Rectangle<int> InstrumentPanel::backBtnRect() const noexcept
{
    return { kPad, (kEditorHeaderH - 20) / 2, 20, 20 };
}

//==============================================================================
// Resize
//==============================================================================

void InstrumentPanel::resized()
{
    if (m_state == State::editor)
    {
        const bool hasPresetBar = m_presetBar.isVisible();
        const int  topOfContent = kEditorHeaderH + (hasPresetBar ? kPresetBarH : 0);

        if (hasPresetBar)
            m_presetBar.setBounds (0, kEditorHeaderH, getWidth(), kPresetBarH);

        const auto editorArea = getLocalBounds().withTrimmedTop (topOfContent);
        m_editorViewport.setBounds (editorArea);

        if (auto* c = m_editorViewport.getViewedComponent())
        {
            int prefH = editorArea.getHeight();
            if (c == static_cast<juce::Component*> (m_polySynthEditor.get()))
                prefH = PolySynthEditorComponent::kRequiredHeight;
            else if (c == static_cast<juce::Component*> (m_drumEditor.get()))
                prefH = DrumEditorComponent::kRequiredHeight;

            c->setSize (editorArea.getWidth(),
                        juce::jmax (editorArea.getHeight(), prefH));
        }
    }
    else
    {
        m_presetBar.setBounds (0, 0, 0, 0);
        m_editorViewport.setBounds (0, 0, 0, 0);
    }

    repaint();
}

//==============================================================================
// Mouse
//==============================================================================

void InstrumentPanel::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    if (m_state == State::browser)
    {
        const int hit = slotAtBrowserY (pos.y);
        if (hit >= 0)
        {
            for (const auto& entry : m_slots)
            {
                if (entry.slotIndex == hit && entry.moduleType.isNotEmpty())
                {
                    if (onSlotSelected)
                    {
                        // The callback (ZoneAComponent → PluginEditor) drives full slot
                        // selection and will call showSlot() for non-REEL types.
                        // For REEL, the callback opens the reel_N panel instead —
                        // InstrumentPanel stays in browser mode. Do not call showSlot() here.
                        onSlotSelected (hit, entry.moduleType);
                    }
                    else
                    {
                        // Standalone / no callback: navigate locally
                        showSlot (hit);
                    }
                    return;
                }
            }
        }
    }
    else
    {
        if (backBtnRect().contains (pos))
            showBrowser();
    }
}

void InstrumentPanel::mouseMove (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();
    const int newHover = (m_state == State::browser) ? slotAtBrowserY (pos.y) : -1;

    if (newHover != m_hoveredSlot)
    {
        m_hoveredSlot = newHover;
        repaint();
    }
}

void InstrumentPanel::mouseExit (const juce::MouseEvent&)
{
    if (m_hoveredSlot != -1)
    {
        m_hoveredSlot = -1;
        repaint();
    }
}

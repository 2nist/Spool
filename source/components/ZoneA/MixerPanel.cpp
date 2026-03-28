#include "MixerPanel.h"
#include "../../PluginProcessor.h"

//==============================================================================
MixerPanel::MixerPanel (PluginProcessor& proc)
    : m_proc (proc)
{
    startTimerHz (30);
}

MixerPanel::~MixerPanel()
{
    stopTimer();
}

//==============================================================================
// Data
//==============================================================================

void MixerPanel::setSlots (const juce::Array<SlotInfo>& slots)
{
    m_slots = slots;
    rebuildActiveIndices();
    repaint();
}

void MixerPanel::rebuildActiveIndices()
{
    m_activeIndices.clear();
    for (int i = 0; i < m_slots.size(); ++i)
        if (!m_slots[i].isEmpty)
            m_activeIndices.add (i);
}

//==============================================================================
// Index helpers
//==============================================================================

int MixerPanel::activeSlotCount() const noexcept
{
    return m_activeIndices.size();
}

int MixerPanel::renderIndexAtY (int y) const noexcept
{
    if (y < kHeaderH) return -1;
    const int ri = (y - kHeaderH) / kStripH;
    if (ri < 0 || ri >= activeSlotCount()) return -1;
    return ri;
}

const MixerPanel::SlotInfo* MixerPanel::slotAtRI (int ri) const noexcept
{
    if (ri < 0 || ri >= m_activeIndices.size()) return nullptr;
    return m_slots.begin() + m_activeIndices[ri];
}

MixerPanel::SlotInfo* MixerPanel::slotAtRI (int ri) noexcept
{
    if (ri < 0 || ri >= m_activeIndices.size()) return nullptr;
    return m_slots.begin() + m_activeIndices[ri];
}

//==============================================================================
// Rect helpers
//==============================================================================

int MixerPanel::stripTopY (int ri) const noexcept
{
    return kHeaderH + ri * kStripH;
}

juce::Rectangle<int> MixerPanel::stripRect (int ri) const noexcept
{
    return { 0, stripTopY (ri), getWidth(), kStripH };
}

juce::Rectangle<int> MixerPanel::muteRect (int ri) const noexcept
{
    const int y = stripTopY (ri);
    return { getWidth() - kPad - kBtnW, y + 5, kBtnW, kBtnH };
}

juce::Rectangle<int> MixerPanel::soloRect (int ri) const noexcept
{
    const int y = stripTopY (ri);
    return { getWidth() - kPad - kBtnW * 2 - 2, y + 5, kBtnW, kBtnH };
}

juce::Rectangle<int> MixerPanel::faderBarRect (int ri) const noexcept
{
    const int y    = stripTopY (ri);
    const int barY = y + kStripH - kFaderH - 8;
    const int dbW  = (getWidth() >= kDbThreshold) ? kDbReadoutW : 0;
    const int barW = getWidth() - kPad - kMeterW - 4 - kPad - dbW;
    return { kPad, barY, barW, kFaderH };
}

juce::Rectangle<int> MixerPanel::meterBarRect (int ri) const noexcept
{
    const int y = stripTopY (ri);
    return { getWidth() - kMeterW - 2, y + 4, kMeterW, kStripH - 8 };
}

juce::Rectangle<int> MixerPanel::masterRect() const noexcept
{
    const int y = kHeaderH + activeSlotCount() * kStripH;
    return { 0, y, getWidth(), kMasterH };
}

juce::Rectangle<int> MixerPanel::masterFaderRect() const noexcept
{
    const auto r = masterRect();
    const int barW = getWidth() - kPad * 2;
    return { kPad, r.getY() + 28, barW, kFaderH };
}

juce::Rectangle<int> MixerPanel::masterPanRect() const noexcept
{
    const auto r = masterRect();
    const int barW = getWidth() - kPad * 2;
    return { kPad, r.getY() + 44, barW, 6 };
}

//==============================================================================
// Paint
//==============================================================================

void MixerPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);

    paintHeader (g);

    for (int ri = 0; ri < activeSlotCount(); ++ri)
        if (const auto* s = slotAtRI (ri))
            paintStrip (g, ri, *s);

    paintMaster (g);
}

void MixerPanel::paintHeader (juce::Graphics& g) const
{
    ZoneAStyle::drawHeader (g, { 0, 0, getWidth(), kHeaderH }, "MIXER",
                            ZoneAStyle::accentForTabId ("mixer"));
}

void MixerPanel::paintStrip (juce::Graphics& g, int ri, const SlotInfo& slot) const
{
    const int w       = getWidth();
    const int y       = stripTopY (ri);
    const bool hov    = (ri == m_hoveredRenderIdx);
    const float alpha = slot.muted ? Theme::Alpha::disabled : 1.0f;

    // Background
    ZoneAStyle::drawRowBackground (g, { 0, y, w, kStripH }, hov);

    // Left group-colour stripe (full height)
    g.setColour (slot.groupColor.withAlpha (slot.muted ? 0.3f : 0.85f));
    g.fillRect  (0, y, kStripeW, kStripH);

    // Type colour dot
    const int dotX = kStripeW + kPad;
    const int dotCY = y + 13;
    g.setColour (slot.groupColor.withAlpha (alpha));
    g.fillEllipse (static_cast<float> (dotX),
                   static_cast<float> (dotCY - kDotDiam / 2),
                   static_cast<float> (kDotDiam),
                   static_cast<float> (kDotDiam));

    // Slot name
    const int nameX = dotX + kDotDiam + kPad;
    const int nameW = w - nameX - kPad - kBtnW * 2 - 4;
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkLight.withAlpha (alpha));
    g.drawText  (slot.name,
                 juce::Rectangle<int> (nameX, y + 3, nameW, 18),
                 juce::Justification::centredLeft, true);

    // Solo button [S]
    const auto soloR = soloRect (ri);
    const bool anySolo = (m_activeIndices.size() > 0) && [&]()
    {
        for (int i = 0; i < m_activeIndices.size(); ++i)
            if (m_slots[m_activeIndices[i]].soloed) return true;
        return false;
    }();
    const bool dimmedBySolo = anySolo && !slot.soloed;
    g.setColour (slot.soloed ? ZoneAStyle::accentForTabId ("mixer")
                             : Theme::Colour::surface3);
    g.fillRoundedRectangle (soloR.toFloat(), 2.0f);
    g.setFont   (Theme::Font::micro());
    g.setColour (slot.soloed ? juce::Colour (0xFF111111)
                             : (dimmedBySolo ? Theme::Colour::inkGhost.withAlpha (0.3f)
                                             : Theme::Colour::inkGhost));
    g.drawText  ("S", soloR, juce::Justification::centred, false);

    // Mute button [M]
    const auto muteR = muteRect (ri);
    g.setColour (slot.muted ? Theme::Colour::error : Theme::Colour::surface3);
    g.fillRoundedRectangle (muteR.toFloat(), 2.0f);
    g.setFont   (Theme::Font::micro());
    g.setColour (slot.muted ? juce::Colour (0xFFeeeeee) : Theme::Colour::inkGhost);
    g.drawText  ("M", muteR, juce::Justification::centred, false);

    // Fader bar — background
    const auto faderR = faderBarRect (ri);
    g.setColour (Theme::Colour::surface0);
    g.fillRoundedRectangle (faderR.toFloat(), 2.0f);

    // Fader fill
    const float effectiveLevel = (slot.muted || dimmedBySolo) ? 0.0f : slot.level;
    if (effectiveLevel > 0.0f)
    {
        const int fillW = juce::roundToInt (faderR.getWidth()
                          * juce::jlimit (0.0f, 1.0f, slot.level));
        g.setColour (slot.groupColor.withAlpha (alpha * 0.75f));
        g.fillRoundedRectangle (faderR.withWidth (fillW).toFloat(), 2.0f);
    }

    // Unity-gain tick mark at 100%
    const int tickX = faderR.getX() + faderR.getWidth();
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.6f));
    g.fillRect  (tickX - 1, faderR.getY() - 2, 1, faderR.getHeight() + 4);

    // dB readout — shown to the right of the fader when panel is wide enough
    if (getWidth() >= kDbThreshold)
    {
        juce::String dbStr;
        if (slot.level <= 0.001f)
        {
            dbStr = "-inf";
        }
        else
        {
            const float dB = 20.0f * std::log10 (slot.level);
            dbStr = juce::String (dB, 1);
        }

        const int dbX = tickX + 3;
        const int dbW = kDbReadoutW - 4;
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMid.withAlpha (alpha));
        g.drawText  (dbStr,
                     juce::Rectangle<int> (dbX, faderR.getY() - 2, dbW, faderR.getHeight() + 4),
                     juce::Justification::centredLeft, false);
    }

    // Live RMS meter bar (vertical, right edge)
    const auto meterR = meterBarRect (ri);
    g.setColour (Theme::Colour::surface0);
    g.fillRect  (meterR);

    if (slot.meter > 0.002f && !slot.muted)
    {
        const float norm = juce::jlimit (0.0f, 1.0f, slot.meter * 5.0f);
        const int   mh   = juce::roundToInt (meterR.getHeight() * norm);
        const juce::Colour mCol = norm > 0.9f ? juce::Colour (Theme::Colour::error)
                                : norm > 0.7f ? juce::Colour (0xFFc4822a)
                                              : juce::Colour (Theme::Colour::accent);
        g.setColour (mCol.withAlpha (Theme::Alpha::meterFill));
        g.fillRect  (meterR.getX(),
                     meterR.getBottom() - mh,
                     meterR.getWidth(),
                     mh);
    }
}

void MixerPanel::paintMaster (juce::Graphics& g) const
{
    const auto r  = masterRect();
    const int  w  = getWidth();
    const int  ry = r.getY();

    // Background
    g.setColour (Theme::Colour::surface1.withAlpha (0.8f));
    g.fillRect  (r);

    // Top border
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.6f));
    g.fillRect  (0, ry, w, 1);

    // Label
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkMid);
    g.drawText  ("MASTER",
                 juce::Rectangle<int> (kPad, ry + 6, w - kPad * 2, 16),
                 juce::Justification::centredLeft, false);

    // Master gain fader
    const auto faderR = masterFaderRect();
    g.setColour (Theme::Colour::surface0);
    g.fillRoundedRectangle (faderR.toFloat(), 2.0f);

    const float masterGain = juce::jlimit (0.0f, 1.0f, m_proc.getMasterGain());
    if (masterGain > 0.0f)
    {
        const int fillW = juce::roundToInt (faderR.getWidth() * masterGain);
        g.setColour (Theme::Colour::accent.withAlpha (0.75f));
        g.fillRoundedRectangle (faderR.withWidth (fillW).toFloat(), 2.0f);
    }

    // Unity tick
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.6f));
    g.fillRect  (faderR.getRight() - 1, faderR.getY() - 2, 1, faderR.getHeight() + 4);

    // Master pan bar (center-origin)
    const auto panR  = masterPanRect();
    const int  cx    = panR.getCentreX();
    const float pan  = juce::jlimit (-1.0f, 1.0f, m_proc.getMasterPan());

    g.setColour (Theme::Colour::surface0);
    g.fillRoundedRectangle (panR.toFloat(), 2.0f);

    if (std::abs (pan) > 0.01f)
    {
        const int fillW  = juce::roundToInt ((panR.getWidth() / 2.0f) * std::abs (pan));
        const int fillX  = pan < 0.0f ? cx - fillW : cx;
        g.setColour (Theme::Colour::accentWarm.withAlpha (0.7f));
        g.fillRect  (fillX, panR.getY(), fillW, panR.getHeight());
    }

    // Center mark
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.7f));
    g.fillRect  (cx, panR.getY() - 1, 1, panR.getHeight() + 2);
}

//==============================================================================
// Resize
//==============================================================================

void MixerPanel::resized()
{
    repaint();
}

//==============================================================================
// Mouse
//==============================================================================

void MixerPanel::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // --- Slot strips ---
    const int ri = renderIndexAtY (pos.y);
    if (ri >= 0)
    {
        auto* slot = slotAtRI (ri);
        if (slot == nullptr) return;

        // Mute button
        if (muteRect (ri).contains (pos))
        {
            slot->muted = !slot->muted;
            m_proc.setSlotMuted (slot->slotIndex, slot->muted);
            if (onMuteChanged) onMuteChanged (slot->slotIndex, slot->muted);
            repaint();
            return;
        }

        // Solo button — exclusive unless Shift held
        if (soloRect (ri).contains (pos))
        {
            const bool additive = e.mods.isShiftDown();
            const bool newSolo  = !slot->soloed;

            if (newSolo && !additive)
            {
                // Clear all other solos
                for (int i = 0; i < m_activeIndices.size(); ++i)
                {
                    auto& other = m_slots.getReference (m_activeIndices[i]);
                    if (other.slotIndex != slot->slotIndex && other.soloed)
                    {
                        other.soloed = false;
                        m_proc.setSlotSoloed (other.slotIndex, false);
                        if (onSoloChanged) onSoloChanged (other.slotIndex, false);
                    }
                }
            }

            slot->soloed = newSolo;
            m_proc.setSlotSoloed (slot->slotIndex, newSolo);
            if (onSoloChanged) onSoloChanged (slot->slotIndex, newSolo);
            repaint();
            return;
        }

        // Fader — start drag from anywhere in the bottom half of the strip
        const int stripMidY = stripTopY (ri) + kStripH / 2;
        if (pos.y >= stripMidY)
        {
            const auto faderR = faderBarRect (ri);
            m_draggingSlotFader = true;
            m_dragSlotArrIdx    = m_activeIndices[ri];
            m_dragBarLeft       = static_cast<float> (faderR.getX());
            m_dragBarWidth      = static_cast<float> (faderR.getWidth());

            // Snap to click position immediately
            const float newLevel = juce::jlimit (
                0.0f, 1.0f,
                (static_cast<float> (pos.x) - m_dragBarLeft) / m_dragBarWidth);
            slot->level = newLevel;
            m_proc.setSlotLevel (slot->slotIndex, newLevel);
            if (onLevelChanged) onLevelChanged (slot->slotIndex, newLevel);
            repaint();
            return;
        }

        return;
    }

    // --- Master strip ---
    if (masterRect().contains (pos))
    {
        if (masterFaderRect().contains (pos) ||
            (pos.y >= masterFaderRect().getY() && pos.y <= masterFaderRect().getBottom()))
        {
            m_draggingMasterLvl = true;
            m_masterDragBarLeft = static_cast<float> (masterFaderRect().getX());
            m_masterDragBarW    = static_cast<float> (masterFaderRect().getWidth());

            const float g = juce::jlimit (
                0.0f, 1.0f,
                (static_cast<float> (pos.x) - m_masterDragBarLeft) / m_masterDragBarW);
            m_proc.setMasterGain (g);
            repaint();
            return;
        }

        if (masterPanRect().contains (pos) ||
            (pos.y >= masterPanRect().getY() && pos.y <= masterPanRect().getBottom()))
        {
            m_draggingMasterPan = true;
            const auto panR     = masterPanRect();
            m_masterPanBarCx    = static_cast<float> (panR.getCentreX());
            m_masterPanBarHalfW = static_cast<float> (panR.getWidth()) / 2.0f;

            const float p = juce::jlimit (
                -1.0f, 1.0f,
                (static_cast<float> (pos.x) - m_masterPanBarCx) / m_masterPanBarHalfW);
            m_proc.setMasterPan (p);
            repaint();
        }
    }
}

void MixerPanel::mouseDrag (const juce::MouseEvent& e)
{
    const float x = static_cast<float> (e.getPosition().x);

    if (m_draggingSlotFader && m_dragSlotArrIdx >= 0
        && m_dragSlotArrIdx < m_slots.size())
    {
        auto& slot = m_slots.getReference (m_dragSlotArrIdx);
        const float newLevel = juce::jlimit (
            0.0f, 1.0f, (x - m_dragBarLeft) / m_dragBarWidth);
        slot.level = newLevel;
        m_proc.setSlotLevel (slot.slotIndex, newLevel);
        if (onLevelChanged) onLevelChanged (slot.slotIndex, newLevel);
        repaint();
        return;
    }

    if (m_draggingMasterLvl)
    {
        const float g = juce::jlimit (
            0.0f, 1.0f, (x - m_masterDragBarLeft) / m_masterDragBarW);
        m_proc.setMasterGain (g);
        repaint();
        return;
    }

    if (m_draggingMasterPan)
    {
        const float p = juce::jlimit (
            -1.0f, 1.0f, (x - m_masterPanBarCx) / m_masterPanBarHalfW);
        m_proc.setMasterPan (p);
        repaint();
    }
}

void MixerPanel::mouseUp (const juce::MouseEvent&)
{
    m_draggingSlotFader = false;
    m_draggingMasterLvl = false;
    m_draggingMasterPan = false;
    m_dragSlotArrIdx    = -1;
}

void MixerPanel::mouseMove (const juce::MouseEvent& e)
{
    const int newHover = renderIndexAtY (e.getPosition().y);
    if (newHover != m_hoveredRenderIdx)
    {
        m_hoveredRenderIdx = newHover;
        repaint();
    }
}

void MixerPanel::mouseExit (const juce::MouseEvent&)
{
    if (m_hoveredRenderIdx != -1)
    {
        m_hoveredRenderIdx = -1;
        repaint();
    }
}

//==============================================================================
// Timer — poll processor for live metering and state sync
//==============================================================================

void MixerPanel::timerCallback()
{
    bool dirty = false;

    for (int i = 0; i < m_activeIndices.size(); ++i)
    {
        auto& slot = m_slots.getReference (m_activeIndices[i]);

        const float newMeter  = m_proc.getSlotMeter  (slot.slotIndex);
        const bool  newMuted  = m_proc.isSlotMuted   (slot.slotIndex);
        const bool  newSoloed = m_proc.isSlotSoloed  (slot.slotIndex);

        if (newMeter  != slot.meter  ||
            newMuted  != slot.muted  ||
            newSoloed != slot.soloed)
        {
            slot.meter  = newMeter;
            slot.muted  = newMuted;
            slot.soloed = newSoloed;
            dirty = true;
        }
    }

    if (dirty)
        repaint();
}

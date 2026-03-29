#include "MenuBarComponent.h"

namespace
{
void drawCompactShellSlider (juce::Graphics& g,
                             juce::Rectangle<float> bounds,
                             const juce::String& label,
                             float normalized,
                             bool bipolar,
                             bool active,
                             juce::Colour accent)
{
    g.setColour (Theme::Colour::surface2);
    g.fillRoundedRectangle (bounds, Theme::Radius::chip);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.65f));
    g.drawRoundedRectangle (bounds, Theme::Radius::chip, Theme::Stroke::subtle);

    auto track = bounds.reduced (18.0f, 3.0f);
    g.setColour (Theme::Colour::surface0);
    g.fillRoundedRectangle (track, Theme::Radius::chip);

    if (bipolar)
    {
        const float centerX = track.getCentreX();
        const float thumbX = track.getX() + juce::jlimit (0.0f, 1.0f, normalized) * track.getWidth();
        const float fillX = juce::jmin (centerX, thumbX);
        const float fillW = std::abs (thumbX - centerX);
        if (fillW > 0.0f)
        {
            g.setColour (accent.withAlpha (0.72f));
            g.fillRoundedRectangle ({ fillX, track.getY(), fillW, track.getHeight() }, Theme::Radius::chip);
        }

        g.setColour (accent.withAlpha (0.35f));
        g.fillRect (centerX, track.getY() - 1.0f, 1.0f, track.getHeight() + 2.0f);
    }
    else
    {
        const float fillW = juce::jlimit (0.0f, track.getWidth(), normalized * track.getWidth());
        if (fillW > 0.0f)
        {
            g.setColour (accent.withAlpha (0.72f));
            g.fillRoundedRectangle ({ track.getX(), track.getY(), fillW, track.getHeight() }, Theme::Radius::chip);
        }
    }

    const float thumbX = track.getX() + juce::jlimit (0.0f, 1.0f, normalized) * track.getWidth();
    g.setColour (active ? accent.brighter (0.18f) : accent);
    g.fillEllipse (thumbX - 3.5f, track.getCentreY() - 3.5f, 7.0f, 7.0f);

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText (label, bounds.removeFromLeft (16.0f), juce::Justification::centred, false);
}
}

MenuBarComponent::MenuBarComponent()
{
    m_logo = juce::ImageCache::getFromMemory (BinaryData::spoollogo_png, BinaryData::spoollogo_pngSize);

    m_items.add ({ "file", "FILE", {}, true });
    m_items.add ({ "edit", "EDIT", {}, true });
    m_items.add ({ "view", "VIEW", {}, true });
    m_items.add ({ "settings", "SETTINGS", {}, true });
}

void MenuBarComponent::setLevel (float value) noexcept
{
    m_level = juce::jlimit (0.0f, 1.0f, value);
    repaint();
}

void MenuBarComponent::setPan (float value) noexcept
{
    m_pan = juce::jlimit (-1.0f, 1.0f, value);
    repaint();
}

void MenuBarComponent::setBuildStamp (juce::String stamp)
{
    stamp = stamp.trim();
    if (m_buildStamp == stamp)
        return;

    m_buildStamp = std::move (stamp);
    rebuildItemLayout();
    repaint();
}

void MenuBarComponent::resized()
{
    rebuildItemLayout();
}

void MenuBarComponent::paint (juce::Graphics& g)
{
    const float w  = static_cast<float> (getWidth());
    const float h  = static_cast<float> (getHeight());
    const float cy = h / 2.0f;

    // ── Background ────────────────────────────────────────────────────────────
    Theme::Helper::drawPanel (g, getLocalBounds(), Theme::Colour::surface0, Theme::Colour::transparent,
                              Theme::Radius::none, Theme::Stroke::subtle);

    // ── Bottom border ─────────────────────────────────────────────────────────
    g.setColour (Theme::Colour::surfaceEdge);
    g.fillRect (0.0f, h - Theme::Stroke::subtle, w, Theme::Stroke::subtle);

    rebuildItemLayout();

    // ── Branding mark ─────────────────────────────────────────────────────────
    if (m_logo.isValid())
    {
        const float targetH = juce::jlimit (12.0f, 22.0f, h - 8.0f);
        const float logoW = targetH * ((float) m_logo.getWidth() / juce::jmax (1.0f, (float) m_logo.getHeight()));
        const float logoX = Theme::Space::sm;
        const float logoY = (h - targetH) * 0.5f;
        m_logoBounds = { logoX, logoY, logoW, targetH };

        g.setOpacity (0.96f);
        g.drawImage (m_logo, m_logoBounds);
        g.setOpacity (1.0f);
    }
    else
    {
        m_logoBounds = {};
    }

    // ── Left side: menu items ─────────────────────────────────────────────────
    const auto labelFont = Theme::Font::label();
    g.setFont (labelFont);
    for (const auto& item : m_items)
    {
        const bool hovered = item.id == m_hoveredItemId;
        const bool actionable = item.interactive;

        if (hovered)
        {
            Theme::Helper::drawInfoChip (g,
                                         item.bounds.expanded (6.0f, -4.0f),
                                         {},
                                         actionable ? juce::Colour (Theme::Colour::accent)
                                                    : juce::Colour (Theme::Colour::surfaceEdge),
                                         Theme::Helper::VisualState::hover);
        }

        g.setColour (actionable ? Theme::Helper::textForState (Theme::Helper::VisualState::idle)
                                : juce::Colour (Theme::Colour::inkMid));
        if (hovered && actionable)
            g.setColour (Theme::Colour::accent);

        g.drawText (item.label,
                    item.bounds,
                    juce::Justification::centredLeft,
                    false);
    }

    if (m_buildStamp.isNotEmpty() && ! m_buildStampBounds.isEmpty())
    {
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.88f));
        g.drawText (m_buildStamp, m_buildStampBounds, juce::Justification::centredLeft, true);
    }

    // ── Right side: status indicators (right → left) ──────────────────────────
    {
        const auto headingFont = Theme::Font::heading();
        const auto microFont   = Theme::Font::micro();
        const float shellSliderW = 48.0f;
        const float shellSliderGap = 5.0f;

        // Pre-compute widths
        const float transportW  = juce::GlyphArrangement::getStringWidth (headingFont, "1 . 1 . 00");
        const float bpmValW     = juce::GlyphArrangement::getStringWidth (headingFont, "120");
        const float bpmLblW     = juce::GlyphArrangement::getStringWidth (microFont, "BPM");
        const float bpmBlockW   = juce::jmax (bpmValW, bpmLblW);
        const float linkTextW   = juce::GlyphArrangement::getStringWidth (microFont, "LINK");
        const float midiTextW   = juce::GlyphArrangement::getStringWidth (microFont, "MIDI");
        const float pipD        = 10.0f; // larger MIDI pips for visibility
        const float pipGap      = 6.0f;
        const float pipsW       = 4.0f * pipD + 3.0f * pipGap;   // 4 pips
        const float midiBlockW  = pipsW + 6.0f + midiTextW;
        const float linkBlockW  = 6.0f + 4.0f + linkTextW;      // circle + gap + label

        // Cursor starts at right inset
        float rx = w - Theme::Space::lg;

        // 1. Transport position ------------------------------------------------
        g.setFont (headingFont);
        g.setColour (Theme::Colour::inkLight);
        g.drawText ("1 . 1 . 00",
            juce::Rectangle<float> (rx - transportW, 0.0f, transportW, h),
            juce::Justification::centredLeft,
            false);
        rx -= transportW + 4.0f;   // 4px separator

        // 2. BPM value + label ------------------------------------------------
        {
            const float valH      = Theme::Font::sizeHeading;
            const float lblH      = Theme::Font::sizeMicro;
            const float gap       = 2.0f;
            const float totalTxtH = valH + gap + lblH;
            const float startY    = (h - totalTxtH) / 2.0f;
            const float bpmLeft   = rx - bpmBlockW;

            g.setFont (headingFont);
            g.setColour (Theme::Colour::accent);
            g.drawText ("120",
                juce::Rectangle<float> (bpmLeft, startY, bpmBlockW, valH),
                juce::Justification::centred,
                false);

            g.setFont (microFont);
            g.setColour (Theme::Colour::inkGhost);
            g.drawText ("BPM",
                juce::Rectangle<float> (bpmLeft, startY + valH + gap, bpmBlockW, lblH),
                juce::Justification::centred,
                false);
        }
        rx -= bpmBlockW + 8.0f;   // 8px separator

        // 3. Compact master controls ------------------------------------------
        m_panRect = { rx - shellSliderW, 4.0f, shellSliderW, h - 8.0f };
        drawCompactShellSlider (g, m_panRect, "PAN", (m_pan + 1.0f) * 0.5f,
                                true, m_dragTarget == DragTarget::pan, Theme::Colour::accentWarm);
        rx -= shellSliderW + shellSliderGap;

        m_levelRect = { rx - shellSliderW, 4.0f, shellSliderW, h - 8.0f };
        drawCompactShellSlider (g, m_levelRect, "LVL", m_level,
                                false, m_dragTarget == DragTarget::level, Theme::Colour::accent);
        rx -= shellSliderW + 8.0f;

        // 4. Link indicator (circle + "LINK" label) ----------------------------
        {
            const float linkLeft = rx - linkBlockW;

            Theme::Helper::drawStateDot (g,
                                         { linkLeft, cy - 3.0f, 6.0f, 6.0f },
                                         Theme::Helper::VisualState::armed,
                                         Theme::Zone::c);

            g.setFont (microFont);
            g.setColour (Theme::Colour::inkGhost);
            g.drawText ("LINK",
                juce::Rectangle<float> (linkLeft + 6.0f + 4.0f,
                                        cy - Theme::Font::sizeMicro / 2.0f,
                                        linkTextW,
                                        Theme::Font::sizeMicro),
                juce::Justification::centredLeft,
                false);
        }
        rx -= linkBlockW + 8.0f;   // 8px separator

        // 5. MIDI activity pips + "MIDI" label ---------------------------------
        {
            const float midiLeft = rx - midiBlockW;

            const juce::Colour pipColours[4] {
                Theme::Signal::audio,
                Theme::Signal::midi,
                Theme::Signal::cv,
                Theme::Signal::gate
            };

                for (int i = 0; i < 4; ++i)
                {
                    const float pipX = midiLeft + static_cast<float> (i) * (pipD + pipGap);
                    g.setColour (pipColours[i].withAlpha (Theme::Alpha::disabled));
                    g.fillEllipse (pipX, cy - pipD/2.0f, pipD, pipD);
                }

            g.setFont (Theme::Font::value());
            g.setColour (Theme::Colour::inkGhost);
            g.drawText ("MIDI",
                juce::Rectangle<float> (midiLeft + pipsW + 6.0f,
                                        cy - Theme::Font::sizeValue / 2.0f,
                                        midiTextW,
                                        Theme::Font::sizeValue),
                juce::Justification::centredLeft,
                false);
        }
        rx -= midiBlockW + 8.0f;   // 8px gap before alert dot

        // 6. Alert dot (inactive state) ----------------------------------------
        {
            constexpr float alertD = 5.0f;
            rx -= alertD;
            Theme::Helper::drawStateDot (g,
                                         { rx, cy - alertD / 2.0f, alertD, alertD },
                                         Theme::Helper::VisualState::disabled,
                                         Theme::Colour::inactive);
        }
    }
}

void MenuBarComponent::mouseMove (const juce::MouseEvent& e)
{
    if (const auto* hit = itemAt (e.position))
        m_hoveredItemId = hit->id;
    else
        m_hoveredItemId.clear();

    repaint();
}

void MenuBarComponent::mouseExit (const juce::MouseEvent&)
{
    if (m_hoveredItemId.isNotEmpty())
    {
        m_hoveredItemId.clear();
        repaint();
    }
}

void MenuBarComponent::mouseDown (const juce::MouseEvent& e)
{
    if (m_levelRect.contains (e.position))
    {
        m_dragTarget = DragTarget::level;
        setLevel ((e.position.x - m_levelRect.getX()) / juce::jmax (1.0f, m_levelRect.getWidth()));
        if (onLevelChanged)
            onLevelChanged (m_level);
        return;
    }

    if (m_panRect.contains (e.position))
    {
        m_dragTarget = DragTarget::pan;
        setPan (((e.position.x - m_panRect.getX()) / juce::jmax (1.0f, m_panRect.getWidth())) * 2.0f - 1.0f);
        if (onPanChanged)
            onPanChanged (m_pan);
        return;
    }

    if (auto* hit = itemAt (e.position); hit != nullptr && hit->interactive)
    {
        if (onMenuClicked)
            onMenuClicked (hit->id, hit->bounds.getSmallestIntegerContainer().translated (getX(), getY()));
    }
}

void MenuBarComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (m_dragTarget == DragTarget::level)
    {
        setLevel ((e.position.x - m_levelRect.getX()) / juce::jmax (1.0f, m_levelRect.getWidth()));
        if (onLevelChanged)
            onLevelChanged (m_level);
        return;
    }

    if (m_dragTarget == DragTarget::pan)
    {
        setPan (((e.position.x - m_panRect.getX()) / juce::jmax (1.0f, m_panRect.getWidth())) * 2.0f - 1.0f);
        if (onPanChanged)
            onPanChanged (m_pan);
    }
}

void MenuBarComponent::mouseUp (const juce::MouseEvent&)
{
    if (m_dragTarget != DragTarget::none)
    {
        m_dragTarget = DragTarget::none;
        repaint();
    }
}

juce::Rectangle<int> MenuBarComponent::getAnchorBoundsForItem (const juce::String& itemId) const noexcept
{
    for (const auto& item : m_items)
        if (item.id == itemId)
            return item.bounds.getSmallestIntegerContainer().translated (getX(), getY());

    return getBounds().removeFromRight (88).reduced (4, 2);
}

void MenuBarComponent::rebuildItemLayout()
{
    const auto labelFont = Theme::Font::label();
    float lx = Theme::Space::lg;
    const auto h = static_cast<float> (getHeight());

    if (m_logo.isValid())
    {
        const float targetH = juce::jlimit (12.0f, 22.0f, h - 8.0f);
        const float logoW = targetH * ((float) m_logo.getWidth() / juce::jmax (1.0f, (float) m_logo.getHeight()));
        lx = Theme::Space::sm + logoW + Theme::Space::sm;
    }

    for (auto& item : m_items)
    {
        const float itemW = juce::GlyphArrangement::getStringWidth (labelFont, item.label);
        item.bounds = { lx, 0.0f, itemW, h };
        lx += itemW + Theme::Space::lg;
    }

    const float rightReserve = 340.0f;
    const float left = lx + 4.0f;
    const float width = juce::jmax (0.0f, (float) getWidth() - rightReserve - left);
    m_buildStampBounds = { left, 0.0f, width, h };
}

MenuBarComponent::MenuItem* MenuBarComponent::itemAt (juce::Point<float> pos) noexcept
{
    for (auto& item : m_items)
        if (item.bounds.contains (pos))
            return &item;
    return nullptr;
}

const MenuBarComponent::MenuItem* MenuBarComponent::itemAt (juce::Point<float> pos) const noexcept
{
    for (const auto& item : m_items)
        if (item.bounds.contains (pos))
            return &item;
    return nullptr;
}

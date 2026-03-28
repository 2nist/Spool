#pragma once

#include "../../Theme.h"

namespace ZoneAStyle
{
inline constexpr int kCompactHeaderH = 24;
inline constexpr int kCompactGroupHeaderH = 18;
inline constexpr int kCompactRowH = 32;
inline constexpr int kCompactPad = 6;
inline constexpr int kBadgeH = 12;
inline constexpr int kAccentStripeH = 3;

inline int compactHeaderHeight() noexcept
{
    return juce::roundToInt (ThemeManager::get().theme().zoneAHeaderHeight);
}

inline int compactGroupHeaderHeight() noexcept
{
    return juce::roundToInt (ThemeManager::get().theme().zoneAGroupHeaderHeight);
}

inline int compactRowHeight() noexcept
{
    return juce::roundToInt (ThemeManager::get().theme().zoneARowHeight);
}

inline int badgeHeight() noexcept
{
    return juce::roundToInt (ThemeManager::get().theme().zoneABadgeHeight);
}

inline int compactPad() noexcept
{
    return juce::jlimit (4, 8, juce::roundToInt (ThemeManager::get().theme().zoneACompactGap) + 1);
}

inline float cardRadius() noexcept
{
    return ThemeManager::get().theme().zoneACardRadius;
}

struct HeaderOptions
{
    juce::Justification justification { juce::Justification::centred };
    bool showAccentStripe { true };
};

inline juce::String shortBadgeForModuleType (const juce::String& moduleType)
{
    if (moduleType == "SYNTH")  return "POLY";
    if (moduleType == "DRUMS")  return "DRUM";
    if (moduleType == "REEL")   return "REEL";
    if (moduleType == "OUTPUT") return "OUT";
    if (moduleType == "SAMPLER") return "SMPL";
    if (moduleType == "VST3")    return "VST3";
    if (moduleType.isNotEmpty()) return moduleType.substring (0, 4).toUpperCase();
    return {};
}

inline juce::Colour accentForTabId (const juce::String& tabId)
{
    if (tabId == "song" || tabId == "structure" || tabId == "lyrics"
        || tabId == "tracks" || tabId == "automate" || tabId == "instrument"
        || tabId == "theme" || tabId == "feed" || tabId == "transport"
        || tabId == "patch")
        return juce::Colour (Theme::Zone::a);
    if (tabId == "mixer" || tabId == "macro") return juce::Colour (Theme::Zone::b);
    if (tabId == "routing") return juce::Colour (Theme::Zone::c);
    if (tabId == "tape" || tabId.startsWith ("reel_")) return juce::Colour (Theme::Zone::d);
    return juce::Colour (Theme::Zone::a);
}

inline juce::Colour accentForModuleType (const juce::String& moduleType)
{
    if (moduleType == "SYNTH")   return juce::Colour (Theme::Zone::a);
    if (moduleType == "DRUMS")   return juce::Colour (Theme::Zone::b);
    if (moduleType == "ROUTING") return juce::Colour (Theme::Zone::c);
    if (moduleType == "REEL" || moduleType == "TAPE") return juce::Colour (Theme::Zone::d);
    if (moduleType == "SAMPLER") return juce::Colour (Theme::Colour::accentWarm);
    if (moduleType == "VST3")    return juce::Colour (Theme::Colour::accent);
    if (moduleType == "OUTPUT")  return juce::Colour (Theme::Colour::error);
    return juce::Colour (Theme::Colour::inkGhost);
}

inline juce::Colour accentForGroupName (const juce::String& groupName)
{
    const auto cleaned = groupName.trim().toUpperCase();
    if (cleaned.isEmpty() || cleaned == "DEFAULT")
        return juce::Colour (Theme::Colour::inkMid);

    if (cleaned.contains ("DRUM"))   return juce::Colour (Theme::Zone::b);
    if (cleaned.contains ("SYNTH"))  return juce::Colour (Theme::Zone::a);
    if (cleaned.contains ("ROUTE"))  return juce::Colour (Theme::Zone::c);
    if (cleaned.contains ("TAPE") || cleaned.contains ("REEL"))
        return juce::Colour (Theme::Zone::d);

    const juce::Colour palette[] =
    {
        juce::Colour (Theme::Zone::a),
        juce::Colour (Theme::Zone::b),
        juce::Colour (Theme::Zone::c),
        juce::Colour (Theme::Zone::d),
        juce::Colour (Theme::Colour::accentWarm),
        juce::Colour (Theme::Colour::accent),
        juce::Colour (Theme::Colour::inkMid),
    };

    uint32_t hash = 2166136261u;
    if (const auto* utf8 = cleaned.toRawUTF8())
    {
        for (auto* p = utf8; *p != 0; ++p)
        {
            hash ^= static_cast<uint8_t> (*p);
            hash *= 16777619u;
        }
    }

    return palette[hash % static_cast<uint32_t> (sizeof (palette) / sizeof (palette[0]))];
}

inline int badgeWidthForText (const juce::String& text) noexcept
{
    return juce::jmax (28, 12 + static_cast<int> (text.length()) * 6);
}

inline void drawHeader (juce::Graphics& g,
                        juce::Rectangle<int> bounds,
                        const juce::String& title,
                        juce::Colour accent,
                        HeaderOptions options = {})
{
    const auto& theme = ThemeManager::get().theme();

    g.setColour (theme.headerBg);
    g.fillRect (bounds);

    if (options.showAccentStripe)
    {
        g.setColour (accent.withAlpha (0.85f));
        g.fillRect (bounds.removeFromTop (kAccentStripeH));
    }

    g.setColour (theme.surfaceEdge.withAlpha (0.5f));
    g.fillRect (bounds.getX(), bounds.getBottom() - 1, bounds.getWidth(), 1);

    if (title.isNotEmpty())
    {
        g.setFont (Theme::Font::micro());
        g.setColour (accent);
        g.drawText (title, bounds.reduced (compactPad(), 0), options.justification, false);
    }
}

inline void drawBadge (juce::Graphics& g,
                       juce::Rectangle<int> bounds,
                       const juce::String& text,
                       juce::Colour accent)
{
    const auto& theme = ThemeManager::get().theme();
    const float radius = juce::jlimit (2.0f, 6.0f, cardRadius() * 0.45f);
    const auto bg = theme.badgeBg.interpolatedWith (accent, 0.18f);

    g.setColour (bg);
    g.fillRoundedRectangle (bounds.toFloat(), radius);
    g.setColour (accent.withAlpha (0.5f));
    g.drawRoundedRectangle (bounds.toFloat(), radius, 0.5f);
    g.setFont (Theme::Font::micro());
    g.setColour (accent);
    g.drawText (text, bounds, juce::Justification::centred, false);
}

inline void drawCard (juce::Graphics& g,
                      juce::Rectangle<int> bounds,
                      juce::Colour accent = juce::Colour (Theme::Zone::a),
                      float radius = 6.0f)
{
    const auto& theme = ThemeManager::get().theme();
    radius = juce::jlimit (3.0f, 12.0f, theme.zoneACardRadius);

    g.setColour (theme.cardBg);
    g.fillRoundedRectangle (bounds.toFloat(), radius);
    g.setColour (theme.surfaceEdge.withAlpha (0.55f));
    g.drawRoundedRectangle (bounds.toFloat(), radius, 1.0f);
    g.setColour (accent.withAlpha (0.16f));
    g.fillRect (bounds.getX(), bounds.getY(), 2, bounds.getHeight());
}

inline void drawRowBackground (juce::Graphics& g,
                               juce::Rectangle<int> bounds,
                               bool hovered,
                               bool selected = false,
                               juce::Colour accent = juce::Colour (Theme::Zone::a))
{
    const auto& theme = ThemeManager::get().theme();
    const auto fill = selected ? theme.rowSelected
                               : hovered ? theme.rowHover
                                         : theme.rowBg;

    g.setColour (fill);
    g.fillRect (bounds);

    if (selected)
    {
        g.setColour (accent.withAlpha (0.9f));
        g.fillRect (bounds.removeFromLeft (2));
    }

    g.setColour (theme.surfaceEdge.withAlpha (0.32f));
    g.fillRect (bounds.getX(), bounds.getBottom() - 1, bounds.getWidth(), 1);
}

inline void drawGroupHeader (juce::Graphics& g,
                             juce::Rectangle<int> bounds,
                             const juce::String& name,
                             juce::Colour accent)
{
    g.setColour (accent.withAlpha (0.12f));
    g.fillRect (bounds);
    g.setColour (accent);
    g.fillRect (bounds.getX(), bounds.getY(), 2, bounds.getHeight());
    g.setFont (Theme::Font::micro());
    g.setColour (accent);
    g.drawText (name, bounds.reduced (compactPad(), 0), juce::Justification::centredLeft, false);
}
}

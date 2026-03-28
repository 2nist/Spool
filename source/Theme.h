#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include "BinaryData.h"
#include "theme/ThemeManager.h"

//==============================================================================
// FontCache — loads each typeface exactly once from BinaryData
//==============================================================================
class FontCache
{
public:
    static FontCache& get()
    {
        static FontCache instance;
        return instance;
    }

    juce::Font poppinsRegular (float size) const
    {
        return juce::Font (juce::FontOptions{}.withTypeface (m_poppinsRegular).withHeight (size));
    }
    juce::Font poppinsMedium (float size) const
    {
        return juce::Font (juce::FontOptions{}.withTypeface (m_poppinsMedium).withHeight (size));
    }
    juce::Font dmMonoRegular (float size) const
    {
        return juce::Font (juce::FontOptions{}.withTypeface (m_dmMonoRegular).withHeight (size));
    }
    juce::Font dmMonoMedium (float size) const
    {
        return juce::Font (juce::FontOptions{}.withTypeface (m_dmMonoMedium).withHeight (size));
    }

private:
    FontCache()
    {
        m_poppinsRegular = juce::Typeface::createSystemTypefaceFor (BinaryData::PoppinsRegular_ttf, BinaryData::PoppinsRegular_ttfSize);
        m_poppinsMedium  = juce::Typeface::createSystemTypefaceFor (BinaryData::PoppinsMedium_ttf,  BinaryData::PoppinsMedium_ttfSize);
        m_dmMonoRegular  = juce::Typeface::createSystemTypefaceFor (BinaryData::DMMonoRegular_ttf,  BinaryData::DMMonoRegular_ttfSize);
        m_dmMonoMedium   = juce::Typeface::createSystemTypefaceFor (BinaryData::DMMonoMedium_ttf,   BinaryData::DMMonoMedium_ttfSize);
    }

    juce::Typeface::Ptr m_poppinsRegular, m_poppinsMedium, m_dmMonoRegular, m_dmMonoMedium;
};

//==============================================================================
/**
    MDAW Theme System
    -----------------
    Single source of truth for every visual value in the application.

    USAGE:
      - Every JUCE Component reads from Theme:: instead of hardcoding values
      - To change the entire app's look: edit values here, recompile
      - Never hardcode colours, fonts, spacing, or radii in component code

    STRUCTURE:
      Theme::Colour::*     — all colours
      Theme::Space::*      — spacing scale
      Theme::Radius::*     — border radii
      Theme::Stroke::*     — border/stroke weights
      Theme::Font::*       — font definitions
      Theme::Zone::*       — per-zone accent colours
      Theme::Signal::*     — signal type colours (audio/midi/cv/gate)
      Theme::Semantic::*   — harmonic/semantic colours
      Theme::Anim::*       — animation durations

    EDITING WORKFLOW:
      Use the live Theme Editor (Zone A → THM button) to change any value at
      runtime. All components repaint automatically. Press EXPORT to generate
      a new compiled Theme.h from the current live state.
*/

//==============================================================================
/**
    LiveColour — a transparent proxy for a juce::Colour field in ThemeData.

    Implicitly converts to juce::Colour by reading the current runtime value
    from ThemeManager on every access. Existing component code that reads
    Theme::Colour::inkLight is unmodified — the proxy intercepts the value and
    returns the live theme colour.

    Common juce::Colour operations (.withAlpha, .brighter, .darker etc.) are
    forwarded so call sites like Theme::Colour::inkLight.withAlpha(0.5f) keep
    working unchanged.
*/
/**
    LiveFloat — a transparent proxy for a float field in ThemeData.

    Implicitly converts to float by reading the current runtime value from
    ThemeManager on every access. Drop-in replacement for constexpr float
    constants — all existing callsites that pass Theme::Radius::sm or
    Theme::Stroke::normal to drawing functions work without modification.
*/
struct LiveFloat
{
    float (ThemeData::* ptr) { nullptr };

    operator float() const
    {
        if (ptr == nullptr) return 0.f;
        return ThemeManager::get().theme().*ptr;
    }
};

struct LiveColour
{
    juce::Colour (ThemeData::* ptr) { nullptr };

    /** Implicit conversion — reads the live theme value from ThemeManager. */
    operator juce::Colour() const
    {
        if (ptr == nullptr) return {};
        return ThemeManager::get().theme().*ptr;
    }

    // Forwarded juce::Colour operations —————————————————————————————————————
    [[nodiscard]] juce::Colour withAlpha     (float a)            const { return juce::Colour (*this).withAlpha (a); }
    [[nodiscard]] juce::Colour brighter      (float amt = 0.4f)   const { return juce::Colour (*this).brighter (amt); }
    [[nodiscard]] juce::Colour darker        (float amt = 0.4f)   const { return juce::Colour (*this).darker (amt); }
    [[nodiscard]] juce::Colour overlaidWith  (juce::Colour bg)    const { return juce::Colour (*this).overlaidWith (bg); }
    [[nodiscard]] juce::Colour interpolatedWith (juce::Colour c, float t) const { return juce::Colour (*this).interpolatedWith (c, t); }
    [[nodiscard]] juce::Colour withMultipliedAlpha (float a)      const { return juce::Colour (*this).withMultipliedAlpha (a); }
    [[nodiscard]] juce::Colour withMultipliedBrightness (float b) const { return juce::Colour (*this).withMultipliedBrightness (b); }
    [[nodiscard]] juce::Colour withMultipliedSaturation (float s) const { return juce::Colour (*this).withMultipliedSaturation (s); }
    [[nodiscard]] float        getAlpha()  const { return juce::Colour (*this).getAlpha(); }
    [[nodiscard]] float        getRed()    const { return juce::Colour (*this).getFloatRed(); }
    [[nodiscard]] float        getGreen()  const { return juce::Colour (*this).getFloatGreen(); }
    [[nodiscard]] float        getBlue()   const { return juce::Colour (*this).getFloatBlue(); }
    [[nodiscard]] juce::String toString()  const { return juce::Colour (*this).toString(); }
    [[nodiscard]] bool         isTransparent() const { return juce::Colour (*this).isTransparent(); }

    bool operator== (juce::Colour other) const { return juce::Colour (*this) == other; }
    bool operator!= (juce::Colour other) const { return juce::Colour (*this) != other; }
};

namespace Theme
{

//==============================================================================
// COLOUR PALETTE
// All colours defined as juce::Colour using 0xAARRGGBB hex format
//==============================================================================

namespace Colour
{
    //-- Surfaces ---------------------------------------------------------------
    inline LiveColour surface0     { &ThemeData::surface0    };
    inline LiveColour surface1     { &ThemeData::surface1    };
    inline LiveColour surface2     { &ThemeData::surface2    };
    inline LiveColour surface3     { &ThemeData::surface3    };
    inline LiveColour surface4     { &ThemeData::surface4    };
    inline LiveColour surfaceEdge  { &ThemeData::surfaceEdge };

    //-- Kraft / Paper (compile-time only — not in ThemeData) ------------------
    inline const juce::Colour paper0       { 0xFFf5edd8 };
    inline const juce::Colour paper1       { 0xFFede0c4 };
    inline const juce::Colour paper2       { 0xFFc8a97e };
    inline const juce::Colour paperEdge    { 0xFFa8854e };

    //-- Ink --------------------------------------------------------------------
    inline LiveColour inkLight  { &ThemeData::inkLight };
    inline LiveColour inkMid    { &ThemeData::inkMid   };
    inline LiveColour inkMuted  { &ThemeData::inkMuted };
    inline LiveColour inkGhost  { &ThemeData::inkGhost };
    inline LiveColour inkDark   { &ThemeData::inkDark  };

    //-- Accent -----------------------------------------------------------------
    inline LiveColour accent     { &ThemeData::accent     };
    inline LiveColour accentWarm { &ThemeData::accentWarm };
    inline LiveColour accentDim  { &ThemeData::accentDim  };

    //-- Functional (compile-time — not in ThemeData) --------------------------
    inline const juce::Colour success  { 0xFF4a7c52 };
    inline const juce::Colour warning  { 0xFFc4822a };
    inline const juce::Colour error    { 0xFFb84830 };
    inline const juce::Colour inactive { 0xFF2a2418 };

    //-- Transparent helpers ---------------------------------------------------
    inline const juce::Colour transparent { 0x00000000 };
    inline       juce::Colour withAlpha (juce::Colour c, float a) { return c.withAlpha (a); }

} // namespace Colour

//==============================================================================
// ZONE ACCENT COLOURS
// Each zone has a single identifying accent colour.
// Applied as: top stripe (3-4px), active borders, label text
//==============================================================================

namespace Zone
{
    // Accent colours (stripe, active border, label)
    inline LiveColour a    { &ThemeData::zoneA    };   // Zone A — nav panel
    inline LiveColour b    { &ThemeData::zoneB    };   // Zone B — modules
    inline LiveColour c    { &ThemeData::zoneC    };   // Zone C — fx engine
    inline LiveColour d    { &ThemeData::zoneD    };   // Zone D — timeline
    inline LiveColour menu { &ThemeData::zoneMenu };   // Menu bar

    // Background fills
    inline LiveColour bgA  { &ThemeData::zoneBgA  };   // Zone A fill
    inline LiveColour bgB  { &ThemeData::zoneBgB  };   // Zone B fill
    inline LiveColour bgC  { &ThemeData::zoneBgC  };   // Zone C fill
    inline LiveColour bgD  { &ThemeData::zoneBgD  };   // Zone D fill

} // namespace Zone

//==============================================================================
// SIGNAL TYPE COLOURS
// Applied to port dots, cable draws, connection matrix cells,
// and any UI element that represents a signal connection
//==============================================================================

namespace Signal
{
    inline LiveColour audio { &ThemeData::signalAudio };   // audio signal
    inline LiveColour midi  { &ThemeData::signalMidi  };   // MIDI signal
    inline LiveColour cv    { &ThemeData::signalCv    };   // CV/modulation
    inline LiveColour gate  { &ThemeData::signalGate  };   // gate/clock
    inline const juce::Colour none { 0xFF3a3020 };          // no signal / unconnected

    // Helper: get signal colour by type string
    inline juce::Colour forType (const juce::String& type)
    {
        if (type == "audio") return audio;
        if (type == "midi")  return midi;
        if (type == "cv")    return cv;
        if (type == "gate")  return gate;
        return none;
    }

} // namespace Signal

//==============================================================================
// HARMONIC / SEMANTIC COLOURS
// Used in chord/harmonic context — progression strength indicators,
// Roman numeral display, LALO integration points
//==============================================================================

namespace Semantic
{
    inline const juce::Colour strong    { 0xFF4a7c52 };   // diatonic strong — sage green
    inline const juce::Colour medium    { 0xFFc4822a };   // medium function — amber
    inline const juce::Colour weak      { 0xFF7a6e5a };   // weak function — warm taupe
    inline const juce::Colour dissonant { 0xFFb84830 };   // dissonant — burnt sienna
    inline const juce::Colour borrowed  { 0xFF5a6e82 };   // borrowed chord — slate teal
    inline const juce::Colour same      { 0xFF7a8a6a };   // unison/tonic — muted olive

} // namespace Semantic

//==============================================================================
// SPACING SCALE
// Base unit is 4px. All spacing derived from multiples.
// Use these everywhere — padding, margins, gaps, insets.
// Never use arbitrary pixel values in component code.
//==============================================================================

namespace Space
{
    // Spacing scale — live, editable from Theme Editor (SPACE tab)
    inline LiveFloat xs  { &ThemeData::spaceXs  };   //  4px base — tight internal gaps
    inline LiveFloat sm  { &ThemeData::spaceSm  };   //  8px — component padding
    inline LiveFloat md  { &ThemeData::spaceMd  };   // 12px — inner section padding
    inline LiveFloat lg  { &ThemeData::spaceLg  };   // 16px — between components
    inline LiveFloat xl  { &ThemeData::spaceXl  };   // 20px — zone internal padding
    inline LiveFloat xxl { &ThemeData::spaceXxl };   // 24px — between zones/sections

    // Specific layout values
    inline constexpr float menuBarHeight    { 28.0f };   // top menu bar — always 28px
    inline constexpr float zoneAWidth       { 160.0f };  // Zone A expanded width
    inline constexpr float zoneACollapsed   { 32.0f };   // Zone A collapsed icon strip
    inline constexpr float zoneCWidth       { 200.0f };  // Zone C default width
    inline constexpr float zoneStripeHeight { 3.0f };    // zone accent top stripe
    inline constexpr float moduleSlotHeight { 80.0f };   // Zone B module slot height
    inline constexpr float stepHeight       { 24.0f };   // step button height
    inline constexpr float stepWidth        { 24.0f };   // step button width
    inline constexpr float stepGap          { 3.0f };    // gap between step buttons
    inline constexpr float nodeMinWidth     { 120.0f };  // FX chain node min width
    inline constexpr float knobSize         { 32.0f };   // macro knob diameter
    inline constexpr float portDotSize      { 8.0f };    // signal port dot diameter
    inline constexpr float zoneDBeatHeight  { 16.0f };   // timeline lane height
    inline constexpr float mixerGutterWidth { 60.0f };   // timeline mixer gutter

    // Zone D placeholder height — revised when Zone D is built
    inline constexpr float zoneDHeight { 180.0f };

} // namespace Space

//==============================================================================
// BORDER RADIUS
// Applied consistently to give the UI its character.
// Sharp where structure needs to read as rigid,
// softened where elements need to feel tactile.
//==============================================================================

namespace Radius
{
    // Structural constants — never need to be runtime-editable
    inline constexpr float none   { 0.0f };    // hard edge — dividers, menu bar
    inline constexpr float pill   { 20.0f };   // status tags, signal type badges
    inline constexpr float circle { 999.0f };  // knobs, port dots, link pip

    // Live — editable from Theme Editor (STYLE tab)
    inline LiveFloat xs   { &ThemeData::radiusXs   };  // step pads, badges, tiny elements
    inline LiveFloat chip { &ThemeData::radiusChip  };  // compact buttons, labels, tracks
    inline LiveFloat sm   { &ThemeData::radiusSm   };  // panels, cards, FX nodes
    inline LiveFloat md   { &ThemeData::radiusMd   };  // module slots, zone panels
    inline LiveFloat lg   { &ThemeData::radiusLg   };  // large panels, overlays

} // namespace Radius

//==============================================================================
// STROKE / BORDER WEIGHTS
//==============================================================================

namespace Stroke
{
    // Live — editable from Theme Editor (STYLE tab)
    inline LiveFloat subtle { &ThemeData::strokeSubtle };  // dividers, inactive, ghost borders
    inline LiveFloat normal { &ThemeData::strokeNormal };  // panels, cards, default borders
    inline LiveFloat accent { &ThemeData::strokeAccent };  // selected, active, focused states
    inline LiveFloat thick  { &ThemeData::strokeThick  };  // pin mode indicator, error states

} // namespace Stroke

//==============================================================================
// TYPOGRAPHY
// Fonts are loaded once and referenced everywhere.
// Change the typeface name here to retheme the entire app's typography.
//
// NOTE: Bundle DM Mono as a BinaryData font in your Pamplejuce project.
// Add to CMakeLists.txt:
//   juce_add_binary_data(MDAWFonts SOURCES
//     assets/fonts/DMMono-Regular.ttf
//     assets/fonts/DMMono-Medium.ttf
//     assets/fonts/DMMono-Light.ttf)
//==============================================================================

namespace Font
{
    // Base sizes
    inline constexpr float sizeDisplay   { 14.0f };   // zone titles, major labels
    inline constexpr float sizeHeading   { 13.0f };   // panel headers, module names
    inline constexpr float sizeLabel     { 11.0f };   // component labels, button text
    inline constexpr float sizeBody      { 11.0f };   // parameter values, content
    inline constexpr float sizeMicro     { 10.0f };   // annotations, hints, breadcrumbs
    inline constexpr float sizeValue     { 11.0f };   // numeric parameter values
    inline constexpr float sizeTransport { 13.0f };   // transport section numerics

    // Letter spacing (applied manually in paint calls)
    inline constexpr float trackingWide   { 0.12f };   // zone labels, section headers
    inline constexpr float trackingNormal { 0.06f };   // body text
    inline constexpr float trackingTight  { 0.02f };   // dense param values

    // Font factories — read sizes from ThemeManager when available, fall back to compile-time constants
    // This allows live font size editing via the Theme Editor.

    inline juce::Font display()      { return FontCache::get().poppinsMedium  (ThemeManager::get().theme().sizeDisplay);   }
    inline juce::Font heading()      { return FontCache::get().poppinsMedium  (ThemeManager::get().theme().sizeHeading);   }
    inline juce::Font label()        { return FontCache::get().poppinsRegular (ThemeManager::get().theme().sizeLabel);     }
    inline juce::Font labelMedium()  { return FontCache::get().poppinsMedium  (ThemeManager::get().theme().sizeLabel);     }
    inline juce::Font body()         { return FontCache::get().dmMonoRegular  (ThemeManager::get().theme().sizeBody);      }
    inline juce::Font value()        { return FontCache::get().dmMonoMedium   (ThemeManager::get().theme().sizeValue);     }
    inline juce::Font micro()        { return FontCache::get().poppinsRegular (ThemeManager::get().theme().sizeMicro);     }
    inline juce::Font microMedium()  { return FontCache::get().poppinsMedium  (ThemeManager::get().theme().sizeMicro);     }
    inline juce::Font transport()    { return FontCache::get().dmMonoMedium   (ThemeManager::get().theme().sizeTransport); }

} // namespace Font

//==============================================================================
// ANIMATION DURATIONS
// Keep consistent — not too snappy, not sluggish.
// Zone A panel transitions, pin mode indicator, alert pulses.
//==============================================================================

namespace Anim
{
    inline constexpr int instant  { 0   };   // ms — immediate state changes
    inline constexpr int fast     { 80  };   // ms — button press response
    inline constexpr int normal   { 150 };   // ms — panel slides, transitions
    inline constexpr int slow     { 280 };   // ms — Zone A expand/collapse
    inline constexpr int pulse    { 900 };   // ms — Link pip, alert dot blink

} // namespace Anim

//==============================================================================
// ALPHA VALUES
// Consistent transparency levels for overlays, disabled states, etc.
//==============================================================================

namespace Alpha
{
    inline constexpr float disabled   { 0.35f };   // muted/inactive elements
    inline constexpr float overlay    { 0.75f };   // panel overlays on timeline
    inline constexpr float ghostHover { 0.08f };   // subtle hover fill
    inline constexpr float pinBorder  { 0.60f };   // Zone C pin dashed border
    inline constexpr float cableLine  { 0.50f };   // signal cable draw opacity
    inline constexpr float meterFill  { 0.80f };   // level meter bar opacity

} // namespace Alpha

//==============================================================================
// HELPER FUNCTIONS
// Convenience methods used across multiple components
//==============================================================================

namespace Helper
{
    /*
        Styling contract for shared helpers:
        - keep chrome compact and token-driven
        - selected and playing are never the same visual state
        - scaffold/guide content must read more ghosted than authored content
        - focused state gets an outline/ring, not extra layout padding
        - use accent sparingly for hierarchy, state, and zone identity
    */

    enum class VisualState
    {
        idle,
        hover,
        selected,
        focused,
        playing,
        armed,
        disabled,
        scaffold
    };

    // Returns the accent colour for a given zone identifier
    inline juce::Colour zoneAccent (const juce::String& zone)
    {
        if (zone == "a")    return Zone::a;
        if (zone == "b")    return Zone::b;
        if (zone == "c")    return Zone::c;
        if (zone == "d")    return Zone::d;
        if (zone == "menu") return Zone::menu;
        return Colour::accent;
    }

    // Returns a surface colour one step more elevated than the input
    inline juce::Colour elevate (const juce::Colour& surface)
    {
        if (surface == Colour::surface1) return Colour::surface2;
        if (surface == Colour::surface2) return Colour::surface3;
        if (surface == Colour::surface3) return Colour::surface4;
        return Colour::surface4;
    }

    // Returns appropriate ink colour for a given surface
    // Dark surfaces get light ink, kraft surfaces get dark ink
    inline juce::Colour inkFor (const juce::Colour& surface)
    {
        // Rough luminance check — kraft colours are brighter
        if (surface.getBrightness() > 0.4f)
            return Colour::inkDark;
        return Colour::inkLight;
    }

    inline float alphaForState (VisualState state) noexcept
    {
        switch (state)
        {
            case VisualState::disabled: return Alpha::disabled;
            case VisualState::scaffold: return 0.56f;
            case VisualState::hover:    return 0.92f;
            default:                    return 1.0f;
        }
    }

    inline juce::Colour emphasisForState (juce::Colour accent, VisualState state)
    {
        switch (state)
        {
            case VisualState::selected: return accent;
            case VisualState::focused:  return Colour::accent;
            case VisualState::playing:  return Colour::accentWarm;
            case VisualState::armed:    return Colour::warning;
            case VisualState::disabled: return Colour::inkGhost;
            case VisualState::scaffold: return Colour::inkGhost;
            default:                    return accent;
        }
    }

    inline juce::Colour fillForState (juce::Colour base, juce::Colour accent, VisualState state)
    {
        switch (state)
        {
            case VisualState::hover:    return elevate (base);
            case VisualState::selected: return elevate (base).interpolatedWith (accent, 0.14f);
            case VisualState::focused:  return elevate (base).interpolatedWith (Colour::accent, 0.10f);
            case VisualState::playing:  return elevate (base).interpolatedWith (Colour::accentWarm, 0.14f);
            case VisualState::armed:    return elevate (base).interpolatedWith (Colour::warning, 0.16f);
            case VisualState::disabled: return base.withMultipliedAlpha (Alpha::disabled);
            case VisualState::scaffold: return base.interpolatedWith (Colour::surface1, 0.35f);
            case VisualState::idle:
            default:                    return base;
        }
    }

    inline juce::Colour textForState (VisualState state, juce::Colour active = Colour::inkLight)
    {
        switch (state)
        {
            case VisualState::disabled: return Colour::inkGhost.withAlpha (Alpha::disabled + 0.15f);
            case VisualState::scaffold: return Colour::inkGhost.withAlpha (0.7f);
            case VisualState::hover:    return active.withAlpha (0.96f);
            default:                    return active;
        }
    }

    // Draws the zone accent stripe at the top of a component bounds
    inline void drawZoneStripe (juce::Graphics& g,
                                 juce::Rectangle<int> bounds,
                                 const juce::Colour& zoneColour)
    {
        auto stripe = bounds.removeFromTop (static_cast<int> (Space::zoneStripeHeight));
        g.setColour (zoneColour);
        g.fillRoundedRectangle (stripe.toFloat(), Radius::xs);
    }

    // Draws a standard panel background with optional border
    inline void drawPanel (juce::Graphics& g,
                            juce::Rectangle<int> bounds,
                            const juce::Colour& bg    = Colour::surface2,
                            const juce::Colour& border = Colour::surfaceEdge,
                            float radius               = Radius::sm,
                            float strokeWidth          = Stroke::normal)
    {
        g.setColour (bg);
        g.fillRoundedRectangle (bounds.toFloat(), radius);
        g.setColour (border);
        g.drawRoundedRectangle (bounds.toFloat(), radius, strokeWidth);
    }

    inline void drawSelectionOutline (juce::Graphics& g,
                                      juce::Rectangle<float> bounds,
                                      juce::Colour accent,
                                      float radius = Radius::sm,
                                      float strokeWidth = Stroke::accent)
    {
        g.setColour (accent);
        g.drawRoundedRectangle (bounds.reduced (0.5f), radius, strokeWidth);
    }

    inline void drawFocusRing (juce::Graphics& g,
                               juce::Rectangle<float> bounds,
                               juce::Colour accent = Colour::accent,
                               float radius = Radius::sm)
    {
        g.setColour (accent.withAlpha (0.9f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), radius, Stroke::accent);
    }

    inline void drawStateDot (juce::Graphics& g,
                              juce::Rectangle<float> bounds,
                              VisualState state,
                              juce::Colour accent = Colour::accent)
    {
        const auto colour = emphasisForState (accent, state).withAlpha (alphaForState (state));
        g.setColour (colour);
        g.fillEllipse (bounds);
    }

    inline void drawInfoChip (juce::Graphics& g,
                              juce::Rectangle<float> bounds,
                              const juce::String& text,
                              juce::Colour accent,
                              VisualState state = VisualState::idle)
    {
        const auto fill = fillForState (Colour::surface3, accent, state);
        const auto border = emphasisForState (accent, state).withAlpha (0.45f);
        const auto ink = textForState (state, emphasisForState (accent, state));

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, Radius::xs);
        g.setColour (border);
        g.drawRoundedRectangle (bounds, Radius::xs, Stroke::subtle);
        g.setColour (ink);
        g.setFont (Font::micro());
        g.drawText (text, bounds.toNearestInt(), juce::Justification::centred, false);
    }

    // Draws a step pad with correct state colours
    inline void drawStepPad (juce::Graphics& g,
                              juce::Rectangle<int> bounds,
                              bool active,
                              bool accent   = false,
                              bool selected = false)
    {
        const auto state = selected ? VisualState::selected
                                    : active ? VisualState::playing
                                             : VisualState::idle;
        juce::Colour fill = active
            ? (accent ? Colour::accent.brighter (0.1f) : Colour::accent)
            : fillForState (Colour::surface3, Colour::accent, state);

        juce::Colour border = selected
            ? emphasisForState (Colour::accent, VisualState::selected)
            : Colour::surfaceEdge;

        g.setColour (fill);
        g.fillRoundedRectangle (bounds.toFloat(), Radius::xs);
        g.setColour (border);
        g.drawRoundedRectangle (bounds.toFloat(), Radius::xs, Stroke::subtle);
    }

} // namespace Helper

} // namespace Theme

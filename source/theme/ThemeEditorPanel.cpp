#include "ThemeEditorPanel.h"
#include "ThemePresets.h"
#include "../components/ZoneA/ZoneAStyle.h"
#include "../components/ZoneA/ZoneAControlStyle.h"

//==============================================================================
// Helpers

static void styleSmallButton (juce::TextButton& b)
{
    b.setLookAndFeel (nullptr);
    b.setColour (juce::TextButton::buttonColourId,   ThemeManager::get().theme().controlBg);
    b.setColour (juce::TextButton::buttonOnColourId, Theme::Colour::accent.withAlpha (0.3f));
    b.setColour (juce::TextButton::textColourOffId,  ThemeManager::get().theme().controlText);
    b.setColour (juce::TextButton::textColourOnId,   ThemeManager::get().theme().controlTextOn);
}

class ThemePreviewPane final : public juce::Component,
                               private ThemeManager::Listener
{
public:
    enum class Mode
    {
        full,
        typography,
        spacing,
        tape,
        controls
    };

    explicit ThemePreviewPane (Mode previewMode) : m_mode (previewMode)
    {
        ThemeManager::get().addListener (this);
    }

    ThemePreviewPane()
    {
        ThemeManager::get().addListener (this);
    }

    ~ThemePreviewPane() override
    {
        ThemeManager::get().removeListener (this);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced (4);

        switch (m_mode)
        {
            case Mode::typography:
                drawTypographyPreview (g, bounds);
                break;
            case Mode::spacing:
                drawSpacingPreview (g, bounds);
                break;
            case Mode::tape:
                drawTapePreview (g, bounds);
                break;
            case Mode::controls:
                drawControlsPreview (g, bounds);
                break;
            case Mode::full:
            default:
                drawZoneAPreview (g, bounds.removeFromTop (190));
                bounds.removeFromTop (8);
                drawTypographyPreview (g, bounds.removeFromTop (112));
                bounds.removeFromTop (8);
                drawSpacingPreview (g, bounds.removeFromTop (92));
                bounds.removeFromTop (8);
                drawTapePreview (g, bounds.removeFromTop (96));
                break;
        }
    }

private:
    Mode m_mode { Mode::full };

    void themeChanged() override
    {
        repaint();
    }

    static void drawZoneAPreview (juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        const auto accent = juce::Colour (Theme::Zone::a);
        ZoneAStyle::drawCard (g, bounds, accent);

        auto inner = bounds.reduced (8);
        const auto headerH = juce::jmax (18, ZoneAStyle::compactHeaderHeight());
        const auto rowH = juce::jmax (18, ZoneAStyle::compactRowHeight());
        const auto gap = juce::jmax (4, ZoneAControlStyle::compactGap());
        const auto badgeH = juce::jmax (10, ZoneAStyle::badgeHeight());

        auto header = inner.removeFromTop (headerH);
        ZoneAStyle::drawHeader (g, header, "ZONE A PREVIEW", accent,
                                { juce::Justification::centredLeft, true });

        inner.removeFromTop (gap);
        auto row1 = inner.removeFromTop (rowH);
        auto row2 = inner.removeFromTop (rowH);
        row2.removeFromTop (gap / 2);
        auto row3 = inner.removeFromTop (rowH);
        row3.removeFromTop (gap / 2);

        ZoneAStyle::drawRowBackground (g, row1, false, false, accent);
        ZoneAStyle::drawRowBackground (g, row2, true,  false, accent);
        ZoneAStyle::drawRowBackground (g, row3, false, true,  accent);

        drawRowText (g, row1, "Browser Row", "NEW", accent, badgeH);
        drawRowText (g, row2, "Hovered Row", "ALT", juce::Colour (Theme::Colour::accentWarm), badgeH);
        drawRowText (g, row3, "Selected Row", "SEL", accent, badgeH);

        inner.removeFromTop (gap);
        auto controls = inner.removeFromTop (juce::jmax (54, juce::roundToInt (ThemeManager::get().theme().zoneAControlHeight * 2.8f)));
        drawControls (g, controls, accent);
    }

    static void drawTypographyPreview (juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        ZoneAStyle::drawCard (g, bounds, juce::Colour (Theme::Colour::accentWarm));
        auto inner = bounds.reduced (8);

        g.setColour (Theme::Colour::inkGhost);
        g.setFont (Theme::Font::micro());
        g.drawText ("TYPOGRAPHY", inner.removeFromTop (14), juce::Justification::centredLeft, false);

        auto line1 = inner.removeFromTop (20);
        g.setColour (Theme::Colour::inkLight);
        g.setFont (Theme::Font::heading());
        g.drawText ("Heading / Section Title", line1, juce::Justification::centredLeft, true);

        auto line2 = inner.removeFromTop (18);
        g.setFont (Theme::Font::label());
        g.drawText ("Label text and compact controls", line2, juce::Justification::centredLeft, true);

        auto line3 = inner.removeFromTop (18);
        g.setFont (Theme::Font::body());
        g.drawText ("Body/value preview 1e&a", line3, juce::Justification::centredLeft, true);

        auto line4 = inner.removeFromTop (20);
        g.setFont (Theme::Font::transport());
        g.drawText ("128.0 BPM", line4, juce::Justification::centredLeft, true);
    }

    static void drawSpacingPreview (juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        const auto& theme = ThemeManager::get().theme();
        ZoneAStyle::drawCard (g, bounds, juce::Colour (Theme::Zone::b));
        auto inner = bounds.reduced (8);

        g.setColour (Theme::Colour::inkGhost);
        g.setFont (Theme::Font::micro());
        g.drawText ("SPACING", inner.removeFromTop (14), juce::Justification::centredLeft, false);

        auto row = inner.removeFromTop (24);
        const int boxH = 14;
        const float values[] = { theme.spaceXs, theme.spaceSm, theme.spaceMd, theme.spaceLg, theme.spaceXl };
        const char* labels[] = { "XS", "SM", "MD", "LG", "XL" };
        int x = row.getX();

        for (int i = 0; i < 5; ++i)
        {
            const int w = juce::roundToInt (juce::jlimit (8.0f, 34.0f, values[i] * 1.5f));
            auto box = juce::Rectangle<int> (x, row.getY() + 6, w, boxH);
            g.setColour (Theme::Colour::surface4);
            g.fillRoundedRectangle (box.toFloat(), 3.0f);
            g.setColour (Theme::Colour::surfaceEdge);
            g.drawRoundedRectangle (box.toFloat(), 3.0f, 1.0f);
            g.setColour (Theme::Colour::inkMid);
            g.setFont (Theme::Font::micro());
            g.drawText (labels[i], x, row.getBottom() - 2, 30, 12, juce::Justification::centredLeft, false);
            x += w + 10;
        }

        auto detail = inner.removeFromTop (18);
        g.setColour (Theme::Colour::inkMuted);
        g.setFont (Theme::Font::micro());
        g.drawText ("Knob " + juce::String (juce::roundToInt (theme.knobSize))
                    + "  Slot " + juce::String (juce::roundToInt (theme.moduleSlotHeight))
                    + "  Step " + juce::String (juce::roundToInt (theme.stepWidth))
                    + "x" + juce::String (juce::roundToInt (theme.stepHeight)),
                    detail, juce::Justification::centredLeft, true);
    }

    static void drawTapePreview (juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        const auto& theme = ThemeManager::get().theme();
        ZoneAStyle::drawCard (g, bounds, juce::Colour (Theme::Zone::d));
        auto inner = bounds.reduced (8);

        g.setColour (Theme::Colour::inkGhost);
        g.setFont (Theme::Font::micro());
        g.drawText ("TAPE / TIMELINE", inner.removeFromTop (14), juce::Justification::centredLeft, false);

        auto lane = inner.removeFromTop (48);
        g.setColour (theme.tapeBase);
        g.fillRoundedRectangle (lane.toFloat(), 6.0f);

        auto clip = lane.reduced (10, 10).withWidth (juce::roundToInt (lane.getWidth() * 0.55f));
        g.setColour (theme.tapeClipBg);
        g.fillRoundedRectangle (clip.toFloat(), 4.0f);
        g.setColour (theme.tapeClipBorder);
        g.drawRoundedRectangle (clip.toFloat(), 4.0f, 1.0f);
        g.setColour (theme.tapeSeam);
        g.drawVerticalLine (clip.getCentreX(), (float) clip.getY() + 2.0f, (float) clip.getBottom() - 2.0f);

        const int tickX = lane.getX() + lane.getWidth() * 3 / 4;
        g.setColour (theme.tapeBeatTick);
        g.drawVerticalLine (tickX, (float) lane.getY() + 6.0f, (float) lane.getBottom() - 6.0f);

        const int playheadX = lane.getX() + lane.getWidth() / 2;
        g.setColour (theme.playheadColor);
        g.drawVerticalLine (playheadX, (float) lane.getY() + 2.0f, (float) lane.getBottom() - 2.0f);
    }

    static void drawRowText (juce::Graphics& g,
                             juce::Rectangle<int> row,
                             const juce::String& text,
                             const juce::String& badge,
                             juce::Colour accent,
                             int badgeH)
    {
        g.setColour (Theme::Colour::inkLight);
        g.setFont (Theme::Font::label());
        g.drawText (text, row.reduced (8, 0).withTrimmedRight (56), juce::Justification::centredLeft, true);

        const auto badgeW = ZoneAStyle::badgeWidthForText (badge);
        auto badgeBounds = juce::Rectangle<int> (row.getRight() - badgeW - 8,
                                                 row.getCentreY() - badgeH / 2,
                                                 badgeW,
                                                 badgeH);
        ZoneAStyle::drawBadge (g, badgeBounds, badge, accent);
    }

    static void drawControlsPreview (juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        const auto& t = ThemeManager::get().theme();
        ZoneAStyle::drawCard (g, bounds, juce::Colour (Theme::Colour::accent));
        auto inner = bounds.reduced (8);

        g.setColour (Theme::Colour::inkGhost);
        g.setFont (Theme::Font::micro());
        g.drawText ("CONTROLS PREVIEW", inner.removeFromTop (14), juce::Justification::centredLeft, false);

        const float btnR  = juce::jlimit (0.f, 12.f, t.btnCornerRadius);
        const float sldR  = juce::jlimit (0.f, 12.f, t.sliderCornerRadius);
        const float sldTk = juce::jlimit (2.f, 16.f, t.sliderTrackThickness);
        const float sldTh = juce::jlimit (4.f, 20.f, t.sliderThumbSize);
        const float kRing = juce::jlimit (1.f,  6.f, t.knobRingThickness);
        const float kCap  = juce::jlimit (0.4f, 0.92f, t.knobCapSize);
        const float swW   = juce::jlimit (16.f, 48.f, t.switchWidth);
        const float swH   = juce::jlimit (8.f,  24.f, t.switchHeight);
        const float swR   = juce::jlimit (0.f,  swH * 0.5f, t.switchCornerRadius);
        const float swIn  = juce::jlimit (1.f,   4.f, t.switchThumbInset);

        // ── Button row ────────────────────────────────────────────────────────
        {
            auto row = inner.removeFromTop (20);
            inner.removeFromTop (4);

            // Idle button
            auto b1 = row.removeFromLeft (52);
            g.setColour (t.controlBg.withMultipliedAlpha (t.btnFillStrength));
            g.fillRoundedRectangle (b1.toFloat(), btnR);
            g.setColour (t.surfaceEdge.withAlpha (t.btnBorderStrength));
            g.drawRoundedRectangle (b1.toFloat(), btnR, 1.0f);
            g.setColour (t.controlText);
            g.setFont (Theme::Font::microMedium());
            g.drawText ("IDLE", b1, juce::Justification::centred, false);

            row.removeFromLeft (4);

            // Active / on button
            auto b2 = row.removeFromLeft (52);
            g.setColour (t.accent.withAlpha (t.btnOnFillStrength));
            g.fillRoundedRectangle (b2.toFloat(), btnR);
            g.setColour (t.accent.withAlpha (t.btnBorderStrength + 0.25f));
            g.drawRoundedRectangle (b2.toFloat(), btnR, 1.0f);
            g.setColour (t.controlTextOn);
            g.drawText ("ON", b2, juce::Justification::centred, false);

            row.removeFromLeft (4);

            // Disabled button
            auto b3 = row.removeFromLeft (52);
            g.setColour (t.controlBg.withMultipliedAlpha (0.4f));
            g.fillRoundedRectangle (b3.toFloat(), btnR);
            g.setColour (t.surfaceEdge.withAlpha (0.25f));
            g.drawRoundedRectangle (b3.toFloat(), btnR, 1.0f);
            g.setColour (t.controlText.withMultipliedAlpha (0.4f));
            g.drawText ("OFF", b3, juce::Justification::centred, false);
        }

        // ── Slider ────────────────────────────────────────────────────────────
        {
            const int trackH = juce::roundToInt (sldTk);
            auto trackArea = inner.removeFromTop (trackH + 8);
            inner.removeFromTop (4);
            auto track = trackArea.reduced (0, (trackArea.getHeight() - trackH) / 2);
            g.setColour (t.controlBg);
            g.fillRoundedRectangle (track.toFloat(), sldR);
            auto fill = track.withWidth (track.getWidth() * 3 / 5);
            g.setColour (t.sliderTrack);
            g.fillRoundedRectangle (fill.toFloat(), sldR);
            g.setColour (t.surfaceEdge.withAlpha (0.6f));
            g.drawRoundedRectangle (track.toFloat(), sldR, 1.0f);
            // thumb dot
            const float tx = (float) fill.getRight();
            const float ty = (float) track.getCentreY();
            g.setColour (t.sliderThumb);
            g.fillEllipse (tx - sldTh * 0.5f, ty - sldTh * 0.5f, sldTh, sldTh);
            g.setColour (t.controlText);
            g.setFont (Theme::Font::micro());
            g.drawText ("SLIDER", track.reduced (6, 0), juce::Justification::centredLeft, false);
        }

        // ── Knob ──────────────────────────────────────────────────────────────
        {
            auto knobRow = inner.removeFromTop (36);
            inner.removeFromTop (4);
            const float kSize = juce::jlimit (24.f, 36.f, (float) knobRow.getHeight());
            const float kx = (float) knobRow.getX();
            const float ky = (float) knobRow.getCentreY();
            const float kr = kSize * 0.5f;
            const juce::Point<float> kc { kx + kr, ky };

            // Arc ring
            const float startAngle = juce::MathConstants<float>::pi * 0.75f;
            const float endAngle   = juce::MathConstants<float>::pi * 2.25f;
            const float arcFill    = 0.65f;
            juce::Path trackArc, fillArc;
            trackArc.addCentredArc (kc.x, kc.y, kr - kRing * 0.5f, kr - kRing * 0.5f,
                                    0.f, startAngle, endAngle, true);
            fillArc.addCentredArc  (kc.x, kc.y, kr - kRing * 0.5f, kr - kRing * 0.5f,
                                    0.f, startAngle, startAngle + (endAngle - startAngle) * arcFill, true);
            g.setColour (t.surface3);
            g.strokePath (trackArc, juce::PathStrokeType (kRing, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (t.sliderTrack);
            g.strokePath (fillArc,  juce::PathStrokeType (kRing, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Cap
            g.setColour (t.surface4);
            g.fillEllipse (kc.x - kr * kCap, kc.y - kr * kCap, kr * kCap * 2.f, kr * kCap * 2.f);
            g.setColour (t.surfaceEdge);
            g.drawEllipse (kc.x - kr * kCap, kc.y - kr * kCap, kr * kCap * 2.f, kr * kCap * 2.f, 1.0f);

            // Pointer dot at arcFill angle
            const float dotAngle = startAngle + (endAngle - startAngle) * arcFill - juce::MathConstants<float>::halfPi;
            const float dotR = kr * kCap * 0.55f;
            const float dotX = kc.x + std::cos (dotAngle) * dotR;
            const float dotY = kc.y + std::sin (dotAngle) * dotR;
            const float dotD = t.knobDotSize;
            g.setColour (t.sliderThumb);
            g.fillEllipse (dotX - dotD * 0.5f, dotY - dotD * 0.5f, dotD, dotD);

            // Label
            auto labelArea = knobRow.withLeft (juce::roundToInt (kx + kSize + 8));
            g.setColour (t.controlText);
            g.setFont (Theme::Font::micro());
            g.drawText ("KNOB", labelArea, juce::Justification::centredLeft, false);
        }

        // ── Switch / toggle ───────────────────────────────────────────────────
        {
            auto swRow = inner.removeFromTop (juce::roundToInt (swH) + 6);
            const float sx = (float) swRow.getX();
            const float sy = (float) swRow.getCentreY() - swH * 0.5f;
            const float thumbD = swH - swIn * 2.f;

            // Off state
            g.setColour (t.controlBg);
            g.fillRoundedRectangle (sx, sy, swW, swH, swR);
            g.setColour (t.surfaceEdge.withAlpha (0.7f));
            g.drawRoundedRectangle (sx, sy, swW, swH, swR, 1.0f);
            g.setColour (t.inkMuted);
            g.fillEllipse (sx + swIn, sy + swIn, thumbD, thumbD);

            // On state (offset to the right)
            const float onX = sx + swW + 10.f;
            g.setColour (t.accent.withAlpha (0.35f));
            g.fillRoundedRectangle (onX, sy, swW, swH, swR);
            g.setColour (t.accent.withAlpha (0.8f));
            g.drawRoundedRectangle (onX, sy, swW, swH, swR, 1.0f);
            g.setColour (t.controlTextOn);
            g.fillEllipse (onX + swW - swIn - thumbD, sy + swIn, thumbD, thumbD);

            // Labels
            g.setColour (t.controlText);
            g.setFont (Theme::Font::micro());
            const float lblX = onX + swW + 10.f;
            g.drawText ("SWITCH  off / on", juce::Rectangle<float> (lblX, sy, 90.f, swH),
                        juce::Justification::centredLeft, false);
        }
    }

    static void drawControls (juce::Graphics& g, juce::Rectangle<int> area, juce::Colour accent)
    {
        const auto& theme = ThemeManager::get().theme();
        const auto controlH = juce::jmax (18, ZoneAControlStyle::controlHeight());
        const auto barH = juce::jmax (12, ZoneAControlStyle::controlBarHeight());
        const auto gap = juce::jmax (4, ZoneAControlStyle::compactGap());

        auto topRow = area.removeFromTop (controlH);
        auto button = topRow.removeFromLeft (64);
        auto combo = topRow.removeFromLeft (92).translated (gap, 0);

        g.setColour (theme.controlBg);
        g.fillRoundedRectangle (button.toFloat(), 4.0f);
        g.setColour (accent.withAlpha (0.45f));
        g.drawRoundedRectangle (button.toFloat(), 4.0f, 1.0f);
        g.setColour (theme.controlTextOn);
        g.setFont (Theme::Font::microMedium());
        g.drawText ("APPLY", button, juce::Justification::centred, false);

        g.setColour (theme.controlBg);
        g.fillRoundedRectangle (combo.toFloat(), 4.0f);
        g.setColour (theme.surfaceEdge.withAlpha (0.55f));
        g.drawRoundedRectangle (combo.toFloat(), 4.0f, 1.0f);
        g.setColour (theme.controlTextOn);
        g.drawText ("MODE v", combo.reduced (8, 0), juce::Justification::centredLeft, false);

        area.removeFromTop (gap);
        auto slider = area.removeFromTop (barH);
        g.setColour (theme.controlBg);
        g.fillRoundedRectangle (slider.toFloat(), 4.0f);
        auto fill = slider.withWidth (slider.getWidth() * 3 / 5);
        g.setColour (theme.sliderTrack);
        g.fillRoundedRectangle (fill.toFloat(), 4.0f);
        g.setColour (theme.surfaceEdge.withAlpha (0.85f));
        g.drawRoundedRectangle (slider.toFloat(), 4.0f, 1.0f);
        g.setColour (theme.controlTextOn);
        g.drawText ("Control Height", slider.reduced (8, 0), juce::Justification::centredLeft, false);
    }
};

//==============================================================================

ThemeEditorPanel::ThemeEditorPanel()
{
    // Title
    m_titleLabel.setText ("THEME EDITOR", juce::dontSendNotification);
    m_titleLabel.setFont (Theme::Font::micro());
    m_titleLabel.setColour (juce::Label::textColourId, Theme::Colour::inkMuted);
    m_titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (m_titleLabel);

    // Preset combo
    m_presetCombo.setTextWhenNothingSelected ("Preset...");
    m_presetCombo.setColour (juce::ComboBox::backgroundColourId, ThemeManager::get().theme().controlBg);
    m_presetCombo.setColour (juce::ComboBox::textColourId,       ThemeManager::get().theme().controlTextOn);
    m_presetCombo.setColour (juce::ComboBox::outlineColourId,    ThemeManager::get().theme().surfaceEdge);
    m_presetCombo.onChange = [this] { onPresetSelected (m_presetCombo.getSelectedId()); };
    addAndMakeVisible (m_presetCombo);
    populatePresetCombo();

    // Export button
    styleSmallButton (m_exportBtn);
    m_exportBtn.onClick = [this] { onExportClicked(); };
    addAndMakeVisible (m_exportBtn);

    // Tab buttons — icon tabs using Lucide SVG paths
    static const char* kIcons[kTabCount] = {
        LucideIcons::surface,    // kSurface
        LucideIcons::ink,        // kInk
        LucideIcons::accent,     // kAccent
        LucideIcons::zones,      // kZones
        LucideIcons::signal,     // kSignal
        LucideIcons::typography, // kType
        LucideIcons::spacing,    // kSpace
        LucideIcons::accent,     // kUi
        LucideIcons::controls,   // kControls
        LucideIcons::spacing,    // kStyle
        LucideIcons::zones,      // kPreview
        LucideIcons::tape,       // kTape
    };
    static const char* kTooltips[kTabCount] = {
        "Surface Colors", "Ink & Text", "Accent Colors", "Zone Colors",
        "Signal Colors",  "Typography", "Spacing",       "UI Chrome",
        "Controls",       "Style (Radius + Stroke)", "Zone A Preview", "Tape Colors",
    };
    for (int i = 0; i < kTabCount; ++i)
    {
        auto* btn = m_tabButtons.add (new IconTabButton (kIcons[i], kTooltips[i]));
        btn->onClick = [this, i] { switchTab (i); };
        addAndMakeVisible (btn);
    }

    // Footer
    styleSmallButton (m_saveAsBtn);
    m_saveAsBtn.onClick = [this] { onSaveAsClicked(); };
    addAndMakeVisible (m_saveAsBtn);

    styleSmallButton (m_resetBtn);
    m_resetBtn.onClick = [this] { onResetClicked(); };
    addAndMakeVisible (m_resetBtn);

    // Build pages
    buildAllTabs();

    // Show first tab
    switchTab (kSurface);

    ThemeManager::get().addListener (this);
}

ThemeEditorPanel::~ThemeEditorPanel()
{
    ThemeManager::get().removeListener (this);
}

//==============================================================================

void ThemeEditorPanel::resized()
{
    auto r = getLocalBounds();

    // Header
    auto header = r.removeFromTop (kHeaderH);
    m_titleLabel .setBounds (header.removeFromLeft (90).reduced (4, 6));
    m_exportBtn  .setBounds (header.removeFromRight (52).reduced (2, 5));
    m_presetCombo.setBounds (header.reduced (4, 5));

    // Tab bar
    auto tabBar = r.removeFromTop (kTabBarH);
    const int tabW = tabBar.getWidth() / kTabCount;
    for (int i = 0; i < kTabCount; ++i)
        m_tabButtons[i]->setBounds (tabBar.removeFromLeft (tabW).reduced (1, 2));

    // Footer
    auto footer = r.removeFromBottom (kFooterH);
    m_resetBtn .setBounds (footer.removeFromRight (60).reduced (3, 5));
    m_saveAsBtn.setBounds (footer.removeFromRight (60).reduced (3, 5));

    // Content
    for (auto& page : m_pages)
        if (page != nullptr)
        {
            page->viewport.setBounds (r);
            relayoutPage (*page);
        }
}

void ThemeEditorPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    // Header separator
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawHorizontalLine (kHeaderH, 0.f, (float) getWidth());

    // Tab bar bottom line
    g.drawHorizontalLine (kHeaderH + kTabBarH, 0.f, (float) getWidth());

    // Footer top line
    g.drawHorizontalLine (getHeight() - kFooterH, 0.f, (float) getWidth());

    // Active tab indicator
    if (m_activeTab >= 0 && m_activeTab < kTabCount)
    {
        const auto tabBounds = m_tabButtons[m_activeTab]->getBounds();
        g.setColour (Theme::Colour::accent);
        g.fillRect (tabBounds.getX(), kHeaderH + kTabBarH - 2, tabBounds.getWidth(), 2);
    }
}

//==============================================================================

void ThemeEditorPanel::themeChanged()
{
    m_presetCombo.setColour (juce::ComboBox::backgroundColourId, ThemeManager::get().theme().controlBg);
    m_presetCombo.setColour (juce::ComboBox::textColourId,       ThemeManager::get().theme().controlTextOn);
    m_presetCombo.setColour (juce::ComboBox::outlineColourId,    ThemeManager::get().theme().surfaceEdge);
    styleSmallButton (m_exportBtn);
    styleSmallButton (m_saveAsBtn);
    styleSmallButton (m_resetBtn);
    refreshAllRows();
    repaint();
}

void ThemeEditorPanel::refreshAllRows()
{
    for (auto& page : m_pages)
    {
        if (page == nullptr) continue;
        for (auto* r : page->colourRows) r->refresh();
        for (auto* r : page->floatRows)  r->refresh();
    }
}

//==============================================================================

const char* ThemeEditorPanel::tabName (Tab t)
{
    switch (t)
    {
        case kSurface: return "SURFACE";
        case kInk:     return "INK";
        case kAccent:  return "ACCENT";
        case kZones:   return "ZONES";
        case kSignal:  return "SIGNAL";
        case kType:    return "TYPE";
        case kSpace:   return "SPACE";
        case kUi:       return "UI";
        case kControls: return "CTRL";
        case kStyle:    return "STYLE";
        case kPreview:  return "PREVIEW";
        case kTape:     return "TAPE";
        default:       return "?";
    }
}

void ThemeEditorPanel::switchTab (int newTab)
{
    m_activeTab = newTab;
    for (int i = 0; i < kTabCount; ++i)
    {
        const bool isActive = (i == newTab);
        m_pages[i]->viewport.setVisible (isActive);
        m_tabButtons[i]->setToggleState (isActive, juce::dontSendNotification);
    }
    repaint();
}

//==============================================================================
// Tab builders

void ThemeEditorPanel::relayoutPage (TabPage& page)
{
    const int width = juce::jmax (200, page.viewport.getWidth() - page.viewport.getScrollBarThickness());
    int y = kRowGap;

    page.container.setSize (width, page.container.getHeight());

    if (page.customContent != nullptr)
    {
        const int contentH = juce::jmax (page.contentHeight, 120);
        page.customContent->setBounds (4, y, width - 8, contentH);
        y = page.customContent->getBottom() + kRowGap + 2;
    }

    for (auto* row : page.colourRows)
    {
        row->setBounds (4, y, width - 8, kRowH);
        y += kRowH + kRowGap;
    }

    for (auto* row : page.floatRows)
    {
        row->setBounds (4, y, width - 8, kRowH);
        y += kRowH + kRowGap;
    }

    page.container.setSize (width, juce::jmax (y, 40));
}

void ThemeEditorPanel::buildAllTabs()
{
    for (int i = 0; i < kTabCount; ++i)
    {
        m_pages[i] = std::make_unique<TabPage>();
        auto& page = *m_pages[i];
        page.viewport.setViewedComponent (&page.container, false);
        page.viewport.setScrollBarsShown (true, false);
        page.container.setSize (200, 400);
        addAndMakeVisible (page.viewport);

        switch (static_cast<Tab> (i))
        {
            case kSurface: buildSurfaceTab (page.container); break;
            case kInk:     buildInkTab     (page.container); break;
            case kAccent:  buildAccentTab  (page.container); break;
            case kZones:   buildZonesTab   (page.container); break;
            case kSignal:  buildSignalTab  (page.container); break;
            case kType:    buildTypeTab    (page.container); break;
            case kSpace:   buildSpaceTab   (page.container); break;
            case kUi:       buildUiTab       (page.container); break;
            case kControls: buildControlsTab (page.container); break;
            case kStyle:    buildStyleTab    (page.container); break;
            case kPreview:  buildPreviewTab  (page.container); break;
            case kTape:     buildTapeTab     (page.container); break;
            default: break;
        }

        relayoutPage (page);
    }
}

static ColourRow* addCR (juce::Component& c, juce::OwnedArray<ColourRow>& arr,
                          const char* label, juce::Colour ThemeData::* ptr)
{
    auto* row = arr.add (new ColourRow (label, ptr));
    c.addAndMakeVisible (row);
    return row;
}

static FloatRow* addFR (juce::Component& c, juce::OwnedArray<FloatRow>& arr,
                         const char* label, float ThemeData::* ptr,
                         float minV, float maxV, float step = 0.5f)
{
    auto* row = arr.add (new FloatRow (label, ptr, minV, maxV, step));
    c.addAndMakeVisible (row);
    return row;
}

void ThemeEditorPanel::buildSurfaceTab (juce::Component& c)
{
    auto& cr = m_pages[kSurface]->colourRows;
    addCR (c, cr, "Surface 0 (base)",      &ThemeData::surface0);
    addCR (c, cr, "Surface 1 (page bg)",   &ThemeData::surface1);
    addCR (c, cr, "Surface 2 (panel)",     &ThemeData::surface2);
    addCR (c, cr, "Surface 3 (elevated)",  &ThemeData::surface3);
    addCR (c, cr, "Surface 4 (raised)",    &ThemeData::surface4);
    addCR (c, cr, "Surface Edge",          &ThemeData::surfaceEdge);
    addCR (c, cr, "Housing Edge",          &ThemeData::housingEdge);
}

void ThemeEditorPanel::buildInkTab (juce::Component& c)
{
    auto& cr = m_pages[kInk]->colourRows;
    addCR (c, cr, "Ink Light (primary)",   &ThemeData::inkLight);
    addCR (c, cr, "Ink Mid (secondary)",   &ThemeData::inkMid);
    addCR (c, cr, "Ink Muted (tertiary)",  &ThemeData::inkMuted);
    addCR (c, cr, "Ink Ghost (hints)",     &ThemeData::inkGhost);
    addCR (c, cr, "Ink Dark (on paper)",   &ThemeData::inkDark);
}

void ThemeEditorPanel::buildAccentTab (juce::Component& c)
{
    auto& cr = m_pages[kAccent]->colourRows;
    addCR (c, cr, "Accent (primary)",  &ThemeData::accent);
    addCR (c, cr, "Accent Warm",       &ThemeData::accentWarm);
    addCR (c, cr, "Accent Dim",        &ThemeData::accentDim);
}

void ThemeEditorPanel::buildZonesTab (juce::Component& c)
{
    auto& cr = m_pages[kZones]->colourRows;
    addCR (c, cr, "Zone A accent",     &ThemeData::zoneA);
    addCR (c, cr, "Zone A background", &ThemeData::zoneBgA);
    addCR (c, cr, "Zone B accent",     &ThemeData::zoneB);
    addCR (c, cr, "Zone B background", &ThemeData::zoneBgB);
    addCR (c, cr, "Zone C accent",     &ThemeData::zoneC);
    addCR (c, cr, "Zone C background", &ThemeData::zoneBgC);
    addCR (c, cr, "Zone D accent",     &ThemeData::zoneD);
    addCR (c, cr, "Zone D background", &ThemeData::zoneBgD);
    addCR (c, cr, "Menu Bar",          &ThemeData::zoneMenu);
}

void ThemeEditorPanel::buildSignalTab (juce::Component& c)
{
    auto& cr = m_pages[kSignal]->colourRows;
    addCR (c, cr, "Audio",  &ThemeData::signalAudio);
    addCR (c, cr, "MIDI",   &ThemeData::signalMidi);
    addCR (c, cr, "CV",     &ThemeData::signalCv);
    addCR (c, cr, "Gate",   &ThemeData::signalGate);
}

void ThemeEditorPanel::buildTypeTab (juce::Component& c)
{
    auto& page = *m_pages[kType];
    page.customContent = std::make_unique<ThemePreviewPane> (ThemePreviewPane::Mode::typography);
    page.contentHeight = 110;
    c.addAndMakeVisible (*page.customContent);

    auto& fr = m_pages[kType]->floatRows;
    addFR (c, fr, "Display size",   &ThemeData::sizeDisplay,   8.f, 24.f);
    addFR (c, fr, "Heading size",   &ThemeData::sizeHeading,   8.f, 24.f);
    addFR (c, fr, "Label size",     &ThemeData::sizeLabel,     7.f, 18.f);
    addFR (c, fr, "Body size",      &ThemeData::sizeBody,      7.f, 18.f);
    addFR (c, fr, "Micro size",     &ThemeData::sizeMicro,     6.f, 16.f);
    addFR (c, fr, "Value size",     &ThemeData::sizeValue,     7.f, 18.f);
    addFR (c, fr, "Transport size", &ThemeData::sizeTransport, 8.f, 24.f);
}

void ThemeEditorPanel::buildSpaceTab (juce::Component& c)
{
    auto& page = *m_pages[kSpace];
    page.customContent = std::make_unique<ThemePreviewPane> (ThemePreviewPane::Mode::spacing);
    page.contentHeight = 90;
    c.addAndMakeVisible (*page.customContent);

    auto& fr = m_pages[kSpace]->floatRows;
    addFR (c, fr, "XS (4px base)",  &ThemeData::spaceXs,  2.f, 12.f, 1.f);
    addFR (c, fr, "SM (8px)",       &ThemeData::spaceSm,  4.f, 20.f, 1.f);
    addFR (c, fr, "MD (12px)",      &ThemeData::spaceMd,  6.f, 24.f, 1.f);
    addFR (c, fr, "LG (16px)",      &ThemeData::spaceLg,  8.f, 32.f, 1.f);
    addFR (c, fr, "XL (20px)",      &ThemeData::spaceXl,  10.f, 40.f, 1.f);
    addFR (c, fr, "XXL (24px)",     &ThemeData::spaceXxl, 12.f, 48.f, 1.f);
    addFR (c, fr, "Knob size",      &ThemeData::knobSize,      20.f, 60.f, 1.f);
    addFR (c, fr, "Module slot H",  &ThemeData::moduleSlotHeight, 40.f, 140.f, 2.f);
    addFR (c, fr, "Step H",         &ThemeData::stepHeight, 12.f, 48.f, 1.f);
    addFR (c, fr, "Step W",         &ThemeData::stepWidth,  12.f, 48.f, 1.f);
}

void ThemeEditorPanel::buildUiTab (juce::Component& c)
{
    auto& cr = m_pages[kUi]->colourRows;
    auto& fr = m_pages[kUi]->floatRows;

    addCR (c, cr, "Header Bg",         &ThemeData::headerBg);
    addCR (c, cr, "Card Bg",           &ThemeData::cardBg);
    addCR (c, cr, "Row Bg",            &ThemeData::rowBg);
    addCR (c, cr, "Row Hover",         &ThemeData::rowHover);
    addCR (c, cr, "Row Selected",      &ThemeData::rowSelected);
    addCR (c, cr, "Badge Bg",          &ThemeData::badgeBg);
    addCR (c, cr, "Control Bg",        &ThemeData::controlBg);
    addCR (c, cr, "Control On",        &ThemeData::controlOnBg);
    addCR (c, cr, "Control Text",      &ThemeData::controlText);
    addCR (c, cr, "Control Text On",   &ThemeData::controlTextOn);
    addCR (c, cr, "Focus Outline",     &ThemeData::focusOutline);
    addCR (c, cr, "Slider Track",      &ThemeData::sliderTrack);
    addCR (c, cr, "Slider Thumb",      &ThemeData::sliderThumb);

    addFR (c, fr, "Header H",          &ThemeData::zoneAHeaderHeight,        18.f, 32.f, 1.f);
    addFR (c, fr, "Group H",           &ThemeData::zoneAGroupHeaderHeight,   14.f, 28.f, 1.f);
    addFR (c, fr, "Row H",             &ThemeData::zoneARowHeight,           18.f, 32.f, 1.f);
    addFR (c, fr, "Badge H",           &ThemeData::zoneABadgeHeight,         10.f, 18.f, 1.f);
    addFR (c, fr, "Section H",         &ThemeData::zoneASectionHeaderHeight, 12.f, 24.f, 1.f);
    addFR (c, fr, "Bar H",             &ThemeData::zoneAControlBarHeight,    10.f, 24.f, 1.f);
    addFR (c, fr, "Control H",         &ThemeData::zoneAControlHeight,       16.f, 28.f, 1.f);
    addFR (c, fr, "Card Radius",       &ThemeData::zoneACardRadius,           3.f, 12.f, 0.5f);
    addFR (c, fr, "Compact Gap",       &ThemeData::zoneACompactGap,           3.f, 12.f, 1.f);
}

void ThemeEditorPanel::buildControlsTab (juce::Component& c)
{
    auto& page = *m_pages[kControls];
    page.customContent = std::make_unique<ThemePreviewPane> (ThemePreviewPane::Mode::controls);
    page.contentHeight = 180;
    c.addAndMakeVisible (*page.customContent);

    auto& fr = m_pages[kControls]->floatRows;

    // Button
    addFR (c, fr, "Btn Corner R",    &ThemeData::btnCornerRadius,   0.f,  12.f, 0.5f);
    addFR (c, fr, "Btn Border",      &ThemeData::btnBorderStrength, 0.f,   1.f, 0.05f);
    addFR (c, fr, "Btn Fill",        &ThemeData::btnFillStrength,   0.f,   1.f, 0.05f);
    addFR (c, fr, "Btn On Fill",     &ThemeData::btnOnFillStrength, 0.f,   1.f, 0.05f);

    // Slider
    addFR (c, fr, "Slider Track H",  &ThemeData::sliderTrackThickness, 2.f, 16.f, 0.5f);
    addFR (c, fr, "Slider Corner R", &ThemeData::sliderCornerRadius,   0.f, 12.f, 0.5f);

    // Knob
    addFR (c, fr, "Knob Ring W",     &ThemeData::knobRingThickness, 1.f,  6.f, 0.25f);
    addFR (c, fr, "Knob Dot D",      &ThemeData::knobDotSize,       1.5f, 8.f, 0.25f);
}

void ThemeEditorPanel::buildStyleTab (juce::Component& c)
{
    auto& fr = m_pages[kStyle]->floatRows;

    // Radius family
    addFR (c, fr, "Radius XS  (steps/badges)",   &ThemeData::radiusXs,   0.f, 8.f,  0.5f);
    addFR (c, fr, "Radius Chip (buttons/labels)", &ThemeData::radiusChip, 0.f, 8.f,  0.5f);
    addFR (c, fr, "Radius SM  (panels/cards)",    &ThemeData::radiusSm,   0.f, 12.f, 0.5f);
    addFR (c, fr, "Radius MD  (modules/zones)",   &ThemeData::radiusMd,   0.f, 16.f, 0.5f);
    addFR (c, fr, "Radius LG  (overlays)",        &ThemeData::radiusLg,   0.f, 20.f, 0.5f);

    // Stroke family
    addFR (c, fr, "Stroke Subtle (dividers)",     &ThemeData::strokeSubtle, 0.f, 2.f, 0.25f);
    addFR (c, fr, "Stroke Normal (borders)",      &ThemeData::strokeNormal, 0.f, 3.f, 0.25f);
    addFR (c, fr, "Stroke Accent (active)",       &ThemeData::strokeAccent, 0.f, 4.f, 0.25f);
    addFR (c, fr, "Stroke Thick  (emphasis)",     &ThemeData::strokeThick,  0.f, 4.f, 0.25f);
}

void ThemeEditorPanel::buildPreviewTab (juce::Component& c)
{
    auto& page = *m_pages[kPreview];
    page.customContent = std::make_unique<ThemePreviewPane> (ThemePreviewPane::Mode::full);
    page.contentHeight = 514;
    c.addAndMakeVisible (*page.customContent);
}

void ThemeEditorPanel::buildTapeTab (juce::Component& c)
{
    auto& page = *m_pages[kTape];
    page.customContent = std::make_unique<ThemePreviewPane> (ThemePreviewPane::Mode::tape);
    page.contentHeight = 94;
    c.addAndMakeVisible (*page.customContent);

    auto& cr = m_pages[kTape]->colourRows;
    addCR (c, cr, "Tape Base",        &ThemeData::tapeBase);
    addCR (c, cr, "Clip Background",  &ThemeData::tapeClipBg);
    addCR (c, cr, "Clip Border",      &ThemeData::tapeClipBorder);
    addCR (c, cr, "Seam",             &ThemeData::tapeSeam);
    addCR (c, cr, "Beat Tick",        &ThemeData::tapeBeatTick);
    addCR (c, cr, "Playhead",         &ThemeData::playheadColor);
}

//==============================================================================
// Preset management

void ThemeEditorPanel::populatePresetCombo()
{
    m_presetCombo.clear (juce::dontSendNotification);
    int id = 1;

    m_presetCombo.addSectionHeading ("Built-in");
    for (auto& p : ThemePresets::builtInPresets())
        m_presetCombo.addItem (p.presetName, id++);

    const auto userP = ThemePresets::userPresets();
    if (!userP.isEmpty())
    {
        m_presetCombo.addSeparator();
        m_presetCombo.addSectionHeading ("Saved");
        for (auto& p : userP)
            m_presetCombo.addItem (p.presetName, id++);
    }
}

void ThemeEditorPanel::onPresetSelected (int comboId)
{
    if (comboId <= 0) return;

    // Build combined list in same order as populatePresetCombo
    juce::Array<ThemeData> all;
    for (auto& p : ThemePresets::builtInPresets()) all.add (p);
    for (auto& p : ThemePresets::userPresets())    all.add (p);

    const int idx = comboId - 1;
    if (idx >= 0 && idx < all.size())
    {
        ThemeManager::get().applyTheme (all[idx]);
        AppPreferences::get().setThemePresetName (all[idx].presetName);
    }
}

void ThemeEditorPanel::onExportClicked()
{
    const auto& t  = ThemeManager::get().theme();
    const auto hex = [] (juce::Colour c) { return "0xFF" + c.toString().toUpperCase().substring (2); };
    const auto flt = [] (float v) { return juce::String (v, 4) + "f"; };

    // ── 1. Save full .spool-theme file to user theme directory ────────────────
    ThemeManager::get().saveToFile (ThemeManager::get().getUserThemeDir()
                                        .getChildFile (t.presetName + ".spool-theme"));

    // ── 2. Build a complete C++ header snippet for clipboard ──────────────────
    juce::String s;
    s << "// Generated by SPOOL Theme Editor — " << t.presetName << "\n"
      << "// Preset file also saved to: " << ThemeManager::get().getUserThemeDir().getFullPathName() << "\n"
      << "// Paste ThemeData defaults into source/theme/ThemeData.h to bake this theme at compile time.\n\n";

    s << "// ── SURFACES\n"
      << "juce::Colour surface0     { " << hex (t.surface0)    << " };\n"
      << "juce::Colour surface1     { " << hex (t.surface1)    << " };\n"
      << "juce::Colour surface2     { " << hex (t.surface2)    << " };\n"
      << "juce::Colour surface3     { " << hex (t.surface3)    << " };\n"
      << "juce::Colour surface4     { " << hex (t.surface4)    << " };\n"
      << "juce::Colour surfaceEdge  { " << hex (t.surfaceEdge) << " };\n\n";

    s << "// ── INK\n"
      << "juce::Colour inkLight     { " << hex (t.inkLight)  << " };\n"
      << "juce::Colour inkMid       { " << hex (t.inkMid)    << " };\n"
      << "juce::Colour inkMuted     { " << hex (t.inkMuted)  << " };\n"
      << "juce::Colour inkGhost     { " << hex (t.inkGhost)  << " };\n"
      << "juce::Colour inkDark      { " << hex (t.inkDark)   << " };\n\n";

    s << "// ── ACCENT\n"
      << "juce::Colour accent       { " << hex (t.accent)     << " };\n"
      << "juce::Colour accentWarm   { " << hex (t.accentWarm) << " };\n"
      << "juce::Colour accentDim    { " << hex (t.accentDim)  << " };\n\n";

    s << "// ── ZONES\n"
      << "juce::Colour zoneA        { " << hex (t.zoneA)    << " };\n"
      << "juce::Colour zoneB        { " << hex (t.zoneB)    << " };\n"
      << "juce::Colour zoneC        { " << hex (t.zoneC)    << " };\n"
      << "juce::Colour zoneD        { " << hex (t.zoneD)    << " };\n"
      << "juce::Colour zoneMenu     { " << hex (t.zoneMenu) << " };\n"
      << "juce::Colour zoneBgA      { " << hex (t.zoneBgA)  << " };\n"
      << "juce::Colour zoneBgB      { " << hex (t.zoneBgB)  << " };\n"
      << "juce::Colour zoneBgC      { " << hex (t.zoneBgC)  << " };\n"
      << "juce::Colour zoneBgD      { " << hex (t.zoneBgD)  << " };\n\n";

    s << "// ── UI CHROME\n"
      << "juce::Colour sliderTrack  { " << hex (t.sliderTrack)  << " };\n"
      << "juce::Colour sliderThumb  { " << hex (t.sliderThumb)  << " };\n"
      << "juce::Colour controlBg    { " << hex (t.controlBg)    << " };\n"
      << "juce::Colour controlOnBg  { " << hex (t.controlOnBg)  << " };\n\n";

    s << "// ── CONTROLS — button\n"
      << "float btnCornerRadius     { " << flt (t.btnCornerRadius)   << " };\n"
      << "float btnBorderStrength   { " << flt (t.btnBorderStrength) << " };\n"
      << "float btnFillStrength     { " << flt (t.btnFillStrength)   << " };\n"
      << "float btnOnFillStrength   { " << flt (t.btnOnFillStrength) << " };\n\n";

    s << "// ── CONTROLS — slider\n"
      << "float sliderTrackThickness{ " << flt (t.sliderTrackThickness) << " };\n"
      << "float sliderCornerRadius  { " << flt (t.sliderCornerRadius)   << " };\n"
      << "float sliderThumbSize     { " << flt (t.sliderThumbSize)      << " };\n\n";

    s << "// ── CONTROLS — knob\n"
      << "float knobRingThickness   { " << flt (t.knobRingThickness) << " };\n"
      << "float knobCapSize         { " << flt (t.knobCapSize)       << " };\n"
      << "float knobDotSize         { " << flt (t.knobDotSize)       << " };\n\n";

    s << "// ── CONTROLS — switch\n"
      << "float switchWidth         { " << flt (t.switchWidth)        << " };\n"
      << "float switchHeight        { " << flt (t.switchHeight)       << " };\n"
      << "float switchCornerRadius  { " << flt (t.switchCornerRadius) << " };\n"
      << "float switchThumbInset    { " << flt (t.switchThumbInset)   << " };\n";

    juce::SystemClipboard::copyTextToClipboard (s);

    juce::AlertWindow::showMessageBoxAsync (
        juce::MessageBoxIconType::InfoIcon,
        "Theme Exported",
        "Full preset saved to:\n" + ThemeManager::get().getUserThemeDir().getFullPathName()
            + "\n\nC++ defaults (all fields incl. controls) copied to clipboard.",
        "OK");
}

void ThemeEditorPanel::onSaveAsClicked()
{
    auto* w = new juce::AlertWindow ("Save Theme", "Enter preset name:", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", ThemeManager::get().theme().presetName, "Name");
    w->addButton ("Save",   1);
    w->addButton ("Cancel", 0);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int result)
    {
        if (result == 1)
        {
            const auto name = w->getTextEditorContents ("name").trim();
            if (name.isNotEmpty())
            {
                ThemeData td  = ThemeManager::get().theme();
                td.presetName = name;
                ThemePresets::saveUserPreset (td);
                ThemeManager::get().applyTheme (td);
                populatePresetCombo();
                m_presetCombo.setText (name, juce::dontSendNotification);
            }
        }
        delete w;
    }), false);  // false: we delete manually in callback
}

void ThemeEditorPanel::onResetClicked()
{
    ThemeManager::get().applyTheme (ThemePresets::presetByName ("Espresso"));
}

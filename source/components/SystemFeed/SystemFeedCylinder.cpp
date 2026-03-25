#include "SystemFeedCylinder.h"

//==============================================================================
SystemFeedCylinder::SystemFeedCylinder()
{
    // Load user preferences
    juce::PropertiesFile::Options opts;
    opts.applicationName     = "SPOOL";
    opts.filenameSuffix      = ".settings";
    opts.osxLibrarySubFolder = "Application Support";
    m_props = std::make_unique<juce::PropertiesFile> (opts);

    m_showMidiIn    = m_props->getBoolValue ("feedMidiIn",    true);
    m_showMidiOut   = m_props->getBoolValue ("feedMidiOut",   true);
    m_showSystem    = m_props->getBoolValue ("feedSystem",    true);
    m_showTransport = m_props->getBoolValue ("feedTransport", true);
    m_showLink      = m_props->getBoolValue ("feedLink",      true);

    startTimerHz (30);
}

SystemFeedCylinder::~SystemFeedCylinder()
{
    stopTimer();
}

//==============================================================================
void SystemFeedCylinder::addMessage (const juce::String& text, juce::Colour color)
{
    const auto f = Theme::Font::micro();

    SystemMessage msg;
    msg.text      = text;
    msg.color     = color;
    msg.timestamp = juce::Time::getMillisecondCounterHiRes();
    msg.width     = juce::GlyphArrangement::getStringWidth (f, text);

    // Position relative to the tape surface (excludes the left tab)
    const float tapeW      = static_cast<float> (juce::jmax (getWidth() - kTabW, 0));
    const float rightEdge  = tapeW + m_scrollOffsetPx;

    if (m_messages.isEmpty())
    {
        msg.baseX = juce::jmax (rightEdge, tapeW > 0.0f ? tapeW : 400.0f);
    }
    else
    {
        const auto& last = m_messages.getLast();
        msg.baseX = juce::jmax (last.baseX + last.width + kMsgGap, rightEdge);
    }

    m_messages.add (msg);

    if (m_messages.size() > 50)
        m_messages.remove (0);
}

//==============================================================================
void SystemFeedCylinder::timerCallback()
{
    m_scrollOffsetPx += m_scrollSpeed / 30.0f;

    // Remove messages that have fully scrolled off the left edge
    while (!m_messages.isEmpty())
    {
        const auto& first = m_messages.getFirst();
        if (first.baseX + first.width - m_scrollOffsetPx < 0.0f)
            m_messages.remove (0);
        else
            break;
    }

    // Keep queue fed when empty
    if (m_messages.isEmpty())
        addMessage ("SPOOL  ready  44.1kHz  120 BPM");

    repaint();
}

//==============================================================================
void SystemFeedCylinder::resized()
{
    // Seed the queue on first layout so something is visible immediately
    if (m_messages.isEmpty())
        addMessage ("SPOOL  ready  44.1kHz  120 BPM");
}

//==============================================================================
void SystemFeedCylinder::paint (juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    // Housing background for the whole strip
    g.setColour (TapeColors::housingEdge);
    g.fillRect (0, 0, w, h);

    // Left label tab
    paintTab (g, h);

    // Tape surface — clipped to x=kTabW, coordinates translated so 0 = tape left edge
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (kTabW, 0, w - kTabW, h);
        g.setOrigin (kTabW, 0);

        const int   tapeW = w - kTabW;
        const float fw    = static_cast<float> (tapeW);

        // Cylindrical diffuse base — cos(θ) model matching CylinderBand
        juce::ColourGradient base (
            TapeColors::tapeBase.darker  (0.58f), 0.0f, 0.0f,
            TapeColors::tapeBase.darker  (0.58f), fw,   0.0f,
            false);
        base.addColour (0.07, TapeColors::tapeBase.darker  (0.42f));
        base.addColour (0.17, TapeColors::tapeBase.darker  (0.18f));
        base.addColour (0.40, TapeColors::tapeBase.brighter (0.05f));
        base.addColour (0.50, TapeColors::tapeBase.brighter (0.24f));
        base.addColour (0.60, TapeColors::tapeBase.brighter (0.05f));
        base.addColour (0.83, TapeColors::tapeBase.darker  (0.18f));
        base.addColour (0.93, TapeColors::tapeBase.darker  (0.42f));
        g.setGradientFill (base);
        g.fillRect (0, 0, tapeW, h);

        // Scrolling messages
        paintMessages (g, tapeW, h);

        // Specular highlight — warm crown line
        paintSpecularHighlight (g, fw * 0.5f, h);

        // Cylinder rims, fades, shadows
        paintRimsAndFades (g, tapeW, h);
    }
}

//==============================================================================
void SystemFeedCylinder::paintTab (juce::Graphics& g, int h) const
{
    // Tab background
    g.setColour (Theme::Colour::surface0);
    g.fillRect (0, 0, kTabW, h);

    // Right border
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine (kTabW - 0.5f, 0.0f, kTabW - 0.5f, static_cast<float> (h), 0.5f);

    // Spool icon — two flanges (circles) side by side with a thin hub connector
    const float lfcx  = 9.0f;    // left flange centre x
    const float rfcx  = 27.0f;   // right flange centre x
    const float iccy  = 9.0f;    // flange centre y (upper half, leaving room for text)
    const float flgR  = 7.0f;    // outer flange radius
    const float hubR  = 3.0f;    // hub fill radius
    const float connH = 2.0f;    // tape connector height

    g.setColour (Theme::Colour::accent);

    // Left flange outline
    g.drawEllipse (lfcx - flgR, iccy - flgR, flgR * 2.0f, flgR * 2.0f, 1.5f);
    // Right flange outline
    g.drawEllipse (rfcx - flgR, iccy - flgR, flgR * 2.0f, flgR * 2.0f, 1.5f);

    // Hub connector (tape between flanges)
    const float connLeft  = lfcx + flgR;
    const float connRight = rfcx - flgR;
    g.fillRect (connLeft, iccy - connH * 0.5f, connRight - connLeft, connH);

    // Hub circles (axle)
    g.fillEllipse (lfcx - hubR, iccy - hubR, hubR * 2.0f, hubR * 2.0f);
    g.fillEllipse (rfcx - hubR, iccy - hubR, hubR * 2.0f, hubR * 2.0f);

    // "FEED" label below icon
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  ("FEED",
                 juce::Rectangle<float> (0.0f, iccy + flgR + 2.0f,
                                         static_cast<float> (kTabW), 12.0f).toNearestInt(),
                 juce::Justification::centred, false);
}

//==============================================================================
void SystemFeedCylinder::paintMessages (juce::Graphics& g, int tapeW, int tapeH) const
{
    if (m_messages.isEmpty())
        return;

    const auto f = Theme::Font::micro();
    g.setFont (f);

    const float fw     = static_cast<float> (tapeW);
    const float fh     = static_cast<float> (tapeH);
    const float fadeL  = static_cast<float> (kRimW + kFadeW);
    const float fadeR  = fw - static_cast<float> (kRimW + kFadeW);

    for (const auto& msg : m_messages)
    {
        const float renderX = msg.baseX - m_scrollOffsetPx;

        if (renderX > fw)
            continue;   // not yet on screen

        if (renderX + msg.width < 0.0f)
            continue;   // already scrolled off

        // Fade alpha near left/right edges to merge into the cylinder gradient
        float alpha = 1.0f;
        if (renderX < fadeL)
            alpha = juce::jlimit (0.0f, 1.0f, (renderX - static_cast<float> (kRimW))
                                               / static_cast<float> (kFadeW));
        if (renderX + msg.width > fadeR)
            alpha = juce::jmin (alpha, juce::jlimit (0.0f, 1.0f,
                                (fw - static_cast<float> (kRimW) - renderX) / static_cast<float> (kFadeW)));

        g.setColour (msg.color.withAlpha (alpha));
        g.drawText  (msg.text,
                     juce::Rectangle<float> (renderX, 0.0f, msg.width + 4.0f, fh).toNearestInt(),
                     juce::Justification::centredLeft, false);
    }
}

//==============================================================================
void SystemFeedCylinder::paintSpecularHighlight (juce::Graphics& g, float centerX, int tapeH) const
{
    // Narrow warm-white specular line at the crown — same as CylinderBand
    const float coreY = 5.0f;
    const float coreH = static_cast<float> (tapeH) - coreY * 2.0f;
    if (coreH <= 0.0f)
        return;

    const float glowW = 10.0f;
    juce::ColourGradient penumbra (
        juce::Colour::fromRGBA (255, 248, 228, 0),
        centerX - glowW, 0.0f,
        juce::Colour::fromRGBA (255, 248, 228, 0),
        centerX + glowW, 0.0f,
        false);
    penumbra.addColour (0.40, juce::Colour::fromRGBA (255, 248, 228, 18));
    penumbra.addColour (0.50, juce::Colour::fromRGBA (255, 248, 228, 42));
    penumbra.addColour (0.60, juce::Colour::fromRGBA (255, 248, 228, 18));
    g.setGradientFill (penumbra);
    g.fillRect (centerX - glowW, coreY, glowW * 2.0f, coreH);

    g.setColour (juce::Colour::fromRGBA (255, 252, 238, 118));
    g.fillRect (centerX - 0.75f, coreY, 1.5f, coreH);
}

//==============================================================================
void SystemFeedCylinder::paintRimsAndFades (juce::Graphics& g, int tapeW, int tapeH) const
{
    const juce::Colour edge = TapeColors::housingEdge;

    // === RIM BEVEL — left (inner highlight suggests housing depth) ===
    g.setColour (juce::Colour (0xFF040302));   // outer very dark
    g.fillRect (0, 0, 2, tapeH);
    g.setColour (juce::Colour (0xFF2E2014));   // mid bevel catch-light
    g.fillRect (2, 0, 1, tapeH);
    g.setColour (juce::Colour (0xFF0C0806));   // inner dark
    g.fillRect (3, 0, 2, tapeH);

    // === RIM BEVEL — right (mirror) ===
    g.setColour (juce::Colour (0xFF0C0806));
    g.fillRect (tapeW - 5, 0, 2, tapeH);
    g.setColour (juce::Colour (0xFF2E2014));
    g.fillRect (tapeW - 3, 0, 1, tapeH);
    g.setColour (juce::Colour (0xFF040302));
    g.fillRect (tapeW - 2, 0, 2, tapeH);

    // === EDGE FADES — left ===
    {
        juce::ColourGradient gradL (juce::Colour::fromRGBA (12, 8, 3, 195),
                                    static_cast<float> (kRimW), 0.0f,
                                    juce::Colours::transparentBlack,
                                    static_cast<float> (kRimW + kFadeW), 0.0f,
                                    false);
        gradL.addColour (0.14, juce::Colour::fromRGBA (12, 8, 3, 128));
        gradL.addColour (0.36, juce::Colour::fromRGBA (12, 8, 3,  52));
        gradL.addColour (0.65, juce::Colour::fromRGBA (12, 8, 3,  14));
        g.setGradientFill (gradL);
        g.fillRect (kRimW, 0, kFadeW, tapeH);
    }
    {
        const int x1 = tapeW - kRimW - kFadeW;
        juce::ColourGradient gradR (juce::Colours::transparentBlack,
                                    static_cast<float> (x1), 0.0f,
                                    juce::Colour::fromRGBA (12, 8, 3, 195),
                                    static_cast<float> (tapeW - kRimW), 0.0f,
                                    false);
        gradR.addColour (0.35, juce::Colour::fromRGBA (12, 8, 3,  14));
        gradR.addColour (0.64, juce::Colour::fromRGBA (12, 8, 3,  52));
        gradR.addColour (0.86, juce::Colour::fromRGBA (12, 8, 3, 128));
        g.setGradientFill (gradR);
        g.fillRect (x1, 0, kFadeW, tapeH);
    }

    // === INSET RECESS SHADOW — top ===
    {
        const int recessH = juce::jmin (kShadowH, tapeH / 3);
        g.setColour (edge);
        g.fillRect (0, 0, tapeW, 1);
        juce::ColourGradient cast;
        cast.isRadial = false;
        cast.point1   = { 0.0f, 1.0f };
        cast.point2   = { 0.0f, static_cast<float> (recessH) };
        cast.addColour (0.00, edge.withAlpha (0.92f));
        cast.addColour (0.14, edge.withAlpha (0.72f));
        cast.addColour (0.30, edge.withAlpha (0.48f));
        cast.addColour (0.52, edge.withAlpha (0.22f));
        cast.addColour (0.74, edge.withAlpha (0.07f));
        cast.addColour (1.00, edge.withAlpha (0.00f));
        g.setGradientFill (cast);
        g.fillRect (0, 1, tapeW, recessH - 1);
    }

    // === INSET RECESS SHADOW — bottom (mirror) ===
    {
        const int recessH = juce::jmin (kShadowH, tapeH / 3);
        g.setColour (edge);
        g.fillRect (0, tapeH - 1, tapeW, 1);
        juce::ColourGradient cast;
        cast.isRadial = false;
        cast.point1   = { 0.0f, static_cast<float> (tapeH - 1) };
        cast.point2   = { 0.0f, static_cast<float> (tapeH - recessH) };
        cast.addColour (0.00, edge.withAlpha (0.92f));
        cast.addColour (0.14, edge.withAlpha (0.72f));
        cast.addColour (0.30, edge.withAlpha (0.48f));
        cast.addColour (0.52, edge.withAlpha (0.22f));
        cast.addColour (0.74, edge.withAlpha (0.07f));
        cast.addColour (1.00, edge.withAlpha (0.00f));
        g.setGradientFill (cast);
        g.fillRect (0, tapeH - recessH, tapeW, recessH - 1);
    }

    // === OUTER RIM LINES ===
    g.setColour (edge.withAlpha (0.99f));
    g.drawHorizontalLine (0,          0.0f, static_cast<float> (tapeW));
    g.drawHorizontalLine (tapeH - 1,  0.0f, static_cast<float> (tapeW));
}

//==============================================================================
void SystemFeedCylinder::mouseDown (const juce::MouseEvent& e)
{
    // Ignore clicks on the left tab area
    if (e.getPosition().getX() < kTabW)
        return;

    if (!e.mods.isRightButtonDown())
        return;

    juce::PopupMenu menu;
    menu.addItem (1, "MIDI In",     true, m_showMidiIn);
    menu.addItem (2, "MIDI Out",    true, m_showMidiOut);
    menu.addItem (3, "System Info", true, m_showSystem);
    menu.addItem (4, "Transport",   true, m_showTransport);
    menu.addItem (5, "Link Status", true, m_showLink);

    menu.showMenuAsync (juce::PopupMenu::Options{},
        [this] (int result)
        {
            bool changed = false;
            switch (result)
            {
                case 1: m_showMidiIn    = !m_showMidiIn;    m_props->setValue ("feedMidiIn",    m_showMidiIn);    changed = true; break;
                case 2: m_showMidiOut   = !m_showMidiOut;   m_props->setValue ("feedMidiOut",   m_showMidiOut);   changed = true; break;
                case 3: m_showSystem    = !m_showSystem;    m_props->setValue ("feedSystem",    m_showSystem);    changed = true; break;
                case 4: m_showTransport = !m_showTransport; m_props->setValue ("feedTransport", m_showTransport); changed = true; break;
                case 5: m_showLink      = !m_showLink;      m_props->setValue ("feedLink",      m_showLink);      changed = true; break;
                default: break;
            }
            if (changed)
                m_props->saveIfNeeded();
        });
}

#include "ReelEditorPanel.h"

//==============================================================================
ReelEditorPanel::ReelEditorPanel (int slotIndex)
    : m_slotIndex (slotIndex)
{
    startTimerHz (20);   // grain cloud + playhead refresh
}

ReelEditorPanel::~ReelEditorPanel()
{
    stopTimer();
}

//==============================================================================
void ReelEditorPanel::setProcessor (ReelProcessor* proc)
{
    m_proc = proc;
    repaint();
}

//==============================================================================
// Layout
//==============================================================================

void ReelEditorPanel::resized()
{
    const int w = getWidth();
    const int h = getHeight();
    int y = 0;

    m_loadRect  = { 0, y, w, kLoadH };   y += kLoadH;
    m_modeRect  = { 0, y, w, kModeH };   y += kModeH;

    const int remaining = juce::jmax (0, h - y - kPosRowH);
    const int waveH     = juce::jmax (40, remaining * kWaveFrac / 100);
    const int paramsH   = juce::jmax (0, remaining - waveH);

    m_waveRect    = { 0, y, w, waveH };             y += waveH;
    m_posRow      = { 0, y, w, kPosRowH };          y += kPosRowH;
    m_paramsRect  = { 0, y, w, paramsH };
}

//==============================================================================
// Coordinate helpers
//==============================================================================

float ReelEditorPanel::normalToWaveX (float norm) const noexcept
{
    const float padL = 6.0f;
    const float usable = static_cast<float> (m_waveRect.getWidth()) - padL * 2.0f;
    return static_cast<float> (m_waveRect.getX()) + padL + norm * usable;
}

float ReelEditorPanel::waveXToNormal (float x) const noexcept
{
    const float padL   = 6.0f;
    const float left   = static_cast<float> (m_waveRect.getX()) + padL;
    const float usable = static_cast<float> (m_waveRect.getWidth()) - padL * 2.0f;
    return juce::jlimit (0.0f, 1.0f, (x - left) / usable);
}

bool ReelEditorPanel::hitTestHandle (float x, float handleNorm) const noexcept
{
    return std::abs (x - normalToWaveX (handleNorm)) <= static_cast<float> (kHandleHit);
}

int ReelEditorPanel::modePillAt (juce::Point<int> pos) const noexcept
{
    if (!m_modeRect.contains (pos))
        return -1;

    const int pillW = m_modeRect.getWidth() / 4;
    const int idx   = (pos.x - m_modeRect.getX()) / pillW;
    return juce::jlimit (0, 3, idx);
}

bool ReelEditorPanel::hitLoadBtn (juce::Point<int> pos) const noexcept
{
    const juce::Rectangle<int> btn (m_loadRect.getX() + 4, m_loadRect.getY() + 6, 80, 20);
    return btn.contains (pos);
}

bool ReelEditorPanel::hitClearBtn (juce::Point<int> pos) const noexcept
{
    const juce::Rectangle<int> btn (m_loadRect.getRight() - 52, m_loadRect.getY() + 6, 48, 20);
    return btn.contains (pos);
}

//==============================================================================
// Paint
//==============================================================================

void ReelEditorPanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);
    paintLoadStrip  (g);
    paintModeStrip  (g);
    paintWaveform   (g);
    paintPosRow     (g);
    paintModeParams (g);
}

//------------------------------------------------------------------------------
void ReelEditorPanel::paintLoadStrip (juce::Graphics& g) const
{
    const auto& r = m_loadRect;

    // Divider
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawHorizontalLine (r.getBottom() - 1,
                          static_cast<float> (r.getX()),
                          static_cast<float> (r.getRight()));

    // [LOAD FILE] button
    const juce::Rectangle<int> loadBtn (r.getX() + 4, r.getY() + 6, 80, 20);
    g.setColour (Theme::Colour::surface2);
    g.fillRoundedRectangle (loadBtn.toFloat(), Theme::Radius::chip);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRoundedRectangle (loadBtn.toFloat(), Theme::Radius::chip, Theme::Stroke::subtle);
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkLight);
    g.drawText ("LOAD FILE", loadBtn, juce::Justification::centred, false);

    // Source info
    const juce::String sourceText = (m_proc && m_proc->isLoaded())
        ? ("Loaded \xc2\xb7 "
           + juce::String (m_proc->getBuffer().getDuration(), 1) + "s  "
           + juce::String (m_proc->getBuffer().getNumChannels()) + "ch")
        : "No audio loaded";

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText (sourceText,
                r.withLeft (loadBtn.getRight() + 8).withRight (r.getRight() - 56),
                juce::Justification::centredLeft, true);

    // [CLEAR] button
    const juce::Rectangle<int> clearBtn (r.getRight() - 52, r.getY() + 6, 48, 20);
    g.setColour (Theme::Colour::surface1);
    g.fillRoundedRectangle (clearBtn.toFloat(), Theme::Radius::chip);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRoundedRectangle (clearBtn.toFloat(), Theme::Radius::chip, Theme::Stroke::subtle);
    g.setFont (Theme::Font::micro());
    g.setColour (m_proc && m_proc->isLoaded() ? (juce::Colour) Theme::Colour::inkMid
                                              : (juce::Colour) Theme::Colour::inkGhost);
    g.drawText ("CLEAR", clearBtn, juce::Justification::centred, false);
}

//------------------------------------------------------------------------------
void ReelEditorPanel::paintModeStrip (juce::Graphics& g) const
{
    const auto& r = m_modeRect;
    const int pillW = r.getWidth() / 4;

    static const char* kLabels[] = { "LOOP", "SAMPLE", "GRAIN", "SLICE" };
    static const ReelMode kModes[] = { ReelMode::loop, ReelMode::sample,
                                       ReelMode::grain, ReelMode::slice };

    const ReelMode current = m_proc ? m_proc->getMode() : ReelMode::loop;

    for (int i = 0; i < 4; ++i)
    {
        const juce::Rectangle<int> pill (r.getX() + i * pillW + 2, r.getY() + 4,
                                         pillW - 4, r.getHeight() - 8);
        const bool active = (kModes[i] == current);

        g.setColour (active ? (juce::Colour) Theme::Zone::a.withAlpha (0.85f)
                            : (juce::Colour) Theme::Colour::surface3);
        g.fillRoundedRectangle (pill.toFloat(), Theme::Radius::sm);
        g.setColour (active ? (juce::Colour) Theme::Colour::inkDark
                            : (juce::Colour) Theme::Colour::inkGhost);
        g.setFont (Theme::Font::micro());
        g.drawText (kLabels[i], pill, juce::Justification::centred, false);
    }

    // Bottom divider
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawHorizontalLine (r.getBottom() - 1,
                          static_cast<float> (r.getX()),
                          static_cast<float> (r.getRight()));
}

//------------------------------------------------------------------------------
void ReelEditorPanel::paintWaveform (juce::Graphics& g) const
{
    const auto& wr = m_waveRect;
    if (wr.isEmpty()) return;
    const auto& theme = ThemeManager::get().theme();

    const float rx = static_cast<float> (wr.getX());
    const float ry = static_cast<float> (wr.getY());
    const float fw = static_cast<float> (wr.getWidth());
    const float fh = static_cast<float> (wr.getHeight());

    // ── Tape base (same cylinder aesthetic as CylinderBand) ─────────────────
    juce::ColourGradient base (
        theme.tapeBase.darker  (0.58f), rx,      ry,
        theme.tapeBase.darker  (0.58f), rx + fw, ry,
        false);
    base.addColour (0.07, theme.tapeBase.darker  (0.42f));
    base.addColour (0.17, theme.tapeBase.darker  (0.18f));
    base.addColour (0.40, theme.tapeBase.brighter (0.05f));
    base.addColour (0.50, theme.tapeBase.brighter (0.24f));
    base.addColour (0.60, theme.tapeBase.brighter (0.05f));
    base.addColour (0.83, theme.tapeBase.darker  (0.18f));
    base.addColour (0.93, theme.tapeBase.darker  (0.42f));
    g.setGradientFill (base);
    g.fillRect (wr);

    if (m_proc == nullptr || !m_proc->isLoaded())
    {
        // Empty state
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkDark.interpolatedWith (theme.tapeBase, 0.16f).withAlpha (0.5f));
        g.drawText ("EMPTY — load audio to begin",
                    wr, juce::Justification::centred, false);
    }
    else
    {
        const auto& buf = m_proc->getBuffer().getBuffer();
        const int   numSamples = buf.getNumSamples();
        const int   numCh      = buf.getNumChannels();

        // ── Waveform path (downsampled to waveRect width) ───────────────────
        if (numSamples > 0 && numCh > 0)
        {
            const float waveStartX = rx + 6.0f;
            const float waveW      = fw - 12.0f;
            const float midY       = ry + fh * 0.5f;
            const float maxH       = fh * 0.40f;
            const int   wPixels    = juce::jmax (1, static_cast<int> (waveW));

            juce::Path p;
            p.startNewSubPath (waveStartX, midY);

            for (int px = 0; px < wPixels; ++px)
            {
                const int sStart = (px * numSamples) / wPixels;
                const int sEnd   = juce::jmin (((px + 1) * numSamples) / wPixels, numSamples);

                float rms = 0.0f;
                for (int s = sStart; s < sEnd; ++s)
                {
                    float v = buf.getSample (0, s);
                    if (numCh > 1) v = (v + buf.getSample (1, s)) * 0.5f;
                    rms += v * v;
                }
                rms = (sEnd > sStart) ? std::sqrt (rms / static_cast<float> (sEnd - sStart)) : 0.0f;

                const float x = waveStartX + static_cast<float> (px);
                p.lineTo (x, midY - rms * maxH);
            }
            for (int px = wPixels - 1; px >= 0; --px)
            {
                const int sStart = (px * numSamples) / wPixels;
                const int sEnd   = juce::jmin (((px + 1) * numSamples) / wPixels, numSamples);

                float rms = 0.0f;
                for (int s = sStart; s < sEnd; ++s)
                {
                    float v = buf.getSample (0, s);
                    if (numCh > 1) v = (v + buf.getSample (1, s)) * 0.5f;
                    rms += v * v;
                }
                rms = (sEnd > sStart) ? std::sqrt (rms / static_cast<float> (sEnd - sStart)) : 0.0f;

                const float x = waveStartX + static_cast<float> (px);
                p.lineTo (x, midY + rms * maxH);
            }
            p.closeSubPath();

            g.setColour (Theme::Zone::a.withAlpha (0.65f));
            g.fillPath (p);
            g.setColour (Theme::Zone::a.withAlpha (0.85f));
            g.strokePath (p, juce::PathStrokeType (Theme::Stroke::normal));
        }

        const ReelParams& params = m_proc->getParams();
        const ReelMode    mode   = params.mode;

        // ── Loop-region shading ──────────────────────────────────────────────
        if (mode == ReelMode::loop || mode == ReelMode::sample)
        {
            const float startX = normalToWaveX (params.play.start);
            const float endX   = normalToWaveX (params.play.end);
            if (endX > startX)
            {
                g.setColour (Theme::Zone::b.withAlpha (0.12f));
                g.fillRect (startX, ry, endX - startX, fh);
            }
        }

        // ── Slice markers ───────────────────────────────────────────────────
        if (mode == ReelMode::slice)
        {
            const auto slices = m_proc->getBuffer().isLoaded()
                                ? m_proc->getParams().slice.count : 0;
            if (slices > 0)
            {
                g.setColour (Theme::Zone::d.withAlpha (0.60f));
                for (int i = 0; i <= slices; ++i)
                {
                    const float norm = m_proc->getParams().play.start
                                       + static_cast<float> (i) / static_cast<float> (slices)
                                         * (m_proc->getParams().play.end
                                            - m_proc->getParams().play.start);
                    const float sx = normalToWaveX (norm);
                    g.drawLine (sx, ry, sx, ry + fh, 1.0f);
                }
            }
        }

        // ── Grain cloud dots ────────────────────────────────────────────────
        if (mode == ReelMode::grain && m_numGrainDots > 0)
        {
            g.setColour (Theme::Zone::a.withAlpha (0.80f));
            for (int i = 0; i < m_numGrainDots; ++i)
            {
                const float gx = normalToWaveX (m_grainPositions[i]);
                g.fillEllipse (gx - 2.0f, ry + fh * 0.5f - 2.0f, 4.0f, 4.0f);
            }
        }

        // ── Start handle ────────────────────────────────────────────────────
        {
            const float sx = normalToWaveX (params.play.start);
            g.setColour (Theme::Zone::c);
            g.drawLine (sx, ry, sx, ry + fh, 1.5f);
            juce::Path tri;
            tri.addTriangle (sx - 4.0f, ry, sx + 4.0f, ry, sx, ry + 6.0f);
            g.fillPath (tri);
        }

        // ── End handle ──────────────────────────────────────────────────────
        {
            const float ex = normalToWaveX (params.play.end);
            g.setColour (Theme::Zone::d);
            g.drawLine (ex, ry, ex, ry + fh, 1.5f);
            juce::Path tri;
            tri.addTriangle (ex - 4.0f, ry, ex + 4.0f, ry, ex, ry + 6.0f);
            g.fillPath (tri);
        }

        // ── Grain position handle ────────────────────────────────────────────
        if (mode == ReelMode::grain)
        {
            const float gx = normalToWaveX (params.grain.position);
            g.setColour (Theme::Zone::a.withAlpha (0.9f));
            g.drawLine (gx, ry, gx, ry + fh, 1.5f);
        }
    }

    // ── Cylindrical rims and fades (drawn last, over content) ───────────────
    {
        const juce::Colour edge = theme.housingEdge;
        constexpr int kRimW  = 5;
        constexpr int kFadeW = 80;   // narrower for a panel — less depth needed
        const int iw = wr.getWidth();
        const int ih = wr.getHeight();

        // Rim bevel — left
        g.setColour (juce::Colour (0xFF040302));
        g.fillRect (wr.getX(),     wr.getY(), 2, ih);
        g.setColour (juce::Colour (0xFF2E2014));
        g.fillRect (wr.getX() + 2, wr.getY(), 1, ih);
        g.setColour (juce::Colour (0xFF0C0806));
        g.fillRect (wr.getX() + 3, wr.getY(), 2, ih);

        // Rim bevel — right
        g.setColour (juce::Colour (0xFF0C0806));
        g.fillRect (wr.getRight() - 5, wr.getY(), 2, ih);
        g.setColour (juce::Colour (0xFF2E2014));
        g.fillRect (wr.getRight() - 3, wr.getY(), 1, ih);
        g.setColour (juce::Colour (0xFF040302));
        g.fillRect (wr.getRight() - 2, wr.getY(), 2, ih);

        // Edge fades
        {
            juce::ColourGradient gl (juce::Colour::fromRGBA (12, 8, 3, 180),
                                     rx + kRimW, ry,
                                     juce::Colours::transparentBlack,
                                     rx + kRimW + kFadeW, ry, false);
            gl.addColour (0.20, juce::Colour::fromRGBA (12, 8, 3, 100));
            gl.addColour (0.55, juce::Colour::fromRGBA (12, 8, 3,  30));
            g.setGradientFill (gl);
            g.fillRect (wr.getX() + kRimW, wr.getY(), kFadeW, ih);
        }
        {
            const int x1 = wr.getRight() - kRimW - kFadeW;
            juce::ColourGradient gr (juce::Colours::transparentBlack,
                                     static_cast<float> (x1), ry,
                                     juce::Colour::fromRGBA (12, 8, 3, 180),
                                     rx + fw - kRimW, ry, false);
            gr.addColour (0.45, juce::Colour::fromRGBA (12, 8, 3,  30));
            gr.addColour (0.80, juce::Colour::fromRGBA (12, 8, 3, 100));
            g.setGradientFill (gr);
            g.fillRect (x1, wr.getY(), kFadeW, ih);
        }

        // Recess shadow — top
        {
            constexpr int shadowH = 12;
            g.setColour (edge);
            g.fillRect (wr.getX(), wr.getY(), iw, 1);
            juce::ColourGradient cast;
            cast.isRadial = false;
            cast.point1   = { rx, ry + 1.0f };
            cast.point2   = { rx, ry + shadowH };
            cast.addColour (0.00, edge.withAlpha (0.88f));
            cast.addColour (0.35, edge.withAlpha (0.40f));
            cast.addColour (0.70, edge.withAlpha (0.12f));
            cast.addColour (1.00, edge.withAlpha (0.00f));
            g.setGradientFill (cast);
            g.fillRect (wr.getX(), wr.getY() + 1, iw, shadowH - 1);
        }

        // Recess shadow — bottom
        {
            constexpr int shadowH = 12;
            g.setColour (edge);
            g.fillRect (wr.getX(), wr.getBottom() - 1, iw, 1);
            juce::ColourGradient cast;
            cast.isRadial = false;
            cast.point1   = { rx, ry + fh - 1.0f };
            cast.point2   = { rx, ry + fh - shadowH };
            cast.addColour (0.00, edge.withAlpha (0.88f));
            cast.addColour (0.35, edge.withAlpha (0.40f));
            cast.addColour (0.70, edge.withAlpha (0.12f));
            cast.addColour (1.00, edge.withAlpha (0.00f));
            g.setGradientFill (cast);
            g.fillRect (wr.getX(), wr.getBottom() - shadowH, iw, shadowH - 1);
        }

        // Outer rim lines
        g.setColour (edge.withAlpha (0.99f));
        g.drawHorizontalLine (wr.getY(),          rx, rx + fw);
        g.drawHorizontalLine (wr.getBottom() - 1, rx, rx + fw);
    }
}

//------------------------------------------------------------------------------
void ReelEditorPanel::paintPosRow (juce::Graphics& g) const
{
    const auto& r = m_posRow;
    g.setColour (Theme::Colour::surface1);
    g.fillRect (r);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawHorizontalLine (r.getBottom() - 1,
                          static_cast<float> (r.getX()),
                          static_cast<float> (r.getRight()));

    if (m_proc == nullptr || !m_proc->isLoaded())
        return;

    const auto&  params    = m_proc->getParams();
    const double durSec    = m_proc->getBuffer().getDuration();
    const double startSec  = params.play.start * durSec;
    const double endSec    = params.play.end   * durSec;
    const double regionSec = endSec - startSec;

    const int colW = r.getWidth() / 3;
    g.setFont (Theme::Font::micro());

    // START
    g.setColour (Theme::Zone::c);
    g.drawText ("START  " + juce::String (startSec, 2) + "s",
                juce::Rectangle<int> (r.getX(), r.getY(), colW, r.getHeight()),
                juce::Justification::centredLeft, false);

    // DURATION
    g.setColour (Theme::Colour::inkMid);
    g.drawText (juce::String (regionSec, 2) + "s",
                juce::Rectangle<int> (r.getX() + colW, r.getY(), colW, r.getHeight()),
                juce::Justification::centred, false);

    // END
    g.setColour (Theme::Zone::d);
    g.drawText ("END  " + juce::String (endSec, 2) + "s",
                juce::Rectangle<int> (r.getX() + colW * 2, r.getY(), colW, r.getHeight()),
                juce::Justification::centredRight, false);
}

//------------------------------------------------------------------------------
void ReelEditorPanel::paintModeParams (juce::Graphics& g) const
{
    const auto& r = m_paramsRect;
    if (r.isEmpty()) return;

    g.setColour (Theme::Colour::surface0);
    g.fillRect (r);

    if (m_proc == nullptr)
        return;

    const auto& p    = m_proc->getParams();
    const ReelMode m = p.mode;
    g.setFont (Theme::Font::micro());

    // Build display lines based on mode
    struct ParamLine { juce::String label; juce::String value; };
    juce::Array<ParamLine> lines;

    if (m == ReelMode::loop)
    {
        lines.add ({ "SPEED",   juce::String (p.loop.speed, 2) + "\xc3\x97" });
        lines.add ({ "PITCH",   (p.loop.pitch >= 0 ? "+" : "") + juce::String (p.loop.pitch, 1) + " st" });
        lines.add ({ "SYNC",    p.loop.sync ? "ON" : "OFF" });
        lines.add ({ "REVERSE", p.play.reverse ? "ON" : "OFF" });
    }
    else if (m == ReelMode::grain)
    {
        lines.add ({ "POSN",    juce::String (p.grain.position, 2) });
        lines.add ({ "SIZE",    juce::String (p.grain.size, 0) + " ms" });
        lines.add ({ "DENSITY", juce::String (p.grain.density, 0) + " /s" });
        lines.add ({ "PITCH",   (p.grain.pitch >= 0 ? "+" : "") + juce::String (p.grain.pitch, 1) + " st" });
        lines.add ({ "SCATTER", juce::String (p.grain.scatter, 2) });
        lines.add ({ "SPREAD",  juce::String (p.grain.spread, 2) });
        static const char* kEnvNames[] = { "Hann", "Linear", "Gaussian" };
        lines.add ({ "ENV",     kEnvNames[juce::jlimit (0, 2, p.grain.envelope)] });
    }
    else if (m == ReelMode::sample)
    {
        const juce::String rootName = juce::MidiMessage::getMidiNoteName (
            p.sample.root, true, true, 3);
        lines.add ({ "ROOT",    rootName + " (" + juce::String (p.sample.root) + ")" });
        lines.add ({ "ONE-SHOT",p.sample.oneshot ? "ON" : "OFF" });
        lines.add ({ "INTERP",  p.sample.interp == 0 ? "Linear" : "Cubic" });
        lines.add ({ "REVERSE", p.play.reverse ? "ON" : "OFF" });
    }
    else if (m == ReelMode::slice)
    {
        static const char* kOrderNames[] = { "Forward", "Reverse", "Random", "Shuffle" };
        lines.add ({ "SLICES",  juce::String (p.slice.count) });
        lines.add ({ "ORDER",   kOrderNames[juce::jlimit (0, 3, p.slice.order)] });
        lines.add ({ "QUANTIZE",p.slice.quantize ? "ON" : "OFF" });
    }

    const int rowH = 18;
    int y = r.getY() + 4;

    for (const auto& line : lines)
    {
        const juce::Rectangle<int> row (r.getX() + 8, y, r.getWidth() - 16, rowH);
        g.setColour (Theme::Colour::inkGhost);
        g.drawText (line.label, row.withWidth (60), juce::Justification::centredLeft, false);
        g.setColour (Theme::Colour::inkLight);
        g.drawText (line.value, row.withLeft (row.getX() + 68),
                    juce::Justification::centredLeft, false);
        y += rowH;
    }
}

//==============================================================================
// Mouse
//==============================================================================

void ReelEditorPanel::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // Mode pill
    const int pill = modePillAt (pos);
    if (pill >= 0)
    {
        if (m_proc)
        {
            static const ReelMode kModes[] = { ReelMode::loop, ReelMode::sample,
                                               ReelMode::grain, ReelMode::slice };
            m_proc->setMode (kModes[pill]);
            if (onParamChanged)
                onParamChanged ("mode.current", static_cast<float> (pill));
        }
        repaint();
        return;
    }

    // Load file button
    if (hitLoadBtn (pos))
    {
        auto chooser = std::make_shared<juce::FileChooser> (
            "Load audio into REEL",
            juce::File::getSpecialLocation (juce::File::userMusicDirectory),
            "*.wav;*.aif;*.aiff;*.flac;*.mp3");

        chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser] (const juce::FileChooser& fc)
            {
                const auto results = fc.getResults();
                if (results.isEmpty() || m_proc == nullptr)
                    return;

                if (m_proc->loadFromFile (results[0]))
                {
                    if (onParamChanged)
                        onParamChanged ("buffer.loaded", 1.0f);
                    repaint();
                }
            });
        return;
    }

    // Clear button
    if (hitClearBtn (pos))
    {
        if (m_proc)
        {
            m_proc->clearBuffer();
            repaint();
        }
        return;
    }

    // Waveform handles
    if (m_proc && m_proc->isLoaded() && m_waveRect.contains (pos))
    {
        const float fx = static_cast<float> (pos.x);
        const auto& p = m_proc->getParams();

        if (hitTestHandle (fx, p.play.start))
            m_dragHandle = HandleDrag::Start;
        else if (hitTestHandle (fx, p.play.end))
            m_dragHandle = HandleDrag::End;
        else if (p.mode == ReelMode::grain && hitTestHandle (fx, p.grain.position))
            m_dragHandle = HandleDrag::GrainPos;
    }
}

void ReelEditorPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (m_dragHandle == HandleDrag::None || m_proc == nullptr || m_syncing)
        return;

    m_syncing = true;
    const float norm = waveXToNormal (static_cast<float> (e.getPosition().x));

    switch (m_dragHandle)
    {
        case HandleDrag::Start:
            m_proc->setParam ("play.start", juce::jmin (norm, m_proc->getParam ("play.end") - 0.01f));
            if (onParamChanged) onParamChanged ("play.start", m_proc->getParam ("play.start"));
            break;
        case HandleDrag::End:
            m_proc->setParam ("play.end", juce::jmax (norm, m_proc->getParam ("play.start") + 0.01f));
            if (onParamChanged) onParamChanged ("play.end", m_proc->getParam ("play.end"));
            break;
        case HandleDrag::GrainPos:
            m_proc->setParam ("grain.position", norm);
            if (onParamChanged) onParamChanged ("grain.position", norm);
            break;
        default:
            break;
    }
    m_syncing = false;
    repaint();
}

void ReelEditorPanel::mouseUp (const juce::MouseEvent&)
{
    m_dragHandle = HandleDrag::None;
}

//==============================================================================
void ReelEditorPanel::timerCallback()
{
    if (m_proc == nullptr) return;

    // Poll grain positions for cloud display
    if (m_proc->getMode() == ReelMode::grain)
    {
        m_numGrainDots = m_proc->getGrainPositions (m_grainPositions, kMaxGrainDots);
        repaint (m_waveRect);
    }
}

#include "StepGridDrum.h"

//==============================================================================

StepGridDrum::StepGridDrum()
{
    m_voiceContent = std::make_unique<VoiceContent> (this);
    m_voiceContent->setInterceptsMouseClicks (true, false);

    m_viewport.setScrollBarsShown (true, false);
    m_viewport.setViewedComponent (m_voiceContent.get(), false);
    addAndMakeVisible (m_viewport);
}

//==============================================================================
// State
//==============================================================================

void StepGridDrum::setDrumData (DrumMachineData* data, juce::Colour groupColor)
{
    m_data       = data;
    m_groupColor = groupColor;
    resized();
    repaint();
}

void StepGridDrum::clearDrumData()
{
    m_data = nullptr;
    resized();
    repaint();
}

//==============================================================================
// Layout
//==============================================================================

void StepGridDrum::resized()
{
    const int viewH = juce::jmax (0, getHeight() - kAddH);
    m_viewport.setBounds (0, 0, getWidth(), viewH);

    if (m_voiceContent != nullptr)
    {
        const int numVoices = (m_data != nullptr) ? m_data->voices.size() : 0;
        const int contentH  = juce::jmax (viewH, numVoices * kRowH + 2);
        m_voiceContent->setSize (getWidth(), contentH);
    }
}

//==============================================================================
// Region helpers (VoiceContent coordinates)
//==============================================================================

juce::Rectangle<int> StepGridDrum::voiceRowRect (int vi) const noexcept
{
    return { 0, vi * kRowH, contentW(), kRowH };
}

juce::Rectangle<int> StepGridDrum::muteRect (int vi) const noexcept
{
    const auto r = voiceRowRect (vi);
    return { r.getX() + 2, r.getCentreY() - 5, 10, 10 };
}

juce::Rectangle<int> StepGridDrum::modeRect (int vi) const noexcept
{
    return muteRect (vi).translated (12, 0);
}

juce::Rectangle<int> StepGridDrum::decBtnRect (int vi) const noexcept
{
    const auto r = voiceRowRect (vi);
    return { r.getRight() - 2 * kBtnW - 2, r.getCentreY() - 6, kBtnW, 12 };
}

juce::Rectangle<int> StepGridDrum::incBtnRect (int vi) const noexcept
{
    return decBtnRect (vi).translated (kBtnW + 2, 0);
}

juce::Rectangle<int> StepGridDrum::stepCellRect (int vi, int step) const noexcept
{
    if (m_data == nullptr || vi >= m_data->voices.size()) return {};

    const auto* voice  = m_data->voices[vi];
    const int   nSteps = voice->pattern.activeStepCount();
    if (step < 0 || step >= nSteps) return {};

    const int gridW = stepGridW();
    if (gridW <= 0 || nSteps <= 0) return {};

    const float cellW  = static_cast<float> (gridW) / nSteps;
    const int   x      = kLabelW + static_cast<int> (step * cellW);
    const int   w      = juce::jmax (1, static_cast<int> ((step + 1) * cellW) - static_cast<int> (step * cellW) - 1);
    const auto  r      = voiceRowRect (vi);

    return { x, r.getY() + 2, w, kRowH - 4 };
}

juce::Rectangle<int> StepGridDrum::addVoiceRect() const noexcept
{
    return { 2, getHeight() - kAddH, getWidth() - 4, kAddH };
}

int StepGridDrum::voiceAtY (int y) const noexcept
{
    if (m_data == nullptr) return -1;
    const int vi = y / kRowH;
    return (vi >= 0 && vi < m_data->voices.size()) ? vi : -1;
}

int StepGridDrum::stepAtX (int vi, int x) const noexcept
{
    if (m_data == nullptr || vi < 0 || vi >= m_data->voices.size()) return -1;
    const auto* voice  = m_data->voices[vi];
    const int   nSteps = voice->pattern.activeStepCount();
    if (nSteps <= 0) return -1;

    const int gridW = stepGridW();
    const int relX  = x - kLabelW;
    if (relX < 0 || relX >= gridW) return -1;

    const int step = static_cast<int> (relX * nSteps / gridW);
    return (step >= 0 && step < nSteps) ? step : -1;
}

//==============================================================================
// Paint
//==============================================================================

void StepGridDrum::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface1);

    if (m_data == nullptr)
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("click a DRUM MACHINE slot",
                     getLocalBounds().withTrimmedBottom (kAddH),
                     juce::Justification::centred, false);
    }

    // ADD VOICE row (below viewport)
    const auto addR = addVoiceRect();
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.drawRect  (addR, 1);
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  ("+  ADD VOICE", addR, juce::Justification::centred, false);
}

void StepGridDrum::paintVoices (juce::Graphics& g)
{
    if (m_data == nullptr)
    {
        g.fillAll (Theme::Colour::surface1);
        return;
    }

    g.fillAll (Theme::Colour::surface1);

    for (int vi = 0; vi < m_data->voices.size(); ++vi)
        paintVoiceRow (g, vi);
}

void StepGridDrum::paintVoiceRow (juce::Graphics& g, int vi) const
{
    if (m_data == nullptr || vi >= m_data->voices.size()) return;

    const auto* voice = m_data->voices[vi];
    const auto  row   = voiceRowRect (vi);
    const bool  sel   = (vi == m_data->focusedVoiceIndex);

    // Row background
    g.setColour (sel ? Theme::Colour::surface2 : Theme::Colour::surface1);
    g.fillRect  (row);

    // Row separator
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.4f));
    g.drawLine (static_cast<float> (row.getX()),
                static_cast<float> (row.getBottom() - 1),
                static_cast<float> (row.getRight()),
                static_cast<float> (row.getBottom() - 1), 0.5f);

    const float alpha = voice->muted ? 0.35f : 1.0f;

    // Voice name label (truncated)
    {
        const int nameX = 26;  // after mute + mode buttons
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMid.withAlpha (alpha));
        g.drawText  (voice->name,
                     juce::Rectangle<int> (nameX, row.getY(), kLabelW - nameX - 2, kRowH),
                     juce::Justification::centredLeft, true);
    }

    // Mute button
    {
        const auto mr = muteRect (vi);
        g.setColour (voice->muted ? (juce::Colour) Theme::Colour::error
                                  : (juce::Colour) Theme::Colour::surface2);
        g.fillRect  (mr);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRect  (mr, 1);
        g.setFont   (Theme::Font::micro());
        g.setColour (voice->muted ? (juce::Colour) Theme::Colour::inkLight
                                  : (juce::Colour) Theme::Colour::inkGhost);
        g.drawText  ("M", mr, juce::Justification::centred, false);
    }

    // Mode button
    {
        const auto mr = modeRect (vi);
        g.setColour (Theme::Colour::surface2);
        g.fillRect  (mr);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRect  (mr, 1);
        g.setFont   (Theme::Font::micro());
        g.setColour (m_groupColor.withAlpha (alpha));
        // "S" for sample, "\xe2\x99\xa9" (♩) for midi
        g.drawText  (voice->mode == VoiceMode::sample ? "S" : "\xe2\x99\xaa",
                     mr, juce::Justification::centred, false);
    }

    // Step cells
    const int   nSteps = voice->pattern.activeStepCount();
    const int   gridW  = stepGridW();
    if (gridW > 0 && nSteps > 0)
    {
        const float cellW = static_cast<float> (gridW) / nSteps;

        for (int s = 0; s < nSteps; ++s)
        {
            const auto cr = stepCellRect (vi, s);
            if (cr.isEmpty()) continue;

            const bool active = voice->pattern.stepActive (s);
            g.setColour ((active ? voice->color : (juce::Colour) Theme::Colour::surface3).withAlpha (alpha));
            g.fillRect  (cr);
        }

        // Beat-group dividers every 4 steps
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f * alpha));
        for (int beat = 1; beat * 4 < nSteps; ++beat)
        {
            const int col   = beat * 4;
            const int lineX = kLabelW + static_cast<int> (col * cellW);
            g.fillRect (lineX, row.getY(), 1, kRowH);
        }
    }

    // Step count controls
    {
        const auto db = decBtnRect (vi);
        const auto ib = incBtnRect (vi);

        g.setColour (Theme::Colour::surface2);
        g.fillRect  (db);
        g.fillRect  (ib);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRect  (db, 1);
        g.drawRect  (ib, 1);

        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMid);
        g.drawText  ("-", db, juce::Justification::centred, false);
        g.drawText  ("+", ib, juce::Justification::centred, false);

        // Step count tooltip just to the left of dec btn
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (alpha));
        g.drawText  (juce::String (nSteps),
                     juce::Rectangle<int> (db.getX() - 18, row.getY(), 16, kRowH),
                     juce::Justification::centredRight, false);
    }
}

//==============================================================================
// Voice mouse handling (in VoiceContent coordinates)
//==============================================================================

void StepGridDrum::voiceMouseDown (const juce::MouseEvent& e)
{
    if (m_data == nullptr) return;

    const auto pos = e.getPosition();
    const int  vi  = voiceAtY (pos.y);
    if (vi < 0) return;

    auto* voice = m_data->voices[vi];

    // Mute button
    if (muteRect (vi).contains (pos))
    {
        voice->muted = !voice->muted;
        m_voiceContent->repaint();
        return;
    }

    // Mode button
    if (modeRect (vi).contains (pos))
    {
        voice->mode = (voice->mode == VoiceMode::sample) ? VoiceMode::midi : VoiceMode::sample;
        m_voiceContent->repaint();
        return;
    }

    // Dec/Inc step count
    if (decBtnRect (vi).contains (pos))
    {
        voice->pattern.setStepCount (voice->pattern.activeStepCount() - 1);
        m_voiceContent->repaint();
        if (onModified) onModified();
        return;
    }
    if (incBtnRect (vi).contains (pos))
    {
        voice->pattern.setStepCount (voice->pattern.activeStepCount() + 1);
        m_voiceContent->repaint();
        if (onModified) onModified();
        return;
    }

    // Right-click context menu
    if (e.mods.isRightButtonDown())
    {
        m_data->focusedVoiceIndex = vi;
        rightClickVoice (vi);
        m_voiceContent->repaint();
        return;
    }

    // Step toggle
    const int step = stepAtX (vi, pos.x);
    if (step >= 0)
    {
        m_data->focusedVoiceIndex = vi;
        m_paintMode = !voice->pattern.stepActive (step);
        m_lastVoice = vi;
        m_lastStep  = step;
        voice->pattern.toggleStep (step);
        m_voiceContent->repaint();
        if (onModified) onModified();
    }
}

void StepGridDrum::voiceDrag (const juce::MouseEvent& e)
{
    if (m_data == nullptr) return;

    const auto pos = e.getPosition();
    const int  vi  = voiceAtY (pos.y);
    if (vi < 0 || vi >= m_data->voices.size()) return;

    const int step = stepAtX (vi, pos.x);
    if (step >= 0 && (vi != m_lastVoice || step != m_lastStep))
    {
        m_lastVoice = vi;
        m_lastStep  = step;

        auto* voice = m_data->voices[vi];
        if (voice->pattern.stepActive (step) != m_paintMode)
        {
            voice->pattern.toggleStep (step);
            m_voiceContent->repaint();
            if (onModified) onModified();
        }
    }
}

void StepGridDrum::voiceMouseUp (const juce::MouseEvent&)
{
    m_lastVoice = -1;
    m_lastStep  = -1;
}

//==============================================================================
// Mouse on StepGridDrum itself (ADD VOICE button)
//==============================================================================

void StepGridDrum::mouseDown (const juce::MouseEvent& e)
{
    if (addVoiceRect().contains (e.getPosition()))
    {
        addNewVoice();
        resized();
        repaint();
    }
}

void StepGridDrum::mouseUp (const juce::MouseEvent&) {}

//==============================================================================
// Add voice
//==============================================================================

void StepGridDrum::addNewVoice()
{
    if (m_data == nullptr) return;

    static const juce::uint32 voiceColors[] =
    {
        0xFFDB8E4B, 0xFF4B9EDB, 0xFF9EDB4B, 0xFFDB4B9E,
        0xFF4BDBB3, 0xFFB34BDB, 0xFFDBDB4B, 0xFF4B6BDB
    };

    const int   idx  = m_data->voices.size();
    auto*       v    = m_data->voices.add (new DrumVoice{});
    v->name          = "V" + juce::String (idx + 1);
    v->midiNote      = 36 + idx;
    v->color         = juce::Colour (voiceColors[idx % 8]);
}

//==============================================================================
// Context menu
//==============================================================================

void StepGridDrum::rightClickVoice (int vi)
{
    if (m_data == nullptr || vi >= m_data->voices.size()) return;

    juce::PopupMenu menu;
    menu.addItem (1, "Rename voice");
    menu.addItem (2, "Set to sample mode");
    menu.addItem (3, "Set to MIDI mode");
    menu.addSeparator();
    menu.addItem (4, "Duplicate voice");
    menu.addItem (5, "Delete voice");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (m_voiceContent.get()),
        [this, vi] (int result)
        {
            if (m_data == nullptr || vi >= m_data->voices.size()) return;
            auto* v = m_data->voices[vi];

            if (result == 1)
            {
                // TODO: inline rename for voice
            }
            else if (result == 2)
            {
                v->mode = VoiceMode::sample;
                m_voiceContent->repaint();
            }
            else if (result == 3)
            {
                v->mode = VoiceMode::midi;
                m_voiceContent->repaint();
            }
            else if (result == 4)
            {
                // Duplicate
                auto* newV = m_data->voices.insert (vi + 1, new DrumVoice (*v));
                juce::ignoreUnused (newV);
                resized();
                m_voiceContent->repaint();
            }
            else if (result == 5)
            {
                if (m_data->voices.size() > 1)
                {
                    m_data->voices.remove (vi);
                    m_data->focusedVoiceIndex = juce::jlimit (
                        0, m_data->voices.size() - 1, m_data->focusedVoiceIndex);
                    resized();
                    m_voiceContent->repaint();
                }
            }
        });
}

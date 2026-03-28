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
    m_playhead = -1;
    m_hoverVoice = -1;
    m_hoverStep = -1;
    resized();
    repaint();
}

void StepGridDrum::setPlayhead (int stepIndex)
{
    if (m_playhead == stepIndex) return;
    m_playhead = stepIndex;
    if (m_voiceContent) m_voiceContent->repaint();
}

void StepGridDrum::mouseMove (const juce::MouseEvent&)
{
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
        const int numVoices = (m_data != nullptr) ? static_cast<int> (m_data->voices.size()) : 0;
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
    if (m_data == nullptr || vi >= static_cast<int> (m_data->voices.size())) return {};

    const auto& voice  = m_data->voices[static_cast<size_t> (vi)];
    const int   nSteps = voice.stepCount;
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
    return (vi >= 0 && vi < static_cast<int> (m_data->voices.size())) ? vi : -1;
}

int StepGridDrum::stepAtX (int vi, int x) const noexcept
{
    if (m_data == nullptr || vi < 0 || vi >= static_cast<int> (m_data->voices.size())) return -1;
    const auto& voice  = m_data->voices[static_cast<size_t> (vi)];
    const int   nSteps = voice.stepCount;
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

    for (int vi = 0; vi < static_cast<int> (m_data->voices.size()); ++vi)
        paintVoiceRow (g, vi);
}

void StepGridDrum::paintVoiceRow (juce::Graphics& g, int vi) const
{
    if (m_data == nullptr || vi >= static_cast<int> (m_data->voices.size())) return;

    const auto& voice = m_data->voices[static_cast<size_t> (vi)];
    const auto  row   = voiceRowRect (vi);
    const bool  sel   = (vi == m_data->focusedVoiceIndex);
    const bool  hover = (vi == m_hoverVoice);

    // Row background
    g.setColour (sel ? Theme::Colour::surface2 : hover ? Theme::Colour::surface2.withAlpha (0.65f) : Theme::Colour::surface1);
    g.fillRect  (row);

    // Row separator
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.4f));
    g.drawLine (static_cast<float> (row.getX()),
                static_cast<float> (row.getBottom() - 1),
                static_cast<float> (row.getRight()),
                static_cast<float> (row.getBottom() - 1), 0.5f);

    const float alpha = voice.muted ? 0.35f : 1.0f;

    // Voice name label (truncated)
    {
        const int nameX = 26;  // after mute + mode buttons
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMid.withAlpha (alpha));
        g.drawText  (juce::String (voice.name),
                     juce::Rectangle<int> (nameX, row.getY(), kLabelW - nameX - 2, kRowH),
                     juce::Justification::centredLeft, true);
    }

    // Mute button
    {
        const auto mr = muteRect (vi);
        g.setColour (voice.muted ? (juce::Colour) Theme::Colour::error
                                  : (juce::Colour) Theme::Colour::surface2);
        g.fillRect  (mr);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRect  (mr, 1);
        g.setFont   (Theme::Font::micro());
        g.setColour (voice.muted ? (juce::Colour) Theme::Colour::inkLight
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
        g.drawText  (voice.mode == DrumVoiceMode::sample ? "S" : "\xe2\x99\xaa",
                     mr, juce::Justification::centred, false);
    }

    // Step cells
    const int   nSteps = voice.stepCount;
    const int   gridW  = stepGridW();
    if (gridW > 0 && nSteps > 0)
    {
        const float cellW = static_cast<float> (gridW) / nSteps;

        for (int s = 0; s < nSteps; ++s)
        {
            const auto cr = stepCellRect (vi, s);
            if (cr.isEmpty()) continue;

            const bool active = voice.stepActive (s);
            const bool selectedStep = (sel && s == m_data->focusedStepIndex);
            const bool hoveredStep = (vi == m_hoverVoice && s == m_hoverStep);
            auto fill = (active ? juce::Colour (voice.colorArgb) : (juce::Colour) Theme::Colour::surface3).withAlpha (alpha);
            if (active)
                fill = fill.interpolatedWith (juce::Colours::white, 1.0f - voice.stepVelocity (s));
            if (hoveredStep && ! selectedStep)
                fill = fill.brighter (0.08f);

            g.setColour (fill);
            g.fillRoundedRectangle (cr.toFloat(), Theme::Radius::xs);
            g.setColour (selectedStep ? Theme::Colour::inkLight
                                      : hoveredStep ? juce::Colour (voice.colorArgb).brighter (0.4f)
                                                    : Theme::Colour::surfaceEdge.withAlpha (0.45f));
            g.drawRoundedRectangle (cr.toFloat(), Theme::Radius::xs, selectedStep ? 1.7f : Theme::Stroke::subtle);

            if (active)
            {
                const int velBars = juce::jlimit (1, 4, juce::roundToInt (voice.stepVelocity (s) * 4.0f));
                g.setColour (Theme::Colour::inkDark.withAlpha (0.85f));
                for (int bar = 0; bar < velBars; ++bar)
                    g.fillRect (cr.getX() + 2 + bar * 2, cr.getBottom() - 4, 1, 2);

                const int ratchets = voice.stepRatchetCount (s);
                if (ratchets > 1)
                {
                    g.setColour (Theme::Colour::inkLight.withAlpha (0.85f));
                    for (int r = 0; r < ratchets && r < 4; ++r)
                        g.fillEllipse ((float) (cr.getRight() - 3 - r * 3), (float) (cr.getY() + 2), 2.0f, 2.0f);
                }

                const int division = voice.stepRepeatDivision (s);
                if (division > 1)
                {
                    g.setColour (Theme::Colour::inkGhost.withAlpha (0.8f));
                    g.setFont (Theme::Font::micro());
                    g.drawText ("/" + juce::String (division),
                                cr.reduced (2, 1),
                                juce::Justification::centredRight,
                                false);
                }
            }
        }

        // Beat-group dividers every 4 steps
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f * alpha));
        for (int beat = 1; beat * 4 < nSteps; ++beat)
        {
            const int col   = beat * 4;
            const int lineX = kLabelW + static_cast<int> (col * cellW);
            g.fillRect (lineX, row.getY(), 1, kRowH);
        }

        // Playhead column highlight
        if (m_playhead >= 0 && nSteps > 0)
        {
            const auto cr = stepCellRect (vi, m_playhead % nSteps);
            if (! cr.isEmpty())
            {
                g.setColour (juce::Colours::white.withAlpha (0.14f));
                g.fillRect  (cr);
            }
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
    m_data->focusedVoiceIndex = vi;

    auto& voice = m_data->voices[static_cast<size_t> (vi)];

    // Mute button
    if (muteRect (vi).contains (pos))
    {
        voice.muted = !voice.muted;
        m_voiceContent->repaint();
        if (onModified) onModified();
        return;
    }

    // Mode button
    if (modeRect (vi).contains (pos))
    {
        voice.mode = (voice.mode == DrumVoiceMode::sample) ? DrumVoiceMode::midi : DrumVoiceMode::sample;
        m_voiceContent->repaint();
        if (onModified) onModified();
        return;
    }

    // Dec/Inc step count
    if (decBtnRect (vi).contains (pos))
    {
        voice.setStepCount (voice.stepCount - 1);
        m_voiceContent->repaint();
        if (onModified) onModified();
        return;
    }
    if (incBtnRect (vi).contains (pos))
    {
        voice.setStepCount (voice.stepCount + 1);
        m_voiceContent->repaint();
        if (onModified) onModified();
        return;
    }

    const int step = stepAtX (vi, pos.x);
    if (step >= 0)
    {
        m_data->focusedStepIndex = step;

        if (e.mods.isRightButtonDown())
        {
            rightClickStep (vi, step);
            m_voiceContent->repaint();
            return;
        }

        if (e.mods.isShiftDown())
        {
            const float currentVelocity = voice.stepVelocity (step);
            voice.setStepVelocity (step, currentVelocity >= 0.99f ? 0.25f : juce::jlimit (0.05f, 1.0f, currentVelocity + 0.25f));
            m_voiceContent->repaint();
            if (onModified) onModified();
            return;
        }

        m_paintMode = !voice.stepActive (step);
        m_lastVoice = vi;
        m_lastStep  = step;
        voice.toggleStep (step);
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
}

void StepGridDrum::voiceDrag (const juce::MouseEvent& e)
{
    if (m_data == nullptr) return;

    const auto pos = e.getPosition();
    const int  vi  = voiceAtY (pos.y);
    if (vi < 0 || vi >= static_cast<int> (m_data->voices.size())) return;

    const int step = stepAtX (vi, pos.x);
    if (step >= 0 && (vi != m_lastVoice || step != m_lastStep))
    {
        m_lastVoice = vi;
        m_lastStep  = step;

        auto& voice = m_data->voices[static_cast<size_t> (vi)];
        if (voice.stepActive (step) != m_paintMode)
        {
            voice.toggleStep (step);
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

    static constexpr DrumVoiceRole roleCycle[] =
    {
        DrumVoiceRole::perc1,
        DrumVoiceRole::perc2,
        DrumVoiceRole::hat,
        DrumVoiceRole::snare,
        DrumVoiceRole::kick,
        DrumVoiceRole::subKick,
    };

    const int idx = static_cast<int> (m_data->voices.size());
    auto voice = DrumModuleState::makeVoiceForRole (roleCycle[idx % static_cast<int> (std::size (roleCycle))],
                                                    voiceColors[idx % 8]);
    voice.name = (juce::String (DrumVoiceParams::defaultName (voice.params.role))
                  + " " + juce::String (idx + 1)).toStdString();
    m_data->voices.push_back (voice);
    m_data->clamp();
    if (onModified) onModified();
}

void StepGridDrum::duplicateStep (int vi, int step)
{
    if (m_data == nullptr || vi < 0 || vi >= static_cast<int> (m_data->voices.size()))
        return;

    auto& voice = m_data->voices[static_cast<size_t> (vi)];
    if (step < 0 || step >= voice.stepCount)
        return;

    const int next = (step + 1) % voice.stepCount;
    voice.setStepActive (next, voice.stepActive (step));
    voice.setStepVelocity (next, voice.stepVelocity (step));
    voice.setStepRatchetCount (next, voice.stepRatchetCount (step));
    voice.setStepRepeatDivision (next, voice.stepRepeatDivision (step));
}

void StepGridDrum::clearStep (int vi, int step)
{
    if (m_data == nullptr || vi < 0 || vi >= static_cast<int> (m_data->voices.size()))
        return;

    auto& voice = m_data->voices[static_cast<size_t> (vi)];
    voice.setStepActive (step, false);
    voice.setStepVelocity (step, 1.0f);
    voice.setStepRatchetCount (step, DrumVoiceState::kDefaultRatchetCount);
    voice.setStepRepeatDivision (step, DrumVoiceState::kDefaultRepeatDivision);
}

void StepGridDrum::voiceMove (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();
    const int vi = voiceAtY (pos.y);
    const int step = vi >= 0 ? stepAtX (vi, pos.x) : -1;
    if (vi != m_hoverVoice || step != m_hoverStep)
    {
        m_hoverVoice = vi;
        m_hoverStep = step;
        if (m_voiceContent) m_voiceContent->repaint();
    }
}

//==============================================================================
// Context menu
//==============================================================================

void StepGridDrum::rightClickVoice (int vi)
{
    if (m_data == nullptr || vi >= static_cast<int> (m_data->voices.size())) return;

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
            if (m_data == nullptr || vi >= static_cast<int> (m_data->voices.size())) return;
            auto& v = m_data->voices[static_cast<size_t> (vi)];

            if (result == 1)
            {
                // TODO: inline rename for voice
            }
            else if (result == 2)
            {
                v.mode = DrumVoiceMode::sample;
                m_voiceContent->repaint();
                if (onModified) onModified();
            }
            else if (result == 3)
            {
                v.mode = DrumVoiceMode::midi;
                m_voiceContent->repaint();
                if (onModified) onModified();
            }
            else if (result == 4)
            {
                m_data->voices.insert (m_data->voices.begin() + vi + 1, v);
                m_data->clamp();
                resized();
                m_voiceContent->repaint();
                if (onModified) onModified();
            }
            else if (result == 5)
            {
                if (m_data->voices.size() > 1)
                {
                    m_data->voices.erase (m_data->voices.begin() + vi);
                    m_data->clamp();
                    resized();
                    m_voiceContent->repaint();
                    if (onModified) onModified();
                }
            }
        });
}

void StepGridDrum::rightClickStep (int vi, int step)
{
    if (m_data == nullptr || vi >= static_cast<int> (m_data->voices.size())) return;

    juce::PopupMenu menu;
    menu.addItem (1, m_data->voices[(size_t) vi].stepActive (step) ? "Deactivate Step" : "Activate Step");
    menu.addItem (2, "Duplicate To Next");
    menu.addItem (3, "Clear Step");
    menu.addSeparator();

    juce::PopupMenu velMenu;
    velMenu.addItem (10, "25%");
    velMenu.addItem (11, "50%");
    velMenu.addItem (12, "75%");
    velMenu.addItem (13, "100%");
    menu.addSubMenu ("Velocity", velMenu);

    juce::PopupMenu ratchetMenu;
    ratchetMenu.addItem (20, "1 hit");
    ratchetMenu.addItem (21, "2 hits");
    ratchetMenu.addItem (22, "3 hits");
    ratchetMenu.addItem (23, "4 hits");
    menu.addSubMenu ("Repeat Count", ratchetMenu);

    juce::PopupMenu divisionMenu;
    divisionMenu.addItem (30, "1");
    divisionMenu.addItem (31, "2");
    divisionMenu.addItem (32, "3");
    divisionMenu.addItem (33, "4");
    divisionMenu.addItem (34, "6");
    divisionMenu.addItem (35, "8");
    menu.addSubMenu ("Repeat Division", divisionMenu);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (m_voiceContent.get()),
                        [this, vi, step] (int result)
                        {
                            if (m_data == nullptr || vi >= static_cast<int> (m_data->voices.size()) || result == 0) return;
                            auto& voice = m_data->voices[(size_t) vi];

                            switch (result)
                            {
                                case 1: voice.setStepActive (step, ! voice.stepActive (step)); break;
                                case 2: duplicateStep (vi, step); break;
                                case 3: clearStep (vi, step); break;
                                case 10: voice.setStepVelocity (step, 0.25f); break;
                                case 11: voice.setStepVelocity (step, 0.50f); break;
                                case 12: voice.setStepVelocity (step, 0.75f); break;
                                case 13: voice.setStepVelocity (step, 1.00f); break;
                                case 20: voice.setStepRatchetCount (step, 1); break;
                                case 21: voice.setStepRatchetCount (step, 2); break;
                                case 22: voice.setStepRatchetCount (step, 3); break;
                                case 23: voice.setStepRatchetCount (step, 4); break;
                                case 30: voice.setStepRepeatDivision (step, 1); break;
                                case 31: voice.setStepRepeatDivision (step, 2); break;
                                case 32: voice.setStepRepeatDivision (step, 3); break;
                                case 33: voice.setStepRepeatDivision (step, 4); break;
                                case 34: voice.setStepRepeatDivision (step, 6); break;
                                case 35: voice.setStepRepeatDivision (step, 8); break;
                                default: break;
                            }

                            m_data->focusedVoiceIndex = vi;
                            m_data->focusedStepIndex = step;
                            if (m_voiceContent) m_voiceContent->repaint();
                            if (onModified) onModified();
                        });
}

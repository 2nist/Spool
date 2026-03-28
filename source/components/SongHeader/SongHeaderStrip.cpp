#include "SongHeaderStrip.h"

//==============================================================================
SongHeaderStrip::SongHeaderStrip()
{
    m_titleEditor.setFont (Theme::Font::labelMedium());
    m_titleEditor.setColour (juce::TextEditor::textColourId,          Theme::Helper::textForState (Theme::Helper::VisualState::idle));
    m_titleEditor.setColour (juce::TextEditor::backgroundColourId,    Theme::Colour::surface2);
    m_titleEditor.setColour (juce::TextEditor::outlineColourId,       Theme::Colour::surfaceEdge);
    m_titleEditor.setColour (juce::TextEditor::focusedOutlineColourId, Theme::Helper::emphasisForState (Theme::Colour::accent,
                                                                                                         Theme::Helper::VisualState::focused));
    m_titleEditor.setJustification (juce::Justification::centredLeft);
    m_titleEditor.onReturnKey = [this] { titleEditingComplete(); };
    m_titleEditor.onFocusLost = [this] { titleEditingComplete(); };
    addChildComponent (m_titleEditor);
}

//==============================================================================
void SongHeaderStrip::resized()
{
    const float fy = 0.0f;
    const float fh = static_cast<float> (getHeight());

    // Left section fields (left to right, with compact gaps aligned to shell density)
    float x = 12.0f;

    if (m_compactMode)
    {
        m_titleRect = { x, fy, 196.0f, fh };  x += 196.0f + 10.0f;
        m_keyRect   = { x, fy,  58.0f, fh };  x +=  58.0f + 10.0f;
        m_bpmRect   = { x, fy,  50.0f, fh };
        m_sigRect   = {};
        m_lenRect   = {};
    }
    else
    {
        m_titleRect = { x, fy, 160.0f, fh };  x += 160.0f + 10.0f;
        m_keyRect   = { x, fy,  60.0f, fh };  x +=  60.0f + 10.0f;
        m_bpmRect   = { x, fy,  52.0f, fh };  x +=  52.0f + 10.0f;
        m_sigRect   = { x, fy,  40.0f, fh };  x +=  40.0f + 10.0f;
        m_lenRect   = { x, fy,  52.0f, fh };
    }

    if (m_editingTitle)
        m_titleEditor.setBounds (m_titleRect.toNearestInt());
}

//==============================================================================
void SongHeaderStrip::paint (juce::Graphics& g)
{
    const float w = static_cast<float> (getWidth());
    const float h = static_cast<float> (getHeight());

    // Background
    Theme::Helper::drawPanel (g, getLocalBounds(), Theme::Colour::surface0, Theme::Colour::transparent,
                              Theme::Radius::none, Theme::Stroke::subtle);

    // Top + bottom borders
    g.setColour (Theme::Colour::surfaceEdge);
    g.fillRect (0.0f, 0.0f,             w, Theme::Stroke::subtle);
    g.fillRect (0.0f, h - Theme::Stroke::subtle, w, Theme::Stroke::subtle);

    // Song title
    if (!m_editingTitle)
    {
        g.setFont   (Theme::Font::labelMedium());
        g.setColour (Theme::Colour::inkLight);
        g.drawText  (m_titleText, m_titleRect.toNearestInt(),
                     juce::Justification::centredLeft, true);
    }

    // Dividers after each field
    paintDivider (g, m_titleRect.getRight() + 4.0f);
    paintDivider (g, m_keyRect.getRight()   + 4.0f);
    if (! m_bpmRect.isEmpty())
        paintDivider (g, m_bpmRect.getRight() + 4.0f);
    if (! m_sigRect.isEmpty())
        paintDivider (g, m_sigRect.getRight() + 4.0f);

    // Fields
    paintField (g, m_keyRect, "KEY", "C Maj", Theme::Colour::inkMid, Theme::Font::value());
    paintField (g, m_bpmRect, "BPM", juce::String (static_cast<int> (m_bpm)),
                Theme::Colour::accent,
                m_compactMode ? Theme::Font::value() : Theme::Font::transport());
    if (! m_sigRect.isEmpty())
        paintField (g, m_sigRect, "SIG", "4/4", Theme::Colour::inkMid, Theme::Font::value());
    if (! m_lenRect.isEmpty())
        paintField (g, m_lenRect, "LEN", m_lengthStr, Theme::Colour::inkMid, Theme::Font::value());

    // Right section: project filename
    if (! m_compactMode)
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("session_01.spool",
                     juce::Rectangle<float> (w - 160.0f, 0.0f, 110.0f, h),
                     juce::Justification::centredRight, false);
    }

    // Save indicator dot
    const float dotX = w - 14.0f;
    const float dotY = (h - 5.0f) * 0.5f;
    Theme::Helper::drawStateDot (g,
                                 { dotX, dotY, 5.0f, 5.0f },
                                 m_unsaved ? Theme::Helper::VisualState::armed
                                           : Theme::Helper::VisualState::disabled,
                                 m_unsaved ? juce::Colour (Theme::Colour::accentWarm)
                                           : juce::Colour (Theme::Colour::inactive));

    // Save label
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Helper::textForState (m_unsaved ? Theme::Helper::VisualState::armed
                                                        : Theme::Helper::VisualState::disabled,
                                              Theme::Colour::inkGhost));
    g.drawText  (m_unsaved ? "UNSAVED" : "SAVED",
                 juce::Rectangle<float> (dotX - (m_compactMode ? 44.0f : 54.0f), 0.0f,
                                         m_compactMode ? 40.0f : 50.0f, h),
                 juce::Justification::centredRight, false);
}

//==============================================================================
void SongHeaderStrip::paintField (juce::Graphics& g,
                                    const juce::Rectangle<float>& r,
                                    const juce::String& labelText,
                                    const juce::String& valueText,
                                    juce::Colour valueColour,
                                    const juce::Font& valueFont) const
{
    const float mid = r.getY() + r.getHeight() * 0.5f;

    // Label — top half, micro Poppins, inkGhost
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  (labelText, r.withBottom (mid).toNearestInt(),
                 juce::Justification::centredLeft, false);

    // Value — bottom half, supplied font and colour
    g.setFont   (valueFont);
    g.setColour (valueColour);
    g.drawText  (valueText, r.withTop (mid).toNearestInt(),
                 juce::Justification::centredLeft, false);
}

//==============================================================================
void SongHeaderStrip::paintDivider (juce::Graphics& g, float x) const
{
    const float h = static_cast<float> (getHeight());
    g.setColour (Theme::Colour::surfaceEdge);
    g.fillRect  (x, (h - 16.0f) * 0.5f, Theme::Stroke::subtle, 16.0f);
}

//==============================================================================
void SongHeaderStrip::mouseDown (const juce::MouseEvent& e)
{
    if (m_bpmRect.contains (e.position))
    {
        m_draggingBpm  = true;
        m_dragStartY   = e.position.y;
        m_dragStartBpm = m_bpm;
    }
}

void SongHeaderStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (!m_draggingBpm)
        return;

    // Drag up = increase BPM, drag down = decrease
    const float delta = m_dragStartY - e.position.y;
    setBpm (m_dragStartBpm + delta);
}

void SongHeaderStrip::mouseUp (const juce::MouseEvent&)
{
    m_draggingBpm = false;
}

void SongHeaderStrip::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (m_titleRect.contains (e.position))
    {
        m_editingTitle = true;
        m_titleEditor.setText (m_titleText, false);
        m_titleEditor.setBounds (m_titleRect.toNearestInt());
        m_titleEditor.setVisible (true);
        m_titleEditor.grabKeyboardFocus();
        m_titleEditor.selectAll();
        repaint();
    }
}

//==============================================================================
void SongHeaderStrip::setBpm (float bpm)
{
    m_bpm = juce::jlimit (40.0f, 240.0f, bpm);
    if (onBpmChanged)
        onBpmChanged (m_bpm);
    repaint();
}

void SongHeaderStrip::setCompactMode (bool shouldCompact)
{
    if (m_compactMode == shouldCompact)
        return;

    m_compactMode = shouldCompact;
    resized();
    repaint();
}

void SongHeaderStrip::setTitle (const juce::String& title)
{
    const auto cleaned = title.trim();
    m_titleText = cleaned.isNotEmpty() ? cleaned : juce::String ("Untitled");

    if (m_editingTitle)
        m_titleEditor.setText (m_titleText, juce::dontSendNotification);

    repaint();
}

void SongHeaderStrip::setPosition (float beatPosition, float bpm, float loopLengthBars)
{
    setBpm (bpm);

    // Convert beat position to mm:ss
    const float secondsPerBeat = 60.0f / juce::jmax (1.0f, bpm);
    const float totalSeconds   = beatPosition * secondsPerBeat;
    const int   mins           = static_cast<int> (totalSeconds) / 60;
    const int   secs           = static_cast<int> (totalSeconds) % 60;
    m_lengthStr = juce::String (mins) + ":" + juce::String (secs).paddedLeft ('0', 2);

    juce::ignoreUnused (loopLengthBars);
    repaint();
}

//==============================================================================
void SongHeaderStrip::titleEditingComplete()
{
    if (!m_editingTitle)
        return;

    m_titleText = m_titleEditor.getText().trim();
    if (m_titleText.isEmpty())
        m_titleText = "Untitled";

    m_editingTitle = false;
    m_titleEditor.setVisible (false);

    if (onTitleChanged)
        onTitleChanged (m_titleText);

    repaint();
}

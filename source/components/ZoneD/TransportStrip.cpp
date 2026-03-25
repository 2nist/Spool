#include "TransportStrip.h"
#include "../../Theme.h"

//==============================================================================
TransportStrip::TransportStrip()
{
    setOpaque (false);
}

TransportStrip::~TransportStrip()
{
    stopTimer();
}

//==============================================================================
void TransportStrip::addTransportListener (TransportListener* l)
{
    m_listeners.add (l);
}

void TransportStrip::removeTransportListener (TransportListener* l)
{
    m_listeners.remove (l);
}

//==============================================================================
void TransportStrip::setPlaying (bool playing)
{
    if (m_playing == playing)
        return;
    m_playing = playing;
    if (m_playing)
        startTimerHz (kTimerHz);
    else
        stopTimer();
    m_listeners.call (&TransportListener::playStateChanged, m_playing);
    repaint();
}

void TransportStrip::setRecording (bool recording)
{
    if (m_recording == recording)
        return;
    m_recording = recording;
    m_listeners.call (&TransportListener::recordStateChanged, m_recording);
    repaint();
}

void TransportStrip::setBpm (float bpm)
{
    m_bpm = bpm;
}

void TransportStrip::setLoopLength (float bars)
{
    m_loopLengthBars = juce::jlimit (1.0f, 32.0f, bars);
    updateLoopThumbRect();
    repaint();
}

void TransportStrip::setStructureFollowState (StructureFollowState state)
{
    if (m_structureFollowState == state)
        return;
    m_structureFollowState = state;
    repaint();
}

//==============================================================================
void TransportStrip::timerCallback()
{
    const float beatsPerBar  = 4.0f;
    const float beatsPerSec  = m_bpm / 60.0f;
    const float beatsPerTick = beatsPerSec / static_cast<float> (kTimerHz);
    const float loopLen      = m_loopLengthBars * beatsPerBar;

    m_currentBeat = std::fmod (m_currentBeat + beatsPerTick, loopLen);

    m_listeners.call (&TransportListener::positionChanged, m_currentBeat);
    if (onBeatAdvanced)
        onBeatAdvanced (m_currentBeat);
}

//==============================================================================
void TransportStrip::resized()
{
    const auto b  = getLocalBounds().toFloat();
    const float h = b.getHeight();
    float x       = 4.0f;

    // Three transport buttons — 20×20 centred vertically
    const float btnSz  = 20.0f;
    const float btnY   = (h - btnSz) * 0.5f;
    const float btnGap = 4.0f;

    // ⚙ Gear button — leftmost, shifts ■ ▶ ● one position right
    m_gearBtnRect = { x, btnY, btnSz, btnSz };  x += btnSz + btnGap;

    m_stopBtnRect = { x, btnY, btnSz, btnSz };  x += btnSz + btnGap;
    m_playBtnRect = { x, btnY, btnSz, btnSz };  x += btnSz + btnGap;
    m_recBtnRect  = { x, btnY, btnSz, btnSz };  x += btnSz + 8.0f;

    // Mini drum circle
    const float drumDiam = 20.0f;
    m_drumRect = { x, (h - drumDiam) * 0.5f, drumDiam, drumDiam };
    x += drumDiam + 8.0f;

    // Loop-length slider — 60px track
    const float trackW = 60.0f;
    const float trackH = 2.0f;
    m_loopSliderTrack = { x, (h - trackH) * 0.5f, trackW, trackH };
    x += trackW + 8.0f;

    updateLoopThumbRect();

    // Right side: snap label and MIX toggle
    const float mixW = 36.0f;
    m_mixBtnRect   = { b.getRight() - mixW - 4.0f, btnY, mixW, btnSz };
    m_snapLabelRect = { b.getRight() - mixW - 96.0f, btnY, 92.0f, btnSz };
}

void TransportStrip::updateLoopThumbRect()
{
    if (m_loopSliderTrack.isEmpty())
        return;
    const float thumbSz   = 8.0f;
    const float t         = (m_loopLengthBars - 1.0f) / 31.0f;
    const float thumbX    = m_loopSliderTrack.getX()
                            + t * (m_loopSliderTrack.getWidth() - thumbSz);
    m_loopThumbRect = { thumbX,
                        m_loopSliderTrack.getCentreY() - thumbSz * 0.5f,
                        thumbSz, thumbSz };
}

//==============================================================================
void TransportStrip::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // Background
    g.setColour (juce::Colour (kBgColour));
    g.fillRect (b);

    // Top border line
    g.setColour (juce::Colour (kBtnBorder));
    g.drawHorizontalLine (0, b.getX(), b.getRight());

    // Gear button
    paintTransportBtn (g, m_gearBtnRect, juce::String (juce::CharPointer_UTF8 ("\xe2\x9a\x99")),
                       false, kBtnSurface);

    // Transport buttons
    paintTransportBtn (g, m_stopBtnRect, juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xa0")),
                       false, kBtnSurface);
    paintTransportBtn (g, m_playBtnRect,
                       m_playing ? juce::String (juce::CharPointer_UTF8 ("\xe2\x8f\xb8"))
                                 : juce::String (juce::CharPointer_UTF8 ("\xe2\x96\xb6")),
                       m_playing, kPlayActive);
    paintTransportBtn (g, m_recBtnRect, juce::String (juce::CharPointer_UTF8 ("\xe2\x97\x8f")),
                       m_recording, kRecordArmed);

    // Mini drum
    paintMiniDrum (g);

    // Loop slider
    paintLoopSlider (g);

    // Structure follow status badge
    juce::String followText = "LEGACY";
    juce::Colour followFill = juce::Colour (kBtnSurface);
    juce::Colour followTextCol = juce::Colour (kTextColour);
    if (m_structureFollowState == StructureFollowState::ready)
    {
        followText = "FOLLOW RDY";
        followFill = Theme::Colour::accent.withAlpha (0.35f);
        followTextCol = Theme::Colour::inkLight;
    }
    else if (m_structureFollowState == StructureFollowState::active)
    {
        followText = "FOLLOW ON";
        followFill = Theme::Colour::accent;
        followTextCol = Theme::Colour::inkDark;
    }

    g.setColour (followFill);
    g.fillRoundedRectangle (m_snapLabelRect, 3.0f);
    g.setColour (juce::Colour (kBtnBorder));
    g.drawRoundedRectangle (m_snapLabelRect, 3.0f, 1.0f);
    g.setColour (followTextCol);
    g.setFont (Theme::Font::label());
    g.drawText (followText, m_snapLabelRect, juce::Justification::centred, false);

    // MIX toggle
    const bool mixActive = m_mixOpen;
    g.setColour (mixActive ? juce::Colour (kPlayActive) : juce::Colour (kBtnSurface));
    g.fillRoundedRectangle (m_mixBtnRect, 3.0f);
    g.setColour (juce::Colour (kBtnBorder));
    g.drawRoundedRectangle (m_mixBtnRect, 3.0f, 1.0f);
    g.setColour (mixActive ? juce::Colours::white.withAlpha (0.9f)
                           : juce::Colour (kTextColour));
    g.setFont (Theme::Font::label());
    g.drawText ("MIX", m_mixBtnRect, juce::Justification::centred, false);
}

void TransportStrip::paintTransportBtn (juce::Graphics& g,
                                        const juce::Rectangle<float>& r,
                                        const juce::String& label,
                                        bool active,
                                        juce::uint32 activeColour) const
{
    g.setColour (active ? juce::Colour (activeColour)
                        : juce::Colour (kBtnSurface));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (juce::Colour (kBtnBorder));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    g.setColour (active ? juce::Colours::white.withAlpha (0.9f)
                        : juce::Colour (kTextColour));
    g.setFont (Theme::Font::transport());
    g.drawText (label, r, juce::Justification::centred, false);
}

void TransportStrip::paintMiniDrum (juce::Graphics& g) const
{
    const float r = 7.0f + (m_loopLengthBars / 32.0f) * 9.0f;
    const auto  c = m_drumRect.getCentre();
    const juce::Rectangle<float> circle (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

    g.setColour (juce::Colour (0xFF1e1a12));   // surface2
    g.fillEllipse (circle);
    g.setColour (juce::Colour (kPlayActive).withAlpha (0.20f));
    g.drawEllipse (circle, 1.0f);
}

void TransportStrip::paintLoopSlider (juce::Graphics& g) const
{
    // Track
    g.setColour (juce::Colour (kBtnBorder));
    g.fillRect (m_loopSliderTrack);

    // Thumb
    g.setColour (juce::Colour (kLoopThumb));
    g.fillEllipse (m_loopThumbRect);

    // Value label above thumb
    const juce::String valText = juce::String (static_cast<int> (m_loopLengthBars));
    const juce::Rectangle<float> labelRect (m_loopThumbRect.getCentreX() - 12.0f,
                                            m_loopSliderTrack.getY() - 12.0f,
                                            24.0f, 10.0f);
    g.setColour (juce::Colour (kPlayActive));
    g.setFont (Theme::Font::micro());
    g.drawText (valText, labelRect, juce::Justification::centred, false);
}

//==============================================================================
void TransportStrip::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.position;

    if (m_gearBtnRect.contains (pos))
    {
        if (onSettingsClicked) onSettingsClicked();
        return;
    }

    if (m_stopBtnRect.contains (pos))
    {
        setPlaying (false);
        m_currentBeat = 0.0f;
        m_listeners.call (&TransportListener::positionChanged, m_currentBeat);
        if (onBeatAdvanced)
            onBeatAdvanced (m_currentBeat);
        repaint();
        return;
    }

    if (m_playBtnRect.contains (pos))
    {
        setPlaying (!m_playing);
        return;
    }

    if (m_recBtnRect.contains (pos))
    {
        setRecording (!m_recording);
        return;
    }

    if (m_mixBtnRect.contains (pos))
    {
        m_mixOpen = !m_mixOpen;
        if (onMixToggled)
            onMixToggled();
        repaint();
        return;
    }

    // Loop-length slider: check proximity to track
    const float expandedH = 12.0f;
    const auto  trackHit  = m_loopSliderTrack.withHeight (expandedH)
                                .translated (0.0f, -(expandedH - m_loopSliderTrack.getHeight()) * 0.5f);
    if (trackHit.contains (pos))
    {
        m_draggingLoop = true;
        m_dragStartX   = pos.x;
        m_dragStartLoopLen = m_loopLengthBars;
    }
}

void TransportStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (!m_draggingLoop)
        return;

    const float delta   = e.position.x - m_dragStartX;
    const float range   = 31.0f;   // bars 1..32
    const float newBars = m_dragStartLoopLen + (delta / m_loopSliderTrack.getWidth()) * range;
    const float clamped = juce::jlimit (1.0f, 32.0f, newBars);

    if (std::fabs (clamped - m_loopLengthBars) > 0.01f)
    {
        setLoopLength (clamped);
        if (onLoopLengthChanged)
            onLoopLengthChanged (m_loopLengthBars);
    }
}

void TransportStrip::mouseUp (const juce::MouseEvent&)
{
    m_draggingLoop = false;
}

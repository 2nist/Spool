#include "LooperStrip.h"

namespace
{
constexpr int kSourceWidth = 68;
constexpr int kLiveWidth = 46;
constexpr int kCommandPad = 4;

const char* looperGrabLabel (int index) noexcept
{
    switch (index)
    {
        case 0: return "LAST 4";
        case 1: return "LAST 8";
        case 2: return "LAST 16";
        case 3: return "FREE";
        case 4: return "LOAD";
        default: break;
    }

    return "";
}

int looperGrabWidth (int index) noexcept
{
    switch (index)
    {
        case 0: return 38;
        case 1: return 38;
        case 2: return 44;
        case 3: return 34;
        case 4: return 36;
        default: break;
    }

    return 0;
}
}

LooperStrip::LooperStrip()
{
    addAndMakeVisible (m_unit);

    m_unit.onEditedClipChanged = [this] (const CapturedAudioClip& clip)
    {
        if (onEditedClipChanged != nullptr)
            onEditedClipChanged (clip);
    };
    m_unit.onPreviewStateChanged = [this] (bool playing, bool looping)
    {
        if (onPreviewStateChanged != nullptr)
            onPreviewStateChanged (playing, looping);
    };
    m_unit.onSendToReel = [this] (const CapturedAudioClip& clip)
    {
        if (onEditedClipSendToReel != nullptr)
            onEditedClipSendToReel (clip);
    };
    m_unit.onSendToTimeline = [this] (const CapturedAudioClip& clip)
    {
        if (onEditedClipSendToTimeline != nullptr)
            onEditedClipSendToTimeline (clip);
    };
    m_unit.onDragClipStarted = [this] (const CapturedAudioClip& clip)
    {
        if (onEditedClipDragStarted != nullptr)
            onEditedClipDragStarted (clip);
    };
    m_unit.onStatus = [this] (const juce::String& message)
    {
        if (onStatus != nullptr)
            onStatus (message);
    };
    m_unit.onQueryPreviewProgress = [this]()
    {
        return onQueryPreviewProgress != nullptr ? onQueryPreviewProgress() : 0.0f;
    };
    m_unit.onQueryPreviewPlaying = [this]()
    {
        return onQueryPreviewPlaying != nullptr ? onQueryPreviewPlaying() : false;
    };
}

void LooperStrip::setSourceNames (const juce::StringArray& names)
{
    m_sourceNames = names;
    if (! m_sourceNames.contains ("MIX"))
        m_sourceNames.insert (0, "MIX");

    m_unit.setSourceNames (m_sourceNames);
    repaint();
}

void LooperStrip::setHasGrabbedClip (bool hasClip)
{
    m_hasGrabbedClip = hasClip;
    repaint();
}

void LooperStrip::loadClip (const CapturedAudioClip& clip)
{
    m_unit.loadClip (clip);
    repaint();
}

void LooperStrip::overdubClip (const CapturedAudioClip& clip)
{
    m_unit.overdubClip (clip);
    repaint();
}

void LooperStrip::resized()
{
    m_unit.setBounds (0, kCommandH, getWidth(), LooperUnit::kHeight);
}

void LooperStrip::drawPillBtn (juce::Graphics& g,
                               juce::Rectangle<int> r,
                               const juce::String& label,
                               bool active,
                               juce::Colour activeCol)
{
    const juce::Colour fill = active
                              ? (activeCol.isTransparent() ? (juce::Colour) Theme::Colour::surface2
                                                           : activeCol.withAlpha (0.25f))
                              : (juce::Colour) Theme::Colour::surface1;
    g.setColour (fill);
    g.fillRoundedRectangle (r.toFloat(), 2.0f);
    g.setColour (active ? (activeCol.isTransparent() ? (juce::Colour) Theme::Colour::surfaceEdge
                                                     : activeCol)
                        : (juce::Colour) Theme::Colour::surfaceEdge);
    g.drawRoundedRectangle (r.toFloat(), 2.0f, 0.5f);

    g.setFont (Theme::Font::micro());
    g.setColour (active ? (juce::Colour) Theme::Colour::inkLight
                        : (juce::Colour) Theme::Colour::inkGhost);
    g.drawText (label, r, juce::Justification::centred, false);
}

void LooperStrip::paintRow1 (juce::Graphics& g) const
{
    const auto r = commandRow();
    const int pad = 4;
    int x = r.getX() + pad;

    {
        const juce::String src = (m_selectedSource < m_sourceNames.size()) ? m_sourceNames[m_selectedSource] : "MIX";
        const juce::Rectangle<int> br (x, r.getCentreY() - 7, kSourceWidth, 14);
        g.setColour (Theme::Colour::surface3);
        g.fillRoundedRectangle (br.toFloat(), 2.0f);
        g.setColour (Theme::Colour::surfaceEdge);
        g.drawRoundedRectangle (br.toFloat(), 2.0f, 0.5f);
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkMid);
        g.drawText ("SRC " + src + " v",
                    br.withTrimmedLeft (3),
                    juce::Justification::centredLeft,
                    true);
        x += kSourceWidth + kCommandPad;
    }

    {
        const juce::Colour liveCol = m_isLive ? juce::Colour (0xFF44DD66) : Theme::Colour::inkGhost;
        g.setColour (liveCol);
        g.fillEllipse ((float) x, (float) (r.getCentreY() - 3), 6.0f, 6.0f);
        g.setFont (Theme::Font::micro());
        g.drawText (m_isLive ? "LIVE" : "PAUSED",
                    juce::Rectangle<int> (x + 8, r.getY(), 38, r.getHeight()),
                    juce::Justification::centredLeft,
                    false);
        x += kLiveWidth;
    }

    const bool actives[] = { true, true, true, false, m_hasGrabbedClip };
    for (int i = 0; i < 5; ++i)
    {
        const int w = looperGrabWidth (i);
        drawPillBtn (g,
                     { x, r.getCentreY() - 7, w, 14 },
                     looperGrabLabel (i),
                     actives[i],
                     i == 4 ? juce::Colour (Theme::Zone::c) : juce::Colour());
        x += w + pad;
    }
}

void LooperStrip::paint (juce::Graphics& g)
{
    g.setColour (Theme::Colour::surface1);
    g.fillRect (getLocalBounds());
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawLine (0.0f, 0.0f, (float) getWidth(), 0.0f, 0.8f);
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.45f));
    g.drawLine (0.0f, (float) kCommandH, (float) getWidth(), (float) kCommandH, 0.6f);
    paintRow1 (g);
}

bool LooperStrip::sourceHitAt (juce::Point<int> pos) const noexcept
{
    return commandRow().contains (pos) && pos.x >= kCommandPad && pos.x <= kCommandPad + kSourceWidth;
}

bool LooperStrip::liveHitAt (juce::Point<int> pos) const noexcept
{
    const int start = kCommandPad + kSourceWidth + kCommandPad;
    return commandRow().contains (pos) && pos.x >= start && pos.x <= start + kLiveWidth;
}

int LooperStrip::row1BtnAt (juce::Point<int> pos) const noexcept
{
    if (! commandRow().contains (pos))
        return -1;

    const int cy = commandRow().getCentreY();
    if (pos.y < cy - 9 || pos.y > cy + 9)
        return -1;

    int x = kCommandPad + kSourceWidth + kCommandPad + kLiveWidth;
    for (int i = 0; i < 5; ++i)
    {
        const int w = looperGrabWidth (i);
        if (pos.x >= x && pos.x <= x + w)
            return i;
        x += w + kCommandPad;
    }

    return -1;
}

void LooperStrip::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    if (sourceHitAt (pos))
    {
        juce::PopupMenu menu;
        for (int i = 0; i < m_sourceNames.size(); ++i)
            menu.addItem (i + 1, m_sourceNames[i]);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [this] (int result)
                            {
                                if (result > 0)
                                {
                                    m_selectedSource = result - 1;
                                    if (onSourceChanged != nullptr)
                                        onSourceChanged (m_selectedSource);
                                    repaint();
                                }
                            });
        return;
    }

    if (liveHitAt (pos))
    {
        m_isLive = ! m_isLive;
        if (onLiveToggled != nullptr)
            onLiveToggled (m_isLive);
        repaint();
        return;
    }

    const int btn = row1BtnAt (pos);
    if (btn == 0 && onGrabLastBars != nullptr) { onGrabLastBars (4); return; }
    if (btn == 1 && onGrabLastBars != nullptr) { onGrabLastBars (8); return; }
    if (btn == 2 && onGrabLastBars != nullptr) { onGrabLastBars (16); return; }
    if (btn == 3 && onGrabFree != nullptr)     { onGrabFree(); return; }
    if (btn == 4 && onSendToLooper != nullptr) { onSendToLooper(); return; }
}

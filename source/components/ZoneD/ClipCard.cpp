#include "ClipCard.h"

//==============================================================================
void ClipCard::paint (juce::Graphics& g,
                      const juce::Rectangle<float>& rect,
                      const Clip& clip,
                      float loopLengthBeats,
                      float pxPerBeat)
{
    const auto& theme = ThemeManager::get().theme();

    // Card background
    const float alpha = (clip.type == ClipType::output) ? 0.6f : clip.type == ClipType::scaffold ? 0.45f : 1.0f;
    juce::Colour fill = theme.tapeClipBg;
    if (clip.tint.getAlpha() > 0)
        fill = theme.tapeClipBg.interpolatedWith (clip.tint, 0.35f);
    if (clip.type == ClipType::scaffold)
        fill = Theme::Colour::surface2.interpolatedWith (clip.tint, 0.18f);
    g.setColour (fill.withAlpha (alpha));
    g.fillRoundedRectangle (rect, Theme::Radius::xs);

    // Section colour tag (left edge)
    if (clip.tint.getAlpha() > 0)
    {
        g.setColour (clip.tint.withAlpha (0.90f));
        g.fillRect (rect.getX(), rect.getY(), 3.0f, rect.getHeight());
    }

    // Border — selected = Zone::d, normal = tapeClipBorder
    const juce::Colour borderCol = clip.type == ClipType::scaffold
                                   ? Theme::Colour::surfaceEdge.withAlpha (0.55f)
                                   : clip.selected
                                   ? Theme::Zone::d
                                   : theme.tapeClipBorder;
    g.setColour (borderCol);
    g.drawRoundedRectangle (rect, Theme::Radius::xs, 0.5f);

    // Clip content area (inset 2px)
    const auto contentR = rect.reduced (2.0f);

    if (clip.type == ClipType::audio)
        paintAudio (g, contentR, clip, loopLengthBeats, pxPerBeat);
    else if (clip.type == ClipType::midi)
        paintMidi (g, contentR, clip, loopLengthBeats, pxPerBeat);
    else if (clip.type == ClipType::scaffold)
        paintScaffold (g, contentR, clip);
    else
        paintOutput (g, contentR, clip);
}

//==============================================================================
void ClipCard::paintAudio (juce::Graphics& g,
                            const juce::Rectangle<float>& contentR,
                            const Clip& clip,
                            float loopLengthBeats,
                            float pxPerBeat)
{
    // Name top-left
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkDark);
    g.drawText (clip.name.toUpperCase(),
                contentR.withHeight (8.0f).toNearestInt(),
                juce::Justification::topLeft, false);

    // Bar count top-right
    const int bars = juce::jmax (1, juce::roundToInt (clip.lengthBeats / 4.0f));
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::paperEdge);
    g.drawText (juce::String (bars) + "b",
                contentR.withHeight (8.0f).toNearestInt(),
                juce::Justification::topRight, false);

    // Waveform placeholder — bottom half
    const float waveY = contentR.getBottom() - 10.0f;
    g.setColour (Theme::Colour::inkDark.withAlpha (0.35f));
    g.drawHorizontalLine (juce::roundToInt (waveY + 5.0f),
                          contentR.getX(),
                          contentR.getRight());
    // TODO: real waveform from audio data

    juce::ignoreUnused (loopLengthBeats, pxPerBeat);
}

//==============================================================================
void ClipCard::paintMidi (juce::Graphics& g,
                           const juce::Rectangle<float>& contentR,
                           const Clip& clip,
                           float loopLengthBeats,
                           float pxPerBeat)
{
    // Name top-left
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkDark);
    g.drawText (clip.name.toUpperCase(),
                contentR.withHeight (8.0f).toNearestInt(),
                juce::Justification::topLeft, false);

    // Bar count top-right
    const int bars = juce::jmax (1, juce::roundToInt (clip.lengthBeats / 4.0f));
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::paperEdge);
    g.drawText (juce::String (bars) + "b",
                contentR.withHeight (8.0f).toNearestInt(),
                juce::Justification::topRight, false);

    // Note grid placeholder — bottom 10px
    // 3 small rectangles at staggered heights
    // TODO: real MIDI note rendering from clip data
    const float gridTop  = contentR.getBottom() - 10.0f;
    const float gridH    = 10.0f;
    const juce::Colour noteCol = Theme::Colour::inkDark.withAlpha (0.50f);

    struct NoteRect { float xOff, yFrac, w; };
    const NoteRect notes[] = { {4.0f, 0.25f, 10.0f},
                                {18.0f, 0.65f, 7.0f},
                                {32.0f, 0.45f, 12.0f} };
    g.setColour (noteCol);
    for (const auto& n : notes)
    {
        const float ny = gridTop + n.yFrac * gridH;
        const float nx = contentR.getX() + n.xOff;
        if (nx + n.w < contentR.getRight())
            g.fillRect (nx, ny, n.w, 1.0f);
    }

    juce::ignoreUnused (loopLengthBeats, pxPerBeat);
}

//==============================================================================
void ClipCard::paintOutput (juce::Graphics& g,
                              const juce::Rectangle<float>& contentR,
                              const Clip& clip)
{
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkDark.withAlpha (0.60f));
    g.drawText (clip.name.toUpperCase(),
                contentR.toNearestInt(),
                juce::Justification::centred, false);
}

void ClipCard::paintScaffold (juce::Graphics& g,
                              const juce::Rectangle<float>& contentR,
                              const Clip& clip)
{
    auto area = contentR;
    g.setColour (Theme::Colour::inkGhost.withAlpha (0.9f));
    g.setFont (Theme::Font::micro());
    g.drawText (clip.name.toUpperCase(),
                area.removeFromTop (8.0f).toNearestInt(),
                juce::Justification::centredLeft,
                true);

    g.setColour (Theme::Colour::inkGhost.withAlpha (0.35f));
    for (float x = area.getX(); x < area.getRight(); x += 6.0f)
        g.drawLine (x, area.getBottom() - 2.0f, juce::jmin (x + 3.0f, area.getRight()), area.getBottom() - 2.0f, 1.0f);
}

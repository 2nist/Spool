#include "LooperUnit.h"

#include <array>
#include <cmath>

namespace
{
juce::Colour looperAccent() noexcept
{
    return juce::Colour (Theme::Zone::c);
}

void buildWavePath (juce::Path& path,
                    const juce::AudioBuffer<float>& buffer,
                    juce::Rectangle<float> area,
                    int startSample,
                    int endSample)
{
    path.clear();

    const int samples = buffer.getNumSamples();
    if (samples <= 0 || buffer.getNumChannels() <= 0 || area.getWidth() <= 1.0f)
        return;

    const int start = juce::jlimit (0, samples - 1, startSample);
    const int end = juce::jlimit (start + 1, samples, endSample);
    const int span = juce::jmax (1, end - start);
    const float centreY = area.getCentreY();
    const float halfH = area.getHeight() * 0.45f;

    path.startNewSubPath (area.getX(), centreY);

    for (int x = 0; x < static_cast<int> (area.getWidth()); ++x)
    {
        const float nx = area.getWidth() > 1.0f ? static_cast<float> (x) / (area.getWidth() - 1.0f) : 0.0f;
        const int sliceStart = start + juce::roundToInt (nx * static_cast<float> (span - 1));
        const int sliceEnd = juce::jmin (end, sliceStart + juce::jmax (1, span / juce::jmax (1, static_cast<int> (area.getWidth()))));

        float peak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            peak = juce::jmax (peak, buffer.getMagnitude (ch, sliceStart, juce::jmax (1, sliceEnd - sliceStart)));

        const float y = centreY - peak * halfH;
        path.lineTo (area.getX() + static_cast<float> (x), y);
    }

    for (int x = static_cast<int> (area.getWidth()) - 1; x >= 0; --x)
    {
        const float nx = area.getWidth() > 1.0f ? static_cast<float> (x) / (area.getWidth() - 1.0f) : 0.0f;
        const int sliceStart = start + juce::roundToInt (nx * static_cast<float> (span - 1));
        const int sliceEnd = juce::jmin (end, sliceStart + juce::jmax (1, span / juce::jmax (1, static_cast<int> (area.getWidth()))));

        float peak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            peak = juce::jmax (peak, buffer.getMagnitude (ch, sliceStart, juce::jmax (1, sliceEnd - sliceStart)));

        const float y = centreY + peak * halfH;
        path.lineTo (area.getX() + static_cast<float> (x), y);
    }

    path.closeSubPath();
}
}

LooperUnit::LooperUnit()
{
    m_formatManager.registerBasicFormats();
    startTimerHz (20);
}

void LooperUnit::setSourceNames (const juce::StringArray& names)
{
    m_sourceNames = names;
}

void LooperUnit::loadClip (const CapturedAudioClip& clip)
{
    m_session.loadFromClip (clip);
    m_session.playbackProgress = 0.0f;
    syncEditedClip();
    syncPreviewState();
    repaint();

    if (onStatus != nullptr)
        onStatus ("Looper loaded " + juce::String (clip.durationSeconds(), 1) + "s from " + clip.sourceName);
}

void LooperUnit::overdubClip (const CapturedAudioClip& clip)
{
    if (! m_session.hasAudio())
    {
        loadClip (clip);
        return;
    }

    m_session.overdubFrom (clip);
    syncEditedClip();
    syncPreviewState();
    repaint();

    if (onStatus != nullptr)
        onStatus ("Looper overdubbed from " + clip.sourceName);
}

juce::Rectangle<int> LooperUnit::toolbarRect() const noexcept
{
    return getLocalBounds().removeFromTop (kToolbarH);
}

juce::Rectangle<int> LooperUnit::waveformRect() const noexcept
{
    auto area = getLocalBounds();
    area.removeFromTop (kToolbarH + 1);
    area.removeFromBottom (kFooterH);
    return area.reduced (4, 2);
}

juce::Rectangle<int> LooperUnit::footerRect() const noexcept
{
    return getLocalBounds().removeFromBottom (kFooterH);
}

juce::Rectangle<int> LooperUnit::buttonRect (ButtonId id) const noexcept
{
    auto row = toolbarRect().reduced (4, 1);
    constexpr int gap = 4;

    auto next = [&row] (int width)
    {
        auto r = row.removeFromLeft (width);
        row.removeFromLeft (gap);
        return r;
    };

    switch (id)
    {
        case ButtonId::play:         return next (34);
        case ButtonId::loop:         return next (34);
        case ButtonId::dub:          return next (30);
        case ButtonId::normalize:    return next (34);
        case ButtonId::resetTrim:    return next (30);
        case ButtonId::open:         return next (32);
        case ButtonId::save:         return next (32);
        case ButtonId::sendReel:     return next (34);
        case ButtonId::sendTimeline: return next (34);
        case ButtonId::drag:         return { row.getRight() - 26, row.getY(), 26, row.getHeight() };
        case ButtonId::none:         break;
    }

    return {};
}

juce::Rectangle<float> LooperUnit::trimHandleRect (bool startHandle) const noexcept
{
    const auto wave = waveformRect().toFloat();
    if (! m_session.hasAudio())
        return {};

    const float startNorm = m_session.totalSamples() > 0 ? static_cast<float> (m_session.trimStartSample) / static_cast<float> (m_session.totalSamples()) : 0.0f;
    const float endNorm = m_session.totalSamples() > 0 ? static_cast<float> (m_session.trimEndSample) / static_cast<float> (m_session.totalSamples()) : 1.0f;
    const float x = startHandle ? wave.getX() + wave.getWidth() * startNorm
                                : wave.getX() + wave.getWidth() * endNorm;
    return { x - 3.0f, wave.getY(), 6.0f, wave.getHeight() };
}

LooperUnit::ButtonId LooperUnit::buttonAt (juce::Point<int> pos) const noexcept
{
    const ButtonId buttons[] =
    {
        ButtonId::play, ButtonId::loop, ButtonId::dub, ButtonId::normalize, ButtonId::resetTrim,
        ButtonId::open, ButtonId::save, ButtonId::sendReel, ButtonId::sendTimeline, ButtonId::drag
    };

    for (auto button : buttons)
        if (buttonRect (button).contains (pos))
            return button;

    return ButtonId::none;
}

LooperUnit::DragTarget LooperUnit::dragTargetAt (juce::Point<int> pos) const noexcept
{
    if (trimHandleRect (true).contains ((float) pos.x, (float) pos.y))
        return DragTarget::trimStart;
    if (trimHandleRect (false).contains ((float) pos.x, (float) pos.y))
        return DragTarget::trimEnd;
    return DragTarget::none;
}

void LooperUnit::drawToolbarButton (juce::Graphics& g,
                                    juce::Rectangle<int> r,
                                    const juce::String& label,
                                    bool active,
                                    juce::Colour accent) const
{
    const auto edge = active ? accent : Theme::Colour::surfaceEdge;
    const auto fill = active ? accent.withAlpha (0.28f) : Theme::Colour::surface2;
    g.setColour (fill);
    g.fillRoundedRectangle (r.toFloat(), 2.5f);
    g.setColour (edge);
    g.drawRoundedRectangle (r.toFloat(), 2.5f, 0.7f);
    g.setColour (active ? Theme::Colour::inkLight : Theme::Colour::inkGhost);
    g.setFont (Theme::Font::micro());
    g.drawText (label, r, juce::Justification::centred, false);
}

void LooperUnit::paintToolbar (juce::Graphics& g) const
{
    const auto accent = looperAccent();
    const auto buttons = std::array<ButtonId, 10>
    {
        ButtonId::play, ButtonId::loop, ButtonId::dub, ButtonId::normalize, ButtonId::resetTrim,
        ButtonId::open, ButtonId::save, ButtonId::sendReel, ButtonId::sendTimeline, ButtonId::drag
    };

    for (auto button : buttons)
    {
        juce::String label;
        bool active = false;

        switch (button)
        {
            case ButtonId::play:         label = m_session.playing ? "STOP" : "PLAY"; active = m_session.playing; break;
            case ButtonId::loop:         label = "LOOP"; active = m_session.looping; break;
            case ButtonId::dub:          label = "DUB"; active = m_session.overdubEnabled; break;
            case ButtonId::normalize:    label = "NORM"; active = m_session.normalized; break;
            case ButtonId::resetTrim:    label = "FULL"; active = false; break;
            case ButtonId::open:         label = "OPEN"; active = false; break;
            case ButtonId::save:         label = "SAVE"; active = false; break;
            case ButtonId::sendReel:     label = "REEL"; active = false; break;
            case ButtonId::sendTimeline: label = "TMLN"; active = false; break;
            case ButtonId::drag:         label = "DRG"; active = false; break;
            case ButtonId::none:         break;
        }

        drawToolbarButton (g, buttonRect (button), label, active, accent);
    }
}

void LooperUnit::paintWaveform (juce::Graphics& g) const
{
    auto wave = waveformRect();
    g.setColour (Theme::Colour::surface2);
    g.fillRoundedRectangle (wave.toFloat(), 4.0f);
    g.setColour (Theme::Colour::surfaceEdge);
    g.drawRoundedRectangle (wave.toFloat(), 4.0f, 0.8f);

    if (! m_session.hasAudio())
    {
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText ("LOAD A CLIP OR SEND TAPE TO LOOPER", wave, juce::Justification::centred, false);
        return;
    }

    const auto accent = looperAccent();
    const float total = static_cast<float> (juce::jmax (1, m_session.totalSamples()));
    const float trimStartX = wave.getX() + wave.getWidth() * (static_cast<float> (m_session.trimStartSample) / total);
    const float trimEndX = wave.getX() + wave.getWidth() * (static_cast<float> (m_session.trimEndSample) / total);

    g.setColour (Theme::Colour::surface0.withAlpha (0.55f));
    g.fillRect (wave.getX(), wave.getY(), juce::roundToInt (trimStartX - (float) wave.getX()), wave.getHeight());
    g.fillRect (juce::roundToInt (trimEndX), wave.getY(), wave.getRight() - juce::roundToInt (trimEndX), wave.getHeight());

    juce::Path wavePath;
    buildWavePath (wavePath, m_session.workingBuffer, wave.toFloat(), 0, m_session.totalSamples());
    g.setColour (accent.withAlpha (0.25f));
    g.fillPath (wavePath);
    g.setColour (accent.withAlpha (0.75f));
    g.strokePath (wavePath, juce::PathStrokeType (1.0f));

    g.setColour (accent.withAlpha (0.18f));
    g.fillRect (juce::Rectangle<float> (trimStartX, (float) wave.getY(),
                                        juce::jmax (2.0f, trimEndX - trimStartX), (float) wave.getHeight()));

    auto drawHandle = [&] (bool startHandle)
    {
        auto handle = trimHandleRect (startHandle);
        g.setColour (accent.withAlpha (0.95f));
        g.fillRoundedRectangle (handle, 2.0f);
    };

    drawHandle (true);
    drawHandle (false);

    const float playheadX = wave.getX() + wave.getWidth() * juce::jlimit (0.0f, 1.0f, m_session.playbackProgress);
    g.setColour (Theme::Colour::inkLight.withAlpha (0.9f));
    g.drawLine (playheadX, (float) wave.getY(), playheadX, (float) wave.getBottom(), 1.1f);

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText ("TRIM", wave.removeFromTop (10).withTrimmedLeft (4), juce::Justification::centredLeft, false);
    g.drawText (m_session.looping ? "LOOP ON" : "ONE SHOT",
                wave.removeFromBottom (10).withTrimmedRight (4),
                juce::Justification::centredRight,
                false);
}

void LooperUnit::paintFooter (juce::Graphics& g) const
{
    auto area = footerRect().reduced (4, 0);
    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);

    if (! m_session.hasAudio())
    {
        g.drawText ("tape history -> looper -> reel/timeline", area, juce::Justification::centredLeft, false);
        return;
    }

    const juce::String left = (m_session.sourceName.isNotEmpty() ? m_session.sourceName : "LOOPER")
                              + "  "
                              + juce::String (m_session.durationSeconds(), 2) + "s"
                              + (m_session.tempo > 0.0 ? "  " + juce::String (m_session.tempo, 0) + " BPM" : juce::String());
    const juce::String right = juce::String (m_session.trimStartSample)
                               + " - "
                               + juce::String (m_session.trimEndSample)
                               + (m_session.bars > 0 ? "  " + juce::String (m_session.bars) + " bars" : juce::String());

    g.drawText (left, area.removeFromLeft (juce::jmax (120, area.getWidth() / 2)), juce::Justification::centredLeft, false);
    g.drawText (right, area, juce::Justification::centredRight, false);
}

void LooperUnit::paint (juce::Graphics& g)
{
    g.setColour (Theme::Colour::surface1);
    g.fillRect (getLocalBounds());
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.65f));
    g.drawLine (0.0f, 0.0f, static_cast<float> (getWidth()), 0.0f, 0.7f);

    paintToolbar (g);
    paintWaveform (g);
    paintFooter (g);
}

void LooperUnit::resized()
{
}

void LooperUnit::syncEditedClip()
{
    if (onEditedClipChanged != nullptr && m_session.hasAudio())
        onEditedClipChanged (m_session.makeEditedClip());
}

void LooperUnit::syncPreviewState()
{
    if (onPreviewStateChanged != nullptr)
        onPreviewStateChanged (m_session.playing && m_session.hasAudio(), m_session.looping);
}

void LooperUnit::chooseLoadFile()
{
    m_fileChooser = std::make_unique<juce::FileChooser> ("Load Looper Clip", juce::File(), "*.wav;*.aif;*.aiff;*.flac");
    m_fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                [safeThis = juce::Component::SafePointer<LooperUnit> (this)] (const juce::FileChooser& chooser)
                                {
                                    if (safeThis == nullptr)
                                        return;

                                    const auto file = chooser.getResult();
                                    if (file.existsAsFile())
                                        safeThis->loadFromFile (file);
                                });
}

void LooperUnit::chooseSaveFile()
{
    if (! m_session.hasAudio())
        return;

    m_fileChooser = std::make_unique<juce::FileChooser> ("Save Looper Clip", juce::File(), "*.wav");
    m_fileChooser->launchAsync (juce::FileBrowserComponent::saveMode
                                | juce::FileBrowserComponent::canSelectFiles
                                | juce::FileBrowserComponent::warnAboutOverwriting,
                                [safeThis = juce::Component::SafePointer<LooperUnit> (this)] (const juce::FileChooser& chooser)
                                {
                                    if (safeThis == nullptr)
                                        return;

                                    auto file = chooser.getResult();
                                    if (file == juce::File())
                                        return;

                                    if (! file.hasFileExtension ("wav"))
                                        file = file.withFileExtension ("wav");

                                    safeThis->saveToFile (file);
                                });
}

bool LooperUnit::loadFromFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader (m_formatManager.createReaderFor (file));
    if (reader == nullptr)
        return false;

    juce::AudioBuffer<float> buffer ((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read (&buffer, 0, (int) reader->lengthInSamples, 0, true, true);

    CapturedAudioClip clip;
    clip.buffer.makeCopyOf (buffer);
    clip.sampleRate = reader->sampleRate;
    clip.sourceName = file.getFileNameWithoutExtension();
    clip.sourceSlot = -1;
    loadClip (clip);
    m_session.sourcePath = file.getFullPathName();

    if (onStatus != nullptr)
        onStatus ("Looper opened " + file.getFileName());

    return true;
}

bool LooperUnit::saveToFile (const juce::File& file)
{
    if (! m_session.hasAudio())
        return false;

    juce::WavAudioFormat format;
    std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
        return false;

    auto edited = m_session.makeEditedClip();
    const auto options = juce::AudioFormatWriterOptions {}
                             .withSampleRate (edited.sampleRate)
                             .withNumChannels (edited.buffer.getNumChannels())
                             .withBitsPerSample (24);
    auto writer = format.createWriterFor (stream, options);
    if (writer == nullptr)
        return false;

    writer->writeFromAudioSampleBuffer (edited.buffer, 0, edited.buffer.getNumSamples());
    m_session.sourcePath = file.getFullPathName();

    if (onStatus != nullptr)
        onStatus ("Looper saved " + file.getFileName());

    return true;
}

int LooperUnit::sampleFromWaveX (int x) const noexcept
{
    const auto wave = waveformRect();
    if (! m_session.hasAudio() || wave.getWidth() <= 1)
        return 0;

    const float norm = juce::jlimit (0.0f, 1.0f, (float) (x - wave.getX()) / (float) wave.getWidth());
    return juce::roundToInt (norm * (float) m_session.totalSamples());
}

void LooperUnit::beginDragOut()
{
    if (! m_session.hasAudio())
        return;

    auto clip = m_session.makeEditedClip();
    if (onDragClipStarted != nullptr)
        onDragClipStarted (clip);

    juce::Image image (juce::Image::ARGB, 92, 24, true);
    juce::Graphics g (image);
    g.fillAll (juce::Colours::transparentBlack);
    g.setColour (looperAccent().withAlpha (0.9f));
    g.fillRoundedRectangle ({ 0.0f, 0.0f, 92.0f, 24.0f }, 4.0f);
    g.setColour (Theme::Colour::inkLight);
    g.setFont (Theme::Font::micro());
    g.drawText ("LOOPER CLIP", image.getBounds(), juce::Justification::centred, false);

    if (auto* ddc = juce::DragAndDropContainer::findParentDragContainerFor (this))
        ddc->startDragging ("CapturedAudioClip", this, juce::ScaledImage (image));
}

void LooperUnit::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();
    m_pressedButton = buttonAt (pos);
    if (m_pressedButton != ButtonId::none)
    {
        switch (m_pressedButton)
        {
            case ButtonId::play:
                if (m_session.hasAudio())
                {
                    m_session.playing = ! m_session.playing;
                    syncPreviewState();
                }
                break;
            case ButtonId::loop:
                m_session.looping = ! m_session.looping;
                syncPreviewState();
                break;
            case ButtonId::dub:
                m_session.overdubEnabled = ! m_session.overdubEnabled;
                repaint();
                return;
            case ButtonId::normalize:
                m_session.normalize();
                syncEditedClip();
                syncPreviewState();
                break;
            case ButtonId::resetTrim:
                m_session.resetTrim();
                syncEditedClip();
                syncPreviewState();
                break;
            case ButtonId::open:
                chooseLoadFile();
                return;
            case ButtonId::save:
                chooseSaveFile();
                return;
            case ButtonId::sendReel:
                if (onSendToReel != nullptr && m_session.hasAudio())
                    onSendToReel (m_session.makeEditedClip());
                return;
            case ButtonId::sendTimeline:
                if (onSendToTimeline != nullptr && m_session.hasAudio())
                    onSendToTimeline (m_session.makeEditedClip());
                return;
            case ButtonId::drag:
                beginDragOut();
                return;
            case ButtonId::none:
                break;
        }

        repaint();
        return;
    }

    m_dragTarget = dragTargetAt (pos);
}

void LooperUnit::mouseDrag (const juce::MouseEvent& e)
{
    if (! m_session.hasAudio())
        return;

    if (m_dragTarget == DragTarget::trimStart)
    {
        m_session.setTrimStart (sampleFromWaveX (e.getPosition().x));
        syncEditedClip();
        syncPreviewState();
        repaint();
    }
    else if (m_dragTarget == DragTarget::trimEnd)
    {
        m_session.setTrimEnd (sampleFromWaveX (e.getPosition().x));
        syncEditedClip();
        syncPreviewState();
        repaint();
    }
}

void LooperUnit::mouseUp (const juce::MouseEvent&)
{
    m_pressedButton = ButtonId::none;
    m_dragTarget = DragTarget::none;
}

juce::MouseCursor LooperUnit::getMouseCursor()
{
    if (m_dragTarget != DragTarget::none)
        return juce::MouseCursor::LeftRightResizeCursor;
    return juce::Component::getMouseCursor();
}

void LooperUnit::timerCallback()
{
    const bool playing = onQueryPreviewPlaying != nullptr ? onQueryPreviewPlaying() : m_session.playing;
    const float progress = onQueryPreviewProgress != nullptr ? onQueryPreviewProgress()
                                                            : (m_session.playing ? m_session.playbackProgress : 0.0f);

    if (m_session.playing != playing || std::abs (m_session.playbackProgress - progress) > 0.01f)
    {
        m_session.playing = playing;
        m_session.playbackProgress = progress;
        repaint (waveformRect());
        repaint (toolbarRect());
    }
}

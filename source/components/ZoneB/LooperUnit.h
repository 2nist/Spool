#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include "../../Theme.h"
#include "LooperSession.h"

class LooperUnit : public juce::Component,
                   private juce::Timer
{
public:
    LooperUnit();
    ~LooperUnit() override = default;

    void loadClip (const CapturedAudioClip& clip);
    void overdubClip (const CapturedAudioClip& clip);
    bool hasClip() const noexcept { return m_session.hasAudio(); }
    bool isOverdubEnabled() const noexcept { return m_session.overdubEnabled; }
    CapturedAudioClip getEditedClip() const { return m_session.makeEditedClip(); }
    void setSourceNames (const juce::StringArray& names);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    juce::MouseCursor getMouseCursor() override;

    static constexpr int kHeight = 84;
    static constexpr int kToolbarH = 18;
    static constexpr int kFooterH = 14;

    std::function<void (const CapturedAudioClip& clip)> onEditedClipChanged;
    std::function<void (bool playing, bool looping)> onPreviewStateChanged;
    std::function<void (const CapturedAudioClip& clip)> onSendToReel;
    std::function<void (const CapturedAudioClip& clip)> onSendToTimeline;
    std::function<void (const CapturedAudioClip& clip)> onDragClipStarted;
    std::function<void (const juce::String& message)> onStatus;
    std::function<float()> onQueryPreviewProgress;
    std::function<bool()> onQueryPreviewPlaying;

private:
    enum class ButtonId
    {
        none = 0,
        play,
        loop,
        dub,
        normalize,
        resetTrim,
        open,
        save,
        sendReel,
        sendTimeline,
        drag
    };

    enum class DragTarget
    {
        none = 0,
        trimStart,
        trimEnd
    };

    LooperSession m_session;
    juce::StringArray m_sourceNames;
    juce::AudioFormatManager m_formatManager;
    std::unique_ptr<juce::FileChooser> m_fileChooser;
    ButtonId m_pressedButton { ButtonId::none };
    DragTarget m_dragTarget { DragTarget::none };

    juce::Rectangle<int> toolbarRect() const noexcept;
    juce::Rectangle<int> waveformRect() const noexcept;
    juce::Rectangle<int> footerRect() const noexcept;
    juce::Rectangle<int> buttonRect (ButtonId id) const noexcept;
    juce::Rectangle<float> trimHandleRect (bool startHandle) const noexcept;

    ButtonId buttonAt (juce::Point<int>) const noexcept;
    DragTarget dragTargetAt (juce::Point<int>) const noexcept;

    void paintToolbar (juce::Graphics&) const;
    void paintWaveform (juce::Graphics&) const;
    void paintFooter (juce::Graphics&) const;
    void drawToolbarButton (juce::Graphics&, juce::Rectangle<int>, const juce::String&, bool active, juce::Colour accent = {}) const;
    void syncEditedClip();
    void syncPreviewState();
    void chooseLoadFile();
    void chooseSaveFile();
    bool loadFromFile (const juce::File&);
    bool saveToFile (const juce::File&);
    int sampleFromWaveX (int x) const noexcept;
    void beginDragOut();
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperUnit)
};

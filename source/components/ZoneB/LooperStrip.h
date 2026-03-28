#pragma once

#include "LooperUnit.h"

class LooperStrip : public juce::Component
{
public:
    LooperStrip();
    ~LooperStrip() override = default;

    void setSourceNames (const juce::StringArray& names);
    void setHasGrabbedClip (bool hasClip);
    void loadClip (const CapturedAudioClip& clip);
    void overdubClip (const CapturedAudioClip& clip);
    bool hasEditedClip() const noexcept { return m_unit.hasClip(); }
    bool isOverdubEnabled() const noexcept { return m_unit.isOverdubEnabled(); }
    CapturedAudioClip getEditedClip() const { return m_unit.getEditedClip(); }

    static constexpr int kCommandH = 24;
    static constexpr int kHeight   = kCommandH + LooperUnit::kHeight;

    std::function<void(int numBars)> onGrabLastBars;
    std::function<void()> onGrabFree;
    std::function<void()> onSendToReel;
    std::function<void()> onSendToLooper;
    std::function<void()> onSendToTimeline;
    std::function<void(bool active)> onLiveToggled;
    std::function<void(int sourceIndex)> onSourceChanged;
    std::function<void(const CapturedAudioClip& clip)> onEditedClipChanged;
    std::function<void(bool playing, bool looping)> onPreviewStateChanged;
    std::function<void(const CapturedAudioClip& clip)> onEditedClipSendToReel;
    std::function<void(const CapturedAudioClip& clip)> onEditedClipSendToTimeline;
    std::function<void(const CapturedAudioClip& clip)> onEditedClipDragStarted;
    std::function<void(const juce::String& message)> onStatus;
    std::function<float()> onQueryPreviewProgress;
    std::function<bool()> onQueryPreviewPlaying;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::StringArray m_sourceNames;
    int m_selectedSource { 0 };
    bool m_isLive { true };
    bool m_hasGrabbedClip { false };
    LooperUnit m_unit;

    juce::Rectangle<int> commandRow() const noexcept { return { 0, 0, getWidth(), kCommandH }; }
    int row1BtnAt (juce::Point<int> pos) const noexcept;
    bool liveHitAt (juce::Point<int> pos) const noexcept;
    bool sourceHitAt (juce::Point<int> pos) const noexcept;
    void paintRow1 (juce::Graphics&) const;

    static void drawPillBtn (juce::Graphics& g,
                             juce::Rectangle<int> r,
                             const juce::String& label,
                             bool active,
                             juce::Colour activeCol = {});

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LooperStrip)
};

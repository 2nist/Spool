#pragma once

#include "../../Theme.h"
#include "DrumMachineData.h"
#include <functional>
#include <memory>

//==============================================================================
/**
    StepGridDrum — multi-voice step grid for a DRUM MACHINE slot.

    Each voice has its own row showing:
      - 48px label column: voice name, mute "M", mode "S/♩"
      - Step cells (fills remaining width)
      - Step count controls [N] − + at row right edge

    Voice rows scroll vertically inside a 36px viewport.
    "+ ADD VOICE" dashed button occupies the bottom 12px.
*/
class StepGridDrum : public juce::Component
{
public:
    StepGridDrum();
    ~StepGridDrum() override = default;

    /** Attach drum data.  Pointer must remain valid while displayed. */
    void setDrumData  (DrumMachineData* data, juce::Colour groupColor);
    void clearDrumData();

    /** Fired on the message thread whenever any step or step-count changes. */
    std::function<void()> onModified;

    /** Highlight the current playhead column across all voice rows. */
    void setPlayhead (int stepIndex);

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

    static constexpr int kRowH    = 16;
    static constexpr int kLabelW  = 48;
    static constexpr int kBtnW    = 14;
    static constexpr int kInspectorH = 18;
    static constexpr int kAddH       = 14;

private:
    DrumMachineData* m_data       { nullptr };
    juce::Colour     m_groupColor { 0xFF4B9EDB };

    //==========================================================================
    // Inner content component
    //==========================================================================
    struct VoiceContent : public juce::Component
    {
        StepGridDrum* owner;
        explicit VoiceContent (StepGridDrum* o) : owner (o) {}
        void paint    (juce::Graphics& g) override { owner->paintVoices (g); }
        void mouseDown (const juce::MouseEvent& e) override { owner->voiceMouseDown (e); }
        void mouseUp   (const juce::MouseEvent& e) override { owner->voiceMouseUp (e); }
        void mouseDrag (const juce::MouseEvent& e) override { owner->voiceDrag (e); }
        void mouseMove (const juce::MouseEvent& e) override { owner->voiceMove (e); }
    };

    std::unique_ptr<VoiceContent> m_voiceContent;
    juce::Viewport                m_viewport;

    //==========================================================================
    // Methods called from VoiceContent delegates
    //==========================================================================
    void paintVoices   (juce::Graphics& g);
    void voiceMouseDown (const juce::MouseEvent& e);
    void voiceMouseUp   (const juce::MouseEvent& e);
    void voiceDrag      (const juce::MouseEvent& e);
    void voiceMove      (const juce::MouseEvent& e);

    //==========================================================================
    // Layout helpers (in VoiceContent coordinates)
    //==========================================================================
    int  contentW()     const noexcept { return m_voiceContent ? m_voiceContent->getWidth() : 0; }
    int  stepGridW()    const noexcept { return contentW() - kLabelW - 2 * kBtnW - 4; }

    juce::Rectangle<int> voiceRowRect    (int vi) const noexcept;
    juce::Rectangle<int> muteRect        (int vi) const noexcept;
    juce::Rectangle<int> modeRect        (int vi) const noexcept;
    juce::Rectangle<int> decBtnRect      (int vi) const noexcept;
    juce::Rectangle<int> incBtnRect      (int vi) const noexcept;
    juce::Rectangle<int> stepCellRect    (int vi, int step) const noexcept;
    juce::Rectangle<int> inspectorRect   () const noexcept;
    juce::Rectangle<int> addVoiceRect    () const noexcept;

    int  voiceAtY (int y) const noexcept;
    int  stepAtX  (int vi, int x) const noexcept;

    void paintVoiceRow (juce::Graphics& g, int vi) const;
    void addNewVoice();
    void rightClickVoice (int vi);
    void rightClickStep (int vi, int step);
    void duplicateStep (int vi, int step);
    void clearStep (int vi, int step);

    // Paint-mode drag tracking
    bool m_paintMode   { false };
    int  m_lastVoice   { -1 };
    int  m_lastStep    { -1 };
    int  m_playhead    { -1 };
    int  m_hoverVoice  { -1 };
    int  m_hoverStep   { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StepGridDrum)
};

#pragma once
#include "../../Theme.h"

//==============================================================================
/** Observer interface for transport state changes. */
class TransportListener
{
public:
    virtual ~TransportListener() = default;
    virtual void playStateChanged   (bool playing)    = 0;
    virtual void positionChanged    (float beat)      = 0;
    virtual void recordStateChanged (bool recording)  = 0;
};

//==============================================================================
/** 28px transport/loop control strip at the bottom of Zone D. */
class TransportStrip  : public juce::Component,
                        public juce::Timer
{
public:
    enum class StructureFollowState
    {
        legacy,
        ready,
        active
    };

    TransportStrip();
    ~TransportStrip() override;

    //--- Listener management -------------------------------------------------
    void addTransportListener    (TransportListener* l);
    void removeTransportListener (TransportListener* l);

    //--- State accessors ------------------------------------------------------
    bool  isPlaying()        const noexcept { return m_playing; }
    bool  isRecording()      const noexcept { return m_recording; }
    float currentBeat()      const noexcept { return m_currentBeat; }
    float loopLengthBars()   const noexcept { return m_loopLengthBars; }
    float bpm()              const noexcept { return m_bpm; }

    //--- External control (from ZoneDComponent) ------------------------------
    void setPlaying     (bool playing);
    void setRecording   (bool recording);
    void setBpm         (float bpm);
    void setLoopLength  (float bars);
    void setStructureFollowState (StructureFollowState state);

    //--- Callbacks to ZoneDComponent ------------------------------------------
    std::function<void(float loopLenBars)> onLoopLengthChanged;
    std::function<void()>                  onMixToggled;
    /** Called every timer tick (60 Hz) with the current beat position. */
    std::function<void(float beat)>        onBeatAdvanced;
    /** Called when the ⚙ gear button is clicked — open transport settings panel. */
    std::function<void()>                  onSettingsClicked;

    //--- juce::Component ------------------------------------------------------
    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown  (const juce::MouseEvent&) override;
    void mouseDrag  (const juce::MouseEvent&) override;
    void mouseUp    (const juce::MouseEvent&) override;

    //--- juce::Timer ----------------------------------------------------------
    void timerCallback() override;

private:
    //--- Visual colour constants (local, not in Theme.h) --------------------
    static constexpr juce::uint32 kBgColour       = 0xFF1a1510;
    static constexpr juce::uint32 kBtnSurface      = 0xFF2a2218;
    static constexpr juce::uint32 kBtnBorder       = 0xFF3a3228;
    static constexpr juce::uint32 kPlayActive      = 0xFF694412;   // Zone::d tint
    static constexpr juce::uint32 kRecordArmed     = 0xFFc03030;   // error
    static constexpr juce::uint32 kTextColour      = 0xFF9a8860;
    static constexpr juce::uint32 kLoopThumb       = 0xFF694412;

    //--- Transport state ------------------------------------------------------
    float m_bpm            = 120.0f;
    float m_loopLengthBars = 8.0f;
    bool  m_playing        = false;
    bool  m_recording      = false;
    float m_currentBeat    = 0.0f;
    bool  m_mixOpen        = false;

    //--- Timer internals ------------------------------------------------------
    static constexpr int kTimerHz = 60;

    //--- Hit-test rects (computed in resized()) -------------------------------
    juce::Rectangle<float> m_gearBtnRect;
    juce::Rectangle<float> m_stopBtnRect;
    juce::Rectangle<float> m_playBtnRect;
    juce::Rectangle<float> m_recBtnRect;
    juce::Rectangle<float> m_drumRect;
    juce::Rectangle<float> m_loopSliderTrack;
    juce::Rectangle<float> m_loopThumbRect;
    juce::Rectangle<float> m_snapLabelRect;
    juce::Rectangle<float> m_mixBtnRect;
    StructureFollowState m_structureFollowState { StructureFollowState::legacy };

    //--- Drag state for loop-length slider ------------------------------------
    bool  m_draggingLoop   = false;
    float m_dragStartX     = 0.0f;
    float m_dragStartLoopLen = 8.0f;

    //--- Listener list --------------------------------------------------------
    juce::ListenerList<TransportListener> m_listeners;

    //--- Helpers --------------------------------------------------------------
    void updateLoopThumbRect();
    void paintTransportBtn (juce::Graphics& g,
                            const juce::Rectangle<float>& r,
                            const juce::String& label,
                            bool active,
                            juce::uint32 activeColour) const;
    void paintMiniDrum    (juce::Graphics& g) const;
    void paintLoopSlider  (juce::Graphics& g) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransportStrip)
};

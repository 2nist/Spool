#pragma once

#include "../../Theme.h"

//==============================================================================
/** A single scrolling message on the system feed tape. */
struct SystemMessage
{
    juce::String text;
    juce::Colour color     { 0xFF3a2e18 };   // dark ink on tan tape
    double       timestamp = 0.0;
    float        baseX     = 0.0f;           // absolute x in scroll space
    float        width     = 0.0f;           // pixel width of rendered text
};

//==============================================================================
/**
    SystemFeedCylinder — 32px scrolling message tape above the zone area.

    Same warm-tan cylinder aesthetic as Zone D, scaled to 32px height.
    Messages scroll right-to-left at ~40px/sec. Right-click shows a
    preference popup to filter message types. Preferences saved to
    juce::PropertiesFile (user settings, not session).
*/
class SystemFeedCylinder  : public juce::Component,
                             private juce::Timer
{
public:
    SystemFeedCylinder();
    ~SystemFeedCylinder() override;

    //--- Message API ---------------------------------------------------------
    /** Append a message that will scroll across the tape. */
    void addMessage (const juce::String& text,
                     juce::Colour color = juce::Colour (0xFF3a2e18));

    //--- juce::Component -----------------------------------------------------
    void paint     (juce::Graphics&) override;
    void resized   () override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    static juce::Colour tapeInk() noexcept;

    //--- juce::Timer ---------------------------------------------------------
    void timerCallback() override;

    //--- Paint helpers -------------------------------------------------------
    void paintTab               (juce::Graphics& g, int h) const;
    void paintMessages          (juce::Graphics& g, int tapeW, int tapeH) const;
    void paintSpecularHighlight (juce::Graphics& g, float centerX, int tapeH) const;
    void paintRimsAndFades      (juce::Graphics& g, int tapeW, int tapeH) const;

    //--- Message queue -------------------------------------------------------
    juce::Array<SystemMessage> m_messages;
    float m_scrollOffsetPx = 0.0f;
    float m_scrollSpeed    = 40.0f;   // px / sec

    //--- Feed visibility flags (saved to prefs) ------------------------------
    bool m_showMidiIn    = true;
    bool m_showMidiOut   = true;
    bool m_showSystem    = true;
    bool m_showTransport = true;
    bool m_showLink      = true;

    //--- User preferences ----------------------------------------------------
    std::unique_ptr<juce::PropertiesFile> m_props;

    //--- Cylinder constants --------------------------------------------------
    static constexpr int   kTabW    = 36;    // left label tab width
    static constexpr int   kRimW    = 5;
    static constexpr int   kFadeW   = 160;
    static constexpr int   kShadowH = 20;
    static constexpr float kMsgGap  = 60.0f;   // px between message end and next start

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SystemFeedCylinder)
};

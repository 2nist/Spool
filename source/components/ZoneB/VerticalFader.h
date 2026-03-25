#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    VerticalFader — SPOOL's primary performance control element for Zone B.

    One fader = one parameter. Laid out horizontally across a module row.

    Visual structure (top → bottom, 135px total):
      16px   value text (floats above thumb)
      105px  track (7px wide, fill below thumb)
      14px   thumb (14×14 kraft square cap)
      ----
      Right of track: rotated label (reads bottom → top)

    Status circle (7×7px) centered in thumb shows macro/MIDI/mod status.
*/
class VerticalFader : public juce::Component,
                      public juce::Timer,
                      public ThemeManager::Listener
{
public:
    //==========================================================================
    VerticalFader();
    ~VerticalFader() override;

    //==========================================================================
    // Setup

    void setParamId      (const juce::String& id)   { m_paramId = id; }
    void setDisplayName  (const juce::String& name) { m_displayName = name; repaint(); }
    void setParamColor   (juce::Colour c)            { m_paramColor = c; repaint(); }
    void setDefaultValue (float normalizedDefault)   { m_defaultValue = normalizedDefault; }
    void setBipolar      (bool bipolar)              { m_bipolar = bipolar; repaint(); }

    //==========================================================================
    // Value

    void  setValue               (float normalized, bool notify = true);
    float getValue               () const noexcept { return m_value; }
    /** Called from message thread by automation — does NOT fire onValueChanged. */
    void  setValueFromProcessor  (float normalized);

    using FormatFn = std::function<juce::String (float)>;
    void setValueFormatter (FormatFn fn) { m_formatter = std::move (fn); }

    //==========================================================================
    // Status

    enum class Status
    {
        none,
        macroMapped,
        midiLearn,
        midiAssigned,
        modulated,
        automated,
        follower,
        bypassed
    };

    void setStatus      (Status s);
    void setStatusLabel (const juce::String& detail) { m_statusDetail = detail; }

    //==========================================================================
    // Callback

    std::function<void (float)> onValueChanged;

    //==========================================================================
    // Sizing

    /** Preferred component width including rotated label column. */
    int getPreferredWidth () const;

    static constexpr int kPreferredHeight = 70;  // TRACK_H(50) + THUMB(10) + value(10)

    //==========================================================================
    // JUCE overrides

    void paint              (juce::Graphics& g)                         override;
    void resized            ()                                          override;
    void mouseDown          (const juce::MouseEvent& e)                 override;
    void mouseDrag          (const juce::MouseEvent& e)                 override;
    void mouseUp            (const juce::MouseEvent& e)                 override;
    void mouseEnter         (const juce::MouseEvent& e)                 override;
    void mouseExit          (const juce::MouseEvent& e)                 override;
    void mouseDoubleClick   (const juce::MouseEvent& e)                 override;
    void mouseWheelMove     (const juce::MouseEvent& e,
                             const juce::MouseWheelDetails& d)          override;

    // Timer — drives MIDI learn blink and double-click flash
    void timerCallback () override;

    // ThemeManager::Listener
    void themeChanged () override { repaint(); }

private:
    //==========================================================================
    // Identity

    juce::String m_paramId;
    juce::String m_displayName;
    juce::Colour m_paramColor   { 0xFF4a9eff };
    float        m_defaultValue { 0.5f };
    bool         m_bipolar      { false };
    FormatFn     m_formatter;

    //==========================================================================
    // Value

    float m_value        { 0.5f };
    float m_dragStartY   { 0.0f };
    float m_dragStartVal { 0.5f };
    bool  m_dragging     { false };
    bool  m_hovered      { false };

    //==========================================================================
    // Status

    Status       m_status      { Status::none };
    juce::String m_statusDetail;

    // MIDI learn blink — asymmetric: 500ms on / 100ms off at 10 Hz
    bool m_blinkOn      { true };
    int  m_blinkCounter { 0 };   // 0-4 = ON phase (5 ticks), 5 = OFF phase (1 tick)

    // Double-click flash
    float m_flashAlpha { 1.0f }; // >1.0 = brightened; decays to 1.0 in timerCallback

    //==========================================================================
    // Inline value editor (shown on "Enter value..." from context menu)

    std::unique_ptr<juce::TextEditor> m_textEditor;

    //==========================================================================
    // Layout constants

    static constexpr int TRACK_W    = 6;
    static constexpr int TRACK_H    = 50;
    static constexpr int THUMB_SIZE = 10;
    static constexpr int CIRCLE_SIZE = 5;
    static constexpr int LABEL_GAP  = 4;
    static constexpr int VALUE_AREA_H = 10;

    //==========================================================================
    // Paint helpers

    void paintTrack  (juce::Graphics& g);
    void paintFill   (juce::Graphics& g);
    void paintLine   (juce::Graphics& g);
    void paintThumb  (juce::Graphics& g);
    void paintCircle (juce::Graphics& g);
    void paintValue  (juce::Graphics& g);
    void paintLabel  (juce::Graphics& g);

    //==========================================================================
    // Coordinate helpers

    juce::Rectangle<int> trackBounds  () const noexcept;
    juce::Rectangle<int> thumbBounds  () const noexcept;
    juce::Rectangle<int> circleBounds () const noexcept;

    /** norm=0 → bottom of track, norm=1 → top. Returns Y of thumb centre. */
    int valueToY (float norm) const noexcept
    {
        const auto tb = trackBounds();
        return tb.getBottom() - static_cast<int> (norm * static_cast<float> (tb.getHeight()));
    }

    float yToValue (int y) const noexcept
    {
        const auto tb = trackBounds();
        return juce::jlimit (0.0f, 1.0f,
            1.0f - static_cast<float> (y - tb.getY()) / static_cast<float> (tb.getHeight()));
    }

    //==========================================================================
    // Helpers

    void showContextMenu ();
    void showValueEditor ();
    void notifyValueChanged ();
    juce::String formatValue (float norm) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VerticalFader)
};

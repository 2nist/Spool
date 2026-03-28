#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    SongHeaderStrip — 28px fixed-height bar below the menu bar.

    Left to right:
      Song title (double-click to edit) | KEY | BPM (drag) | SIG | LEN
    Right:
      Save indicator dot + "UNSAVED"/"SAVED" | project filename
*/
class SongHeaderStrip : public juce::Component
{
public:
    SongHeaderStrip();
    ~SongHeaderStrip() override = default;

    //--- Accessors -----------------------------------------------------------
    float        getBpm()   const noexcept { return m_bpm; }
    juce::String getTitle() const noexcept { return m_titleText; }

    void setBpm (float bpm);
    void setCompactMode (bool shouldCompact);
    void setTitle (const juce::String& title);

    /** Called by PluginEditor when transport position updates. */
    void setPosition (float beatPosition, float bpm, float loopLengthBars);

    //--- Callbacks -----------------------------------------------------------
    std::function<void(float)> onBpmChanged;
    std::function<void(const juce::String&)> onTitleChanged;

    //--- juce::Component -----------------------------------------------------
    void paint            (juce::Graphics&) override;
    void resized          () override;
    void mouseDown        (const juce::MouseEvent&) override;
    void mouseDrag        (const juce::MouseEvent&) override;
    void mouseUp          (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void paintField   (juce::Graphics& g,
                       const juce::Rectangle<float>& r,
                       const juce::String& labelText,
                       const juce::String& valueText,
                       juce::Colour valueColour,
                       const juce::Font& valueFont) const;
    void paintDivider (juce::Graphics& g, float x) const;
    void titleEditingComplete();

    //--- State ---------------------------------------------------------------
    juce::String m_titleText   { "Untitled" };
    float        m_bpm         = 120.0f;
    bool         m_unsaved     = true;
    juce::String m_lengthStr   { "0:00" };
    bool         m_compactMode = false;

    //--- BPM drag ------------------------------------------------------------
    bool  m_draggingBpm  = false;
    float m_dragStartY   = 0.0f;
    float m_dragStartBpm = 120.0f;

    //--- Layout rects (computed in resized) ----------------------------------
    juce::Rectangle<float> m_titleRect;
    juce::Rectangle<float> m_keyRect;
    juce::Rectangle<float> m_bpmRect;
    juce::Rectangle<float> m_sigRect;
    juce::Rectangle<float> m_lenRect;

    //--- Inline title editor -------------------------------------------------
    juce::TextEditor m_titleEditor;
    bool             m_editingTitle = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongHeaderStrip)
};

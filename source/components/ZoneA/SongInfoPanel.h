#pragma once

#include "../../Theme.h"
#include "../../structure/StructureState.h"
#include "ZoneAControlStyle.h"
#include "ZoneAStyle.h"

//==============================================================================
/**
    SongInfoPanel — song metadata editor.

    Content:
      Title (editable inline text field)
      Key selector (steppers: C, C#, D ... B)
      Scale selector (MAJOR / MINOR / DORIAN etc.)
      BPM drag control (syncs to song header + Zone D)
      Time signature (4/4, 3/4, etc.)
      Composition length (in bars)
      Notes text area (freeform)
*/
class SongInfoPanel : public juce::Component
{
public:
    SongInfoPanel();
    ~SongInfoPanel() override = default;

    /** Sync BPM from external source (e.g. song header). */
    void setBpm (float bpm);

    /** Sync title from external source. */
    void setTitle (const juce::String& title);
    void setKey (const juce::String& key);
    void setScale (const juce::String& scale);
    void setStructureSummary (const StructureState& structure);

    // Callbacks
    std::function<void(const juce::String& title)> onTitleChanged;
    std::function<void(float bpm)>                  onBpmChanged;
    std::function<void(const juce::String& key)>    onKeyChanged;
    std::function<void(const juce::String& scale)>  onScaleChanged;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    static const juce::StringArray kKeys;
    static const juce::StringArray kScales;

    juce::TextEditor  m_titleEditor;
    juce::Label       m_titleLabel;
    juce::Label       m_defaultsLabel;
    juce::Label       m_notesLabel;
    juce::ComboBox    m_keyCombo;
    juce::ComboBox    m_scaleCombo;
    juce::Label       m_bpmLabel;
    juce::Label       m_timeSigLabel;
    juce::TextEditor  m_notesEditor;
    juce::Label       m_summaryTitle;
    juce::Label       m_summaryTempo;
    juce::Label       m_summaryKey;
    juce::Label       m_summaryMode;
    juce::Label       m_summaryBars;
    juce::Label       m_summaryDuration;
    juce::Label       m_summarySections;
    juce::Label       m_summaryBlocks;

    float m_bpm = 120.0f;

    // BPM drag
    bool  m_draggingBpm  = false;
    float m_dragStartY   = 0.0f;
    float m_dragStartBpm = 120.0f;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;

    juce::Rectangle<int> bpmRect() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SongInfoPanel)
};

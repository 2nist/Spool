#pragma once

#include "SlotPattern.h"

//==============================================================================
/**
    SequencerHeader — 16px bar above the step grid.

    Left:   6px colour dot + focused slot label ("KICK · DRUMS")
    Centre: pattern selector "◀ PAT 1 ▶"
    Right:  COPY | PASTE | CLR buttons
*/
class SequencerHeader : public juce::Component
{
public:
    SequencerHeader();
    ~SequencerHeader() override = default;

    /** Update the focus label.  Pass empty strings to clear. */
    void setFocusedSlot (const juce::String& slotName,
                         const juce::String& groupName,
                         juce::Colour        groupColor);
    void clearFocus();

    /** Point to the active slot pattern.  Null = no active slot. */
    void setPattern (SlotPattern* pattern);

    /** Paste-ready clipboard entry (owned by ZoneBComponent). */
    void setClipboard (const SlotPattern* cb, bool hasData);
    void setStructureContext (const juce::String& sectionName,
                              const juce::String& positionLabel,
                              const juce::String& currentChord,
                              const juce::String& nextChord,
                              const juce::String& transitionIntent,
                              bool followingStructure,
                              bool locallyOverriding);

    std::function<void()>    onCopy;
    std::function<void()>    onPaste;
    std::function<void()>    onClear;
    std::function<void(int)> onPatternChanged;

    static constexpr int kHeight = 28;

    void paint    (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::String m_slotName;
    juce::String m_groupName;
    juce::Colour m_groupColor { 0x00000000 };
    bool         m_hasFocus   { false };
    juce::String m_structureSection;
    juce::String m_structurePosition;
    juce::String m_currentChord;
    juce::String m_nextChord;
    juce::String m_transitionIntent;
    bool         m_followingStructure { false };
    bool         m_locallyOverriding { false };

    SlotPattern* m_pattern    { nullptr };
    bool         m_hasClipboard { false };

    juce::Rectangle<int> dotRect()     const noexcept;
    juce::Rectangle<int> labelRect()   const noexcept;
    juce::Rectangle<int> patPrevRect() const noexcept;
    juce::Rectangle<int> patLabelRect()const noexcept;
    juce::Rectangle<int> patNextRect() const noexcept;
    juce::Rectangle<int> copyRect()    const noexcept;
    juce::Rectangle<int> pasteRect()   const noexcept;
    juce::Rectangle<int> clearRect()   const noexcept;

    static constexpr int kPad    = 6;
    static constexpr int kBtnW   = 24;
    static constexpr int kPatW   = 48;   // width of full PAT selector group
    static constexpr int kArrowW = 10;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerHeader)
};

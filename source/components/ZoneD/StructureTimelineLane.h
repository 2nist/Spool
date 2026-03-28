#pragma once

#include "../../Theme.h"
#include "../../structure/StructureEngine.h"
#include "../../state/LyricsState.h"
#include "../../state/AutomationState.h"

class StructureTimelineLane : public juce::Component
{
public:
    StructureTimelineLane() = default;

    void setStructure (const StructureState* state, const StructureEngine* engine);
    void setAuthoredTimelineData (const LyricsState* lyricsState, const AutomationState* automationState);
    void setCurrentBeat (double beat);

    void paint (juce::Graphics& g) override;

private:
    const StructureState* m_state { nullptr };
    const StructureEngine* m_engine { nullptr };
    const LyricsState* m_lyricsState { nullptr };
    const AutomationState* m_automationState { nullptr };
    double m_currentBeat { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StructureTimelineLane)
};

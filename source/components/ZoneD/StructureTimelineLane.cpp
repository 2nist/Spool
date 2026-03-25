#include "StructureTimelineLane.h"
#include "ClipData.h"

namespace
{
int computeTotalBars (const StructureState& state)
{
    int maxBars = juce::jmax (0, state.totalBars);
    for (const auto& section : state.sections)
        maxBars = juce::jmax (maxBars, section.startBar + section.lengthBars);
    return juce::jmax (1, maxBars);
}
} // namespace

void StructureTimelineLane::setStructure (const StructureState* state, const StructureEngine* engine)
{
    m_state = state;
    m_engine = engine;
    repaint();
}

void StructureTimelineLane::setCurrentBeat (double beat)
{
    m_currentBeat = beat;
    repaint();
}

void StructureTimelineLane::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colour::surface2.darker (0.12f));

    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.7f));
    g.drawRect (getLocalBounds());

    const bool structureFollowEnabled = (m_state != nullptr && ! m_state->sections.empty());
    const auto statusText = structureFollowEnabled ? "Structure Follow: ON" : "Legacy Fallback";
    const auto statusColour = structureFollowEnabled ? Theme::Colour::accent : Theme::Colour::inkGhost;

    auto statusRect = getLocalBounds().removeFromTop (14).reduced (6, 0);
    g.setColour (statusColour);
    g.setFont (Theme::Font::micro());
    g.drawText (statusText, statusRect, juce::Justification::centredRight, false);

    if (m_state == nullptr || m_state->sections.empty())
    {
        g.setColour (Theme::Colour::inkGhost);
        g.setFont (Theme::Font::micro());
        g.drawText ("Structure lane: apply section intent in Zone A", getLocalBounds().reduced (6), juce::Justification::centredLeft, false);
        return;
    }

    const auto totalBars = computeTotalBars (*m_state);
    const auto activeSection = (m_engine != nullptr) ? m_engine->getSectionAtBeat (m_currentBeat) : nullptr;
    const auto activeChord = (m_engine != nullptr) ? m_engine->getChordAtBeat (m_currentBeat) : Chord { "C", "maj" };

    auto content = getLocalBounds().reduced (4, 3);
    auto sectionBand = content.removeFromTop (juce::jmax (10, content.getHeight() / 2));
    auto chordBand = content;

    for (size_t s = 0; s < m_state->sections.size(); ++s)
    {
        const auto& section = m_state->sections[s];
        const auto sectionTint = sectionColourForIndex (static_cast<int> (s));
        const float x0 = sectionBand.getX() + sectionBand.getWidth() * (static_cast<float> (section.startBar) / static_cast<float> (totalBars));
        const float x1 = sectionBand.getX() + sectionBand.getWidth() * (static_cast<float> (section.startBar + section.lengthBars) / static_cast<float> (totalBars));
        const juce::Rectangle<float> secRect (x0, static_cast<float> (sectionBand.getY()),
                                              juce::jmax (2.0f, x1 - x0), static_cast<float> (sectionBand.getHeight() - 1));

        const bool sectionIsActive = (activeSection != nullptr && activeSection->name == section.name && activeSection->startBar == section.startBar);
        const auto secFill = sectionIsActive ? sectionTint : sectionTint.withAlpha (0.45f);
        g.setColour (secFill);
        g.fillRoundedRectangle (secRect, 2.0f);

        g.setColour (Theme::Colour::inkDark);
        g.setFont (Theme::Font::micro());
        g.drawText (section.name, secRect.toNearestInt().reduced (4, 0), juce::Justification::centredLeft, true);

        const int chordCount = juce::jmax (1, static_cast<int> (section.progression.size()));
        const float barsPerChord = static_cast<float> (section.lengthBars) / static_cast<float> (chordCount);

        // Even distribution across section bars. For non-even divisions, the final segment absorbs rounding.
        for (int c = 0; c < chordCount; ++c)
        {
            const float chordStartBar = static_cast<float> (section.startBar) + barsPerChord * static_cast<float> (c);
            const float chordEndBar = (c == chordCount - 1)
                                        ? static_cast<float> (section.startBar + section.lengthBars)
                                        : static_cast<float> (section.startBar) + barsPerChord * static_cast<float> (c + 1);

            const float cx0 = chordBand.getX() + chordBand.getWidth() * (chordStartBar / static_cast<float> (totalBars));
            const float cx1 = chordBand.getX() + chordBand.getWidth() * (chordEndBar / static_cast<float> (totalBars));
            const juce::Rectangle<float> chordRect (cx0, static_cast<float> (chordBand.getY()),
                                                    juce::jmax (1.0f, cx1 - cx0), static_cast<float> (chordBand.getHeight() - 1));

            Chord chord = { "C", "maj" };
            if (c < static_cast<int> (section.progression.size()))
                chord = section.progression[(size_t) c];

            const bool chordIsActive = (chord.root == activeChord.root && chord.type == activeChord.type) && sectionIsActive;
            const auto chordTint = sectionTint.interpolatedWith (Theme::Colour::accentWarm, 0.35f);
            g.setColour (chordIsActive ? chordTint : chordTint.withAlpha (0.40f));
            g.fillRoundedRectangle (chordRect.reduced (0.5f, 0.5f), 2.0f);

            g.setColour (Theme::Colour::inkLight);
            g.setFont (Theme::Font::micro());
            g.drawText (chord.root + chord.type, chordRect.toNearestInt().reduced (3, 0), juce::Justification::centredLeft, true);
        }
    }

    if (totalBars > 0)
    {
        const float beatPerBar = 4.0f;
        const float barPos = static_cast<float> (m_currentBeat / beatPerBar);
        const float x = sectionBand.getX() + sectionBand.getWidth() * (barPos / static_cast<float> (totalBars));
        g.setColour (Theme::Colour::inkLight.withAlpha (0.65f));
        g.drawVerticalLine (juce::roundToInt (x), static_cast<float> (getLocalBounds().getY() + 1), static_cast<float> (getLocalBounds().getBottom() - 1));
    }
}

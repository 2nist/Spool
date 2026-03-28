#include "StructureTimelineLane.h"
#include "ClipData.h"

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

    const bool structureReady = (m_state != nullptr && structureHasScaffold (*m_state));
    const auto statusText = structureReady ? "Structure Follow: ON" : "Structure Scaffold: EMPTY";
    const auto statusColour = structureReady ? Theme::Colour::accent : Theme::Colour::inkGhost;

    auto statusRect = getLocalBounds().removeFromTop (14).reduced (6, 0);
    g.setColour (statusColour);
    g.setFont (Theme::Font::micro());
    g.drawText (statusText, statusRect, juce::Justification::centredRight, false);

    if (m_state == nullptr || ! structureReady)
    {
        g.setColour (Theme::Colour::inkGhost);
        g.setFont (Theme::Font::micro());
        g.drawText ("Structure lane: add sections and arrangement blocks in Zone A",
                    getLocalBounds().reduced (6),
                    juce::Justification::centredLeft,
                    false);
        return;
    }

    const auto resolved = buildResolvedStructure (*m_state);
    const auto totalBeats = juce::jmax (1, m_state->totalBeats);
    const auto wrappedBeat = std::fmod (juce::jmax (0.0, m_currentBeat), static_cast<double> (totalBeats));
    const auto activeSection = (m_engine != nullptr) ? m_engine->getSectionAtBeat (wrappedBeat) : std::nullopt;
    const auto activeChord = (m_engine != nullptr) ? m_engine->getChordAtBeat (wrappedBeat) : Chord { "C", "maj" };
    const auto totalBars = juce::jmax (1, m_state->totalBars);

    auto content = getLocalBounds().reduced (4, 3);
    auto sectionBand = content.removeFromTop (juce::jmax (10, content.getHeight() / 2));
    auto chordBand = content;

    for (size_t s = 0; s < resolved.size(); ++s)
    {
        const auto& instance = resolved[s];
        if (instance.section == nullptr)
            continue;

        const auto sectionTint = sectionColourForIndex (static_cast<int> (s));
        const float x0 = sectionBand.getX() + sectionBand.getWidth() * (static_cast<float> (instance.startBar) / static_cast<float> (totalBars));
        const float x1 = sectionBand.getX() + sectionBand.getWidth() * (static_cast<float> (instance.startBar + instance.totalBars) / static_cast<float> (totalBars));
        const juce::Rectangle<float> secRect (x0,
                                              static_cast<float> (sectionBand.getY()),
                                              juce::jmax (2.0f, x1 - x0),
                                              static_cast<float> (sectionBand.getHeight() - 1));

        const bool sectionIsActive = activeSection.has_value()
                                     && activeSection->block != nullptr
                                     && instance.block != nullptr
                                     && activeSection->block->id == instance.block->id;
        const auto secFill = sectionIsActive ? sectionTint : sectionTint.withAlpha (0.45f);
        g.setColour (secFill);
        g.fillRoundedRectangle (secRect, 2.0f);

        g.setColour (Theme::Colour::inkDark);
        g.setFont (Theme::Font::micro());
        g.drawText (instance.section->name, secRect.toNearestInt().reduced (4, 0), juce::Justification::centredLeft, true);

        const int chordCount = juce::jmax (1, static_cast<int> (instance.section->progression.size()));
        const float barsPerChord = static_cast<float> (instance.barsPerRepeat) / static_cast<float> (chordCount);

        for (int repeatIndex = 0; repeatIndex < instance.repeats; ++repeatIndex)
        {
            const float repeatBarOffset = static_cast<float> (instance.startBar + repeatIndex * instance.barsPerRepeat);

            for (int c = 0; c < chordCount; ++c)
            {
                const float chordStartBar = repeatBarOffset + barsPerChord * static_cast<float> (c);
                const float chordEndBar = (c == chordCount - 1)
                                            ? repeatBarOffset + static_cast<float> (instance.barsPerRepeat)
                                            : repeatBarOffset + barsPerChord * static_cast<float> (c + 1);

                const float cx0 = chordBand.getX() + chordBand.getWidth() * (chordStartBar / static_cast<float> (totalBars));
                const float cx1 = chordBand.getX() + chordBand.getWidth() * (chordEndBar / static_cast<float> (totalBars));
                const juce::Rectangle<float> chordRect (cx0,
                                                        static_cast<float> (chordBand.getY()),
                                                        juce::jmax (1.0f, cx1 - cx0),
                                                        static_cast<float> (chordBand.getHeight() - 1));

                Chord chord = { "C", "maj" };
                if (c < static_cast<int> (instance.section->progression.size()))
                    chord = instance.section->progression[(size_t) c];

                const bool chordIsActive = sectionIsActive
                                           && chord.root == activeChord.root
                                           && chord.type == activeChord.type;
                const auto chordTint = sectionTint.interpolatedWith (Theme::Colour::accentWarm, 0.35f);
                g.setColour (chordIsActive ? chordTint : chordTint.withAlpha (0.40f));
                g.fillRoundedRectangle (chordRect.reduced (0.5f, 0.5f), 2.0f);

                g.setColour (Theme::Colour::inkLight);
                g.setFont (Theme::Font::micro());
                g.drawText (chord.root + chord.type,
                            chordRect.toNearestInt().reduced (3, 0),
                            juce::Justification::centredLeft,
                            true);
            }
        }
    }

    if (totalBars > 0)
    {
        const float barPos = static_cast<float> (m_currentBeat / 4.0);
        const float x = sectionBand.getX() + sectionBand.getWidth() * (barPos / static_cast<float> (totalBars));
        g.setColour (Theme::Colour::inkLight.withAlpha (0.65f));
        g.drawVerticalLine (juce::roundToInt (x),
                            static_cast<float> (getLocalBounds().getY() + 1),
                            static_cast<float> (getLocalBounds().getBottom() - 1));
    }
}

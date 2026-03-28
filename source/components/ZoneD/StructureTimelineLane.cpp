#include "StructureTimelineLane.h"
#include "ClipData.h"
#include <cmath>

void StructureTimelineLane::setStructure (const StructureState* state, const StructureEngine* engine)
{
    m_state = state;
    m_engine = engine;
    repaint();
}

void StructureTimelineLane::setAuthoredTimelineData (const LyricsState* lyricsState, const AutomationState* automationState)
{
    m_lyricsState = lyricsState;
    m_automationState = automationState;
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
    const auto activeSection = (m_engine != nullptr) ? m_engine->getSectionAtBeat (m_currentBeat) : std::nullopt;
    const auto activeChord = (m_engine != nullptr) ? m_engine->getChordAtBeat (m_currentBeat) : Chord { "C", "maj" };

    auto content = getLocalBounds().reduced (4, 3);
    auto sectionBand = content.removeFromTop (12);
    auto chordBand = content.removeFromTop (10);
    auto authoredBand = content;
    const float sectionBandStart = static_cast<float> (sectionBand.getX());
    const float sectionBandWidth = static_cast<float> (juce::jmax (1, sectionBand.getWidth()));
    const float chordBandStart = static_cast<float> (chordBand.getX());
    const float chordBandWidth = static_cast<float> (juce::jmax (1, chordBand.getWidth()));
    const double loopBeats = static_cast<double> (totalBeats);
    const double normalizedCurrentBeat = wrappedBeat;
    auto timelineXForBeat = [normalizedCurrentBeat, loopBeats] (float startX, float width, double beat)
    {
        return startX + width * 0.5f + static_cast<float> (((beat - normalizedCurrentBeat) / loopBeats) * static_cast<double> (width));
    };
    auto drawWrappedBeatRange = [&] (float bandStart,
                                     float bandWidth,
                                     int bandY,
                                     int bandHeight,
                                     double startBeat,
                                     double endBeat,
                                     const std::function<void (juce::Rectangle<float>)>& paintSegment)
    {
        const float bandEnd = bandStart + bandWidth;
        for (int wrap = -1; wrap <= 1; ++wrap)
        {
            const double offset = static_cast<double> (wrap) * loopBeats;
            const float x0 = timelineXForBeat (bandStart, bandWidth, startBeat + offset);
            const float x1 = timelineXForBeat (bandStart, bandWidth, endBeat + offset);
            const float left = juce::jmax (bandStart, juce::jmin (x0, x1));
            const float right = juce::jmin (bandEnd, juce::jmax (x0, x1));
            if (right <= left)
                continue;

            paintSegment (juce::Rectangle<float> (left,
                                                  static_cast<float> (bandY),
                                                  juce::jmax (1.0f, right - left),
                                                  static_cast<float> (juce::jmax (1, bandHeight - 1))));
        }
    };

    for (size_t s = 0; s < resolved.size(); ++s)
    {
        const auto& instance = resolved[s];
        if (instance.section == nullptr)
            continue;

        const auto sectionTint = sectionColourForIndex (static_cast<int> (s));
        const bool sectionIsActive = activeSection.has_value()
                                     && activeSection->block != nullptr
                                     && instance.block != nullptr
                                     && activeSection->block->id == instance.block->id;
        const auto secFill = sectionIsActive ? sectionTint : sectionTint.withAlpha (0.45f);
        drawWrappedBeatRange (sectionBandStart,
                              sectionBandWidth,
                              sectionBand.getY(),
                              sectionBand.getHeight(),
                              static_cast<double> (instance.startBeat),
                              static_cast<double> (instance.startBeat + instance.totalBeats),
                              [&] (juce::Rectangle<float> secRect)
                              {
                                  g.setColour (secFill);
                                  g.fillRoundedRectangle (secRect, 2.0f);

                                  if (secRect.getWidth() >= 22.0f)
                                  {
                                      g.setColour (Theme::Colour::inkDark);
                                      g.setFont (Theme::Font::micro());
                                      g.drawText (instance.section->name,
                                                  secRect.toNearestInt().reduced (4, 0),
                                                  juce::Justification::centredLeft,
                                                  true);
                                  }
                              });

        const auto repeatBeats = juce::jmax (1, instance.barsPerRepeat * instance.beatsPerBar);
        const bool hasProgression = ! instance.section->progression.empty();
        const auto fallbackChord = Chord { "C", "maj" };

        for (int repeatIndex = 0; repeatIndex < instance.repeats; ++repeatIndex)
        {
            const int repeatStartBeat = instance.startBeat + repeatIndex * repeatBeats;
            int beatCursor = 0;
            int sequenceIndex = 0;

            while (beatCursor < repeatBeats && sequenceIndex < juce::jmax (1, repeatBeats * 2))
            {
                const Chord chord = hasProgression
                                      ? instance.section->progression[(size_t) (sequenceIndex % static_cast<int> (instance.section->progression.size()))]
                                      : fallbackChord;
                const int chordDuration = hasProgression ? juce::jmax (1, chord.durationBeats)
                                                         : repeatBeats;
                const int chordStartBeat = repeatStartBeat + beatCursor;
                const int chordEndBeat = juce::jmin (repeatStartBeat + repeatBeats,
                                                     chordStartBeat + chordDuration);
                if (chordEndBeat <= chordStartBeat)
                    break;

                const bool chordIsActive = sectionIsActive
                                           && wrappedBeat >= static_cast<double> (chordStartBeat)
                                           && wrappedBeat < static_cast<double> (chordEndBeat)
                                           && chord.root == activeChord.root
                                           && chord.type == activeChord.type;
                const auto chordTint = sectionTint.interpolatedWith (Theme::Colour::accentWarm, 0.35f);
                drawWrappedBeatRange (chordBandStart,
                                      chordBandWidth,
                                      chordBand.getY(),
                                      chordBand.getHeight(),
                                      static_cast<double> (chordStartBeat),
                                      static_cast<double> (chordEndBeat),
                                      [&] (juce::Rectangle<float> chordRect)
                                      {
                                          g.setColour (chordIsActive ? chordTint : chordTint.withAlpha (0.40f));
                                          g.fillRoundedRectangle (chordRect.reduced (0.5f, 0.5f), 2.0f);
                                          if (chordRect.getWidth() >= 12.0f)
                                          {
                                              g.setColour (Theme::Colour::inkLight);
                                              g.setFont (Theme::Font::micro());
                                              g.drawText (abbreviatedChordLabel (chord),
                                                          chordRect.toNearestInt().reduced (3, 0),
                                                          juce::Justification::centredLeft,
                                                          true);
                                          }
                                      });

                beatCursor = chordEndBeat - repeatStartBeat;
                ++sequenceIndex;
            }
        }
    }

    if (totalBeats > 0)
    {
        const float x = sectionBandStart + sectionBandWidth * 0.5f;
        g.setColour (Theme::Colour::inkLight.withAlpha (0.65f));
        g.drawVerticalLine (juce::roundToInt (x),
                            static_cast<float> (getLocalBounds().getY() + 1),
                            static_cast<float> (getLocalBounds().getBottom() - 1));
    }

    if (authoredBand.getHeight() <= 0)
        return;

    const int lyricsRowH = juce::jmax (4, authoredBand.getHeight() / 2);
    auto autoBand = authoredBand.removeFromTop (lyricsRowH);
    auto lyricBand = authoredBand;

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost.withAlpha (0.75f));
    g.drawText ("AUTO", autoBand.removeFromLeft (28), juce::Justification::centredLeft, false);
    g.drawText ("LYR", lyricBand.removeFromLeft (28), juce::Justification::centredLeft, false);

    if (m_automationState != nullptr && ! m_automationState->lanes.empty())
    {
        const int visibleLanes = juce::jmin (2, static_cast<int> (m_automationState->lanes.size()));
        for (int i = 0; i < visibleLanes; ++i)
        {
            const auto& lane = m_automationState->lanes[(size_t) i];
            g.setColour (lane.enabled ? Theme::Zone::b.withAlpha (0.85f) : Theme::Colour::inkGhost.withAlpha (0.45f));
            juce::Path path;
            for (int wrap = -1; wrap <= 1; ++wrap)
            {
                bool started = false;
                for (const auto& point : lane.points)
                {
                    const float x = timelineXForBeat (static_cast<float> (autoBand.getX()),
                                                      static_cast<float> (juce::jmax (1, autoBand.getWidth())),
                                                      point.beat + static_cast<double> (wrap * totalBeats));
                    const float y = autoBand.getBottom() - 1.0f - point.value * (float) juce::jmax (1, autoBand.getHeight() - 2);
                    if (! started)
                    {
                        path.startNewSubPath (x, y);
                        started = true;
                    }
                    else
                    {
                        path.lineTo (x, y);
                    }
                }
            }
            g.strokePath (path, juce::PathStrokeType (1.0f));
            g.setColour (Theme::Colour::inkGhost.withAlpha (0.8f));
            g.drawText (lane.displayName, autoBand.withTrimmedLeft (4 + i * 56).withWidth (54), juce::Justification::centredLeft, true);
        }
        if ((int) m_automationState->lanes.size() > visibleLanes)
        {
            g.setColour (Theme::Colour::inkGhost);
            g.drawText ("+" + juce::String ((int) m_automationState->lanes.size() - visibleLanes),
                        autoBand.withTrimmedLeft (autoBand.getWidth() - 18),
                        juce::Justification::centredRight,
                        false);
        }
    }

    if (m_lyricsState != nullptr)
    {
        for (const auto& lyric : m_lyricsState->items)
        {
            if (lyric.beatPosition < 0.0)
                continue;

            for (int wrap = -1; wrap <= 1; ++wrap)
            {
                const float x = timelineXForBeat (static_cast<float> (lyricBand.getX()),
                                                  static_cast<float> (juce::jmax (1, lyricBand.getWidth())),
                                                  lyric.beatPosition + static_cast<double> (wrap * totalBeats));
                if (x < static_cast<float> (lyricBand.getX() - 22) || x > static_cast<float> (lyricBand.getRight() + 2))
                    continue;
                const auto marker = juce::Rectangle<float> (x, (float) lyricBand.getY() + 1.0f, 20.0f, (float) juce::jmax (4, lyricBand.getHeight() - 2));
                g.setColour (juce::Colour (lyric.style.colourArgb).withAlpha (0.9f));
                g.fillRoundedRectangle (marker, 2.0f);
                g.setColour (Theme::Colour::inkDark);
                g.setFont (Theme::Font::micro());
                g.drawText (lyric.text.upToFirstOccurrenceOf (" ", false, false),
                            marker.toNearestInt().reduced (2, 0),
                            juce::Justification::centredLeft,
                            true);
            }
        }
    }
}

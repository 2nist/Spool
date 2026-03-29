#include "ZoneDComponent.h"
#include <cmath>

//==============================================================================
ZoneDComponent::ZoneDComponent()
{
    addAndMakeVisible (m_structureLane);
    addAndMakeVisible (m_cylinderBand);
    addAndMakeVisible (m_zoomStrip);
    addAndMakeVisible (m_transportStrip);

    // Wire ZoomStrip → CylinderBand zoom
    m_zoomStrip.onZoomChanged = [this] (float zoom)
    {
        m_cylinderBand.setPxPerBeat (zoom);
    };

    // Wire ZoomStrip → lane height scale
    m_zoomStrip.onLaneHeightChanged = [this] (float scale)
    {
        setLaneHeightScale (scale);
    };

    // Wire ZoomStrip → height mode changes
    m_zoomStrip.onHeightModeChanged = [this] (ZoomStrip::HeightMode mode)
    {
        switch (mode)
        {
            case ZoomStrip::HeightMode::min:    m_cylinderBandH = 60;  break;
            case ZoomStrip::HeightMode::norm:   m_cylinderBandH = 132; break;
            case ZoomStrip::HeightMode::max:    m_cylinderBandH = 200; break;
            case ZoomStrip::HeightMode::expand: m_cylinderBandH = 300; break;
        }
        resized();
        if (onHeightChanged)
            onHeightChanged (currentHeight());
    };

    // Wire TransportStrip → loop length
    m_transportStrip.onLoopLengthChanged = [this] (float bars)
    {
        m_loopLengthBars = bars;
        m_cylinderBand.setLoopLengthBars (bars);
        if (onLoopLengthChanged)
            onLoopLengthChanged (bars);
    };

    // Wire TransportStrip → mixer gutter toggle
    m_transportStrip.onMixToggled = [this]
    {
        m_cylinderBand.setMixerGutterOpen (!m_cylinderBand.isMixerGutterOpen());
    };

    // Beat sync is driven from PluginEditor::positionChanged using processor beat,
    // keeping the structure lane and cylinder lane in lockstep.
    m_transportStrip.onBeatAdvanced = nullptr;

    // Wire gear button → proxy to PluginEditor
    m_transportStrip.onSettingsClicked = [this]
    {
        if (onSettingsClicked) onSettingsClicked();
    };

    m_cylinderBand.setLoopLengthBars (m_loopLengthBars);

    m_cylinderBand.onClipDropped = [this] (int laneIndex)
    {
        if (onClipDropped)
            onClipDropped (laneIndex);
    };

    m_cylinderBand.onClipEditCommitted = [this] (int laneIndex,
                                                 const juce::String& clipId,
                                                 float startBeat,
                                                 float lengthBeats)
    {
        if (laneIndex >= 0 && laneIndex < static_cast<int> (m_lanes.size()))
        {
            auto& lane = m_lanes[static_cast<size_t> (laneIndex)];
            int fallbackIndex = -1;
            for (int i = 0; i < lane.clips.size(); ++i)
            {
                auto& clip = lane.clips.getReference (i);
                if (clip.type == ClipType::scaffold)
                    continue;

                if (fallbackIndex < 0)
                    fallbackIndex = i;

                if (clipId.isNotEmpty() && clip.clipId == clipId)
                {
                    clip.startBeat = startBeat;
                    clip.lengthBeats = lengthBeats;
                    fallbackIndex = -1;
                    break;
                }
            }

            if (fallbackIndex >= 0)
            {
                auto& clip = lane.clips.getReference (fallbackIndex);
                clip.startBeat = startBeat;
                clip.lengthBeats = lengthBeats;
            }
        }

        if (onTimelineClipEdited)
            onTimelineClipEdited (laneIndex, clipId, startBeat, lengthBeats);
    };

    m_cylinderBand.onScrubBeatChanged = [this] (float beat)
    {
        if (onScrubBeatChanged)
            onScrubBeatChanged (beat);
    };

    m_cylinderBand.onClipSplitRequested = [this] (int laneIndex,
                                                  const juce::String& clipId,
                                                  float splitBeat)
    {
        if (onTimelineClipSplitRequested)
            onTimelineClipSplitRequested (laneIndex, clipId, splitBeat);
    };
}

//==============================================================================
void ZoneDComponent::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgD);
    Theme::Helper::drawZoneStripe (g, getLocalBounds(), Theme::Zone::d);
}

void ZoneDComponent::resized()
{
    const int w           = getWidth();
    const int stripeH     = static_cast<int> (Theme::Space::zoneStripeHeight);
    const int structureH  = kStructureLaneH;
    const int zoomH       = 20;
    const int transportH  = 28;
    int       y           = stripeH;

    m_structureLane .setBounds (0, y, w, structureH);         y += structureH;
    m_cylinderBand  .setBounds (0, y, w, m_cylinderBandH);    y += m_cylinderBandH;
    m_zoomStrip     .setBounds (0, y, w, zoomH);               y += zoomH;
    m_transportStrip.setBounds (0, y, w, transportH);
}

//==============================================================================
void ZoneDComponent::setLaneInfo (int slotIndex, bool active, const juce::String& moduleType)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int> (m_lanes.size()))
        return;

    auto& lane = m_lanes[static_cast<size_t> (slotIndex)];
    lane.slotIndex  = slotIndex;
    lane.active     = active;
    lane.moduleType = moduleType;
    lane.dotColor   = dotColorForModule (moduleType);

    // Add a default empty clip spanning the loop if no clips present
    if (lane.clips.isEmpty())
    {
        Clip c;
        c.startBeat   = 0.0f;
        c.lengthBeats = m_loopLengthBars * 4.0f;
        c.name        = moduleType.isEmpty() ? "—" : moduleType;
        c.type        = clipTypeForModule (moduleType);
        lane.clips.add (c);
    }

    // Propagate to cylinder band
    {
        juce::Array<LaneData> lanes;
        lanes.addArray (m_lanes.data(), static_cast<int> (m_lanes.size()));
        m_cylinderBand.setLanes (lanes);
    }
}

void ZoneDComponent::clearLaneInfo (int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int> (m_lanes.size()))
        return;

    m_lanes[static_cast<size_t> (slotIndex)] = LaneData{};
    {
        juce::Array<LaneData> lanes;
        lanes.addArray (m_lanes.data(), static_cast<int> (m_lanes.size()));
        m_cylinderBand.setLanes (lanes);
    }
}

void ZoneDComponent::armLane (int slotIndex, bool armed)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int> (m_lanes.size()))
        return;

    m_lanes[static_cast<size_t> (slotIndex)].armed = armed;
    m_cylinderBand.setLaneArmed (slotIndex, armed);
}

//==============================================================================
void ZoneDComponent::highlightLane (int slotIndex)
{
    m_cylinderBand.setHighlightLane (slotIndex);
}

void ZoneDComponent::clearHighlight()
{
    m_cylinderBand.clearHighlight();
}

//==============================================================================
void ZoneDComponent::addTransportListener (TransportListener* l)
{
    m_transportStrip.addTransportListener (l);
}

void ZoneDComponent::removeTransportListener (TransportListener* l)
{
    m_transportStrip.removeTransportListener (l);
}

//==============================================================================
void ZoneDComponent::setPlaying (bool playing)
{
    m_transportStrip.setPlaying (playing);
    m_cylinderBand.setTransportPlaying (playing);
}

void ZoneDComponent::setRecording (bool recording)
{
    m_transportStrip.setRecording (recording);
}

void ZoneDComponent::setBpm (float bpm)
{
    m_bpm = bpm;
    m_transportStrip.setBpm (bpm);
}

void ZoneDComponent::setStructureView (const StructureState* state, const StructureEngine* engine)
{
    m_structureLane.setStructure (state, engine);

    if (state != nullptr && structureHasScaffold (*state) && state->totalBars > 0)
    {
        const auto bars = static_cast<float> (juce::jlimit (1, 32, state->totalBars));
        m_loopLengthBars = bars;
        m_transportStrip.setLoopLength (bars);
        m_cylinderBand.setLoopLengthBars (bars);
    }
}

void ZoneDComponent::setAuthoredTimelineData (const LyricsState* lyricsState, const AutomationState* automationState)
{
    m_structureLane.setAuthoredTimelineData (lyricsState, automationState);
}

void ZoneDComponent::setStructureBeat (double beat)
{
    m_structureLane.setCurrentBeat (beat);
    m_cylinderBand.setCurrentBeat (static_cast<float> (beat));
}

void ZoneDComponent::setStructureFollowState (TransportStrip::StructureFollowState state)
{
    m_transportStrip.setStructureFollowState (state);
}

void ZoneDComponent::clearTimelineClips()
{
    bool changed = false;
    for (auto& lane : m_lanes)
    {
        for (int i = lane.clips.size(); --i >= 0;)
        {
            if (lane.clips.getReference (i).type != ClipType::scaffold)
            {
                lane.clips.remove (i);
                changed = true;
            }
        }
    }

    if (changed)
    {
        juce::Array<LaneData> lanes;
        lanes.addArray (m_lanes.data(), static_cast<int> (m_lanes.size()));
        m_cylinderBand.setLanes (lanes);
    }
}

void ZoneDComponent::addTimelineClip (int slotIndex, const Clip& clip, const juce::String& moduleType)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int> (m_lanes.size()))
        return;

    auto& lane = m_lanes[static_cast<size_t> (slotIndex)];
    lane.slotIndex = slotIndex;
    lane.active = true;

    if (moduleType.isNotEmpty())
        lane.moduleType = moduleType;
    if (lane.moduleType.isEmpty())
        lane.moduleType = "AUDIO";
    lane.dotColor = dotColorForModule (lane.moduleType);

    // Remove the auto-seeded full-lane placeholder the first time a real clip arrives.
    if (lane.clips.size() == 1)
    {
        const auto& existing = lane.clips.getReference (0);
        const float loopBeats = m_loopLengthBars * 4.0f;
        const bool isPlaceholder = std::abs (existing.startBeat) < 0.0001f
                                   && std::abs (existing.lengthBeats - loopBeats) < 0.0001f
                                   && (existing.name.trim().isEmpty()
                                       || existing.name == lane.moduleType
                                       || existing.name == "—");
        if (isPlaceholder)
            lane.clips.clearQuick();
    }

    lane.clips.add (clip);

    juce::Array<LaneData> lanes;
    lanes.addArray (m_lanes.data(), static_cast<int> (m_lanes.size()));
    m_cylinderBand.setLanes (lanes);
}

void ZoneDComponent::seedStructureRails (const StructureState& structure)
{
    if (! structureHasScaffold (structure))
        return;

    const auto resolved = buildResolvedStructure (structure);
    if (resolved.empty())
        return;

    auto appendClipsForLane = [&] (LaneData& lane)
    {
        lane.clips.clear();
        lane.active = true;

        for (size_t s = 0; s < resolved.size(); ++s)
        {
            const auto& instance = resolved[s];
            if (instance.section == nullptr)
                continue;

            const auto sectionTint = sectionColourForIndex (static_cast<int> (s));
            const int repeatBeats = juce::jmax (1, instance.barsPerRepeat * instance.beatsPerBar);
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

                    Clip clip;
                    clip.startBeat = static_cast<float> (chordStartBeat);
                    clip.lengthBeats = juce::jmax (0.25f, static_cast<float> (chordEndBeat - chordStartBeat));
                    clip.name = instance.section->name + " · " + abbreviatedChordLabel (chord);
                    clip.type = ClipType::scaffold;
                    clip.tint = sectionTint;
                    lane.clips.add (clip);

                    beatCursor = chordEndBeat - repeatStartBeat;
                    ++sequenceIndex;
                }
            }
        }
    };

    bool hadActiveLane = false;
    for (auto& lane : m_lanes)
    {
        if (lane.active)
        {
            appendClipsForLane (lane);
            hadActiveLane = true;
        }
    }

    if (! hadActiveLane)
    {
        auto& lane0 = m_lanes[0];
        lane0.slotIndex = 0;
        lane0.moduleType = "STRUCT";
        lane0.dotColor = Theme::Colour::accent;
        appendClipsForLane (lane0);
    }

    juce::Array<LaneData> lanes;
    lanes.addArray (m_lanes.data(), static_cast<int> (m_lanes.size()));
    m_cylinderBand.setLanes (lanes);
}

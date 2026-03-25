#include "ZoneDComponent.h"

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

void ZoneDComponent::seedStructureRails (const StructureState& structure)
{
    if (structure.sections.empty())
        return;

    auto appendClipsForLane = [&] (LaneData& lane)
    {
        lane.clips.clear();
        lane.active = true;

        for (size_t s = 0; s < structure.sections.size(); ++s)
        {
            const auto& section = structure.sections[s];
            const auto sectionTint = sectionColourForIndex (static_cast<int> (s));
            const int chordCount = juce::jmax (1, static_cast<int> (section.progression.size()));
            const float barsPerChord = static_cast<float> (section.lengthBars) / static_cast<float> (chordCount);

            for (int c = 0; c < chordCount; ++c)
            {
                const float startBar = static_cast<float> (section.startBar) + barsPerChord * static_cast<float> (c);
                const float endBar = (c == chordCount - 1)
                                       ? static_cast<float> (section.startBar + section.lengthBars)
                                       : static_cast<float> (section.startBar) + barsPerChord * static_cast<float> (c + 1);

                Clip clip;
                clip.startBeat = startBar * 4.0f;
                clip.lengthBeats = juce::jmax (0.25f, (endBar - startBar) * 4.0f);
                if (c < static_cast<int> (section.progression.size()))
                {
                    const auto& chord = section.progression[(size_t) c];
                    clip.name = section.name + " · " + chord.root + chord.type;
                }
                else
                {
                    clip.name = section.name;
                }
                clip.type = ClipType::midi;
                clip.tint = sectionTint;
                lane.clips.add (clip);
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

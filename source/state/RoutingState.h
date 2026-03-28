#pragma once

#include <juce_core/juce_core.h>
#include <vector>

enum class RouteSignalType
{
    midi = 0,
    audio,
    fx
};

struct RouteEntry
{
    int id { 0 };
    RouteSignalType signalType { RouteSignalType::audio };
    juce::String sourceId;
    juce::String destinationId;
    juce::String busContext;
    int orderIndex { 0 };
    bool enabled { true };
};

struct RoutingState
{
    std::vector<RouteEntry> midiRoutes;
    std::vector<RouteEntry> audioRoutes;
    std::vector<RouteEntry> fxRoutes;
    int nextId { 1 };

    static RoutingState makeDefault()
    {
        RoutingState state;
        auto add = [&] (RouteSignalType type, const juce::String& source, const juce::String& destination, const juce::String& bus, int order)
        {
            RouteEntry entry;
            entry.id = state.nextId++;
            entry.signalType = type;
            entry.sourceId = source;
            entry.destinationId = destination;
            entry.busContext = bus;
            entry.orderIndex = order;
            auto* list = &state.audioRoutes;
            if (type == RouteSignalType::midi) list = &state.midiRoutes;
            else if (type == RouteSignalType::fx) list = &state.fxRoutes;
            list->push_back (entry);
        };

        add (RouteSignalType::midi,  "Keyboard In", "Focused Slot", "Ch 1", 0);
        add (RouteSignalType::midi,  "Sequencer",   "Instrument Rack", "Phrase", 1);
        add (RouteSignalType::audio, "Input",       "Tape History", "Stereo", 0);
        add (RouteSignalType::audio, "Looper",      "Timeline", "Stereo", 1);
        add (RouteSignalType::fx,    "Midi FX",     "Instruments", "Pre-Instrument", 0);
        add (RouteSignalType::fx,    "Audio FX",    "Master Out", "Post-Fader", 1);
        return state;
    }
};

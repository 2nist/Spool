#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <atomic>

enum class RouteSignalType
{
    midi = 0,
    audio,
    fx
};

//==============================================================================
// Stable string IDs for FX send matrix columns.
// Never use display names as IDs — always use these constants.
namespace FXBusID
{
    inline constexpr const char* busA   = "bus_a";
    inline constexpr const char* busB   = "bus_b";
    inline constexpr const char* busC   = "bus_c";
    inline constexpr const char* busD   = "bus_d";
    inline constexpr const char* master = "master";

    // Column index to stable ID (0=bus_a, 1=bus_b, 2=bus_c, 3=bus_d, 4=master)
    inline const char* fromColumn (int col)
    {
        switch (col)
        {
            case 0: return busA;
            case 1: return busB;
            case 2: return busC;
            case 3: return busD;
            case 4: return master;
            default: return master;
        }
    }

    inline int toColumn (const juce::String& id)
    {
        if (id == busA)   return 0;
        if (id == busB)   return 1;
        if (id == busC)   return 2;
        if (id == busD)   return 3;
        if (id == master) return 4;
        return 4; // fallback to master
    }
}

// Stable ID prefix for slot rows: "slot_0" through "slot_7"
namespace FXSlotID
{
    inline juce::String fromSlot (int slot) { return "slot_" + juce::String (slot); }
}

//==============================================================================
// Number of FX matrix slots and bus targets.
static constexpr int kFXSlots   = 8;
static constexpr int kFXTargets = 5; // bus_a, bus_b, bus_c, bus_d, master

//==============================================================================
/**
    FXSendSnapshot — flat read-only copy consumed by the audio thread once per block.

    Layout: enabled[slot][target], level[slot][target]
      slot   0..7  = slot_0..slot_7
      target 0..4  = bus_a, bus_b, bus_c, bus_d, master

    The message thread writes a snapshot; the audio thread reads it.
    No locking: the audio thread reads a single atomic int flag to detect new
    snapshots, then copies the plain struct.  All sends are parallel (signal
    is copied, not diverted).
*/
struct FXSendSnapshot
{
    bool  enabled[kFXSlots][kFXTargets] {};
    float level  [kFXSlots][kFXTargets] {};
};

//==============================================================================
/**
    FXSendMatrix — message-thread state for the 8-slot x 5-target send grid.

    Default: all slots send to MASTER at level 1.0; all bus sends are off.
*/
struct FXSendMatrix
{
    bool  enabled[kFXSlots][kFXTargets] {};
    float level  [kFXSlots][kFXTargets] {};

    static FXSendMatrix makeDefault()
    {
        FXSendMatrix m;
        for (int s = 0; s < kFXSlots; ++s)
        {
            // master = column 4
            m.enabled[s][4] = true;
            m.level  [s][4] = 1.0f;
        }
        return m;
    }

    FXSendSnapshot toSnapshot() const
    {
        FXSendSnapshot snap;
        for (int s = 0; s < kFXSlots; ++s)
            for (int t = 0; t < kFXTargets; ++t)
            {
                snap.enabled[s][t] = enabled[s][t];
                snap.level  [s][t] = level[s][t];
            }
        return snap;
    }
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
    float level { 1.0f };   // post-fader send level 0..1
};

struct RoutingState
{
    std::vector<RouteEntry> midiRoutes;
    std::vector<RouteEntry> audioRoutes;
    std::vector<RouteEntry> fxRoutes;
    FXSendMatrix fxSendMatrix;
    int nextId { 1 };

    static RoutingState makeDefault()
    {
        RoutingState state;
        state.fxSendMatrix = FXSendMatrix::makeDefault();

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

        add (RouteSignalType::midi,  "Keyboard In", "Focused Slot",    "Ch 1",           0);
        add (RouteSignalType::midi,  "Sequencer",   "Instrument Rack", "Phrase",          1);
        add (RouteSignalType::audio, "Input",        "Tape History",   "Stereo",          0);
        add (RouteSignalType::audio, "Looper",       "Timeline",       "Stereo",          1);
        return state;
    }
};

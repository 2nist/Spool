#pragma once

#include "ModuleRow.h"

//==============================================================================
/**
    ModuleGroup — data model for one collapsible, user-colored module group.

    Owns its ModuleRow instances (full-width rows in Zone B's list).
    GroupHeader is the visual companion component.
*/
struct ModuleGroup
{
    juce::String             name      = "GROUP";
    juce::Colour             color     { 0xFF4B9EDB };   // default blue
    bool                     collapsed = false;
    juce::String             id;

    juce::OwnedArray<ModuleRow> slots;   // "slots" kept for API compat
};

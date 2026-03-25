#pragma once

#include <juce_data_structures/juce_data_structures.h>

class ParamRef
{
public:
    ParamRef (juce::ValueTree tree, const juce::Identifier& id)
        : state (std::move (tree)), paramID (id)
    {
    }

    float get() const
    {
        if (! state.isValid())
            return 0.0f;

        return static_cast<float> (state.getProperty (paramID, 0.0f));
    }

    void set (float value)
    {
        if (! state.isValid())
            return;

        state.setProperty (paramID, value, nullptr);
    }

private:
    juce::ValueTree state;
    juce::Identifier paramID;
};

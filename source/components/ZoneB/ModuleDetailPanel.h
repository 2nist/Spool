#pragma once

#include "../../Theme.h"
#include "InlineEditor.h"

//==============================================================================
/**
    ModuleDetailPanel — full-width parameter editor for the currently focused
    module slot.

    Shown in Zone B between the groups viewport and the sequencer section when
    a slot is focused.  Hidden (setVisible(false)) when nothing is focused.

    Layout:
      Header bar (kHeaderH = 22px): colour swatch | type badge | slot name | group
      Editor area (fills remainder): InlineEditor with all module params
*/
class ModuleDetailPanel : public juce::Component
{
public:
    static constexpr int kHeaderH = 22;

    ModuleDetailPanel();
    ~ModuleDetailPanel() override = default;

    /**
        Update the panel with data from a focused slot.
        @param params       All parameters for this module type.
        @param inPorts      Input port names.
        @param outPorts     Output port names.
        @param signalColour Signal-type colour (used in InlineEditor).
        @param typeName     Module type string (e.g. "SYNTH").
        @param groupName    Group this slot belongs to.
        @param groupColour  Group accent colour (used for header swatch).
    */
    void setSlotInfo (const juce::Array<InlineEditor::Param>& params,
                      const juce::StringArray&                inPorts,
                      const juce::StringArray&                outPorts,
                      juce::Colour                            signalColour,
                      const juce::String&                     typeName,
                      const juce::String&                     groupName,
                      juce::Colour                            groupColour);

    void clearSlot();

    /** Forwarded from InlineEditor — fires when a param value changes. */
    std::function<void (int paramIdx, float newValue)> onParamChanged;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    juce::Colour m_groupColour  { 0xFF4B9EDB };
    juce::String m_groupName;
    juce::String m_typeName;
    bool         m_hasSlot      { false };

    InlineEditor m_editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleDetailPanel)
};

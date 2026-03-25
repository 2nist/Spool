#pragma once

#include "../../Theme.h"

//==============================================================================
/**
    PatchBayPanel — module-to-module signal routing matrix.

    Shown in Zone A under the "PTCH" tab.  Rows = signal sources,
    columns = destinations.  Each cell toggles a connection on/off.

    PluginEditor calls setModuleNames() whenever Zone B's module list changes.
*/
class PatchBayPanel : public juce::Component
{
public:
    PatchBayPanel();
    ~PatchBayPanel() override = default;

    /** Replace the displayed module list and reset the connection matrix. */
    void setModuleNames (const juce::StringArray& names);

    /** Fired when the user toggles a connection cell. */
    std::function<void (int fromIdx, int toIdx, bool connected)> onConnectionChanged;

    void paint    (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::StringArray m_names;

    static constexpr int kMaxModules = 8;
    bool m_conn[kMaxModules][kMaxModules] {};   // m_conn[from][to]

    // Layout
    static constexpr int kHeaderH  = 24;
    static constexpr int kLabelW   = 56;
    static constexpr int kCellSize = 18;

    int moduleCount() const noexcept { return juce::jmin (m_names.size(), kMaxModules); }

    juce::Rectangle<int> cellRect (int row, int col) const noexcept;
    void paintHeader (juce::Graphics& g) const;
    void paintGrid   (juce::Graphics& g) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PatchBayPanel)
};

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Theme.h"
#include "../../structure/StructureState.h"

class PluginProcessor;

class StructurePanel : public juce::Component
{
public:
    explicit StructurePanel (PluginProcessor&);
    ~StructurePanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setIntentKeyMode (const juce::String& keyRoot, const juce::String& mode);

    std::function<void()> onStructureApplied;
    std::function<void(bool)> onStructureFollowModeChanged;
    std::function<void(const juce::Array<int>&)> onRailsSeeded;

private:
    class ContentComponent : public juce::Component
    {
    public:
        explicit ContentComponent (StructurePanel& ownerIn) : owner (ownerIn) {}
        void paint (juce::Graphics& g) override;
    private:
        StructurePanel& owner;
    };

    enum class SelectionTarget
    {
        none,
        section,
        arrangementBlock
    };

    class SectionListModel : public juce::ListBoxModel
    {
    public:
        explicit SectionListModel (StructurePanel& ownerIn) : owner (ownerIn) {}
        int getNumRows() override;
        void paintListBoxItem (int rowNumber, juce::Graphics&, int width, int height, bool selected) override;
        void selectedRowsChanged (int lastRowSelected) override;
    private:
        StructurePanel& owner;
    };

    class ArrangementListModel : public juce::ListBoxModel
    {
    public:
        explicit ArrangementListModel (StructurePanel& ownerIn) : owner (ownerIn) {}
        int getNumRows() override;
        void paintListBoxItem (int rowNumber, juce::Graphics&, int width, int height, bool selected) override;
        void selectedRowsChanged (int lastRowSelected) override;
    private:
        StructurePanel& owner;
    };

    class ProgressionStrip : public juce::Component
    {
    public:
        explicit ProgressionStrip (StructurePanel& ownerIn) : owner (ownerIn) {}
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent& event) override;
        void mouseDoubleClick (const juce::MouseEvent& event) override;
    private:
        StructurePanel& owner;
    };

    PluginProcessor& processorRef;
    ContentComponent content { *this };
    juce::Viewport viewport;

    SectionListModel sectionListModel { *this };
    ArrangementListModel arrangementListModel { *this };
    ProgressionStrip progressionStrip { *this };

    juce::ToggleButton structureFollowToggle { "Use Structure For Tracks" };
    juce::TextButton seedRailsButton { "Seed Rails" };

    juce::Label sectionListTitle { {}, "Sections" };
    juce::ListBox sectionList { "Section List", &sectionListModel };
    juce::TextButton addSectionButton { "Add" };
    juce::TextButton duplicateSectionButton { "Duplicate" };
    juce::TextButton deleteSectionButton { "Delete" };
    juce::TextButton moveSectionUpButton { "Up" };
    juce::TextButton moveSectionDownButton { "Down" };

    juce::Label arrangementTitle { {}, "Arrangement" };
    juce::ListBox arrangementList { "Arrangement List", &arrangementListModel };
    juce::TextButton addArrangementButton { "Add" };
    juce::TextButton duplicateArrangementButton { "Duplicate" };
    juce::TextButton deleteArrangementButton { "Delete" };
    juce::TextButton moveArrangementUpButton { "Up" };
    juce::TextButton moveArrangementDownButton { "Down" };

    juce::Label editorHeader { {}, "Section Editor" };
    juce::Label editorHint { {}, "Select a section or arrangement block to sculpt song form." };
    juce::Label nameLabel { {}, "Name" };
    juce::Label barsLabel { {}, "Length (Bars)" };
    juce::Label repeatsLabel { {}, "Repeats" };
    juce::Label transitionLabel { {}, "Transition" };
    juce::Label beatsLabel { {}, "Meter" };
    juce::Label keyLabel { {}, "Key / Root" };
    juce::Label modeLabel { {}, "Mode" };
    juce::Label centerLabel { {}, "Harmonic Center" };
    juce::TextEditor nameEditor;
    juce::Slider barsSlider;
    juce::Slider repeatsSlider;
    juce::ComboBox beatsPerBarBox;
    juce::ComboBox transitionIntentBox;
    juce::ComboBox keyRootBox;
    juce::ComboBox modeBox;
    juce::ComboBox harmonicCenterBox;

    juce::Label progressionHeader { {}, "Progression Cells" };
    juce::Label progressionHint { {}, "One harmonic cell per bar. Cells loop across the section length." };
    juce::Label chordRootLabel { {}, "Selected Chord Root" };
    juce::Label chordTypeLabel { {}, "Selected Chord Quality" };
    juce::TextButton addChordCellButton { "Add Cell" };
    juce::TextButton removeChordCellButton { "Remove" };
    juce::ComboBox chordRootBox;
    juce::ComboBox chordTypeBox;

    juce::Label summaryHeader { {}, "Song Summary" };
    juce::Label summaryTempo { {}, "-" };
    juce::Label summaryKey { {}, "-" };
    juce::Label summaryMode { {}, "-" };
    juce::Label summaryBars { {}, "-" };
    juce::Label summaryDuration { {}, "-" };

    SelectionTarget selectionTarget { SelectionTarget::none };
    int selectedSectionIndex = -1;
    int selectedArrangementIndex = -1;
    int selectedChordIndex = -1;
    bool suppressListCallbacks = false;
    bool suppressControlCallbacks = false;

    void addSection();
    void duplicateSelectedSection();
    void deleteSelectedSection();
    void moveSelectedSection (int delta);

    void addArrangementBlock();
    void duplicateSelectedArrangementBlock();
    void deleteSelectedArrangementBlock();
    void moveSelectedArrangementBlock (int delta);
    void resequenceArrangement();

    void addChordCell();
    void removeSelectedChordCell();
    void selectChordCell (int index);
    void showChordCellMenu();
    void showChordEditMenu (int index);
    void applyTheoryAction (int commandId);

    void selectSection (int index);
    void selectArrangementBlock (int index);
    void clearSelection();

    Section* getSelectedSection();
    const Section* getSelectedSection() const;
    ArrangementBlock* getSelectedArrangementBlock();
    const ArrangementBlock* getSelectedArrangementBlock() const;
    Chord* getSelectedChordCell();
    const Chord* getSelectedChordCell() const;

    juce::Rectangle<int> chordCellBounds (int index) const;
    void refreshLists();
    void refreshEditor();
    void refreshSummary();
    void commitStructureChange (bool seedRails);
    void paintContent (juce::Graphics&);
    void layoutContent (juce::Rectangle<int> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StructurePanel)
};

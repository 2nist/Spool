#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "../../Theme.h"

class PluginProcessor;  

class StructurePanel : public juce::Component
{
public:
    StructurePanel (PluginProcessor&);
    ~StructurePanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setIntentKeyMode (const juce::String& keyRoot, const juce::String& mode);

    std::function<void()> onStructureApplied;
    std::function<void(bool)> onStructureFollowModeChanged;
    std::function<void(const juce::Array<int>&)> onRailsSeeded;

private:
    enum class SelectionType
    {
        none,
        pattern,
        section,
        chord
    };

    class PatternsModel : public juce::ListBoxModel
    {
    public:
        explicit PatternsModel (StructurePanel& ownerIn) : owner (ownerIn) {}
        int getNumRows() override;
        void paintListBoxItem (int rowNumber, juce::Graphics&, int width, int height, bool selected) override;
        void selectedRowsChanged (int lastRowSelected) override;

    private:
        StructurePanel& owner;
    };

    class SectionsModel : public juce::ListBoxModel
    {
    public:
        explicit SectionsModel (StructurePanel& ownerIn) : owner (ownerIn) {}
        int getNumRows() override;
        void paintListBoxItem (int rowNumber, juce::Graphics&, int width, int height, bool selected) override;
        void selectedRowsChanged (int lastRowSelected) override;

    private:
        StructurePanel& owner;
    };

    class ChordsModel : public juce::ListBoxModel
    {
    public:
        explicit ChordsModel (StructurePanel& ownerIn) : owner (ownerIn) {}
        int getNumRows() override;
        void paintListBoxItem (int rowNumber, juce::Graphics&, int width, int height, bool selected) override;
        void selectedRowsChanged (int lastRowSelected) override;

    private:
        StructurePanel& owner;
    };

    PluginProcessor& processorRef;
    PatternsModel patternsModel { *this };
    SectionsModel sectionsModel { *this };
    ChordsModel chordsModel { *this };
    juce::TabbedComponent listTabs { juce::TabbedButtonBar::TabsAtTop };

    juce::ListBox patternsList { "Patterns", &patternsModel };
    juce::TextButton addPatternButton { "Add Pattern" };
    juce::TextButton addSectionButton { "Add Section" };
    juce::TextButton addChordButton { "Add Chord" };
    juce::TextButton deleteButton { "Delete Selected" };

    juce::ListBox sectionsList { "Sections", &sectionsModel };
    juce::ListBox chordsList { "Chords", &chordsModel };
    
    juce::TextButton loadButton {"Load"};
    juce::TextButton saveButton {"Save"};
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Thin-slice structure intent input.
    juce::Label intentTitle { {}, "Structure Intent" };
    juce::Label intentSectionLabel { {}, "Section" };
    juce::TextEditor intentSectionEditor;
    juce::Label intentRootLabel { {}, "Root" };
    juce::ComboBox intentRootBox;
    juce::Label intentModeLabel { {}, "Mode" };
    juce::ComboBox intentModeBox;
    juce::Label intentProgLabel { {}, "Roman" };
    juce::TextEditor intentProgressionEditor;
    juce::Label intentBarsLabel { {}, "Bars" };
    juce::TextEditor intentBarsEditor;
    juce::TextButton intentApplyButton { "Apply Structure" };
    juce::TextButton intentApplyToRailsButton { "Apply To Rails" };
    juce::ToggleButton intentFollowToggle { "Use Structure For Tracks" };
    juce::ToggleButton intentSelectedOnlyToggle { "Selected Lane Only" };
    juce::ToggleButton advancedToggle { "Advanced" };
    juce::Label intentFeedbackLabel { {}, "" };

    juce::Label detailsTitle { {}, "Details" };
    juce::Label quickTitle { {}, "Quick Progression" };
    juce::Label progressionLabel { {}, "Progression" };
    juce::TextEditor progressionEditor;
    juce::Label barsLabel { {}, "Bars/Chord" };
    juce::ComboBox barsPerChordBox;
    juce::Label targetPatternLabel { {}, "Pattern" };
    juce::ComboBox targetPatternBox;
    juce::ToggleButton replaceExistingToggle { "Replace sections/chords" };
    juce::TextButton buildProgressionButton { "Build Progression" };

    // Pattern editor
    juce::Label patternNameLabel { {}, "Name" };
    juce::TextEditor patternNameEditor;
    juce::Label patternLengthLabel { {}, "Length (beats)" };
    juce::TextEditor patternLengthEditor;

    // Section editor
    juce::Label sectionPatternLabel { {}, "Pattern" };
    juce::ComboBox sectionPatternBox;
    juce::Label sectionStartLabel { {}, "Start" };
    juce::TextEditor sectionStartEditor;
    juce::Label sectionLengthLabel { {}, "Length" };
    juce::TextEditor sectionLengthEditor;
    juce::Label sectionRepeatsLabel { {}, "Repeats" };
    juce::TextEditor sectionRepeatsEditor;
    juce::Label sectionSlotsLabel { {}, "Slot Mask" };
    juce::TextEditor sectionSlotsEditor;
    juce::Label sectionTransposeLabel { {}, "Transpose" };
    juce::TextEditor sectionTransposeEditor;

    // Chord editor
    juce::Label chordStartLabel { {}, "Start" };
    juce::TextEditor chordStartEditor;
    juce::Label chordDurationLabel { {}, "Duration" };
    juce::TextEditor chordDurationEditor;
    juce::Label chordRootLabel { {}, "Root MIDI" };
    juce::TextEditor chordRootEditor;
    juce::Label chordTypeLabel { {}, "Type" };
    juce::TextEditor chordTypeEditor;

    juce::TextButton applyButton { "Apply Changes" };

    SelectionType selectionType { SelectionType::none };
    int selectedIndex = -1;
    bool suppressSelectionCallbacks = false;
    std::vector<std::pair<int, int>> chordRowMap;

    void setSelection (SelectionType type, int index);
    void clearSelection();
    void populateEditorsForSelection();
    void applySelectionChanges();
    void showPatternEditors (bool show);
    void showSectionEditors (bool show);
    void showChordEditors (bool show);

    void addPattern();
    void addSection();
    void addChord();
    void deleteSelected();
    void buildProgression();
    void refreshPatternChoices();
    void rebuildChordRowMap();
    void loadSong();
    void saveSong();
    void refreshLists();
    void applyIntentStructure();
    void applyIntentToRails();
    void updateLegacyVisibility();

    bool showAdvanced = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StructurePanel)
};

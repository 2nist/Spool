#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Theme.h"
#include "../../structure/StructureState.h"
#include "ZoneAStyle.h"

class PluginProcessor;

class StructurePanel : public juce::Component,
                       public juce::DragAndDropContainer
{
public:
    explicit StructurePanel (PluginProcessor&);
    ~StructurePanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setIntentKeyMode (const juce::String& keyRoot, const juce::String& mode);
    void refreshFromModel();

    std::function<void()> onStructureApplied;
    std::function<void(bool)> onStructureFollowModeChanged;
    std::function<void(const juce::Array<int>&)> onRailsSeeded;

private:
    enum class ListKind
    {
        sections,
        arrangement
    };

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
        void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;
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

    class StructureListBox : public juce::ListBox,
                             public juce::DragAndDropTarget
    {
    public:
        StructureListBox (StructurePanel& ownerIn,
                          ListKind kindIn,
                          const juce::String& name,
                          juce::ListBoxModel* modelIn);

        void mouseDown (const juce::MouseEvent& event) override;
        void mouseDrag (const juce::MouseEvent& event) override;
        void mouseUp (const juce::MouseEvent& event) override;
        void paintOverChildren (juce::Graphics& g) override;

        bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
        void itemDragEnter (const SourceDetails& dragSourceDetails) override;
        void itemDragMove (const SourceDetails& dragSourceDetails) override;
        void itemDragExit (const SourceDetails& dragSourceDetails) override;
        void itemDropped (const SourceDetails& dragSourceDetails) override;

    private:
        StructurePanel& owner;
        ListKind kind;
        int dragStartRow = -1;
        bool dragStarted = false;
        int dropRow = -1;
        bool dropHover = false;
        bool dropAcceptsExternalSource = false;
    };

    class TrashDropZone : public juce::Component,
                          public juce::DragAndDropTarget
    {
    public:
        TrashDropZone (StructurePanel& ownerIn, ListKind kindIn) : owner (ownerIn), kind (kindIn) {}

        void paint (juce::Graphics& g) override;
        void mouseUp (const juce::MouseEvent& event) override;

        bool isInterestedInDragSource (const SourceDetails& dragSourceDetails) override;
        void itemDragEnter (const SourceDetails& dragSourceDetails) override;
        void itemDragExit (const SourceDetails& dragSourceDetails) override;
        void itemDropped (const SourceDetails& dragSourceDetails) override;

    private:
        StructurePanel& owner;
        ListKind kind;
        bool dragHover = false;
    };

    class ProgressionStrip : public juce::Component
    {
    public:
        explicit ProgressionStrip (StructurePanel& ownerIn) : owner (ownerIn) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent& event) override;
        void mouseDrag (const juce::MouseEvent& event) override;
        void mouseUp (const juce::MouseEvent& event) override;
        void mouseDoubleClick (const juce::MouseEvent& event) override;
    private:
        StructurePanel& owner;
        int dragCellIndex = -1;
        int dragStartDuration = 0;
        bool resizingDuration = false;
        StructureState dragBeforeState;
    };

    class DiatonicPaletteStrip : public juce::Component
    {
    public:
        explicit DiatonicPaletteStrip (StructurePanel& ownerIn) : owner (ownerIn) {}
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent& event) override;
    private:
        StructurePanel& owner;
    };

    PluginProcessor& processorRef;
    ContentComponent content { *this };
    juce::Viewport viewport;

    SectionListModel sectionListModel { *this };
    ArrangementListModel arrangementListModel { *this };
    ProgressionStrip progressionStrip { *this };
    DiatonicPaletteStrip diatonicPaletteStrip { *this };

    juce::ToggleButton structureFollowToggle { "Use Structure For Tracks" };
    juce::TextButton seedRailsButton { "Seed Rails" };

    juce::Label sectionListTitle { {}, "Sections" };
    juce::Label sectionListHint { {}, "Reusable section loops" };
    StructureListBox sectionList { *this, ListKind::sections, "Section List", &sectionListModel };
    juce::TextButton addSectionButton { "Add" };
    juce::TextButton duplicateSectionButton { "Duplicate" };
    TrashDropZone deleteSectionButton { *this, ListKind::sections };
    juce::TextButton moveSectionUpButton { "Up" };
    juce::TextButton moveSectionDownButton { "Down" };
    juce::TextEditor inlineSectionNameEditor;

    juce::Label arrangementTitle { {}, "Arrangement" };
    juce::Label arrangementFlowHint { {}, "Drag sections here to build song order" };
    StructureListBox arrangementList { *this, ListKind::arrangement, "Arrangement List", &arrangementListModel };
    juce::Label arrangementHint { {}, "Drag sections here" };
    juce::TextButton addArrangementButton { "Add" };
    juce::TextButton duplicateArrangementButton { "Duplicate" };
    TrashDropZone deleteArrangementButton { *this, ListKind::arrangement };
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
    juce::Label progressionPaletteHint { {}, "" };
    juce::Label chordRootLabel { {}, "Selected Chord Root" };
    juce::Label chordTypeLabel { {}, "Selected Chord Quality" };
    juce::TextButton addChordCellButton { "Add Cell" };
    juce::TextButton progressionPresetButton { "Preset" };
    juce::TextButton cadenceButton { "Cadence" };
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
    int inlineRenameRow = -1;
    bool suppressListCallbacks = false;
    bool suppressControlCallbacks = false;

    void addSection();
    void duplicateSelectedSection();
    void deleteSelectedSection();
    void moveSelectedSection (int delta);
    void moveSectionTo (int fromIndex, int toIndex);
    void deleteSectionAt (int index);

    void addArrangementBlock();
    void duplicateSelectedArrangementBlock();
    void deleteSelectedArrangementBlock();
    void moveSelectedArrangementBlock (int delta);
    void moveArrangementBlockTo (int fromIndex, int toIndex);
    void deleteArrangementBlockAt (int index);
    void resequenceArrangement();
    void addArrangementBlockFromSection (int sectionIndex, int targetIndex);

    juce::String dragDescriptionForSectionIndex (int index) const;
    juce::String dragDescriptionForArrangementIndex (int index) const;
    bool isDragDescriptionForKind (const juce::var& description, ListKind kind) const;
    int dragIndexFromDescription (const juce::var& description, ListKind kind) const;
    void handleListDrop (ListKind kind, int fromIndex, int toIndex);
    void handleTrashDrop (ListKind kind, int fromIndex);

    void addChordCell();
    void removeSelectedChordCell();
    void selectChordCell (int index);
    void showChordCellMenu (int index = -1, juce::Point<int> anchorInProgressionStrip = {});
    void showChordEditMenu (int index, juce::Point<int> anchorInProgressionStrip = {});
    void showProgressionPresetMenu();
    void showCadenceMenu();
    void applyDiatonicPaletteChord (int degreeIndex, bool append);
    void applyTheoryAction (int commandId);

    void selectSection (int index);
    void selectArrangementBlock (int index);
    void clearSelection();
    void beginInlineSectionRename (int index);
    void finishInlineSectionRename (bool commit);

    Section* getSelectedSection();
    const Section* getSelectedSection() const;
    ArrangementBlock* getSelectedArrangementBlock();
    const ArrangementBlock* getSelectedArrangementBlock() const;
    Chord* getSelectedChordCell();
    const Chord* getSelectedChordCell() const;

    int chordStartBeat (int index) const;
    int chordIndexForBeat (int beat) const;
    int progressionBeatAtPoint (juce::Point<int> position) const;
    juce::Rectangle<float> progressionGridBounds() const;
    juce::Rectangle<int> chordCellBounds (int index) const;
    juce::Rectangle<float> progressionChipBounds (int index) const;
    void updateSectionDerivedLength (Section& section);
    void refreshLists();
    void refreshEditor();
    void refreshSummary();
    void commitStructureChange (const StructureState& beforeState, const juce::String& actionName, bool seedRails);
    void paintContent (juce::Graphics&);
    void layoutContent (juce::Rectangle<int> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StructurePanel)
};

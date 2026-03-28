#pragma once

#include "../../Theme.h"
#include "../../state/LyricsState.h"
#include "ZoneAStyle.h"
#include "ZoneAControlStyle.h"

class PluginProcessor;

class LyricsPanel : public juce::Component,
                    private juce::ListBoxModel
{
public:
    explicit LyricsPanel (PluginProcessor& processor);
    ~LyricsPanel() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshFromModel();

private:
    PluginProcessor& processorRef;
    juce::ListBox itemList;
    juce::Viewport editorViewport;
    juce::Component editorContent;
    juce::TextButton addItemButton { "+ NOTE" };
    juce::TextButton duplicateButton { "DUP" };
    juce::TextButton deleteButton { "DEL" };
    juce::TextButton pinButton { "PIN" };
    juce::Label listHeader;
    juce::Label editorHeader;
    juce::Label editorHint;
    juce::Label textLabel;
    juce::Label fontLabel;
    juce::Label sizeLabel;
    juce::Label colorLabel;
    juce::Label beatLabel;
    juce::TextEditor textEditor;
    juce::ComboBox fontBox;
    juce::ComboBox sizeBox;
    juce::ComboBox colorBox;
    juce::Slider beatSlider;
    juce::Label beatValue;
    juce::Label placementSummary;
    int selectedIndex { -1 };
    bool suppressCallbacks { false };

    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;

    LyricsState& lyricsForEdit();
    const LyricsState& lyrics() const;
    LyricItem* selectedItem();
    const LyricItem* selectedItem() const;
    void commitChanges (const LyricsState& beforeState, const juce::String& actionName);
    void refreshList();
    void refreshEditor();
    void addItem();
    void duplicateItem();
    void deleteItem();
    void pinSelectedToCurrentBeat();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LyricsPanel)
};

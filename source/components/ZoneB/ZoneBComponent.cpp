#include "ZoneBComponent.h"

//==============================================================================
// Constructor / Destructor
//==============================================================================

ZoneBComponent::ZoneBComponent()
{
    // Viewport holds the scrollable row list
    m_rowsViewport.setScrollBarsShown (true, false);
    m_rowsViewport.setViewedComponent (&m_rowsContent, false);
    m_rowsContent.onResized = [this] { layoutRowsContent(); };
    addAndMakeVisible (m_rowsViewport);

    // ADD GROUP button (inside rows content)
    m_addGroupBtn.setColour (juce::TextButton::buttonColourId,  juce::Colours::transparentBlack);
    m_addGroupBtn.setColour (juce::TextButton::textColourOffId, Theme::Colour::inkGhost);
    m_addGroupBtn.setButtonText ("+  GROUP");
    m_addGroupBtn.onClick = [this]
    {
        const juce::String name  = "GROUP " + juce::String (m_groups.size() + 1);
        const juce::Colour color { 0xFF4B9EDB };
        addGroup (name, color);
        const int newIdx = m_groups.size() - 1;
        if (auto* hdr = m_groupHeaders[newIdx])
            hdr->startRename();
    };
    m_rowsContent.addAndMakeVisible (m_addGroupBtn);

    // Split handle
    addAndMakeVisible (m_splitHandle);
    m_splitHandle.onDragStarted = [this]
    {
        // Materialise the visual position before starting a drag if default (-1).
        if (m_splitY < 0)
            m_splitY = juce::jmax (kStripeH + kMinRowsH,
                                   getHeight() - kHandleH - kMinBottomH);
        m_splitDragStartY = m_splitY;
    };
    m_splitHandle.onDrag = [this] (int dy)
    {
        const int minSplit = kStripeH + kMinRowsH;
        const int maxSplit = getHeight() - kHandleH - kMinBottomH;
        m_splitY = juce::jlimit (minSplit, juce::jmax (minSplit, maxSplit),
                                 m_splitDragStartY + dy);
        resized();
    };

    // Sequencer area
    addAndMakeVisible (m_seqHeader);
    addAndMakeVisible (m_stepGridSingle);
    addChildComponent (m_stepGridDrum);
    addAndMakeVisible (m_looperStrip);

    // Sequencer header callbacks
    m_seqHeader.onCopy = [this]
    {
        if (m_focusedRow != nullptr)
        {
            m_patternClipboard = m_focusedRow->pattern().copyPattern();
            m_hasClipboard = true;
            m_seqHeader.setClipboard (&m_patternClipboard, true);
        }
    };

    m_seqHeader.onPaste = [this]
    {
        if (m_focusedRow != nullptr && m_hasClipboard)
        {
            m_focusedRow->pattern() = m_patternClipboard;
            m_stepGridSingle.repaint();
            m_stepGridDrum.repaint();
        }
    };

    m_seqHeader.onClear = [this]
    {
        if (m_focusedRow != nullptr)
        {
            m_focusedRow->pattern().clearPattern();
            m_stepGridSingle.repaint();
            m_stepGridDrum.repaint();
        }
    };

    m_seqHeader.onPatternChanged = [this] (int) { m_stepGridSingle.repaint(); m_stepGridDrum.repaint(); };

    createDefaultGroups();
}

ZoneBComponent::~ZoneBComponent()
{
    for (auto* grp : m_groups)
        for (auto* row : grp->slots)
            m_rowsContent.removeChildComponent (row);

    for (auto* hdr : m_groupHeaders)
        m_rowsContent.removeChildComponent (hdr);
}

//==============================================================================
// Default groups
//==============================================================================

void ZoneBComponent::createDefaultGroups()
{
    addGroup ("SYNTHS", juce::Colour (0xFF4B9EDB));
    for (int i = 0; i < 3; ++i)
        addSlotToGroup (0, ModuleRow::ModuleType::Synth);

    addGroup ("DRUMS", juce::Colour (0xFFDBA34B));
    addSlotToGroup (1, ModuleRow::ModuleType::DrumMachine);

    updateSourceNames();
}

//==============================================================================
// Group / row management
//==============================================================================

void ZoneBComponent::addGroup (const juce::String& name, juce::Colour color)
{
    auto* grp    = m_groups.add (new ModuleGroup{});
    grp->name    = name;
    grp->color   = color;
    grp->id      = juce::Uuid{}.toString();

    auto* header = m_groupHeaders.add (new GroupHeader (*grp));
    m_rowsContent.addAndMakeVisible (*header);
    connectGroupHeader (m_groups.size() - 1);

    layoutRowsContent();
    updateSourceNames();
}

void ZoneBComponent::addSlotToGroup (int groupIndex, ModuleRow::ModuleType type)
{
    if (groupIndex < 0 || groupIndex >= m_groups.size()) return;

    auto& grp   = *m_groups[groupIndex];
    const int ri = grp.slots.size();
    auto* row   = grp.slots.add (new ModuleRow (ri));

    row->setModuleType (type);
    row->setGroupColor (grp.color);

    row->onRowClicked  = [this, row, groupIndex] (int) { onRowClicked  (row, groupIndex); };
    row->onEmptyClicked= [this, row, groupIndex] (int) { onEmptyClicked(row, groupIndex); };
    row->onRightClicked= [this, row, groupIndex] (int) { onRightClick  (row, groupIndex); };

    row->onCollapseToggled = [this] (int)
    {
        layoutRowsContent();
        resized();
    };

    m_rowsContent.addAndMakeVisible (*row);
    layoutRowsContent();
    updateSourceNames();
}

void ZoneBComponent::removeGroup (int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= m_groups.size()) return;

    if (m_focusedGroupIdx == groupIndex)
        defocusRow();

    if (auto* hdr = m_groupHeaders[groupIndex])
        m_rowsContent.removeChildComponent (hdr);

    for (auto* row : m_groups[groupIndex]->slots)
        m_rowsContent.removeChildComponent (row);

    m_groupHeaders.remove (groupIndex);
    m_groups.remove (groupIndex);

    for (int i = 0; i < m_groups.size(); ++i)
        connectGroupHeader (i);

    layoutRowsContent();
    updateSourceNames();
}

void ZoneBComponent::connectGroupHeader (int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= m_groups.size()) return;
    auto* hdr = m_groupHeaders[groupIndex];
    if (hdr == nullptr) return;

    hdr->onAddSlot = [this, groupIndex]
    {
        addSlotToGroup (groupIndex, ModuleRow::ModuleType::Empty);
    };

    hdr->onToggleCollapse = [this] { layoutRowsContent(); };

    hdr->onDelete = [this, groupIndex] { removeGroup (groupIndex); };

    hdr->onColorChanged = [this, groupIndex] (juce::Colour color)
    {
        for (auto* row : m_groups[groupIndex]->slots)
            row->setGroupColor (color);
        m_rowsContent.repaint();
    };

    hdr->onNameChanged = [this] (const juce::String&)
    {
        m_rowsContent.repaint();
        updateSourceNames();
    };
}

//==============================================================================
// Layout
//==============================================================================

int ZoneBComponent::calculateRowsContentH() const noexcept
{
    int h = 0;
    for (auto* grp : m_groups)
    {
        h += GroupHeader::kHeight;
        if (!grp->collapsed)
            for (auto* row : grp->slots)
                h += row->currentHeight() + kRowGap;
    }
    h += 18;  // ADD GROUP button
    return h;
}

void ZoneBComponent::layoutRowsContent()
{
    const int vpW  = m_rowsViewport.getWidth();
    const int vpH  = m_rowsViewport.getHeight();

    const int contentH = juce::jmax (calculateRowsContentH(), vpH);
    m_rowsContent.setSize (vpW, contentH);

    int y = 0;
    for (int gi = 0; gi < m_groups.size(); ++gi)
    {
        auto& grp = *m_groups[gi];
        auto* hdr = m_groupHeaders[gi];

        hdr->setBounds (0, y, vpW, GroupHeader::kHeight);
        y += GroupHeader::kHeight;

        // Calculate a desired expanded row height so that a single expanded
        // module occupies at least 1/3 of the rows area (so a full module
        // is visible on start). We only apply the override when it is
        // larger than the default ModuleRow::kExpandedH.
        const int rowsH = rowsAreaH();
        // Make a single expanded module occupy a larger fraction of the rows
        // area so modules are more visible by default. Use half the rows area
        // (instead of one third) as the target when available.
        const int desiredPerRow = rowsH > 0 ? juce::jmax (ModuleRow::kExpandedH, rowsH / 2) : ModuleRow::kExpandedH;

        for (auto* row : grp.slots)
        {
            if (!grp.collapsed)
            {
                // Apply override if desiredPerRow is larger than default
                if (desiredPerRow > ModuleRow::kExpandedH)
                    row->setExpandedHeightOverride (desiredPerRow);
                else
                    row->setExpandedHeightOverride (0);

                row->setBounds (0, y, vpW, row->currentHeight());
                row->setVisible (true);
                y += row->currentHeight() + kRowGap;
            }
            else
            {
                row->setVisible (false);
            }
        }
    }

    m_addGroupBtn.setBounds (4, y, 80, 14);
}

void ZoneBComponent::updateSourceNames()
{
    juce::StringArray looperNames;
    looperNames.add ("MIX");
    for (auto* grp : m_groups)
        looperNames.add (grp->name);
    m_looperStrip.setSourceNames (looperNames);

    if (onModuleListChanged)
    {
        juce::StringArray slotNames;
        for (auto* grp : m_groups)
            for (auto* row : grp->slots)
                slotNames.add (grp->name + ":" + row->getModuleTypeName());
        onModuleListChanged (slotNames);
    }
}

//==============================================================================
// resized
//==============================================================================

void ZoneBComponent::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    const int minSplit = kStripeH + kMinRowsH;
    const int maxSplit = h - kHandleH - kMinBottomH;

    if (m_splitY >= 0)
    {
        // User has set the split: respect their choice, just clamp to valid range.
        m_splitY = juce::jlimit (minSplit, juce::jmax (minSplit, maxSplit), m_splitY);
    }
    // else m_splitY remains -1 — layout helpers (rowsAreaH / bottomStart / paint)
    // use the -1 sentinel to give rows *maximum* space by default.

    // Visual split position used for the handle and step-grid height
    const int splitPos = (m_splitY >= 0) ? m_splitY
                                         : juce::jmax (minSplit, maxSplit);

    // Dynamic grid height — fills all space between split handle and looper
    m_gridH = juce::jmax ((int) kMinGridH, h - (splitPos + kHandleH) - kSeqHdrH - kLooperH);

    // Rows viewport
    m_rowsViewport.setBounds (0, kStripeH, w, rowsAreaH());
    layoutRowsContent();

    // Split handle
    m_splitHandle.setBounds (0, splitPos, w, kHandleH);

    // Bottom section
    const int y0 = bottomStart();
    m_seqHeader.setBounds      (0, y0,                   w, kSeqHdrH);
    m_stepGridSingle.setBounds (0, y0 + kSeqHdrH,        w, m_gridH);
    m_stepGridDrum.setBounds   (0, y0 + kSeqHdrH,        w, m_gridH);
    m_looperStrip.setBounds    (0, y0 + kSeqHdrH + m_gridH, w, kLooperH);

    if (m_loadDialog != nullptr)
        m_loadDialog->setBounds (m_loadDialog->getBounds().constrainedWithin (getLocalBounds()));
}

//==============================================================================
// Paint
//==============================================================================

void ZoneBComponent::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgB);
    Theme::Helper::drawZoneStripe (g, getLocalBounds(), Theme::Zone::b);

    // Split handle
    {
        const int hy = (m_splitY >= 0) ? m_splitY : (kStripeH + rowsAreaH());
        g.setColour (Theme::Colour::surface1);
        g.fillRect  (0, hy, getWidth(), kHandleH);

        const int cx = getWidth() / 2;
        const int cy = hy + kHandleH / 2;
        for (int i = -1; i <= 1; ++i)
        {
            g.setColour (Theme::Colour::inkGhost.withAlpha (0.6f));
            g.fillEllipse (static_cast<float> (cx + i * 6 - 1),
                           static_cast<float> (cy - 1), 3.0f, 3.0f);
        }
        g.setColour (Theme::Colour::surfaceEdge);
        g.fillRect  (0, hy, getWidth(), 1);
        g.fillRect  (0, hy + kHandleH - 1, getWidth(), 1);
    }

    // Sequencer panel background
    {
        const int seqY = seqHeaderY();
        g.setColour (Theme::Colour::surface0);
        g.fillRect  (0, seqY, getWidth(), kSeqHdrH + m_gridH);
    }

    // Looper panel background
    {
        const int loopY = seqHeaderY() + kSeqHdrH + m_gridH;
        g.setColour (Theme::Colour::surface1);
        g.fillRect  (0, loopY, getWidth(), kLooperH);
        g.setColour (Theme::Colour::surfaceEdge);
        g.fillRect  (0, loopY, getWidth(), 1);
    }
}

//==============================================================================
// Focus
//==============================================================================

void ZoneBComponent::focusRow (ModuleRow* row, int groupIndex)
{
    if (m_focusedRow == row) return;

    if (m_focusedRow != nullptr)
        m_focusedRow->setSelected (false);

    m_focusedRow      = row;
    m_focusedGroupIdx = groupIndex;

    if (row != nullptr)
    {
        row->setSelected (true);

        juce::String groupName;
        juce::Colour groupColor { 0x00000000 };
        if (groupIndex >= 0 && groupIndex < m_groups.size())
        {
            groupName  = m_groups[groupIndex]->name;
            groupColor = m_groups[groupIndex]->color;
        }

        m_seqHeader.setFocusedSlot (row->getModuleTypeName(), groupName, groupColor);
        m_seqHeader.setPattern     (&row->pattern());
        m_seqHeader.setClipboard   (&m_patternClipboard, m_hasClipboard);

        if (row->isDrumMachine())
        {
            resized();
            m_stepGridSingle.setVisible (false);
            m_stepGridSingle.clearPattern();
            m_stepGridDrum.setVisible (true);
            m_stepGridDrum.setDrumData (row->drumData(), groupColor);

            // Compute flat index for DSP wiring
            int drumFlatIdx = row->getRowIndex();
            for (int g = 0; g < groupIndex; ++g)
                drumFlatIdx += m_groups[g]->slots.size();
            const int capturedDrumIdx = drumFlatIdx;

            if (onDrumSlotFocused)
                onDrumSlotFocused (capturedDrumIdx, row->drumData());

            m_stepGridDrum.onModified = [this, capturedDrumIdx]()
            {
                if (m_focusedRow != nullptr && onDrumPatternModified)
                    onDrumPatternModified (capturedDrumIdx, m_focusedRow->drumData());
            };
        }
        else
        {
            resized();
            m_stepGridDrum.setVisible (false);
            m_stepGridDrum.clearDrumData();
            m_stepGridSingle.setVisible (true);
            m_stepGridSingle.setPattern (&row->pattern(), groupColor);

            // Compute flat index for pattern→processor wiring
            int flatIdx = row->getRowIndex();
            for (int g = 0; g < groupIndex; ++g)
                flatIdx += m_groups[g]->slots.size();
            const int capturedIdx = flatIdx;

            m_stepGridSingle.onModified = [this, capturedIdx]()
            {
                if (m_focusedRow != nullptr && onPatternModified)
                    onPatternModified (capturedIdx, m_focusedRow->pattern());
            };
        }

        // Compute flat slot index (same logic as capturedIdx above)
        int flatSlot = row->getRowIndex();
        for (int g = 0; g < groupIndex; ++g)
            flatSlot += m_groups[g]->slots.size();

        m_listeners.call (&Listener::slotSelected,
                          flatSlot,
                          row->getModuleTypeName());
    }
}

void ZoneBComponent::defocusRow()
{
    if (m_focusedRow != nullptr)
    {
        m_focusedRow->setSelected (false);
        m_focusedRow = nullptr;
    }
    m_focusedGroupIdx = -1;

    resized();

    m_seqHeader.clearFocus();
    m_stepGridSingle.clearPattern();
    m_stepGridDrum.clearDrumData();
    m_stepGridSingle.setVisible (true);
    m_stepGridDrum.setVisible   (false);

    m_listeners.call (&Listener::slotDeselected);
}

void ZoneBComponent::setPlayheadStep (int step)
{
    m_stepGridSingle.setPlayhead (step);
}

void ZoneBComponent::seedPatternForSlot (int flatSlotIndex, int stepCount, const std::initializer_list<int>& activeSteps)
{
    if (flatSlotIndex < 0)
        return;

    int currentFlat = 0;
    for (int gi = 0; gi < m_groups.size(); ++gi)
    {
        auto* group = m_groups[gi];
        if (group == nullptr)
            continue;

        for (int ri = 0; ri < group->slots.size(); ++ri, ++currentFlat)
        {
            auto* row = group->slots[ri];
            if (row == nullptr || currentFlat != flatSlotIndex)
                continue;

            if (row->getModuleType() == ModuleRow::ModuleType::Empty)
                row->setModuleType (ModuleRow::ModuleType::Synth);

            auto& pattern = row->pattern();
            pattern.setStepCount (stepCount);
            pattern.clearPattern();
            for (const auto s : activeSteps)
            {
                if (s >= 0 && s < pattern.activeStepCount())
                    pattern.active[pattern.currentPattern][s] = true;
            }

            if (m_focusedRow == row)
                m_stepGridSingle.repaint();

            if (onPatternModified)
                onPatternModified (flatSlotIndex, pattern);
            row->repaint();
            return;
        }
    }
}

//==============================================================================
// Row interactions
//==============================================================================

void ZoneBComponent::onRowClicked (ModuleRow* row, int groupIndex)
{
    if (row == m_focusedRow)
        defocusRow();
    else
        focusRow (row, groupIndex);
}

void ZoneBComponent::onEmptyClicked (ModuleRow* row, int groupIndex)
{
    const auto boundsInContent = row->getBoundsInParent();
    showLoadDialog (row, boundsInContent);
    juce::ignoreUnused (groupIndex);
}

void ZoneBComponent::onRightClick (ModuleRow* row, int groupIndex)
{
    if (row == nullptr) return;

    juce::PopupMenu menu;
    menu.addItem (1, "Mute");
    if (row->getModuleType() != ModuleRow::ModuleType::Output)
        menu.addItem (2, "Clear");
    menu.addSeparator();
    menu.addItem (3, "Cancel");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (row),
        [this, row, groupIndex] (int result)
        {
            if (result == 1)
                row->setMuted (!row->isMuted());
            else if (result == 2)
            {
                if (m_focusedRow == row) defocusRow();
                row->setModuleType (ModuleRow::ModuleType::Empty);
            }
            juce::ignoreUnused (groupIndex);
        });
}

void ZoneBComponent::onRowTypeChosen (ModuleRow* row, ModuleRow::ModuleType type)
{
    if (row == nullptr) return;
    row->setModuleType (type);

    for (int gi = 0; gi < m_groups.size(); ++gi)
        for (auto* s : m_groups[gi]->slots)
            if (s == row)
                row->setGroupColor (m_groups[gi]->color);

    dismissLoadDialog();
    repaint();
}

//==============================================================================
// Load dialog
//==============================================================================

void ZoneBComponent::showLoadDialog (ModuleRow* row, juce::Rectangle<int> boundsInContent)
{
    dismissLoadDialog();

    const auto contentInSelf = m_rowsContent.getBoundsInParent()
                                .translated (-m_rowsViewport.getViewPositionX(),
                                             -m_rowsViewport.getViewPositionY())
                                .getPosition();

    const int cx = boundsInContent.getCentreX() + contentInSelf.x + m_rowsViewport.getX();
    const int cy = boundsInContent.getCentreY() + contentInSelf.y + m_rowsViewport.getY();

    m_loadDialog = std::make_unique<ModuleLoadDialog> (row->getRowIndex());
    const int dx = cx - ModuleLoadDialog::kWidth  / 2;
    const int dy = cy - ModuleLoadDialog::kHeight / 2;

    m_loadDialog->setBounds (
        juce::Rectangle<int> (dx, dy, ModuleLoadDialog::kWidth, ModuleLoadDialog::kHeight)
            .constrainedWithin (getLocalBounds()));

    // ModuleLoadDialog fires ModuleSlot::ModuleType — map to ModuleRow::ModuleType
    m_loadDialog->onTypeChosen = [this, row] (int, ModuleSlot::ModuleType slotType)
    {
        ModuleRow::ModuleType t = ModuleRow::ModuleType::Empty;
        switch (slotType)
        {
            case ModuleSlot::ModuleType::Synth:       t = ModuleRow::ModuleType::Synth;       break;
            case ModuleSlot::ModuleType::Sampler:     t = ModuleRow::ModuleType::Sampler;     break;
            case ModuleSlot::ModuleType::Sequencer:   t = ModuleRow::ModuleType::Sequencer;   break;
            case ModuleSlot::ModuleType::DrumMachine: t = ModuleRow::ModuleType::DrumMachine; break;
            case ModuleSlot::ModuleType::Vst3:        t = ModuleRow::ModuleType::Vst3;        break;
            case ModuleSlot::ModuleType::Reel:        t = ModuleRow::ModuleType::Reel;        break;
            default: break;
        }
        onRowTypeChosen (row, t);
    };
    m_loadDialog->onCancel = [this] { dismissLoadDialog(); };

    addAndMakeVisible (*m_loadDialog);
    m_loadDialog->toFront (true);
}

void ZoneBComponent::dismissLoadDialog()
{
    if (m_loadDialog != nullptr)
    {
        removeChildComponent (m_loadDialog.get());
        m_loadDialog.reset();
    }
}

//==============================================================================
// SplitHandle
//==============================================================================

void ZoneBComponent::SplitHandle::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);  // Painted by ZoneBComponent::paint()
}

void ZoneBComponent::SplitHandle::mouseDown (const juce::MouseEvent& e)
{
    juce::ignoreUnused (e);
    if (onDragStarted) onDragStarted();
}

void ZoneBComponent::SplitHandle::mouseDrag (const juce::MouseEvent& e)
{
    if (onDrag) onDrag (e.getDistanceFromDragStartY());
}

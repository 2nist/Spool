#include "ZoneBComponent.h"
#include "../ZoneA/ZoneAControlStyle.h"

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
    ZoneAControlStyle::styleTextButton (m_addGroupBtn, Theme::Zone::b.withAlpha (0.75f));
    m_addGroupBtn.setButtonText ("+  GROUP");
    m_addGroupBtn.onClick = [this]
    {
        const juce::String name  = "GROUP " + juce::String (m_groups.size() + 1);
        const juce::Colour color { 0xFF4B9EDB };
        addGroup (name, color, true);
        const int newIdx = m_groups.size() - 1;
        if (auto* hdr = m_groupHeaders[newIdx])
            hdr->startRename();
    };
    m_rowsContent.addAndMakeVisible (m_addGroupBtn);
        const int inspectorW = 220;
        const int gridW = juce::jmin (800, juce::jmax (400, getWidth() - inspectorW - 48));
        const int gridX = juce::jmax (0, m_seqHeader.getRight() - gridW - inspectorW);
        m_seqHeader.setBounds (seqHeaderY(), 0, getWidth(), kSeqHdrH);
        m_stepGridSingle.setBounds (gridX, seqHeaderY() + kSeqHdrH, gridW, m_gridH);
        m_stepGridDrum.setBounds (gridX, seqHeaderY() + kSeqHdrH, gridW, m_gridH);
        m_stepGridSingleInspector.setBounds (gridX + gridW + 8, seqHeaderY() + kSeqHdrH, inspectorW, m_gridH);
        m_looperStrip.setBounds (0, seqHeaderY() + kSeqHdrH + m_gridH, getWidth(), looperHeight());
    {
        // Materialise the visual position before starting a drag if default (-1).
        if (m_splitY < 0)
            m_splitY = juce::jmax (kStripeH + kMinRowsH,
                                   getHeight() - kHandleH - minBottomHeight());
        m_splitDragStartY = m_splitY;
    };
    m_splitHandle.onDrag = [this] (int dy)
    {
        const int minSplit = kStripeH + kMinRowsH;
        const int maxSplit = getHeight() - kHandleH - minBottomHeight();
        m_splitY = juce::jlimit (minSplit, juce::jmax (minSplit, maxSplit),
                                 m_splitDragStartY + dy);
        if (m_sequencerFocusMode)
        {
            m_sequencerFocusMode = false;
            m_seqHeader.setSequencerFocusMode (false);
        }
        resized();
    };

    // Sequencer area
    addAndMakeVisible (m_seqHeader);
    addAndMakeVisible (m_stepGridSingle);
    addAndMakeVisible (m_stepGridSingleInspector);
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
            if (! m_focusedRow->isDrumMachine() && onPatternModified)
            {
                int flatIdx = m_focusedRow->getRowIndex();
                for (int g = 0; g < m_focusedGroupIdx; ++g)
                    flatIdx += m_groups[g]->slots.size();
                onPatternModified (flatIdx, m_focusedRow->pattern());
            }
        }
    };

    m_seqHeader.onClear = [this]
    {
        if (m_focusedRow != nullptr)
        {
            m_focusedRow->pattern().clearPattern();
            m_stepGridSingle.repaint();
            m_stepGridDrum.repaint();
            if (! m_focusedRow->isDrumMachine() && onPatternModified)
            {
                int flatIdx = m_focusedRow->getRowIndex();
                for (int g = 0; g < m_focusedGroupIdx; ++g)
                    flatIdx += m_groups[g]->slots.size();
                onPatternModified (flatIdx, m_focusedRow->pattern());
            }
        }
    };

    m_seqHeader.onPatternChanged = [this] (int)
    {
        m_stepGridSingle.repaint();
        m_stepGridDrum.repaint();

        if (m_focusedRow == nullptr || m_focusedRow->isDrumMachine() || ! onPatternModified)
            return;

        int flatIdx = m_focusedRow->getRowIndex();
        for (int g = 0; g < m_focusedGroupIdx; ++g)
            flatIdx += m_groups[g]->slots.size();
        onPatternModified (flatIdx, m_focusedRow->pattern());
    };

    m_seqHeader.onToggleSequencerFocus = [this] { toggleSequencerFocusMode(); };
    m_seqHeader.setSequencerFocusMode (false);

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

bool ZoneBComponent::test_forceSlotToDrumMachine (int flatSlotIndex)
{
    auto* row = rowForFlatSlot (flatSlotIndex);
    if (row == nullptr)
        return false;
    row->setModuleType (ModuleRow::ModuleType::DrumMachine);
    return true;
}

void ZoneBComponent::setLooperVisible (bool shouldShow)
{
    if (m_showLooper == shouldShow)
        return;

    m_showLooper = shouldShow;
    m_looperStrip.setVisible (shouldShow);
    resized();
    repaint();
}

ModuleRow* ZoneBComponent::rowForFlatSlot (int flatSlotIndex, int* groupIndexOut) noexcept
{
    if (flatSlotIndex < 0)
        return nullptr;

    int currentFlat = 0;
    for (int gi = 0; gi < m_groups.size(); ++gi)
    {
        auto* group = m_groups[gi];
        if (group == nullptr)
            continue;

        for (int ri = 0; ri < group->slots.size(); ++ri, ++currentFlat)
        {
            if (currentFlat != flatSlotIndex)
                continue;

            if (groupIndexOut != nullptr)
                *groupIndexOut = gi;
            return group->slots[ri];
        }
    }

    return nullptr;
}

const ModuleRow* ZoneBComponent::rowForFlatSlot (int flatSlotIndex, int* groupIndexOut) const noexcept
{
    return const_cast<ZoneBComponent*> (this)->rowForFlatSlot (flatSlotIndex, groupIndexOut);
}

int ZoneBComponent::findGroupIndexForRow (const ModuleRow* row) const noexcept
{
    if (row == nullptr)
        return -1;

    for (int gi = 0; gi < m_groups.size(); ++gi)
    {
        auto* group = m_groups[gi];
        if (group == nullptr)
            continue;

        for (auto* groupRow : group->slots)
        {
            if (groupRow == row)
                return gi;
        }
    }

    return -1;
}

DrumMachineData* ZoneBComponent::getDrumDataForSlot (int flatSlotIndex) noexcept
{
    if (auto* row = rowForFlatSlot (flatSlotIndex))
        return row->drumData();
    return nullptr;
}

const DrumMachineData* ZoneBComponent::getDrumDataForSlot (int flatSlotIndex) const noexcept
{
    if (auto* row = rowForFlatSlot (flatSlotIndex))
        return row->drumData();
    return nullptr;
}

bool ZoneBComponent::setDrumDataForSlot (int flatSlotIndex, const DrumMachineData& data)
{
    int groupIndex = -1;
    auto* row = rowForFlatSlot (flatSlotIndex, &groupIndex);
    if (row == nullptr || !row->isDrumMachine())
        return false;

    row->setDrumData (data);

    if (m_focusedRow == row)
    {
        m_stepGridDrum.setDrumData (row->drumData(), m_groups[groupIndex]->color);
        m_stepGridDrum.repaint();
    }

    return true;
}

//==============================================================================
// Default groups
//==============================================================================

void ZoneBComponent::createDefaultGroups()
{
    // Intentionally start empty so startup behaviour reflects normal user flow.
    m_seqHeader.clearFocus();
    m_stepGridSingle.clearPattern();
    m_stepGridSingle.setVisible (true);
    m_stepGridDrum.clearDrumData();
    m_stepGridDrum.setVisible (false);
    updateSourceNames();
}

//==============================================================================
// Group / row management
//==============================================================================

void ZoneBComponent::addGroup (const juce::String& name, juce::Colour color, bool addInitialEmptySlot)
{
    auto* grp    = m_groups.add (new ModuleGroup{});
    grp->name    = name;
    grp->color   = color;
    grp->id      = juce::Uuid{}.toString();

    auto* header = m_groupHeaders.add (new GroupHeader (*grp));
    m_rowsContent.addAndMakeVisible (*header);
    const int groupIndex = m_groups.size() - 1;
    connectGroupHeader (groupIndex);

    if (addInitialEmptySlot)
        addSlotToGroup (groupIndex, ModuleRow::ModuleType::Empty);

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

    row->onRowClicked  = [this, row] (int)
    {
        const int gi = findGroupIndexForRow (row);
        if (gi >= 0)
            onRowClicked (row, gi);
    };
    row->onEmptyClicked= [this, row] (int)
    {
        const int gi = findGroupIndexForRow (row);
        if (gi >= 0)
            onEmptyClicked (row, gi);
    };
    row->onRightClicked= [this, row] (int)
    {
        const int gi = findGroupIndexForRow (row);
        if (gi >= 0)
            onRightClick (row, gi);
    };

    row->onCollapseToggled = [this] (int)
    {
        layoutRowsContent();
        resized();
    };

    // DnD drop from AudioHistoryStrip —————————————————————————————————————
    row->onClipDropped = [this, row] (int)
    {
        const int groupIndex = findGroupIndexForRow (row);
        if (groupIndex < 0)
            return;

        // If the row was converted from Empty to Reel inside itemDropped,
        // re-sync the module list and update the row layout.
        updateSourceNames();
        layoutRowsContent();

        // Focus the row so Zone A/C/D update consistently with a normal click.
        focusRow (row, groupIndex);

        // Compute flat slot index using the same formula as focusRow.
        int flatSlot = row->getRowIndex();
        for (int g = 0; g < groupIndex; ++g)
            flatSlot += m_groups[g]->slots.size();

        if (onClipDropped)
            onClipDropped (flatSlot);
    };

    // Forward fader changes to PluginEditor so they reach the DSP layer.
    // Compute the flat slot index identically to focusRow / onClipDropped.
    row->onParamChanged = [this, row] (int paramIdx, float /*norm*/)
    {
        const int groupIndex = findGroupIndexForRow (row);
        if (groupIndex < 0)
            return;

        if (!onQuickParamChanged) return;
        const auto& params = row->getAllParams();
        if (paramIdx < 0 || paramIdx >= params.size()) return;
        int flatSlot = row->getRowIndex();
        for (int g = 0; g < groupIndex; ++g)
            flatSlot += m_groups[g]->slots.size();
        onQuickParamChanged (flatSlot, params[paramIdx].label, params[paramIdx].value);
    };

    // FaceplatePanel events (SYNTH / DRUM rows — paramId-routed, replaces onParamChanged for those types)
    row->onFaceplateChanged = [this, row] (const juce::String& paramId, float value)
    {
        const int groupIndex = findGroupIndexForRow (row);
        if (groupIndex < 0)
            return;

        if (!onQuickParamChanged) return;
        int flatSlot = row->getRowIndex();
        for (int g = 0; g < groupIndex; ++g)
            flatSlot += m_groups[g]->slots.size();
        onQuickParamChanged (flatSlot, paramId, value);
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
        {
            // Rows are displayed in pairs (2 side-by-side per vertical slot)
            for (int ri = 0; ri < grp->slots.size(); ri += 2)
                h += grp->slots[ri]->currentHeight() + kRowGap;
        }
    }
    h += 22;  // ADD GROUP button + breathing room
    return h;
}

void ZoneBComponent::layoutRowsContent()
{
    const int vpW  = m_rowsViewport.getWidth();
    const int vpH  = m_rowsViewport.getHeight();

    const int contentH = juce::jmax (calculateRowsContentH(), vpH);
    m_rowsContent.setSize (vpW, contentH);

    // Half-module width: viewport minus outer padding (both sides) and centre gutter
    const int halfW = juce::jmax (200, (vpW - kOuterPad * 2 - kPairGutter) / 2);

    int y = 0;
    for (int gi = 0; gi < m_groups.size(); ++gi)
    {
        auto& grp = *m_groups[gi];
        auto* hdr = m_groupHeaders[gi];

        hdr->setBounds (0, y, vpW, GroupHeader::kHeight);
        y += GroupHeader::kHeight;

        const int rowsH      = rowsAreaH();
        const int desiredPerRow = rowsH > 0 ? juce::jmax (ModuleRow::kExpandedH, rowsH / 2)
                                            : ModuleRow::kExpandedH;

        // Process rows in pairs: left row at kOuterPad, right row at kOuterPad + halfW + kPairGutter
        for (int ri = 0; ri < grp.slots.size(); ri += 2)
        {
            ModuleRow* leftRow  = grp.slots[ri];
            ModuleRow* rightRow = (ri + 1 < grp.slots.size()) ? grp.slots[ri + 1] : nullptr;

            if (!grp.collapsed)
            {
                const int overrideH = (desiredPerRow > ModuleRow::kExpandedH) ? desiredPerRow : 0;

                leftRow->setExpandedHeightOverride (overrideH);
                const int rowH = leftRow->currentHeight();

                leftRow->setBounds (kOuterPad, y, halfW, rowH);
                leftRow->setVisible (true);

                if (rightRow != nullptr)
                {
                    rightRow->setExpandedHeightOverride (overrideH);
                    rightRow->setBounds (kOuterPad + halfW + kPairGutter, y, halfW, rowH);
                    rightRow->setVisible (true);
                }

                y += rowH + kRowGap;
            }
            else
            {
                leftRow->setVisible (false);
                if (rightRow != nullptr)
                    rightRow->setVisible (false);
            }
        }
    }

    m_addGroupBtn.setBounds (4, y, 106, 18);
}

void ZoneBComponent::broadcastModuleList()
{
    updateSourceNames();
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

void ZoneBComponent::toggleSequencerFocusMode()
{
    const int minSplit = kStripeH + kMinRowsH;
    const int maxSplit = getHeight() - kHandleH - minBottomHeight();
    const int clampedMin = juce::jlimit (minSplit, juce::jmax (minSplit, maxSplit), minSplit);

    if (! m_sequencerFocusMode)
    {
        // Capture the current split so SPLIT can restore the prior layout.
        if (m_splitY >= 0)
            m_splitBeforeSequencerFocus = m_splitY;
        else
            m_splitBeforeSequencerFocus = juce::jmax (minSplit, maxSplit);

        m_splitY = clampedMin;
        m_sequencerFocusMode = true;
    }
    else
    {
        if (m_splitBeforeSequencerFocus >= 0)
            m_splitY = juce::jlimit (minSplit, juce::jmax (minSplit, maxSplit), m_splitBeforeSequencerFocus);
        else
            m_splitY = -1;

        m_sequencerFocusMode = false;
    }

    m_seqHeader.setSequencerFocusMode (m_sequencerFocusMode);
    resized();
    repaint();
}

//==============================================================================
// resized
//==============================================================================

void ZoneBComponent::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    const int minSplit = kStripeH + kMinRowsH;
    const int maxSplit = h - kHandleH - minBottomHeight();

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

    // Dynamic grid height — fills all space between split handle and the optional looper
    const int looperH = looperHeight();
    m_gridH = juce::jmax ((int) kMinGridH, h - (splitPos + kHandleH) - kSeqHdrH - looperH);

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
    m_looperStrip.setVisible (looperH > 0);
    if (looperH > 0)
        m_looperStrip.setBounds (0, y0 + kSeqHdrH + m_gridH, w, looperH);
    else
        m_looperStrip.setBounds (0, y0 + kSeqHdrH + m_gridH, 0, 0);

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
        g.fillRect  (0, loopY, getWidth(), looperHeight());
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

    // Discard any in-flight snapshot when focus changes to a different row.
    m_hasEditSnapshot = false;

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

        m_focusedGroupColor = groupColor;
        m_seqHeader.setFocusedSlot (row->getModuleTypeName(), groupName, groupColor);
        m_seqHeader.setPattern     (&row->pattern());
        m_seqHeader.setClipboard   (&m_patternClipboard, m_hasClipboard);

        int flatSlot = row->getRowIndex();
        for (int g = 0; g < groupIndex; ++g)
            flatSlot += m_groups[g]->slots.size();

        if (row->isDrumMachine())
        {
            resized();
            m_stepGridSingle.setVisible (false);
            m_stepGridSingle.clearPattern();
            m_stepGridDrum.setVisible (true);
            m_stepGridDrum.setDrumData (row->drumData(), groupColor);

            const int capturedDrumIdx = flatSlot;

            if (onDrumSlotFocused)
                onDrumSlotFocused (capturedDrumIdx, row->drumData());

            m_stepGridDrum.onModified = [this, capturedDrumIdx]()
            {
                if (m_focusedRow != nullptr && onDrumPatternModified)
                    onDrumPatternModified (capturedDrumIdx, m_focusedRow->drumData());
            };

            // Drum gesture undo/redo: capture snapshot at gesture start, emit
            // an undo callback on gesture end. PluginEditor will push the
            // corresponding UndoableAction into the UndoManager.
            m_stepGridDrum.onBeforeEdit = [this, capturedDrumIdx]()
            {
                if (m_focusedRow != nullptr)
                {
                    m_drumEditSnapshot = *m_focusedRow->drumData();
                    m_drumEditSnapshotSlot = capturedDrumIdx;
                    m_hasDrumEditSnapshot = true;
                }
            };

            m_stepGridDrum.onGestureEnd = [this, capturedDrumIdx]()
            {
                if (m_hasDrumEditSnapshot
                    && m_drumEditSnapshotSlot == capturedDrumIdx
                    && m_focusedRow != nullptr
                    && onDrumUndoableEdit)
                {
                    onDrumUndoableEdit (capturedDrumIdx, m_drumEditSnapshot, *m_focusedRow->drumData());
                    m_hasDrumEditSnapshot = false;
                }
            };
        }
        else
        {
            resized();
            m_stepGridDrum.setVisible (false);
            m_stepGridDrum.clearDrumData();
            m_stepGridSingle.setVisible (true);
            m_stepGridSingle.setPattern (&row->pattern(), groupColor, flatSlot);

            const int capturedIdx = flatSlot;

            m_stepGridSingle.onBeforeEdit = [this, capturedIdx]()
            {
                if (m_focusedRow != nullptr)
                {
                    m_editSnapshot     = m_focusedRow->pattern();
                    m_editSnapshotSlot = capturedIdx;
                    m_hasEditSnapshot  = true;
                }
            };

            m_stepGridSingle.onGestureEnd = [this, capturedIdx]()
            {
                if (m_hasEditSnapshot
                    && m_editSnapshotSlot == capturedIdx
                    && m_focusedRow != nullptr
                    && onUndoableEdit)
                {
                    onUndoableEdit (capturedIdx, m_editSnapshot, m_focusedRow->pattern());
                    m_hasEditSnapshot = false;
                }
            };

            m_stepGridSingle.onModified = [this, capturedIdx]()
            {
                if (m_focusedRow != nullptr && onPatternModified)
                    onPatternModified (capturedIdx, m_focusedRow->pattern());
            };

                // Connect inspector to the single step grid
                m_stepGridSingleInspector.setStepGridSingle (&m_stepGridSingle);
                m_stepGridSingleInspector.setVisible (true);
        }

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
    m_stepGridDrum.setPlayhead   (step);
}

void ZoneBComponent::applyPatternForUndo (int slotIdx, const SlotPattern& snap)
{
    int groupIdx = -1;
    auto* row = rowForFlatSlot (slotIdx, &groupIdx);
    if (row == nullptr)
        return;

    // Write the snapshot directly into the row's pattern (the StepGridSingle
    // holds a pointer into this same object, so the grid sees the new data).
    row->pattern() = snap;

    if (row == m_focusedRow)
    {
        m_stepGridSingle.repaint();
        m_seqHeader.repaint();
    }

    if (onPatternModified)
        onPatternModified (slotIdx, row->pattern());
}

void ZoneBComponent::applyDrumStateForUndo (int slotIdx, const DrumMachineData& snap)
{
    int groupIdx = -1;
    auto* row = rowForFlatSlot (slotIdx, &groupIdx);
    if (row == nullptr)
        return;

    // Overwrite the row's drum data with the snapshot. The StepGridDrum points
    // into this data so repaint will reflect the new state immediately.
    row->setDrumData (snap);

    if (row == m_focusedRow)
    {
        m_stepGridDrum.repaint();
        m_seqHeader.repaint();
    }

    if (onDrumPatternModified)
        onDrumPatternModified (slotIdx, row->drumData());
}

void ZoneBComponent::setStructureContext (const juce::String& sectionName,
                                          const juce::String& positionLabel,
                                          const juce::String& keyRoot,
                                          const juce::String& keyScale,
                                          const juce::String& currentChord,
                                          const juce::String& nextChord,
                                          const juce::String& transitionIntent,
                                          bool followingStructure,
                                          bool locallyOverriding)
{
    bool effectiveLocalOverride = locallyOverriding;
    if (m_focusedRow != nullptr && ! m_focusedRow->isDrumMachine())
        effectiveLocalOverride = effectiveLocalOverride || ! m_stepGridSingle.isSelectedStepFollowingStructure();

    const bool effectiveFollowingStructure = followingStructure && ! effectiveLocalOverride;
    m_seqHeader.setStructureContext (sectionName,
                                     positionLabel,
                                     currentChord,
                                     nextChord,
                                     transitionIntent,
                                     effectiveFollowingStructure,
                                     effectiveLocalOverride);
    m_stepGridSingle.setMusicalContext (keyRoot,
                                        keyScale,
                                        currentChord,
                                        nextChord,
                                        effectiveFollowingStructure,
                                        effectiveLocalOverride);
}

void ZoneBComponent::setFocusedPatternFollowStructure (bool enabled, bool activeOnly)
{
    if (m_focusedRow == nullptr || m_focusedRow->isDrumMachine())
        return;

    auto& pattern = m_focusedRow->pattern();
    pattern.setFollowStructureForAllSteps (enabled, activeOnly);

    if (onPatternModified)
    {
        int flatSlot = m_focusedRow->getRowIndex();
        for (int g = 0; g < m_focusedGroupIdx; ++g)
            flatSlot += m_groups[g]->slots.size();
        onPatternModified (flatSlot, pattern);
    }

    m_stepGridSingle.repaint();
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
                    pattern.toggleStep (s);
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
                onRowTypeChosen (row, ModuleRow::ModuleType::Empty);
            }
            juce::ignoreUnused (groupIndex);
        });
}

void ZoneBComponent::onRowTypeChosen (ModuleRow* row, ModuleRow::ModuleType type)
{
    if (row == nullptr) return;

    const int groupIndex = findGroupIndexForRow (row);
    if (groupIndex < 0 || groupIndex >= m_groups.size())
        return;

    const auto computeFlatSlot = [this, row, groupIndex]() -> int
    {
        int flatSlot = row->getRowIndex();
        for (int g = 0; g < groupIndex; ++g)
            flatSlot += m_groups[g]->slots.size();
        return flatSlot;
    };

    const bool wasFocused = (m_focusedRow == row);
    const int flatSlot = computeFlatSlot();

    // Force a full focus refresh when changing the module type of the active row.
    if (wasFocused)
        defocusRow();

    row->setModuleType (type);
    row->setGroupColor (m_groups[groupIndex]->color);

    // Clearing a row should also clear sequencer runtime state immediately.
    if (type == ModuleRow::ModuleType::Empty)
    {
        row->pattern().clearPattern();
        if (onPatternModified)
            onPatternModified (flatSlot, row->pattern());
    }

    updateSourceNames();
    layoutRowsContent();

    dismissLoadDialog();

    if (type != ModuleRow::ModuleType::Empty)
        focusRow (row, groupIndex);

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

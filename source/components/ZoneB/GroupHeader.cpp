#include "GroupHeader.h"
#include "../ZoneA/ZoneAControlStyle.h"

//==============================================================================

GroupHeader::GroupHeader (ModuleGroup& group)
    : m_group (group)
{
    setInterceptsMouseClicks (true, true);
}

GroupHeader::~GroupHeader()
{
    if (m_selectorPtr != nullptr)
        m_selectorPtr->removeChangeListener (this);
}

//==============================================================================
// Region helpers
//==============================================================================

juce::Rectangle<int> GroupHeader::nameRect() const noexcept
{
    // Full bar minus left stripe and a small right margin for collapse indicator
    return { kStripeW + 4, 0, getWidth() - kStripeW - 20, kHeight };
}

juce::Rectangle<int> GroupHeader::collapseRect() const noexcept
{
    return { getWidth() - 16, 0, 16, kHeight };
}

//==============================================================================
// Paint
//==============================================================================

void GroupHeader::paint (juce::Graphics& g)
{
    // Slim left-edge stripe in group colour
    g.setColour (m_group.color);
    g.fillRect  (0, 0, kStripeW, kHeight);

    // Very faint tint so it's distinguishable from the slot grid bg
    g.setColour (m_group.color.withAlpha (0.08f));
    g.fillRect  (kStripeW, 0, getWidth() - kStripeW, kHeight);

    // Group name (hide if text editor is active)
    if (m_nameEditor == nullptr)
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (m_group.color.brighter (0.15f));
        g.drawText  (m_group.name.toUpperCase(), nameRect(),
                     juce::Justification::centred, true);
    }

    // Collapse indicator
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  (m_group.collapsed ? "\xe2\x96\xb6" : "\xe2\x96\xbd",
                 collapseRect(), juce::Justification::centred, false);
}

//==============================================================================
// Mouse
//==============================================================================

void GroupHeader::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Add Slot");
        menu.addItem (2, "Rename");
        menu.addItem (3, "Change Color");
        menu.addSeparator();
        menu.addItem (4, m_group.collapsed ? "Expand" : "Collapse");
        menu.addSeparator();
        menu.addItem (5, "Delete Group");

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
            [this] (int result)
            {
                switch (result)
                {
                    case 1:
                        if (onAddSlot) onAddSlot();
                        break;
                    case 2:
                        startRename();
                        break;
                    case 3:
                        openColorPicker();
                        break;
                    case 4:
                        m_group.collapsed = !m_group.collapsed;
                        if (onToggleCollapse) onToggleCollapse();
                        repaint();
                        break;
                    case 5:
                        juce::AlertWindow::showOkCancelBox (
                            juce::MessageBoxIconType::WarningIcon,
                            "Delete Group",
                            "Delete group \"" + m_group.name + "\" and all its slots?",
                            "Delete", "Cancel", nullptr,
                            juce::ModalCallbackFunction::create ([this] (int r)
                            {
                                if (r == 1 && onDelete) onDelete();
                            }));
                        break;
                    default: break;
                }
            });
        return;
    }

    // Left click = toggle collapse
    m_group.collapsed = !m_group.collapsed;
    if (onToggleCollapse) onToggleCollapse();
    repaint();
}

void GroupHeader::mouseDoubleClick (const juce::MouseEvent&)
{
    startRename();
}

//==============================================================================
// Colour picker
//==============================================================================

void GroupHeader::openColorPicker()
{
    auto* selector = new juce::ColourSelector (
        juce::ColourSelector::showColourAtTop |
        juce::ColourSelector::showSliders     |
        juce::ColourSelector::showColourspace);

    selector->setCurrentColour (m_group.color);
    selector->addChangeListener (this);
    selector->setSize (300, 280);
    m_selectorPtr = selector;

    auto* topLevel = getTopLevelComponent();
    const auto area = topLevel->getLocalArea (this, getLocalBounds());
    juce::CallOutBox::launchAsynchronously (std::unique_ptr<juce::Component> (selector),
                                             area, topLevel);
}

void GroupHeader::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (auto* sel = dynamic_cast<juce::ColourSelector*> (source))
    {
        m_group.color = sel->getCurrentColour();
        repaint();
        if (onColorChanged) onColorChanged (m_group.color);
        m_selectorPtr = nullptr;   // CallOutBox owns the lifetime; we just clear the ptr
    }
}

//==============================================================================
// Inline rename
//==============================================================================

void GroupHeader::startRename()
{
    if (m_nameEditor != nullptr) return;

    m_nameEditor = std::make_unique<juce::TextEditor>();
    m_nameEditor->setText (m_group.name);
    m_nameEditor->setFont (Theme::Font::label());
    ZoneAControlStyle::styleTextEditor (*m_nameEditor);
    m_nameEditor->setColour (juce::TextEditor::focusedOutlineColourId, m_group.color);
    m_nameEditor->setColour (juce::TextEditor::textColourId,            m_group.color);

    m_nameEditor->onReturnKey = [this] { commitRename(); };
    m_nameEditor->onEscapeKey = [this] { m_nameEditor.reset(); repaint(); };
    m_nameEditor->onFocusLost = [this] { commitRename(); };

    addAndMakeVisible (*m_nameEditor);
    // Place editor as a 20px overlay starting at the header top (extends below)
    m_nameEditor->setBounds (kStripeW + 4, 0, juce::jmax (120, getWidth() - kStripeW - 8), 20);
    m_nameEditor->grabKeyboardFocus();
    m_nameEditor->selectAll();
    repaint();
}

void GroupHeader::commitRename()
{
    if (m_nameEditor == nullptr) return;

    const juce::String newName = m_nameEditor->getText().trim();
    m_nameEditor.reset();

    if (newName.isNotEmpty())
    {
        m_group.name = newName;
        if (onNameChanged) onNameChanged (newName);
    }
    repaint();
}

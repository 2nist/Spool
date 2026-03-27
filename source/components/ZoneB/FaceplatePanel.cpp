#include "FaceplatePanel.h"
#include "ModuleRow.h"   // for paramColorForLabel

//==============================================================================
// Constructor / Destructor
//==============================================================================

FaceplatePanel::FaceplatePanel()
{
}

FaceplatePanel::~FaceplatePanel()
{
}

//==============================================================================
// setSlots
//==============================================================================

void FaceplatePanel::setSlots (const std::array<QuickControlSlot, kNumSliders>& sliders,
                                const std::array<QuickControlSlot, kNumKnobs>&   knobs,
                                const std::array<QuickControlSlot, kNumButtons>& buttons,
                                const juce::Array<QuickControlSlot>&             available,
                                const juce::String&                              label)
{
    m_sliderSlots   = sliders;
    m_knobSlots     = knobs;
    m_buttonSlots   = buttons;

    m_defaultSliders = sliders;
    m_defaultKnobs   = knobs;
    m_defaultButtons = buttons;

    m_available     = available;
    m_sectionLabel  = label.isEmpty() ? "QUICK CONTROLS" : label.toUpperCase();

    rebuildWidgets();
}

//==============================================================================
// rebuildWidgets
// Called on setSlots() and whenever an assignment changes.
//==============================================================================

void FaceplatePanel::rebuildWidgets()
{
    // ---- Remove old widgets ----
    for (auto* f : m_faderWidgets)  removeChildComponent (f);
    for (auto* k : m_knobWidgets)   removeChildComponent (k);
    for (auto* b : m_buttonWidgets) removeChildComponent (b);

    m_faderWidgets.clear();
    m_knobWidgets.clear();
    m_buttonWidgets.clear();

    // ---- Build slider widgets ----
    for (int i = 0; i < kNumSliders; ++i)
    {
        const auto& slot = m_sliderSlots[i];
        auto* fader = m_faderWidgets.add (new VerticalFader());
        applySlotToFader (i);   // configures the fader from slot metadata

        const float norm = slot.assigned && (slot.maxVal > slot.minVal)
                           ? (slot.defVal - slot.minVal) / (slot.maxVal - slot.minVal)
                           : 0.5f;
        fader->setDefaultValue (norm);
        fader->setValue        (norm, false);

        const int capturedIdx = i;
        fader->onValueChanged = [this, capturedIdx] (float n)
        {
            auto& s = m_sliderSlots[capturedIdx];
            const float val = s.minVal + n * (s.maxVal - s.minVal);
            notifyControl (s, val);
        };

        // Right-click injection — adds ASSIGN section to the existing fader menu
        fader->onBuildExtraMenuItems = [this, capturedIdx] (juce::PopupMenu& menu)
        {
            const auto& s = m_sliderSlots[capturedIdx];
            menu.addSectionHeader ("ASSIGN");
            {
                juce::PopupMenu sub;
                for (int ai = 0; ai < m_available.size(); ++ai)
                {
                    const auto& a = m_available[ai];
                    if (a.type == QuickControlSlot::ControlType::Slider
                        || a.type == QuickControlSlot::ControlType::Knob)
                    {
                        sub.addItem (kMenuBase + ai, a.label + "  [" + a.paramId + "]",
                                     true, a.paramId == s.paramId);
                    }
                }
                menu.addSubMenu ("Assign parameter\xe2\x80\xa6", sub);
            }
            menu.addItem (kMenuReset, "Reset to default");
            menu.addItem (kMenuClear, "Clear assignment");
        };

        fader->onExtraMenuItemSelected = [this, capturedIdx] (int id)
        {
            if (id >= kMenuBase && id < kMenuBase + m_available.size())
            {
                m_sliderSlots[capturedIdx] = m_available[id - kMenuBase];
                m_sliderSlots[capturedIdx].type = QuickControlSlot::ControlType::Slider;
                applySlotToFader (capturedIdx);
            }
            else if (id == kMenuReset)
            {
                m_sliderSlots[capturedIdx] = m_defaultSliders[capturedIdx];
                applySlotToFader (capturedIdx);
            }
            else if (id == kMenuClear)
            {
                m_sliderSlots[capturedIdx] = QuickControlSlot::empty (QuickControlSlot::ControlType::Slider);
                applySlotToFader (capturedIdx);
            }
        };

        addAndMakeVisible (*fader);
    }

    // ---- Build knob widgets ----
    for (int i = 0; i < kNumKnobs; ++i)
    {
        auto* knob = m_knobWidgets.add (new SlotKnob());
        knob->setSliderStyle        (juce::Slider::RotaryHorizontalVerticalDrag);
        knob->setTextBoxStyle       (juce::Slider::NoTextBox, false, 0, 0);
        knob->setDoubleClickReturnValue (true, 0.0);
        applySlotToKnob (i);

        const int capturedIdx = i;
        knob->onValueChange = [this, capturedIdx]()
        {
            const auto& s = m_knobSlots[capturedIdx];
            notifyControl (s, static_cast<float> (m_knobWidgets[capturedIdx]->getValue()));
        };

        knob->onRightClick = [this, capturedIdx]() { showKnobMenu (capturedIdx); };

        addAndMakeVisible (*knob);
    }

    // ---- Build button widgets ----
    for (int i = 0; i < kNumButtons; ++i)
    {
        auto* btn = m_buttonWidgets.add (new SlotButton());
        applySlotToButton (i);

        const int capturedIdx = i;
        btn->onClick = [this, capturedIdx]()
        {
            const auto& s = m_buttonSlots[capturedIdx];
            if (!s.assigned) return;
            const float val = m_buttonWidgets[capturedIdx]->getToggleState() ? 1.f : 0.f;
            notifyControl (s, val);
        };
        btn->onRightClick = [this, capturedIdx]() { showButtonMenu (capturedIdx); };

        addAndMakeVisible (*btn);
    }

    resized();
    repaint();
}

//==============================================================================
// applySlot helpers
// Re-configure an existing widget in-place after a slot reassignment.
//==============================================================================

void FaceplatePanel::applySlotToFader (int idx)
{
    const auto& slot  = m_sliderSlots[idx];
    auto*       fader = m_faderWidgets[idx];
    if (fader == nullptr) return;

    fader->setDisplayName (slot.assigned ? slot.label : "--");
    fader->setParamId     (slot.paramId);
    fader->setBipolar     (slot.minVal < 0.f);

    if (slot.assigned)
    {
        fader->setParamColor (ModuleRow::paramColorForLabel (slot.paramId));

        // Value formatter based on range
        const float mn = slot.minVal, mx = slot.maxVal;
        if (mx >= 100.f)
            fader->setValueFormatter ([mn, mx] (float n)
            {
                const float hz = mn + n * (mx - mn);
                return hz >= 1000.f ? juce::String (hz / 1000.f, 1) + "k"
                                    : juce::String (static_cast<int> (hz));
            });
        else if (mn < 0.f && mx <= 50.f)
            fader->setValueFormatter ([mn, mx] (float n)
            {
                const int v = juce::roundToInt (mn + n * (mx - mn));
                return (v >= 0 ? "+" : "") + juce::String (v);
            });
        else if (mn < 0.f && mx <= 1.f)
            fader->setValueFormatter ([mn, mx] (float n)
            {
                const float v = mn + n * (mx - mn);
                if (std::abs (v) < 0.01f) return juce::String ("0");
                return (v > 0.f ? "+" : "") + juce::String (v, 2);
            });
        else
        {
            const auto lbl = slot.label.toUpperCase();
            const bool isTime = (lbl == "ATK" || lbl == "DEC" || lbl == "REL"
                                 || lbl.endsWith ("DEC") || lbl.endsWith ("DEC"));
            if (isTime)
                fader->setValueFormatter ([mn, mx] (float n)
                {
                    const float s = mn + n * (mx - mn);
                    return s < 1.f ? juce::String (juce::roundToInt (s * 1000.f)) + "m"
                                   : juce::String (s, 1) + "s";
                });
            else
                fader->setValueFormatter ([mn, mx] (float n)
                {
                    return juce::String (mn + n * (mx - mn), 2);
                });
        }
    }
    else
    {
        fader->setParamColor (juce::Colour (0xFF2a2a2a));
        fader->setValueFormatter ([] (float) { return juce::String ("--"); });
    }

    fader->repaint();
}

void FaceplatePanel::applySlotToKnob (int idx)
{
    const auto& slot = m_knobSlots[idx];
    auto*       knob = m_knobWidgets[idx];
    if (knob == nullptr) return;

    if (slot.assigned)
    {
        knob->setRange (slot.minVal, slot.maxVal);
        knob->setValue (slot.defVal, juce::dontSendNotification);
        knob->setDoubleClickReturnValue (true, slot.defVal);

        const juce::Colour col = ModuleRow::paramColorForLabel (slot.paramId);
        knob->setColour (juce::Slider::rotarySliderFillColourId,    col.withAlpha (0.7f));
        knob->setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF1a1410));
        knob->setColour (juce::Slider::thumbColourId,               juce::Colour (0xFFc8a97e));
        knob->setColour (juce::Slider::backgroundColourId,          juce::Colour (0xFF0e0c09));
        knob->setEnabled (true);
        knob->setAlpha   (1.0f);
    }
    else
    {
        knob->setRange (0.f, 1.f);
        knob->setValue (0.5, juce::dontSendNotification);
        knob->setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (0xFF222222));
        knob->setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF1a1410));
        knob->setAlpha   (0.4f);
    }

    repaint();
}

void FaceplatePanel::applySlotToButton (int idx)
{
    const auto& slot = m_buttonSlots[idx];
    auto*       btn  = m_buttonWidgets[idx];
    if (btn == nullptr) return;

    btn->setButtonText (slot.assigned ? slot.label : "--");
    btn->isMomentary = (slot.behavior == QuickControlSlot::Behavior::Momentary);

    const bool isToggle = (slot.behavior == QuickControlSlot::Behavior::Toggle
                           || slot.behavior == QuickControlSlot::Behavior::EnumStep);
    btn->setClickingTogglesState (isToggle);
    btn->setToggleState (false, juce::dontSendNotification);

    if (slot.assigned)
    {
        btn->setColour (juce::TextButton::buttonColourId,   juce::Colour (0xFF1a1610));
        btn->setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xFF3a2e1e));
        btn->setColour (juce::TextButton::textColourOffId,  juce::Colour (0xFFa09080));
        btn->setColour (juce::TextButton::textColourOnId,   juce::Colour (0xFFe8d4a0));
        btn->setAlpha  (1.0f);
    }
    else
    {
        btn->setColour (juce::TextButton::buttonColourId,   juce::Colour (0xFF141210));
        btn->setColour (juce::TextButton::textColourOffId,  juce::Colour (0xFF3a3028));
        btn->setAlpha  (0.5f);
    }

    repaint();
}

//==============================================================================
// Context menus
//==============================================================================

void FaceplatePanel::showKnobMenu (int slotIdx)
{
    showAssignMenu (
        m_knobSlots[slotIdx],
        [this, slotIdx] (const QuickControlSlot& chosen)
        {
            m_knobSlots[slotIdx] = chosen;
            m_knobSlots[slotIdx].type = QuickControlSlot::ControlType::Knob;
            applySlotToKnob (slotIdx);
        },
        [this, slotIdx]() { m_knobSlots[slotIdx] = m_defaultKnobs[slotIdx]; applySlotToKnob (slotIdx); },
        [this, slotIdx]() { m_knobSlots[slotIdx] = QuickControlSlot::empty (QuickControlSlot::ControlType::Knob); applySlotToKnob (slotIdx); }
    );
}

void FaceplatePanel::showButtonMenu (int slotIdx)
{
    showAssignMenu (
        m_buttonSlots[slotIdx],
        [this, slotIdx] (const QuickControlSlot& chosen)
        {
            m_buttonSlots[slotIdx] = chosen;
            m_buttonSlots[slotIdx].type = QuickControlSlot::ControlType::Button;
            applySlotToButton (slotIdx);
        },
        [this, slotIdx]() { m_buttonSlots[slotIdx] = m_defaultButtons[slotIdx]; applySlotToButton (slotIdx); },
        [this, slotIdx]() { m_buttonSlots[slotIdx] = QuickControlSlot::empty (QuickControlSlot::ControlType::Button); applySlotToButton (slotIdx); }
    );
}

void FaceplatePanel::showAssignMenu (QuickControlSlot& /*slot*/,
                                     std::function<void(const QuickControlSlot&)> onAssigned,
                                     std::function<void()>                        onReset,
                                     std::function<void()>                        onClear)
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("ASSIGN");

    {
        juce::PopupMenu sub;
        for (int i = 0; i < m_available.size(); ++i)
            sub.addItem (kMenuBase + i, m_available[i].label + "  [" + m_available[i].paramId + "]");
        menu.addSubMenu ("Assign parameter\xe2\x80\xa6", sub);
    }

    menu.addItem (kMenuReset, "Reset to default");
    menu.addItem (kMenuClear, "Clear assignment");

    // Capture copies for the async callback (slot reference would dangle)
    auto avail    = m_available;
    auto assigned = onAssigned;
    auto reset    = onReset;
    auto clear    = onClear;

    menu.showMenuAsync (
        juce::PopupMenu::Options().withTargetComponent (this),
        [avail, assigned, reset, clear] (int result) mutable
        {
            if (result >= kMenuBase && result < kMenuBase + avail.size())
                assigned (avail[result - kMenuBase]);
            else if (result == kMenuReset)
                reset();
            else if (result == kMenuClear)
                clear();
        });
}

//==============================================================================
// Paint
//==============================================================================

void FaceplatePanel::paint (juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    // Background
    g.setColour (Theme::Colour::surface0);
    g.fillRect  (getLocalBounds());

    // Left stripe continuation (matches ModuleRow's kStripeW = 3px)
    g.setColour (m_groupColor.withAlpha (0.4f));
    g.fillRect  (0, 0, kStripeW, h);

    // Section label strip
    g.setColour (Theme::Colour::surface2);
    g.fillRect  (kStripeW, 0, w - kStripeW, kSectionH);
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    g.drawText  (m_sectionLabel,
                 kStripeW + 6, 0, w - kStripeW - 8, kSectionH,
                 juce::Justification::centredLeft, false);

    // Bar background
    g.setColour (Theme::Colour::surface1);
    g.fillRect  (kStripeW, kBarY, w - kStripeW, kBarH);

    // Separator above bar
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect  (kStripeW, kBarY, w - kStripeW, 1);

    // Knob labels (painted above each knob; knobs themselves are child components)
    g.setFont   (Theme::Font::micro());
    for (int i = 0; i < kNumKnobs; ++i)
    {
        auto* knob = m_knobWidgets[i];
        if (knob == nullptr) continue;
        const auto& slot = m_knobSlots[i];

        const juce::Rectangle<int> labelBounds {
            knob->getX() - 2, kBarY + 1,
            kKnobSize + 4, kBarLabelH - 1
        };
        g.setColour (slot.assigned ? Theme::Colour::inkGhost
                                   : Theme::Colour::inkGhost.withAlpha (0.3f));
        g.drawText  (slot.label, labelBounds, juce::Justification::centred, false);
    }

    // Button column background + separator (buttons are alongside the sliders, not in the bar)
    {
        const int btnColX = getWidth() - kBtnColW - kBtnColMgn;

        // Subtle column tint so buttons read as a distinct section
        g.setColour (juce::Colour (0xFF0f0d0b));
        g.fillRect  (btnColX, kSectionH, kBtnColW + kBtnColMgn, kFaderH);

        // Thin separator line between fader area and button column
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.4f));
        g.fillRect  (btnColX - 1, kSectionH + 2, 1, kFaderH - 4);
    }
}

//==============================================================================
// Resized
//==============================================================================

void FaceplatePanel::resized()
{
    if (m_faderWidgets.isEmpty()) return;

    const int w = getWidth();

    // ---- Button column: compact vertical strip anchored to the right ----
    {
        const int btnColX = w - kBtnColW - kBtnColMgn;
        const int btnH    = (kFaderH - (kNumButtons - 1) * kBtnVGap) / kNumButtons;

        for (int i = 0; i < kNumButtons; ++i)
        {
            if (m_buttonWidgets[i] == nullptr) continue;
            const int by = kSectionH + i * (btnH + kBtnVGap);
            m_buttonWidgets[i]->setBounds (btnColX, by, kBtnColW, btnH);
        }
    }

    // ---- Slider row: fills left area up to button column, gap computed dynamically ----
    {
        const int btnColX     = w - kBtnColW - kBtnColMgn;
        const int sliderLeft  = kStripeW + 2;
        const int sliderRight = btnColX - 4;
        const int n           = m_faderWidgets.size();

        if (n > 0)
        {
            const int prefW   = m_faderWidgets[0]->getPreferredWidth();
            const int totalFW = n * prefW;
            const int dynGap  = juce::jmax (2, (sliderRight - sliderLeft - totalFW) / juce::jmax (1, n - 1));

            int x = sliderLeft;
            for (auto* f : m_faderWidgets)
            {
                f->setBounds (x, kSectionH, prefW, VerticalFader::kPreferredHeight);
                x += prefW + dynGap;
            }
        }
    }

    // ---- Bottom bar: 4 knobs in a single centred row ----
    {
        const int knobGap     = 8;
        const int totalKnobsW = kNumKnobs * kKnobSize + (kNumKnobs - 1) * knobGap;
        const int knobsX0     = kStripeW + juce::jmax (0, (w - kStripeW - totalKnobsW) / 2);

        for (int i = 0; i < kNumKnobs; ++i)
        {
            if (m_knobWidgets[i] == nullptr) continue;
            const int kx = knobsX0 + i * (kKnobSize + knobGap);
            const int ky = kBarY + kBarLabelH;
            m_knobWidgets[i]->setBounds (kx, ky, kKnobSize, kKnobSize);
        }
    }
}

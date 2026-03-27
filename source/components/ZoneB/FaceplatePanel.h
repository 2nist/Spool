#pragma once

#include "QuickControlSlot.h"
#include "VerticalFader.h"

//==============================================================================
/**
    FaceplatePanel — Zone B module quick-control surface.

    Renders a family-specific faceplate with:
      8 assignable slider slots  (VerticalFader, left column)
      4 assignable knob slots    (compact rotary juce::Slider, single row in bar)
      4 assignable button slots  (juce::TextButton, compact vertical column on the right)

    All slots can be reassigned at runtime via right-click context menus.
    Default assignments per family are provided by QuickFaceplateDefaults.

    Layout (component-local y, component fills ModuleRow::paramAreaRect()):
      y = 0..kSectionH               : section label strip
      y = kSectionH..kBarY-kBarSep   : 8 vertical faders (left) | 4 buttons (right column)
      y = kBarY..kBarY+kBarH         : 4 knobs in a single centred row
*/
class FaceplatePanel : public juce::Component
{
public:
    static constexpr int kNumSliders = QuickFaceplateDefaults::kNumSliders;   // 8
    static constexpr int kNumKnobs   = QuickFaceplateDefaults::kNumKnobs;     // 2
    static constexpr int kNumButtons = QuickFaceplateDefaults::kNumButtons;   // 4

    static constexpr int kSectionH   = 10;
    static constexpr int kFaderH     = VerticalFader::kPreferredHeight;       // 90
    static constexpr int kFaderGap   = 8;
    static constexpr int kBarSep     = 6;
    static constexpr int kBarY       = kSectionH + kFaderH + kBarSep;        // 106
    static constexpr int kBarLabelH  = 10;
    static constexpr int kKnobSize   = 36;
    static constexpr int kButtonH    = 14;
    static constexpr int kBarH       = kBarLabelH + kKnobSize + 4;           // 50

    //==========================================================================
    FaceplatePanel();
    ~FaceplatePanel() override;

    //==========================================================================
    /** Load a full set of default assignments.
        @param available  All params the user can reassign any slot to (for right-click menus).
        @param label      Short section header text (e.g. "SYNTH" or "DRUMS"). */
    void setSlots (const std::array<QuickControlSlot, kNumSliders>& sliders,
                   const std::array<QuickControlSlot, kNumKnobs>&   knobs,
                   const std::array<QuickControlSlot, kNumButtons>& buttons,
                   const juce::Array<QuickControlSlot>&             available,
                   const juce::String&                              label = {});

    /** Pass the parent row's group colour so the left stripe is continuous. */
    void setGroupColor (juce::Colour c) { m_groupColor = c; repaint(); }

    //==========================================================================
    /** Fired for any control change.
        paramId = slot.paramId (routing key, NOT necessarily the same as the label).
        value   = actual value in slot {min..max} range.
        For toggle/momentary buttons, value is 0.0 or 1.0. */
    std::function<void (const juce::String& paramId, float value)> onControlChanged;

    //==========================================================================
    void paint   (juce::Graphics&) override;
    void resized () override;

    //==========================================================================
    // Right-click-aware inner widget types (definition is in FaceplatePanel.cpp).
    // Declared here so OwnedArray can know the complete layout at destructor time.

    struct SlotKnob : public juce::Slider
    {
        std::function<void()> onRightClick;
        void mouseDown (const juce::MouseEvent& e) override
        {
            if (e.mods.isRightButtonDown()) { if (onRightClick) onRightClick(); }
            else                              juce::Slider::mouseDown (e);
        }
    };

    struct SlotButton : public juce::TextButton
    {
        std::function<void()> onRightClick;
        bool                  isMomentary { false };

        void mouseDown (const juce::MouseEvent& e) override
        {
            if (e.mods.isRightButtonDown()) { if (onRightClick) onRightClick(); }
            else                              juce::TextButton::mouseDown (e);
        }
        void mouseUp (const juce::MouseEvent& e) override
        {
            juce::TextButton::mouseUp (e);
            if (isMomentary) setToggleState (false, juce::dontSendNotification);
        }
    };

private:
    //==========================================================================
    std::array<QuickControlSlot, kNumSliders> m_sliderSlots;
    std::array<QuickControlSlot, kNumKnobs>   m_knobSlots;
    std::array<QuickControlSlot, kNumButtons> m_buttonSlots;

    // Defaults preserved for "Reset to default" action
    std::array<QuickControlSlot, kNumSliders> m_defaultSliders;
    std::array<QuickControlSlot, kNumKnobs>   m_defaultKnobs;
    std::array<QuickControlSlot, kNumButtons> m_defaultButtons;

    juce::Array<QuickControlSlot>  m_available;
    juce::String                   m_sectionLabel;
    juce::Colour                   m_groupColor { 0xFF4B9EDB };

    //==========================================================================
    juce::OwnedArray<VerticalFader> m_faderWidgets;
    juce::OwnedArray<SlotKnob>      m_knobWidgets;
    juce::OwnedArray<SlotButton>    m_buttonWidgets;

    //==========================================================================
    static constexpr int kStripeW    = 3;   // matches ModuleRow left stripe width
    static constexpr int kBtnColW    = 48;  // button column width (touch-safe)
    static constexpr int kBtnColMgn  = 4;   // gap between faders and button column
    static constexpr int kBtnVGap    = 4;   // vertical gap between stacked buttons
    static constexpr int kMenuBase   = 300; // first ID for extra context-menu items
    static constexpr int kMenuReset  = kMenuBase - 2;   // 298
    static constexpr int kMenuClear  = kMenuBase - 1;   // 299

    //==========================================================================
    void rebuildWidgets();

    void applySlotToFader  (int idx);
    void applySlotToKnob   (int idx);
    void applySlotToButton (int idx);

    void showFaderMenu  (int slotIdx);
    void showKnobMenu   (int slotIdx);
    void showButtonMenu (int slotIdx);

    void showAssignMenu (QuickControlSlot&  slot,
                         std::function<void(const QuickControlSlot&)> onAssigned,
                         std::function<void()>                        onReset,
                         std::function<void()>                        onClear);

    void notifyControl (const QuickControlSlot& slot, float value)
    {
        if (onControlChanged && slot.assigned)
            onControlChanged (slot.paramId, value);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FaceplatePanel)
};

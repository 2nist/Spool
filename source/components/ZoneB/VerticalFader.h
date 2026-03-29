#pragma once

#include "../../Theme.h"

class VerticalFader : public juce::Component
{
public:
    VerticalFader();
    ~VerticalFader() override;

    void setParamId      (const juce::String& id)   { m_paramId = id; }
    void setDisplayName  (const juce::String& name);
    void setParamColor   (juce::Colour c);
    void setDefaultValue (float normalizedDefault);
    void setBipolar      (bool bipolar)              { m_bipolar = bipolar; }

    void  setValue               (float normalized, bool notify = true);
    float getValue               () const noexcept { return m_value; }
    void  setValueFromProcessor  (float normalized);

    using FormatFn = std::function<juce::String (float)>;
    using ParseFn  = std::function<float (const juce::String&, float)>;
    void setValueFormatter (FormatFn fn) { m_formatter = std::move (fn); updateTooltip(); }
    void setValueParser    (ParseFn fn)  { m_valueParser = std::move (fn); }

    enum class Status
    {
        none,
        macroMapped,
        midiLearn,
        midiAssigned,
        modulated,
        automated,
        follower,
        bypassed
    };

    void setStatus      (Status s);
    void setStatusLabel (const juce::String& detail) { m_statusDetail = detail; }

    std::function<void (float)> onValueChanged;

    static constexpr int kExtraMenuBase = 300;

    std::function<void (juce::PopupMenu&)> onBuildExtraMenuItems;
    std::function<void (int)>              onExtraMenuItemSelected;

    int getPreferredWidth () const;

    static constexpr int kPreferredHeight = 90;
    static constexpr int kValueChipH = 14;

    void paint   (juce::Graphics& g) override;
    void resized () override;

private:
    juce::String m_paramId;
    juce::String m_displayName;
    juce::Colour m_paramColor   { 0xFF4a9eff };
    float        m_defaultValue { 0.5f };
    bool         m_bipolar      { false };
    FormatFn     m_formatter;
    float m_value        { 0.5f };
    Status       m_status      { Status::none };
    juce::String m_statusDetail;
    bool m_internalSetValue { false };
    std::unique_ptr<juce::TextEditor> m_textEditor;
    ParseFn m_valueParser;

    struct SlotSlider : public juce::Slider
    {
        explicit SlotSlider (VerticalFader& ownerIn) : owner (ownerIn) {}
        void mouseDown (const juce::MouseEvent& e) override;
        VerticalFader& owner;
    };

    SlotSlider m_slider { *this };

    void handleMenuResult (int result);
    void showContextMenu ();
    void showValueEditor ();
    void notifyValueChanged ();
    void updateTooltip ();
    juce::String formatValue (float norm) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VerticalFader)
};

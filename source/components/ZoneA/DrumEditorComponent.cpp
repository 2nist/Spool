#include "DrumEditorComponent.h"

//==============================================================================
DrumEditorComponent::DrumEditorComponent()
{
    setupControls();
}

DrumEditorComponent::~DrumEditorComponent()
{
    for (auto* slider : { &m_tonePitch, &m_toneSweep, &m_tonePtDecay, &m_toneAttack, &m_toneDecay, &m_toneLevel, &m_toneBody,
                          &m_noiseDecay, &m_noiseLevel, &m_noiseHP, &m_noiseLP,
                          &m_clickLevel, &m_clickDecay,
                          &m_outLevel, &m_outPan, &m_outTune,
                          &m_charDrive, &m_charGrit, &m_charSnap })
        slider->setLookAndFeel (nullptr);
}

//==============================================================================
void DrumEditorComponent::modifyVoice (std::function<void(DrumVoiceParams&)> fn)
{
    auto* voice = selectedVoice();
    if (voice == nullptr)
        return;

    fn (m_localParams);
    voice->params = m_localParams;
    voice->params.midiNote = voice->midiNote;

    if (onStateChanged)
        onStateChanged();
}

//==============================================================================
void DrumEditorComponent::setupControls()
{
    ZoneAControlStyle::styleTextButton (m_trigBtn, Theme::Colour::accentWarm);
    //----------------------------------------------------------------------
    // Trigger button
    m_trigBtn.onClick = [this] {
        if (selectedVoice() != nullptr && onPreviewVoice)
            onPreviewVoice (m_selectedVoice, 1.0f);
    };
    addAndMakeVisible (m_trigBtn);

    //----------------------------------------------------------------------
    // Tone enable
    m_toneEnable.setClickingTogglesState (true);
    m_toneEnable.setToggleState (true, juce::dontSendNotification);
    ZoneAControlStyle::styleTextButton (m_toneEnable, juce::Colour (0xFFe8a840));
    m_toneEnable.onClick = [this] {
        const bool on = m_toneEnable.getToggleState();
        modifyVoice ([on](DrumVoiceParams& p) { p.tone.enable = on; });
    };
    addAndMakeVisible (m_toneEnable);

    initSlider (m_tonePitch,   20.0, 600.0,  100.0,  55.0, [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.tone.pitch = static_cast<float> (v); });
    });
    initSlider (m_toneSweep,   1.0,  8.0,    3.0,    3.5,  [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.tone.pitchstart = static_cast<float> (v); });
    });
    initSlider (m_tonePtDecay, 0.001, 0.5,   0.05,   0.06, [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.tone.pitchdecay = static_cast<float> (v); });
    });
    initSlider (m_toneAttack,  0.0,   0.08,  0.01,   0.0,  [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.tone.attack = static_cast<float> (v); });
    });
    initSlider (m_toneDecay,   0.01, 4.0,    0.3,    0.6,  [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.tone.decay = static_cast<float> (v); });
    });
    initSlider (m_toneLevel,   0.0,  1.0,    0.5,    1.0,  [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.tone.level = static_cast<float> (v); });
    });
    initSlider (m_toneBody,    0.0,  1.0,    0.5,    0.5,  [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.tone.body = static_cast<float> (v); });
    });

    // Tone wave toggle (SIN / TRI)
    m_toneWave.setClickingTogglesState (true);
    ZoneAControlStyle::styleTextButton (m_toneWave, juce::Colour (0xFFe8a840));
    m_toneWave.onClick = [this] {
        const bool tri = m_toneWave.getToggleState();
        m_toneWave.setButtonText (tri ? "TRI" : "SIN");
        modifyVoice ([tri](DrumVoiceParams& p) { p.tone.triangleWave = tri; });
    };
    addAndMakeVisible (m_toneWave);

    //----------------------------------------------------------------------
    // Noise enable
    m_noiseEnable.setClickingTogglesState (true);
    m_noiseEnable.setToggleState (true, juce::dontSendNotification);
    ZoneAControlStyle::styleTextButton (m_noiseEnable, juce::Colour (0xFF88aaff));
    m_noiseEnable.onClick = [this] {
        const bool on = m_noiseEnable.getToggleState();
        modifyVoice ([on](DrumVoiceParams& p) { p.noise.enable = on; });
    };
    addAndMakeVisible (m_noiseEnable);

    initSlider (m_noiseDecay, 0.001, 2.0,    0.1,    0.02,  [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.noise.decay = static_cast<float> (v); });
    });
    initSlider (m_noiseLevel, 0.0,   1.0,    0.5,    0.15,  [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.noise.level = static_cast<float> (v); });
    });
    initSlider (m_noiseHP,    20.0,  8000.0, 500.0,  80.0,  [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.noise.hpfreq = static_cast<float> (v); });
    });
    initSlider (m_noiseLP,    200.0, 20000.0, 4000.0, 800.0, [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.noise.lpfreq = static_cast<float> (v); });
    });

    //----------------------------------------------------------------------
    // Click enable
    m_clickEnable.setClickingTogglesState (true);
    m_clickEnable.setToggleState (true, juce::dontSendNotification);
    ZoneAControlStyle::styleTextButton (m_clickEnable, juce::Colour (0xFFff6688));
    m_clickEnable.onClick = [this] {
        const bool on = m_clickEnable.getToggleState();
        modifyVoice ([on](DrumVoiceParams& p) { p.click.enable = on; });
    };
    addAndMakeVisible (m_clickEnable);

    initSlider (m_clickLevel, 0.0,   1.0,    0.5,   0.6,   [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.click.level = static_cast<float> (v); });
    });
    initSlider (m_clickDecay, 0.001, 0.05,   0.01,  0.003, [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.click.decay = static_cast<float> (v); });
    });

    //----------------------------------------------------------------------
    // Output
    initSlider (m_outLevel, 0.0,  2.0,  1.0,  1.0, [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.out.level = static_cast<float> (v); });
    });
    initSlider (m_outPan,   -1.0, 1.0,  0.0,  0.0, [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.out.pan = static_cast<float> (v); });
    });
    initSlider (m_outTune,  -24.0, 24.0, 0.0, 0.0, [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.out.tune = static_cast<float> (v); });
    });

    //----------------------------------------------------------------------
    // Character
    initSlider (m_charDrive, 0.0, 1.0, 0.35, 0.2, [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.character.drive = static_cast<float> (v); });
    });
    initSlider (m_charGrit, 0.0, 1.0, 0.35, 0.1, [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.character.grit = static_cast<float> (v); });
    });
    initSlider (m_charSnap, 0.0, 1.0, 0.5, 0.2, [this](double v) {
        modifyVoice ([v](DrumVoiceParams& p) { p.character.snap = static_cast<float> (v); });
    });

    for (auto* slider : { &m_tonePitch, &m_toneSweep, &m_tonePtDecay, &m_toneAttack, &m_toneDecay, &m_toneLevel, &m_toneBody,
                          &m_charDrive, &m_charGrit, &m_charSnap })
        ZoneAControlStyle::tintBarSlider (*slider, juce::Colour (0xFFe8a840));

    for (auto* slider : { &m_noiseDecay, &m_noiseLevel, &m_noiseHP, &m_noiseLP })
        ZoneAControlStyle::tintBarSlider (*slider, juce::Colour (0xFF88aaff));

    for (auto* slider : { &m_clickLevel, &m_clickDecay })
        ZoneAControlStyle::tintBarSlider (*slider, juce::Colour (0xFFff6688));

    for (auto* slider : { &m_outLevel, &m_outPan, &m_outTune })
        ZoneAControlStyle::tintBarSlider (*slider, Theme::Colour::error);

    for (auto* button : { &m_trigBtn, &m_toneEnable, &m_toneWave, &m_noiseEnable, &m_clickEnable })
        button->setTriggeredOnMouseDown (false);
}

//==============================================================================
void DrumEditorComponent::initSlider (juce::Slider& s,
                                      double min, double max,
                                      double mid, double def,
                                      std::function<void(double)> fn)
{
    ZoneAControlStyle::initBarSlider (s, {});
    s.setRange (min, max);
    s.setSkewFactorFromMidPoint (mid);
    s.setValue (def, juce::dontSendNotification);
    s.onValueChange = [fn = std::move (fn), &s] { fn (s.getValue()); };
    addAndMakeVisible (s);
}

//==============================================================================
void DrumEditorComponent::setState (DrumModuleState* state)
{
    m_state         = state;
    m_selectedVoice = 0;
    if (m_state != nullptr)
        m_selectedVoice = m_state->focusedVoiceIndex;
    rebuildVoiceTabs();
    readFromState();
    repaint();
}

//==============================================================================
void DrumEditorComponent::selectVoice (int voiceIndex)
{
    if (m_state == nullptr || voiceIndex < 0 || voiceIndex >= static_cast<int> (m_state->voices.size()))
        return;

    m_selectedVoice = voiceIndex;
    m_state->focusedVoiceIndex = voiceIndex;

    for (int i = 0; i < m_voiceTabs.size(); ++i)
        if (auto* tab = m_voiceTabs[i])
            tab->setToggleState (i == m_selectedVoice, juce::dontSendNotification);

    readFromState();
}

//==============================================================================
DrumVoiceState* DrumEditorComponent::selectedVoice() noexcept
{
    if (m_state == nullptr || m_selectedVoice < 0 || m_selectedVoice >= static_cast<int> (m_state->voices.size()))
        return nullptr;
    return &m_state->voices[static_cast<size_t> (m_selectedVoice)];
}

const DrumVoiceState* DrumEditorComponent::selectedVoice() const noexcept
{
    return const_cast<DrumEditorComponent*> (this)->selectedVoice();
}

void DrumEditorComponent::rebuildVoiceTabs()
{
    for (auto* tab : m_voiceTabs)
        removeChildComponent (tab);
    m_voiceTabs.clear();

    if (m_state == nullptr)
        return;

    for (int i = 0; i < static_cast<int> (m_state->voices.size()); ++i)
    {
        auto* tab = m_voiceTabs.add (new juce::TextButton());
        tab->setClickingTogglesState (false);
        ZoneAControlStyle::styleTextButton (*tab, Theme::Zone::a);
        tab->onClick = [this, i] { selectVoice (i); };
        addAndMakeVisible (tab);
    }
}

//==============================================================================
void DrumEditorComponent::readFromState()
{
    for (int i = 0; i < m_voiceTabs.size(); ++i)
    {
        if (auto* tab = m_voiceTabs[i])
        {
            const auto& voice = m_state->voices[static_cast<size_t> (i)];
            tab->setButtonText (juce::String (voice.name).toUpperCase().substring (0, 4));
            tab->setToggleState (i == m_selectedVoice, juce::dontSendNotification);
        }
    }

    const auto* voice = selectedVoice();
    if (voice == nullptr)
        return;

    // Snapshot the current voice into the local mirror
    m_localParams = voice->params;

    const auto& p      = m_localParams;
    const auto  dnSend = juce::dontSendNotification;

    // Tone
    m_toneEnable .setToggleState (p.tone.enable,                     dnSend);
    m_tonePitch  .setValue (static_cast<double> (p.tone.pitch),      dnSend);
    m_toneSweep  .setValue (static_cast<double> (p.tone.pitchstart), dnSend);
    m_tonePtDecay.setValue (static_cast<double> (p.tone.pitchdecay), dnSend);
    m_toneAttack .setValue (static_cast<double> (p.tone.attack),     dnSend);
    m_toneDecay  .setValue (static_cast<double> (p.tone.decay),      dnSend);
    m_toneLevel  .setValue (static_cast<double> (p.tone.level),      dnSend);
    m_toneBody   .setValue (static_cast<double> (p.tone.body),       dnSend);
    m_toneWave   .setToggleState (p.tone.triangleWave,               dnSend);
    m_toneWave   .setButtonText  (p.tone.triangleWave ? "TRI" : "SIN");

    // Noise
    m_noiseEnable.setToggleState (p.noise.enable,                    dnSend);
    m_noiseDecay .setValue (static_cast<double> (p.noise.decay),     dnSend);
    m_noiseLevel .setValue (static_cast<double> (p.noise.level),     dnSend);
    m_noiseHP    .setValue (static_cast<double> (p.noise.hpfreq),    dnSend);
    m_noiseLP    .setValue (static_cast<double> (p.noise.lpfreq),    dnSend);

    // Click
    m_clickEnable.setToggleState (p.click.enable,                    dnSend);
    m_clickLevel .setValue (static_cast<double> (p.click.level),     dnSend);
    m_clickDecay .setValue (static_cast<double> (p.click.decay),     dnSend);

    // Output
    m_outLevel.setValue (static_cast<double> (p.out.level), dnSend);
    m_outPan  .setValue (static_cast<double> (p.out.pan),   dnSend);
    m_outTune .setValue (static_cast<double> (p.out.tune),  dnSend);

    // Character
    m_charDrive.setValue (static_cast<double> (p.character.drive), dnSend);
    m_charGrit .setValue (static_cast<double> (p.character.grit),  dnSend);
    m_charSnap .setValue (static_cast<double> (p.character.snap),  dnSend);

    m_tonePitch.setName ("PITCH");
    m_toneSweep.setName ("SWEEP");
    m_tonePtDecay.setName ("PTDCY");
    m_toneAttack.setName ("ATTACK");
    m_toneDecay.setName ("DECAY");
    m_toneLevel.setName ("LEVEL");
    m_toneBody.setName ("BODY");
    m_noiseDecay.setName ("DECAY");
    m_noiseLevel.setName ("LEVEL");
    m_noiseHP.setName ("HP");
    m_noiseLP.setName ("LP");
    m_clickLevel.setName ("LEVEL");
    m_clickDecay.setName ("DECAY");
    m_outLevel.setName ("LEVEL");
    m_outPan.setName ("PAN");
    m_outTune.setName ("TUNE");
    m_charDrive.setName ("DRIVE");
    m_charGrit.setName ("GRIT");
    m_charSnap.setName ("SNAP");
}

//==============================================================================
void DrumEditorComponent::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);

    static constexpr int kSecH = ZoneAControlStyle::kSectionHeaderH;

    for (const auto& sec : m_sections)
    {
        g.setColour (Theme::Colour::surface1);
        g.fillRect  (0, sec.y, getWidth(), kSecH);
        g.setColour (sec.colour.withAlpha (0.8f));
        g.fillRect  (0, sec.y, 3, kSecH);
        g.setFont   (Theme::Font::micro());
        g.setColour (sec.colour);
        g.drawText  (sec.name, 8, sec.y, getWidth() - 70, kSecH,
                     juce::Justification::centredLeft, false);
    }

    if (m_state == nullptr)
    {
        g.setColour (Theme::Zone::bgA.withAlpha (0.65f));
        g.fillAll();
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  ("No slot bound", getLocalBounds(), juce::Justification::centred, false);
    }
}

//==============================================================================
void DrumEditorComponent::resized()
{
    m_sections.clear();
    m_labels.clear();

    const int w      = getWidth();
    const int kSecH  = ZoneAControlStyle::kSectionHeaderH;
    const int kRowH  = 20;
    const int kPad   = ZoneAControlStyle::kPad;
    const int kCtrlX = kPad;
    const int kCtrlW = w - kPad * 2;

    int y = kPad;

    auto fullCtrl = [&] (int rowY) -> juce::Rectangle<int>
    {
        return { kCtrlX, rowY + 2, kCtrlW, kRowH - 4 };
    };

    auto dualCtrlL = [&] (int rowY) -> juce::Rectangle<int>
    {
        const int hw = (kCtrlW - 2) / 2;
        return { kCtrlX, rowY + 2, hw, kRowH - 4 };
    };

    auto dualCtrlR = [&] (int rowY) -> juce::Rectangle<int>
    {
        const int hw = (kCtrlW - 2) / 2;
        return { kCtrlX + hw + 2, rowY + 2, kCtrlW - hw - 2, kRowH - 4 };
    };

    //----------------------------------------------------------------------
    // Voice tabs + trigger
    const int numTabs = juce::jmax (1, m_voiceTabs.size());
    const int kTabW = juce::jmax (36, juce::jmin (72, (w - kPad * 2 - 48 - (numTabs - 1) * 2) / numTabs));
    int tabX = kPad;
    for (int i = 0; i < m_voiceTabs.size(); ++i)
    {
        if (auto* tab = m_voiceTabs[i])
        {
            tab->setBounds (tabX, y, kTabW, 24);
            tabX += kTabW + 2;
        }
    }
    m_trigBtn.setBounds (w - 48 - kPad, y, 48, 24);
    y += 24 + kPad;

    //----------------------------------------------------------------------
    //----------------------------------------------------------------------
    // CHARACTER
    m_sections.add ({ "CHARACTER", juce::Colour (0xFFffaa44), y });
    y += kSecH;

    m_charDrive.setBounds (fullCtrl (y)); y += kRowH;
    m_charGrit.setBounds (fullCtrl (y)); y += kRowH;
    m_charSnap.setBounds (fullCtrl (y)); y += kRowH + kPad;

    //----------------------------------------------------------------------
    // TONE
    m_sections.add ({ "TONE", juce::Colour (0xFFe8a840), y });
    m_toneEnable.setBounds (w - 30 - kPad, y + 2, 30, kSecH - 4);
    y += kSecH;

    m_tonePitch.setBounds (fullCtrl (y)); y += kRowH;
    m_toneSweep.setBounds (fullCtrl (y)); y += kRowH;
    m_tonePtDecay.setBounds (fullCtrl (y)); y += kRowH;
    m_toneAttack.setBounds (fullCtrl (y)); y += kRowH;
    m_toneDecay.setBounds (fullCtrl (y)); y += kRowH;
    m_toneLevel.setBounds (dualCtrlL (y));
    m_toneWave .setBounds (dualCtrlR (y));
    y += kRowH;
    m_toneBody.setBounds (fullCtrl (y));
    y += kRowH + 2;

    //----------------------------------------------------------------------
    // NOISE
    m_sections.add ({ "NOISE", juce::Colour (0xFF88aaff), y });
    m_noiseEnable.setBounds (w - 30 - kPad, y + 2, 30, kSecH - 4);
    y += kSecH;

    m_noiseDecay.setBounds (fullCtrl (y)); y += kRowH;
    m_noiseLevel.setBounds (fullCtrl (y)); y += kRowH;
    m_noiseHP.setBounds (fullCtrl (y)); y += kRowH;
    m_noiseLP.setBounds (fullCtrl (y)); y += kRowH + 2;

    //----------------------------------------------------------------------
    // CLICK
    m_sections.add ({ "CLICK", juce::Colour (0xFFff6688), y });
    m_clickEnable.setBounds (w - 30 - kPad, y + 2, 30, kSecH - 4);
    y += kSecH;

    m_clickLevel.setBounds (fullCtrl (y)); y += kRowH;
    m_clickDecay.setBounds (fullCtrl (y)); y += kRowH + 2;

    //----------------------------------------------------------------------
    // OUTPUT
    m_sections.add ({ "OUTPUT", juce::Colour (Theme::Colour::inkMid), y });
    y += kSecH;

    m_outLevel.setBounds (fullCtrl (y)); y += kRowH;
    m_outPan.setBounds (fullCtrl (y)); y += kRowH;
    m_outTune.setBounds (fullCtrl (y)); y += kRowH + 2;

    juce::ignoreUnused (y);  // total height proscribed by kRequiredHeight
}

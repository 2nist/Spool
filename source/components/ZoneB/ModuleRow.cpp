#include "ModuleRow.h"
#include "FaceplatePanel.h"
#include "../../instruments/reel/ReelParams.h"

//==============================================================================
ModuleRow::ModuleRow (int index)
    : m_index (index)
{
}

//==============================================================================
// State setters
//==============================================================================

void ModuleRow::setModuleType (ModuleType type)
{
    m_type = type;
    initParams();
    initFaders();

    if (type == ModuleType::DrumMachine)
    {
        m_drumData = std::make_unique<DrumMachineData>();
        *m_drumData = DrumMachineData::makeDefault();
    }
    else
    {
        m_drumData.reset();
    }

    repaint();
}

void ModuleRow::setDrumData (const DrumMachineData& data)
{
    if (m_type != ModuleType::DrumMachine)
        return;

    if (!m_drumData)
        m_drumData = std::make_unique<DrumMachineData>();

    *m_drumData = data;
    if (m_drumData)
        m_drumData->clamp();
    repaint();
}

void ModuleRow::setSelected (bool selected)
{
    m_selected = selected;
    repaint();
}

void ModuleRow::setMuted (bool muted)
{
    m_muted = muted;
    repaint();
}

void ModuleRow::setGroupColor (juce::Colour c)
{
    m_groupColor = c;
    if (m_faceplate)
        m_faceplate->setGroupColor (c);
    repaint();
}

void ModuleRow::setCollapsed (bool collapsed)
{
    m_collapsed = collapsed;
    repaint();
    if (onCollapseToggled) onCollapseToggled (m_index);
}

// No-op: setExpandedHeightOverride is inline in header

//==============================================================================
// Identity
//==============================================================================

juce::String ModuleRow::getModuleTypeName() const
{
    switch (m_type)
    {
        case ModuleType::Synth:        return "SYNTH";
        case ModuleType::Sampler:      return "SAMPLER";
        case ModuleType::Sequencer:    return "SEQ";
        case ModuleType::Vst3:         return "VST3";
        case ModuleType::DrumMachine:  return "DRUMS";
        case ModuleType::Output:       return "OUTPUT";
        case ModuleType::Reel:         return "REEL";
        default:                       return "";
    }
}

juce::Colour ModuleRow::getSignalColour() const
{
    switch (m_type)
    {
        case ModuleType::Synth:
        case ModuleType::Sampler:      return Theme::Signal::audio;
        case ModuleType::Sequencer:    return Theme::Signal::midi;
        case ModuleType::Vst3:         return Theme::Zone::menu;
        case ModuleType::DrumMachine:  return Theme::Signal::midi;
        case ModuleType::Output:       return Theme::Colour::error;
        case ModuleType::Reel:         return Theme::Zone::a;
        default:                       return Theme::Colour::surfaceEdge;
    }
}

//==============================================================================
// Param init
//==============================================================================

void ModuleRow::initParams()
{
    m_params.clear();
    m_inPorts.clear();
    m_outPorts.clear();
    m_paramRow2Start = -1;
    m_row1Label      = {};
    m_row2Label      = {};

    using P = InlineEditor::Param;

    switch (m_type)
    {
        case ModuleType::Synth:
            // Curated 8-control performance surface.
            // Full synthesis editing lives in Zone A (PolySynthEditorComponent).
            // Defaults mirror PolySynthProcessor initial values.
            m_params.add ({ "CUT",  20.0f, 20000.0f, 4000.0f });  // filter cutoff (Hz)
            m_params.add ({ "RES",   0.0f,    1.0f,    0.3f  });  // resonance
            m_params.add ({ "ENVA", -1.0f,    1.0f,    0.5f  });  // filter env amount
            m_params.add ({ "DET2", -50.0f,  50.0f,    7.0f  });  // osc2 detune (cents)
            m_params.add ({ "MIX2",  0.0f,    1.0f,    0.7f  });  // osc2 level
            m_params.add ({ "ATK",   0.0f,    2.0f,    0.01f });  // amp attack (s)
            m_params.add ({ "REL",   0.0f,    3.0f,    0.3f  });  // amp release (s)
            m_params.add ({ "LFDP",  0.0f,    1.0f,    0.0f  });  // LFO depth
            // m_paramRow2Start stays -1 (single fader row)
            m_row1Label = "FILTER \xc2\xb7 OSC \xc2\xb7 ENV \xc2\xb7 LFO";
            m_inPorts  = {};
            m_outPorts = { "AUDIO" };
            break;

        case ModuleType::Sampler:
            m_params.add ({ "PITCH",  -24.0f,  24.0f,  0.0f  });
            m_params.add ({ "LEVEL",    0.0f,   1.0f,  1.0f  });
            m_params.add ({ "START",    0.0f,   1.0f,  0.0f  });
            m_params.add ({ "END",      0.0f,   1.0f,  1.0f  });
            m_params.add ({ "LOOP",     0.0f,   1.0f,  0.0f  });
            m_row1Label = "SAMPLE";
            m_inPorts  = {};
            m_outPorts = { "AUDIO" };
            break;

        case ModuleType::Vst3:
            m_params.add ({ "MIX",    0.0f,  1.0f, 1.0f });
            m_params.add ({ "GAIN",   0.0f,  2.0f, 0.8f });
            m_params.add ({ "BYPASS", 0.0f,  1.0f, 0.0f });
            m_row1Label = "PLUGIN";
            m_inPorts  = { "AUDIO" };
            m_outPorts = { "AUDIO" };
            break;

        case ModuleType::Sequencer:
            m_pattern.setStepCount (32);
            m_params.add ({ "GATE",  0.0f,  1.0f,  0.5f });
            m_params.add ({ "VEL",   0.0f,  1.0f,  1.0f });
            m_params.add ({ "OCT",  -2.0f,  2.0f,  0.0f });
            m_row1Label = "SEQUENCER";
            m_inPorts  = {};
            m_outPorts = { "MIDI" };
            break;

        case ModuleType::DrumMachine:
            m_params.add ({ "LEVEL",  0.0f,  1.0f,  0.8f });
            m_params.add ({ "PAN",   -1.0f,  1.0f,  0.0f });
            m_row1Label = "OUTPUT";
            m_inPorts  = {};
            m_outPorts = { "AUDIO", "MIDI" };
            break;

        case ModuleType::Output:
            m_params.add ({ "LEVEL",  0.0f,  1.0f,  1.0f });
            m_params.add ({ "PAN",   -1.0f,  1.0f,  0.0f });
            m_row1Label = "OUTPUT";
            m_inPorts  = { "AUDIO" };
            m_outPorts = {};
            break;

        case ModuleType::Reel:
        {
            // Default LOOP mode mapping — remapped when mode changes
            const auto* slots = ReelDefaultMapping::forMode (ReelMode::loop);
            for (int i = 0; i < ReelDefaultMapping::kNumSlots; ++i)
                m_params.add ({ slots[i].label, slots[i].min, slots[i].max, slots[i].def });
            m_row1Label = "REEL";
            m_inPorts  = {};
            m_outPorts = { "AUDIO" };
            break;
        }

        default:
            break;
    }
}

//==============================================================================
// Layout helpers
//==============================================================================

juce::Rectangle<int> ModuleRow::headerRect() const noexcept
{
    return { 0, 0, getWidth(), kHeaderH };
}

juce::Rectangle<int> ModuleRow::paramAreaRect() const noexcept
{
    return { 0, kHeaderH, getWidth(), kParamH };
}

juce::Rectangle<int> ModuleRow::collapseRect() const noexcept
{
    return { getWidth() - kCollapseW, 0, kCollapseW, kHeaderH };
}

juce::Rectangle<int> ModuleRow::muteRect() const noexcept
{
    return { getWidth() - kCollapseW - kMuteW, 0, kMuteW, kHeaderH };
}

//==============================================================================
// Fader colour + formatter helpers
//==============================================================================

juce::Colour ModuleRow::paramColorForLabel (const juce::String& label)
{
    const auto l = label.toUpperCase();

    // Oscillator (blue)
    if (l == "OSC1" || l == "OSC2" || l == "DET1" || l == "DET2"
        || l == "FREQ" || l == "DETUNE" || l == "SHAPE" || l == "WAVE"
        || l == "PW"   || l == "PHASE"  || l == "PITCH"
        || l == "START" || l == "END"   || l == "LOOP")
        return juce::Colour (0xFF4a9eff);

    // Oscillator mix / octave (warm blue-teal)
    if (l == "MIX2")
        return juce::Colour (0xFFd4c9b0);   // output warm — it's a blend control
    if (l == "OCT2")
        return juce::Colour (0xFFee7c4a);   // MIDI orange — pitch offset

    // Filter (orange)
    if (l == "CUT"  || l == "CUTOFF" || l == "RES" || l == "DRIVE"
        || l == "ENVA" || l.startsWith ("FILT"))
        return juce::Colour (0xFFc4822a);

    // Amp envelope (sky blue)
    if (l == "ATK" || l == "DEC" || l == "SUS" || l == "REL"
        || l == "ATTACK" || l == "DECAY" || l == "SUSTAIN" || l == "RELEASE"
        || l == "GATE")
        return juce::Colour (0xFF4a9eff);

    // Filter envelope (amber)
    if (l == "FATK" || l == "FDEC" || l == "FSUS" || l == "FREL")
        return juce::Colour (0xFFc4822a);

    // LFO (purple)
    if (l == "LFRT" || l == "LFDP" || l == "RATE" || l == "DEPTH")
        return juce::Colour (0xFFee4aee);

    // Output / mix (warm white)
    if (l == "LEVEL" || l == "GAIN" || l == "PAN" || l == "MIX"
        || l == "VOL"  || l == "BYPASS")
        return juce::Colour (0xFFd4c9b0);

    // MIDI / sequencer (orange)
    if (l == "VEL" || l == "OCT" || l == "NOTE")
        return juce::Colour (0xFFee7c4a);

    // Noise
    if (l == "NOISE")
        return juce::Colour (0xFFeec44a);

    return juce::Colour (0xFF4a9eff);  // default: osc blue
}

VerticalFader::FormatFn ModuleRow::makeFormatter (const InlineEditor::Param& p)
{
    const auto l = p.label.toUpperCase();

    // ADSR time params (seconds, show as "350m" / "1.2s")
    const bool isTimeParm = (l == "ATK" || l == "DEC" || l == "REL"
                             || l == "FATK" || l == "FDEC" || l == "FREL");
    if (isTimeParm)
        return [p] (float norm) -> juce::String
        {
            const float s = p.min + norm * (p.max - p.min);
            if (s < 1.0f)
                return juce::String (juce::roundToInt (s * 1000.0f)) + "m";
            return juce::String (s, 1) + "s";
        };

    // Wave shape (integer enum 0–3)
    if (l == "OSC1" || l == "OSC2")
    {
        static const char* shapes[] = { "SIN", "SAW", "SQ", "TRI" };
        return [p] (float norm) -> juce::String
        {
            const int i = juce::roundToInt (norm * 3.0f);
            return shapes[juce::jlimit (0, 3, i)];
        };
    }

    // Hz range (e.g. CUT 20–20000, LFRT 0.1–20)
    if (p.max >= 100.0f)
        return [p] (float norm) -> juce::String
        {
            const float hz = p.min + norm * (p.max - p.min);
            return hz >= 1000.0f ? juce::String (hz / 1000.0f, 1) + "k"
                                 : juce::String (static_cast<int> (hz));
        };

    // LFO rate sub-100 Hz range
    if (l == "LFRT" || (p.min >= 0.0f && p.max <= 25.0f && p.max > 1.0f))
        return [p] (float norm) -> juce::String
        {
            const float hz = p.min + norm * (p.max - p.min);
            return juce::String (hz, 1) + "hz";
        };

    // Bipolar integer range (e.g. DET1/DET2 -50–50, OCT2 -2–2)
    if (p.min < 0.0f && p.max > 0.0f && p.max <= 50.0f)
        return [p] (float norm) -> juce::String
        {
            const int v = juce::roundToInt (p.min + norm * (p.max - p.min));
            return (v >= 0 ? "+" : "") + juce::String (v);
        };

    // Bipolar float -1–1 (pan, enva)
    if (p.min < 0.0f && p.max <= 1.0f)
        return [p] (float norm) -> juce::String
        {
            const float v = p.min + norm * (p.max - p.min);
            if (std::abs (v) < 0.01f) return "0";
            return (v > 0.0f ? "+" : "") + juce::String (v, 2);
        };

    // Normalised 0–1
    if (p.min >= 0.0f && p.max <= 1.0f)
        return [p] (float norm) -> juce::String
        {
            return juce::String (norm, 2);
        };

    // Generic: show as integer
    return [p] (float norm) -> juce::String
    {
        return juce::String (static_cast<int> (p.min + norm * (p.max - p.min)));
    };
}

//==============================================================================
// Fader initialisation
//==============================================================================

void ModuleRow::initFaders()
{
    // ---- Tear down any previous state ----
    for (auto* f : m_faders)  removeChildComponent (f);
    m_faders.clear();
    if (m_faceplate)          removeChildComponent (m_faceplate.get());
    m_faceplate.reset();

    // ---- SYNTH and DRUM use the assignable FaceplatePanel ----
    if (m_type == ModuleType::Synth || m_type == ModuleType::DrumMachine)
    {
        m_faceplate = std::make_unique<FaceplatePanel>();
        m_faceplate->setGroupColor (m_groupColor);

        if (m_type == ModuleType::Synth)
        {
            m_faceplate->setSlots (
                QuickFaceplateDefaults::synthSliders(),
                QuickFaceplateDefaults::synthKnobs(),
                QuickFaceplateDefaults::synthButtons(),
                QuickFaceplateDefaults::synthAvailable(),
                "FILTER \xc2\xb7 OSC \xc2\xb7 ENV \xc2\xb7 LFO");
        }
        else // DrumMachine
        {
            m_faceplate->setSlots (
                QuickFaceplateDefaults::drumSliders(),
                QuickFaceplateDefaults::drumKnobs(),
                QuickFaceplateDefaults::drumButtons(),
                QuickFaceplateDefaults::drumAvailable(),
                "KICK \xc2\xb7 SNARE \xc2\xb7 HAT \xc2\xb7 PERC");
        }

        m_faceplate->onControlChanged = [this] (const juce::String& paramId, float value)
        {
            if (onFaceplateChanged) onFaceplateChanged (paramId, value);
        };

        addAndMakeVisible (*m_faceplate);
        resized();
        return;
    }

    // ---- All other types use the legacy VerticalFader row ----
    for (int i = 0; i < m_params.size(); ++i)
    {
        const auto& p     = m_params[i];
        auto*       fader = m_faders.add (new VerticalFader());

        fader->setDisplayName    (p.label);
        fader->setParamColor     (paramColorForLabel (p.label));
        fader->setBipolar        (p.min < 0.0f);
        fader->setValueFormatter (makeFormatter (p));

        const float defaultNorm = (p.max > p.min)
                                  ? (p.value - p.min) / (p.max - p.min)
                                  : 0.5f;
        fader->setDefaultValue (defaultNorm);
        fader->setValue        (defaultNorm, false);

        const int capturedIdx = i;
        fader->onValueChanged = [this, capturedIdx] (float norm)
        {
            auto& param = m_params.getReference (capturedIdx);
            param.value = param.min + norm * (param.max - param.min);
            if (onParamChanged) onParamChanged (capturedIdx, norm);
        };

        addAndMakeVisible (fader);
    }

    resized();
}

//==============================================================================
// Paint
//==============================================================================

void ModuleRow::paint (juce::Graphics& g)
{
    paintHeader (g);
    if (!m_collapsed)
        paintParamArea (g);
}

void ModuleRow::paintHeader (juce::Graphics& g) const
{
    const auto hdr = headerRect();

    // Background
    if (m_selected)
    {
        g.setColour (m_groupColor.withAlpha (0.22f));
        g.fillRect  (hdr);
    }
    else
    {
        g.setColour (Theme::Colour::surface1);
        g.fillRect  (hdr);
    }

    // Left colour stripe
    g.setColour (m_groupColor);
    g.fillRect  (0, 0, kStripeW, kHeaderH);

    if (isEmpty())
    {
        paintEmptyHint (g);
    }
    else
    {
        // Row number
        const int leftEdge = kStripeW + 4;
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  (juce::String (m_index + 1).paddedLeft ('0', 2),
                     leftEdge, 0, kRowNumW, kHeaderH,
                     juce::Justification::centredLeft, false);

        // Module name / type badge
        const juce::String typeName = getModuleTypeName();
        const juce::Colour sigCol   = getSignalColour();
        constexpr int kBadgeW = 46, kBadgeH = 12;
        const int badgeX = leftEdge + kRowNumW + 4;
        const int badgeY = (kHeaderH - kBadgeH) / 2;

        // Signal colour badge
        g.setColour (sigCol.withAlpha (0.20f));
        g.fillRoundedRectangle (juce::Rectangle<int> (badgeX, badgeY,
                                                      kBadgeW, kBadgeH).toFloat(), 2.5f);
        g.setFont   (Theme::Font::micro());
        g.setColour (m_selected ? sigCol.brighter (0.2f) : sigCol);
        g.drawText  (typeName,
                     juce::Rectangle<int> (badgeX, badgeY, kBadgeW, kBadgeH),
                     juce::Justification::centred, false);

        // Port summary (right of badge, if ports exist)
        if (m_outPorts.size() > 0)
        {
            const juce::String portStr = m_outPorts.joinIntoString (" ");
            g.setFont   (Theme::Font::micro());
            g.setColour (Theme::Colour::inkGhost.withAlpha (0.5f));
            const int portX = badgeX + kBadgeW + 6;
            const int rightBound = muteRect().getX() - 4;
            if (portX < rightBound)
                g.drawText (portStr, portX, 0, rightBound - portX, kHeaderH,
                            juce::Justification::centredLeft, false);
        }
    }

    // Mute dot
    {
        const auto mr = muteRect();
        const int cx = mr.getCentreX();
        const int cy = mr.getCentreY();
        const int r  = 3;
        if (m_muted)
        {
            g.setColour (Theme::Colour::error);
            g.fillEllipse (static_cast<float> (cx - r),
                           static_cast<float> (cy - r),
                           static_cast<float> (r * 2),
                           static_cast<float> (r * 2));
        }
        else
        {
            g.setColour (Theme::Colour::inkGhost.withAlpha (0.4f));
            g.drawEllipse (static_cast<float> (cx - r),
                           static_cast<float> (cy - r),
                           static_cast<float> (r * 2),
                           static_cast<float> (r * 2), 1.0f);
        }
    }

    // Collapse arrow
    {
        const auto cr = collapseRect();
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  (m_collapsed ? "+" : "-",
                     cr, juce::Justification::centred, false);
    }

    // Bottom border
    g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.5f));
    g.fillRect  (kStripeW, kHeaderH - 1, getWidth() - kStripeW, 1);

    // DnD hover highlight — drawn on top of all header content
    if (m_isDragOver)
    {
        g.setColour (Theme::Colour::accent.withAlpha (0.22f));
        g.fillRect  (hdr);
        g.setColour (Theme::Colour::accent);
        g.drawRect  (hdr.toFloat(), 1.5f);
    }
}

void ModuleRow::paintParamArea (juce::Graphics& g) const
{
    // FaceplatePanel paints itself — skip all param-area painting.
    if (m_faceplate) return;

    const auto area = paramAreaRect();

    // Background
    g.setColour (Theme::Colour::surface0);
    g.fillRect  (area);

    // Left stripe continuation
    g.setColour (m_groupColor.withAlpha (0.4f));
    g.fillRect  (0, kHeaderH, kStripeW, kParamH);

    // Section labels
    const auto paintSectionLabel = [&] (const juce::String& text, int y)
    {
        g.setColour (Theme::Colour::surface2);
        g.fillRect  (kStripeW, y, getWidth() - kStripeW, kSectionH);
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText  (text,
                     kStripeW + 6, y, getWidth() - kStripeW - 8, kSectionH,
                     juce::Justification::centredLeft, false);
    };

    if (!m_row1Label.isEmpty())
        paintSectionLabel (m_row1Label, kHeaderH);

    if (m_paramRow2Start > 0 && !m_row2Label.isEmpty())
    {
        const int rowH  = (kParamH - 2 * kSectionH) / 2;
        const int row2LabelY = kHeaderH + kSectionH + rowH;

        // Separator line above row 2 label
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.6f));
        g.fillRect  (kStripeW, row2LabelY, getWidth() - kStripeW, 1);

        paintSectionLabel (m_row2Label, row2LabelY);
    }

    // Hints for special / empty states (faders are child components, no paint needed)
    if (isEmpty())
    {
        g.setFont   (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost.withAlpha (0.5f));
        g.drawText  ("CLICK TO LOAD MODULE",
                     area.withTrimmedLeft (kStripeW).reduced (8, 0),
                     juce::Justification::centred, false);
    }
}

void ModuleRow::paintEmptyHint (juce::Graphics& g) const
{
    g.setFont   (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost.withAlpha (0.4f));
    g.drawText  ("+ EMPTY",
                 kStripeW + kRowNumW + 4, 0,
                 getWidth() - kStripeW - kRowNumW - 4 - kMuteW - kCollapseW, kHeaderH,
                 juce::Justification::centredLeft, false);
}

//==============================================================================
// Resize
//==============================================================================

void ModuleRow::resized()
{
    // FaceplatePanel fills the entire param area and handles its own layout.
    if (m_faceplate)
    {
        m_faceplate->setBounds (paramAreaRect());
        return;
    }

    if (m_faders.isEmpty())
        return;

    const int areaX = kStripeW + kFaderGap / 2;

    if (m_paramRow2Start > 0)
    {
        // Two-row layout: kParamH = kSectionH + rowH + kSectionH + rowH
        const int rowH  = (kParamH - 2 * kSectionH) / 2;   // 70px each row
        const int row1Y = kHeaderH + kSectionH;
        const int row2Y = row1Y + rowH + kSectionH;
        const int fH    = juce::jmin (VerticalFader::kPreferredHeight, rowH);

        int x1 = areaX;
        int x2 = areaX;
        for (int i = 0; i < m_faders.size(); ++i)
        {
            auto* f = m_faders[i];
            const int w = f->getPreferredWidth();
            if (i < m_paramRow2Start)
            {
                f->setBounds (x1, row1Y + (rowH - fH) / 2, w, fH);
                x1 += w + kFaderGap;
            }
            else
            {
                f->setBounds (x2, row2Y + (rowH - fH) / 2, w, fH);
                x2 += w + kFaderGap;
            }
        }
    }
    else
    {
        // Single-row layout
        const int rowH = kParamH - kSectionH;
        const int rowY = kHeaderH + kSectionH;
        const int fH   = juce::jmin (VerticalFader::kPreferredHeight, rowH);

        int x = areaX;
        for (auto* f : m_faders)
        {
            const int w = f->getPreferredWidth();
            f->setBounds (x, rowY + (rowH - fH) / 2, w, fH);
            x += w + kFaderGap;
        }
    }
}

//==============================================================================
// Mouse
//==============================================================================

void ModuleRow::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.getPosition();

    // Collapse toggle
    if (collapseRect().contains (pos))
    {
        setCollapsed (!m_collapsed);
        return;
    }

    // Mute dot toggle
    if (muteRect().contains (pos))
    {
        setMuted (!m_muted);
        return;
    }

    if (e.mods.isRightButtonDown())
    {
        if (onRightClicked) onRightClicked (m_index);
        return;
    }

    // Faders are child components — they handle their own mouse events.
    // Only fire row-level callbacks for clicks that land in the header.
    if (headerRect().contains (pos))
    {
        if (isEmpty())
        {
            if (onEmptyClicked) onEmptyClicked (m_index);
        }
        else
        {
            if (onRowClicked) onRowClicked (m_index);
        }
    }
}

void ModuleRow::mouseDrag (const juce::MouseEvent&) {}

void ModuleRow::mouseUp (const juce::MouseEvent&) {}

//==============================================================================
// DragAndDropTarget
//==============================================================================

bool ModuleRow::isInterestedInDragSource (const SourceDetails& details)
{
    if (details.description.toString() != "CapturedAudioClip") return false;
    // Accept drops onto Reel rows and Empty rows (auto-convert to Reel on drop)
    return m_type == ModuleType::Reel || m_type == ModuleType::Empty;
}

void ModuleRow::itemDragEnter (const SourceDetails&)
{
    m_isDragOver = true;
    repaint();
}

void ModuleRow::itemDragExit (const SourceDetails&)
{
    m_isDragOver = false;
    repaint();
}

void ModuleRow::itemDropped (const SourceDetails&)
{
    m_isDragOver = false;

    // Auto-convert empty rows to REEL before notifying the parent
    if (m_type == ModuleType::Empty)
        setModuleType (ModuleType::Reel);

    repaint();

    if (onClipDropped)
        onClipDropped (m_index);
}

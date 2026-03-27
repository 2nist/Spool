#include "PolySynthEditorComponent.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
const auto kSaffron = juce::Colour (0xFFF2B544);
const auto kOscSection = juce::Colour (0xFFF2B544);
const auto kFilterSection = juce::Colour (0xFF91C978);
const auto kAmpSection = juce::Colour (0xFFF4C36A);
const auto kLfoSection = juce::Colour (0xFFE58BB7);
const auto kOutputSection = juce::Colour (0xFFF2B544);
constexpr int kZoneABarHeight = 14;

class ZoneABarSliderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float /*minSliderPos*/,
                           float /*maxSliderPos*/,
                           const juce::Slider::SliderStyle style,
                           juce::Slider& slider) override
    {
        if (style != juce::Slider::LinearBar && style != juce::Slider::LinearBarVertical)
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, 0.0f, 0.0f, style, slider);
            return;
        }

        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (0.5f);
        const auto fill = slider.findColour (juce::Slider::trackColourId);
        const auto bg = slider.findColour (juce::Slider::backgroundColourId);
        const bool isVertical = style == juce::Slider::LinearBarVertical;
        const float proportion = (float) slider.valueToProportionOfLength (slider.getValue());

        g.setColour (bg);
        g.fillRoundedRectangle (bounds, 4.0f);

        if (isVertical)
        {
            const float fillH = bounds.getHeight() * juce::jlimit (0.0f, 1.0f, proportion);
            auto fillBounds = bounds.withY (bounds.getBottom() - fillH).withHeight (fillH);
            if (fillBounds.getHeight() > 0.0f)
            {
                g.setColour (fill);
                g.fillRoundedRectangle (fillBounds, 4.0f);
            }
        }
        else
        {
            const float posX = juce::jlimit (bounds.getX(), bounds.getRight(), sliderPos);
            auto fillBounds = bounds.withWidth (juce::jmax (0.0f, posX - bounds.getX()));
            if (fillBounds.getWidth() > 0.0f)
            {
                g.setColour (fill);
                g.fillRoundedRectangle (fillBounds, 4.0f);
            }
        }

        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.85f));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        const auto label = slider.getName();
        if (label.isNotEmpty())
        {
            const bool overFill = proportion >= (isVertical ? 0.58f : 0.62f);
            const auto labelBg = overFill ? fill : bg;
            const auto textColour = Theme::Helper::inkFor (labelBg).withAlpha (0.98f);
            const auto shadowColour = textColour.getPerceivedBrightness() > 0.5f
                                        ? Theme::Colour::inkDark.withAlpha (0.28f)
                                        : Theme::Colour::inkLight.withAlpha (0.18f);

            g.setFont (Theme::Font::microMedium());

            if (isVertical)
            {
                juce::Graphics::ScopedSaveState state (g);
                const auto centre = bounds.getCentre();
                g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, centre.x, centre.y));

                auto textBounds = bounds.toNearestInt().reduced (0, 5);
                textBounds.translate (1, 1);
                g.setColour (shadowColour);
                g.drawText (label, textBounds, juce::Justification::centred, false);

                textBounds.translate (-1, -1);
                g.setColour (textColour);
                g.drawText (label, textBounds, juce::Justification::centred, false);
            }
            else
            {
                auto textBounds = bounds.toNearestInt().reduced (8, 0);
                g.setColour (shadowColour);
                g.drawText (label, textBounds.translated (1, 1), juce::Justification::centredLeft, false);
                g.setColour (textColour);
                g.drawText (label, textBounds, juce::Justification::centredLeft, false);
            }
        }
    }
};

ZoneABarSliderLookAndFeel& zoneABarLookAndFeel()
{
    static ZoneABarSliderLookAndFeel lookAndFeel;
    return lookAndFeel;
}

float normalizeTime (float seconds)
{
    return std::sqrt (juce::jlimit (0.0f, 1.0f, (seconds - 0.001f) / 3.999f));
}

float denormalizeTime (float normalized)
{
    const float clamped = juce::jlimit (0.0f, 1.0f, normalized);
    return 0.001f + clamped * clamped * 3.999f;
}

const char* waveformShortLabel (int index)
{
    switch (juce::jlimit (0, 4, index))
    {
        case 0: return "SAW";
        case 1: return "SQR";
        case 2: return "SIN";
        case 3: return "TRI";
        case 4: return "NOI";
        default: break;
    }

    return "SAW";
}
}

class PolySynthEditorComponent::EnvelopeGraphComponent : public juce::Component
{
public:
    std::function<void(float, float, float, float)> onEnvelopeChange;

    void setValues (float a, float d, float s, float r)
    {
        attack = a;
        decay = d;
        sustain = s;
        release = r;
        repaint();
    }

    void setAccent (juce::Colour newAccent)
    {
        accent = newAccent;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (Theme::Colour::surface2);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.8f));
        g.drawRoundedRectangle (r, 8.0f, 1.0f);

        const auto plot = r.reduced (10.0f, 10.0f);
        for (int row = 1; row < 4; ++row)
        {
            const float y = plot.getY() + plot.getHeight() * row / 4.0f;
            g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.35f));
            g.drawHorizontalLine (juce::roundToInt (y), plot.getX(), plot.getRight());
        }

        for (int col = 1; col < 6; ++col)
        {
            const float x = plot.getX() + plot.getWidth() * col / 6.0f;
            g.setColour (Theme::Colour::surfaceEdge.withAlpha (0.2f));
            g.drawVerticalLine (juce::roundToInt (x), plot.getY(), plot.getBottom());
        }

        const float attackX = plot.getX() + plot.getWidth() * (0.10f + normalizeTime (attack) * 0.22f);
        const float decayX = attackX + plot.getWidth() * (0.08f + normalizeTime (decay) * 0.22f);
        const float sustainY = plot.getBottom() - plot.getHeight() * juce::jlimit (0.0f, 1.0f, sustain);
        const float releaseX = plot.getRight() - plot.getWidth() * (0.10f + normalizeTime (release) * 0.20f);

        juce::Path env;
        env.startNewSubPath (plot.getX(), plot.getBottom());
        env.lineTo (attackX, plot.getY());
        env.lineTo (decayX, sustainY);
        env.lineTo (releaseX, sustainY);
        env.lineTo (plot.getRight(), plot.getBottom());

        g.setColour (accent.withAlpha (0.14f));
        g.strokePath (env, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved));
        g.setColour (accent);
        g.strokePath (env, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved));

        drawHandle (g, { attackX, plot.getY() });
        drawHandle (g, { decayX, sustainY });
        drawHandle (g, { releaseX, sustainY });

        g.setColour (Theme::Colour::inkGhost);
        g.setFont (Theme::Font::micro());
        g.drawText ("attack", juce::Rectangle<int> ((int) plot.getX(), (int) plot.getBottom() + 2, 44, 10), juce::Justification::left, false);
        g.drawText ("decay", juce::Rectangle<int> ((int) decayX - 18, (int) plot.getBottom() + 2, 40, 10), juce::Justification::centred, false);
        g.drawText ("release", juce::Rectangle<int> ((int) releaseX - 16, (int) plot.getBottom() + 2, 48, 10), juce::Justification::centred, false);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        activeHandle = nearestHandle (e.position);
        mouseDrag (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (activeHandle < 0)
            return;

        const auto plot = getLocalBounds().toFloat().reduced (11.0f, 11.0f);
        const float nx = juce::jlimit (0.0f, 1.0f, (e.position.x - plot.getX()) / juce::jmax (1.0f, plot.getWidth()));
        const float ny = juce::jlimit (0.0f, 1.0f, 1.0f - ((e.position.y - plot.getY()) / juce::jmax (1.0f, plot.getHeight())));

        if (activeHandle == 0)
            attack = denormalizeTime (juce::jlimit (0.0f, 1.0f, (nx - 0.10f) / 0.22f));
        else if (activeHandle == 1)
        {
            decay = denormalizeTime (juce::jlimit (0.0f, 1.0f, (nx - 0.26f) / 0.24f));
            sustain = juce::jlimit (0.0f, 1.0f, ny);
        }
        else if (activeHandle == 2)
        {
            release = denormalizeTime (juce::jlimit (0.0f, 1.0f, (0.90f - nx) / 0.20f));
            sustain = juce::jlimit (0.0f, 1.0f, ny);
        }

        if (onEnvelopeChange)
            onEnvelopeChange (attack, decay, sustain, release);

        repaint();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        activeHandle = -1;
    }

private:
    float attack { 0.01f }, decay { 0.1f }, sustain { 0.8f }, release { 0.3f };
    int activeHandle { -1 };
    juce::Colour accent { kSaffron };

    void drawHandle (juce::Graphics& g, juce::Point<float> p)
    {
        g.setColour (Theme::Colour::inkLight);
        g.fillEllipse (p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f);
        g.setColour (accent.darker (0.35f));
        g.drawEllipse (p.x - 4.0f, p.y - 4.0f, 8.0f, 8.0f, 1.0f);
    }

    int nearestHandle (juce::Point<float> pos) const
    {
        const auto plot = getLocalBounds().toFloat().reduced (11.0f, 11.0f);
        const juce::Point<float> handles[] =
        {
            { plot.getX() + plot.getWidth() * (0.10f + normalizeTime (attack) * 0.22f), plot.getY() },
            { plot.getX() + plot.getWidth() * (0.34f + normalizeTime (decay) * 0.22f), plot.getBottom() - plot.getHeight() * sustain },
            { plot.getRight() - plot.getWidth() * (0.10f + normalizeTime (release) * 0.20f), plot.getBottom() - plot.getHeight() * sustain },
        };

        float bestDistance = std::numeric_limits<float>::max();
        int bestIndex = 0;
        for (int i = 0; i < 3; ++i)
        {
            const float distance = handles[i].getDistanceSquaredFrom (pos);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestIndex = i;
            }
        }

        return bestIndex;
    }
};

PolySynthEditorComponent::PolySynthEditorComponent()
{
    m_ampEnvGraph = std::make_unique<EnvelopeGraphComponent>();
    m_filtEnvGraph = std::make_unique<EnvelopeGraphComponent>();
    m_ampEnvGraph->setAccent (kSaffron);
    m_filtEnvGraph->setAccent (kFilterSection);
    addAndMakeVisible (*m_ampEnvGraph);
    addAndMakeVisible (*m_filtEnvGraph);
    setupControls();
}

PolySynthEditorComponent::~PolySynthEditorComponent()
{
    for (auto* slider : { &m_osc1Detune, &m_osc2Detune, &m_osc1PulseWidth, &m_osc2PulseWidth,
                          &m_osc2Level, &m_filterCutoff, &m_filterRes, &m_filterEnvAmt,
                          &m_filtA, &m_filtD, &m_filtS, &m_filtR, &m_lfoRate, &m_lfoDepth,
                          &m_charDrive, &m_charAsym, &m_charDrift, &m_outLevel, &m_outPan })
        slider->setLookAndFeel (nullptr);
}

void PolySynthEditorComponent::setupControls()
{
    auto initToggle = [this] (juce::Button& b)
    {
        b.setClickingTogglesState (true);
        addAndMakeVisible (b);
    };

    for (auto* cb : { &m_osc1Octave, &m_osc2Octave })
    {
        cb->addItem ("-2", 1);
        cb->addItem ("-1", 2);
        cb->addItem ("0", 3);
        cb->addItem ("+1", 4);
        cb->addItem ("+2", 5);
        cb->setSelectedId (3, juce::dontSendNotification);
        addAndMakeVisible (*cb);
    }

    auto initShapeButton = [this] (juce::TextButton& button, const char* paramId)
    {
        button.onClick = [this, &button, param = juce::String (paramId)]
        {
            const int current = m_proc != nullptr ? (int) m_proc->getParam (param) : 0;
            const int next = (current + 1) % 5;
            button.setButtonText (juce::String (waveformShortLabel (next)) + " v");
            if (m_proc)
                m_proc->setParam (param, (float) next);
        };
        addAndMakeVisible (button);
    };

    initShapeButton (m_osc1ShapeBtn, "osc1.shape");
    initShapeButton (m_osc2ShapeBtn, "osc2.shape");
    m_osc1Octave.onChange = [this] { if (m_proc) m_proc->setParam ("osc1.octave", (float) (m_osc1Octave.getSelectedItemIndex() - 2)); };
    m_osc2Octave.onChange = [this] { if (m_proc) m_proc->setParam ("osc2.octave", (float) (m_osc2Octave.getSelectedItemIndex() - 2)); };

    initToggle (m_osc1On);
    initToggle (m_osc2On);
    initToggle (m_lfoOn);

    m_osc1On.onClick = [this] { if (m_proc) m_proc->setParam ("osc1.on", m_osc1On.getToggleState() ? 1.0f : 0.0f); };
    m_osc2On.onClick = [this] { if (m_proc) m_proc->setParam ("osc2.on", m_osc2On.getToggleState() ? 1.0f : 0.0f); };
    m_lfoOn .onClick = [this] { if (m_proc) m_proc->setParam ("lfo.on",  m_lfoOn.getToggleState()  ? 1.0f : 0.0f); };

    auto initLinear = [this] (juce::Slider& s, double min, double max, double mid, double def, std::function<void(double)> fn)
    {
        initSlider (s, min, max, mid, def, std::move (fn));
    };

    initLinear (m_osc1Detune, -50.0, 50.0,   0.0,    0.0, [this](double v){ if (m_proc) m_proc->setParam ("osc1.detune", (float) v); });
    initLinear (m_osc2Detune, -50.0, 50.0,   0.0,    7.0, [this](double v){ if (m_proc) m_proc->setParam ("osc2.detune", (float) v); });
    initLinear (m_osc1PulseWidth, 0.05, 0.95, 0.5,   0.5, [this](double v){ if (m_proc) m_proc->setParam ("osc1.pulse_width", (float) v); });
    initLinear (m_osc2PulseWidth, 0.05, 0.95, 0.5,   0.5, [this](double v){ if (m_proc) m_proc->setParam ("osc2.pulse_width", (float) v); });
    initLinear (m_osc2Level,      0.0,  1.0,  0.5,   0.5, [this](double v){ if (m_proc) m_proc->setParam ("osc2.level", (float) v); });
    initLinear (m_filterCutoff,  20.0, 20000.0, 1000.0, 4000.0, [this](double v){ if (m_proc) m_proc->setParam ("filter.cutoff", (float) v); });
    initLinear (m_filterRes,      0.0,  1.0,  0.5,   0.3, [this](double v){ if (m_proc) m_proc->setParam ("filter.res", (float) v); });
    initLinear (m_filterEnvAmt,   0.0,  1.0,  0.5,   0.5, [this](double v){ if (m_proc) m_proc->setParam ("filter.env_amt", (float) v); });
    initLinear (m_filtA, 0.001, 4.0, 0.5, 0.01, [this](double v){ if (m_proc) m_proc->setParam ("filt.attack", (float) v); });
    initLinear (m_filtD, 0.001, 4.0, 0.5, 0.2,  [this](double v){ if (m_proc) m_proc->setParam ("filt.decay", (float) v); });
    initLinear (m_filtS, 0.0,   1.0, 0.5, 0.5,  [this](double v){ if (m_proc) m_proc->setParam ("filt.sustain", (float) v); });
    initLinear (m_filtR, 0.001, 4.0, 0.5, 0.3,  [this](double v){ if (m_proc) m_proc->setParam ("filt.release", (float) v); });
    initLinear (m_lfoRate,  0.01, 20.0, 2.0, 5.0, [this](double v){ if (m_proc) m_proc->setParam ("lfo.rate", (float) v); });
    initLinear (m_lfoDepth, 0.0,  1.0,  0.5, 0.0, [this](double v){ if (m_proc) m_proc->setParam ("lfo.depth", (float) v); });
    initLinear (m_charDrive, 0.0, 1.0, 0.5, 0.20, [this](double v){ if (m_proc) m_proc->setParam ("char.drive", (float) v); });
    initLinear (m_charAsym,  0.0, 1.0, 0.5, 0.15, [this](double v){ if (m_proc) m_proc->setParam ("char.asym", (float) v); });
    initLinear (m_charDrift, 0.0, 1.0, 0.5, 0.15, [this](double v){ if (m_proc) m_proc->setParam ("char.drift", (float) v); });
    initLinear (m_outLevel,  0.0, 1.5, 0.8, 1.0, [this](double v){ if (m_proc) m_proc->setParam ("out.level", (float) v); });
    initLinear (m_outPan,   -1.0, 1.0, 0.0, 0.0, [this](double v){ if (m_proc) m_proc->setParam ("out.pan", (float) v); });

    m_charDrive.setName ("DRV");
    m_charAsym.setName ("ASY");
    m_charDrift.setName ("DRF");
    m_osc1Detune.setName ("DET");
    m_osc2Detune.setName ("DET");
    m_osc1PulseWidth.setName ("PW");
    m_osc2PulseWidth.setName ("PW");
    m_osc2Level.setName ("MIX");
    m_filterCutoff.setName ("CUTOFF");
    m_filterRes.setName ("RES");
    m_filterEnvAmt.setName ("ENV");
    m_lfoRate.setName ("RATE");
    m_lfoDepth.setName ("DEPTH");
    m_outLevel.setName ("LEVEL");
    m_outPan.setName ("PAN");

    for (auto* slider : { &m_osc1Detune, &m_osc2Detune,
                          &m_filterCutoff, &m_filterRes, &m_filterEnvAmt,
                          &m_lfoRate, &m_lfoDepth })
        slider->setSliderStyle (juce::Slider::LinearBarVertical);

    auto tintSlider = [] (juce::Slider& slider, juce::Colour colour)
    {
        slider.setColour (juce::Slider::trackColourId, colour);
        slider.setColour (juce::Slider::thumbColourId, colour.brighter (0.08f));
        slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    };

    for (auto* slider : { &m_filterCutoff, &m_filterRes, &m_filterEnvAmt })
        tintSlider (*slider, kFilterSection);

    for (auto* slider : { &m_lfoRate, &m_lfoDepth })
        tintSlider (*slider, kLfoSection);

    tintSlider (m_outLevel, kOutputSection);
    tintSlider (m_outPan, kOutputSection.withMultipliedBrightness (0.88f));

    m_lfoTarget.addItem ("Pitch", 1);
    m_lfoTarget.addItem ("Filter", 2);
    m_lfoTarget.setSelectedId (1, juce::dontSendNotification);
    m_lfoTarget.onChange = [this] { if (m_proc) m_proc->setParam ("lfo.target", (float) m_lfoTarget.getSelectedItemIndex()); };
    addAndMakeVisible (m_lfoTarget);
    m_lfoOn.setButtonText ("ON");

    m_polyBtn.setClickingTogglesState (false);
    m_monoBtn.setClickingTogglesState (false);
    m_polyBtn.setConnectedEdges (juce::Button::ConnectedOnRight);
    m_monoBtn.setConnectedEdges (juce::Button::ConnectedOnLeft);
    m_polyBtn.onClick = [this]
    {
        if (! m_proc) return;
        m_proc->setParam ("voice.mono", 0.0f);
        m_polyBtn.setToggleState (true, juce::dontSendNotification);
        m_monoBtn.setToggleState (false, juce::dontSendNotification);
    };
    m_monoBtn.onClick = [this]
    {
        if (! m_proc) return;
        m_proc->setParam ("voice.mono", 1.0f);
        m_polyBtn.setToggleState (false, juce::dontSendNotification);
        m_monoBtn.setToggleState (true, juce::dontSendNotification);
    };
    addAndMakeVisible (m_polyBtn);
    addAndMakeVisible (m_monoBtn);

    m_ampEnvGraph->onEnvelopeChange = [this] (float a, float d, float s, float r)
    {
        m_ampAttack = a;
        m_ampDecay = d;
        m_ampSustain = s;
        m_ampRelease = r;
        if (m_proc)
        {
            m_proc->setParam ("amp.attack", a);
            m_proc->setParam ("amp.decay", d);
            m_proc->setParam ("amp.sustain", s);
            m_proc->setParam ("amp.release", r);
        }
    };

    m_filtEnvGraph->onEnvelopeChange = [this] (float a, float d, float s, float r)
    {
        m_filtAttack = a;
        m_filtDecay = d;
        m_filtSustain = s;
        m_filtRelease = r;
        m_filtA.setValue (a, juce::dontSendNotification);
        m_filtD.setValue (d, juce::dontSendNotification);
        m_filtS.setValue (s, juce::dontSendNotification);
        m_filtR.setValue (r, juce::dontSendNotification);
        if (m_proc)
        {
            m_proc->setParam ("filt.attack", a);
            m_proc->setParam ("filt.decay", d);
            m_proc->setParam ("filt.sustain", s);
            m_proc->setParam ("filt.release", r);
        }
    };
}

void PolySynthEditorComponent::initSlider (juce::Slider& s,
                                           double min, double max,
                                           double midPt, double defaultVal,
                                           std::function<void(double)> onChange)
{
    s.setSliderStyle (juce::Slider::LinearBar);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setLookAndFeel (&zoneABarLookAndFeel());
    s.setRange (min, max);
    s.setSkewFactorFromMidPoint (midPt);
    s.setValue (defaultVal, juce::dontSendNotification);
    s.setColour (juce::Slider::backgroundColourId, Theme::Colour::surface3.withAlpha (0.95f));
    s.setColour (juce::Slider::trackColourId, kSaffron);
    s.setColour (juce::Slider::thumbColourId, kSaffron.brighter (0.08f));
    s.setColour (juce::Slider::rotarySliderFillColourId, kSaffron);
    s.onValueChange = [fn = std::move (onChange), &s] { fn (s.getValue()); };
    addAndMakeVisible (s);
}

void PolySynthEditorComponent::setProcessor (ModuleProcessor* proc)
{
    m_proc = proc;
    readFromProcessor();
    repaint();
}

void PolySynthEditorComponent::syncAmpEnvelopeGraph()
{
    if (m_ampEnvGraph)
        m_ampEnvGraph->setValues (m_ampAttack, m_ampDecay, m_ampSustain, m_ampRelease);
}

void PolySynthEditorComponent::syncFilterEnvelopeGraph()
{
    if (m_filtEnvGraph)
        m_filtEnvGraph->setValues (m_filtAttack, m_filtDecay, m_filtSustain, m_filtRelease);
}

void PolySynthEditorComponent::readFromProcessor()
{
    if (m_proc == nullptr)
        return;

    const auto dnSend = juce::dontSendNotification;

    m_osc1On.setToggleState (m_proc->getParam ("osc1.on") >= 0.5f, dnSend);
    m_osc2On.setToggleState (m_proc->getParam ("osc2.on") >= 0.5f, dnSend);
    m_osc1ShapeBtn.setButtonText (juce::String (waveformShortLabel ((int) m_proc->getParam ("osc1.shape"))) + " v");
    m_osc2ShapeBtn.setButtonText (juce::String (waveformShortLabel ((int) m_proc->getParam ("osc2.shape"))) + " v");
    m_osc1Octave.setSelectedItemIndex ((int) m_proc->getParam ("osc1.octave") + 2, dnSend);
    m_osc2Octave.setSelectedItemIndex ((int) m_proc->getParam ("osc2.octave") + 2, dnSend);
    m_osc1Detune.setValue (m_proc->getParam ("osc1.detune"), dnSend);
    m_osc2Detune.setValue (m_proc->getParam ("osc2.detune"), dnSend);
    m_osc1PulseWidth.setValue (m_proc->getParam ("osc1.pulse_width"), dnSend);
    m_osc2PulseWidth.setValue (m_proc->getParam ("osc2.pulse_width"), dnSend);
    m_osc2Level.setValue (m_proc->getParam ("osc2.level"), dnSend);

    m_filterCutoff.setValue (m_proc->getParam ("filter.cutoff"), dnSend);
    m_filterRes.setValue (m_proc->getParam ("filter.res"), dnSend);
    m_filterEnvAmt.setValue (m_proc->getParam ("filter.env_amt"), dnSend);

    m_ampAttack = m_proc->getParam ("amp.attack");
    m_ampDecay = m_proc->getParam ("amp.decay");
    m_ampSustain = m_proc->getParam ("amp.sustain");
    m_ampRelease = m_proc->getParam ("amp.release");
    syncAmpEnvelopeGraph();

    m_filtAttack = m_proc->getParam ("filt.attack");
    m_filtDecay = m_proc->getParam ("filt.decay");
    m_filtSustain = m_proc->getParam ("filt.sustain");
    m_filtRelease = m_proc->getParam ("filt.release");
    m_filtA.setValue (m_filtAttack, dnSend);
    m_filtD.setValue (m_filtDecay, dnSend);
    m_filtS.setValue (m_filtSustain, dnSend);
    m_filtR.setValue (m_filtRelease, dnSend);
    syncFilterEnvelopeGraph();

    m_lfoOn.setToggleState (m_proc->getParam ("lfo.on") >= 0.5f, dnSend);
    m_lfoRate.setValue (m_proc->getParam ("lfo.rate"), dnSend);
    m_lfoDepth.setValue (m_proc->getParam ("lfo.depth"), dnSend);
    m_lfoTarget.setSelectedItemIndex ((int) m_proc->getParam ("lfo.target"), dnSend);

    const bool mono = m_proc->getParam ("voice.mono") >= 0.5f;
    m_polyBtn.setToggleState (! mono, dnSend);
    m_monoBtn.setToggleState (mono, dnSend);

    m_charDrive.setValue (m_proc->getParam ("char.drive"), dnSend);
    m_charAsym.setValue (m_proc->getParam ("char.asym"), dnSend);
    m_charDrift.setValue (m_proc->getParam ("char.drift"), dnSend);
    m_outLevel.setValue (m_proc->getParam ("out.level"), dnSend);
    m_outPan.setValue (m_proc->getParam ("out.pan"), dnSend);
}

void PolySynthEditorComponent::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Zone::bgA);

    static constexpr int kSecH = 16;
    for (const auto& sec : m_sections)
    {
        g.setColour (Theme::Colour::surface1);
        g.fillRect (0, sec.y, getWidth(), kSecH);
        g.setColour (sec.colour.withAlpha (0.85f));
        g.fillRect (0, sec.y, 3, kSecH);
        g.setColour (sec.colour);
        g.setFont (Theme::Font::micro());
        g.drawText (sec.name, 8, sec.y, getWidth() - 12, kSecH, juce::Justification::centredLeft, false);
    }

    g.setFont (Theme::Font::micro());
    g.setColour (Theme::Colour::inkGhost);
    for (const auto& lbl : m_labels)
        g.drawText (lbl.text, lbl.bounds, juce::Justification::centredLeft, false);

    if (m_proc == nullptr)
    {
        g.setColour (Theme::Zone::bgA.withAlpha (0.65f));
        g.fillAll();
        g.setFont (Theme::Font::micro());
        g.setColour (Theme::Colour::inkGhost);
        g.drawText ("No slot bound", getLocalBounds(), juce::Justification::centred, false);
    }
}

void PolySynthEditorComponent::resized()
{
    m_sections.clear();
    m_labels.clear();

    const int w = getWidth();
    const int pad = 6;
    const int secH = 16;
    const int gap = 6;
    const int barH = kZoneABarHeight;

    auto addLabel = [this] (const juce::String& text, juce::Rectangle<int> bounds)
    {
        m_labels.add ({ text, bounds });
    };

    int y = pad;
    m_sections.add ({ "CHARACTER / VOICE", kOutputSection, y });
    y += secH;

    const auto topArea = juce::Rectangle<int> (pad, y, w - pad * 2, 34);
    const int third = (topArea.getWidth() - 8) / 3;
    m_charDrive.setBounds (topArea.getX(), topArea.getY() + 4, third, barH);
    m_charAsym.setBounds (topArea.getX() + third + 4, topArea.getY() + 4, third, barH);
    m_charDrift.setBounds (topArea.getX() + (third + 4) * 2, topArea.getY() + 4, topArea.getWidth() - (third + 4) * 2, barH);
    addLabel ("CHAR", { topArea.getX(), topArea.getY() + 20, 36, 10 });
    y = topArea.getBottom() + gap;

    m_sections.add ({ "OSCILLATORS", kOscSection, y });
    y += secH;

    const auto oscArea = juce::Rectangle<int> (pad, y, w - pad * 2, 84);
    const int oscGap = 6;
    const int sideW = (oscArea.getWidth() - oscGap) / 2;
    const auto osc1 = juce::Rectangle<int> (oscArea.getX(), oscArea.getY(), sideW, 66);
    const auto osc2 = juce::Rectangle<int> (osc1.getRight() + oscGap, oscArea.getY(), sideW, 66);
    const int detuneW = 18;
    const int controlW1 = osc1.getWidth() - detuneW - 4;
    const int controlW2 = osc2.getWidth() - detuneW - 4;
    const int splitW1 = (controlW1 - 4) / 2;
    const int splitW2 = (controlW2 - 4) / 2;
    const int row2Y = osc1.getY() + 18;
    const int row3Y = osc1.getY() + 36;
    const int controlH = 14;
    const int detuneY = row2Y;
    const int detuneH = 34;

    m_osc1On.setBounds (osc1.getX(), osc1.getY(), controlW1, 16);
    m_osc1Octave.setBounds (osc1.getX(), row2Y, controlW1, controlH);
    m_osc1ShapeBtn.setBounds (osc1.getX(), row3Y, splitW1, controlH);
    m_osc1PulseWidth.setBounds (osc1.getX() + splitW1 + 4, row3Y, controlW1 - splitW1 - 4, barH);
    m_osc1Detune.setBounds (osc1.getRight() - detuneW, detuneY, detuneW, detuneH);

    m_osc2On.setBounds (osc2.getX(), osc2.getY(), controlW2, 16);
    m_osc2Octave.setBounds (osc2.getX(), row2Y, controlW2, controlH);
    m_osc2ShapeBtn.setBounds (osc2.getX(), row3Y, splitW2, controlH);
    m_osc2PulseWidth.setBounds (osc2.getX() + splitW2 + 4, row3Y, controlW2 - splitW2 - 4, barH);
    m_osc2Detune.setBounds (osc2.getRight() - detuneW, detuneY, detuneW, detuneH);

    m_osc2Level.setBounds (oscArea.getX(), osc1.getBottom() + 4, oscArea.getWidth(), barH);
    y = oscArea.getBottom() + gap;

    m_sections.add ({ "ENVELOPES / FILTER", kAmpSection, y });
    y += secH;

    const auto sculptArea = juce::Rectangle<int> (pad, y, w - pad * 2, 132);
    const int leftW = (sculptArea.getWidth() - 6) / 2;
    const auto ampArea = juce::Rectangle<int> (sculptArea.getX(), sculptArea.getY(), leftW, sculptArea.getHeight());
    auto filterBlock = juce::Rectangle<int> (ampArea.getRight() + 6, sculptArea.getY(), sculptArea.getWidth() - leftW - 6, sculptArea.getHeight());
    const auto filterControls = filterBlock.removeFromTop (54);
    const auto filterGraphArea = filterBlock.withTrimmedTop (6);

    m_ampEnvGraph->setBounds (ampArea.getX(), ampArea.getY(), ampArea.getWidth(), ampArea.getHeight() - 12);
    addLabel ("AMP", { ampArea.getX(), ampArea.getBottom() - 10, 32, 10 });

    const int filterColW = (filterControls.getWidth() - 8) / 3;
    m_filterCutoff.setBounds (filterControls.getX(), filterControls.getY(), filterColW, filterControls.getHeight());
    m_filterRes.setBounds (filterControls.getX() + filterColW + 4, filterControls.getY(), filterColW, filterControls.getHeight());
    m_filterEnvAmt.setBounds (filterControls.getX() + (filterColW + 4) * 2, filterControls.getY(),
                              filterControls.getWidth() - (filterColW + 4) * 2, filterControls.getHeight());
    m_filtEnvGraph->setBounds (filterGraphArea.getX(), filterGraphArea.getY(), filterGraphArea.getWidth(), filterGraphArea.getHeight() - 12);
    addLabel ("FILTER ENV", { filterGraphArea.getX(), filterGraphArea.getBottom() - 10, 56, 10 });
    y = sculptArea.getBottom() + gap;

    m_sections.add ({ "LFO", kLfoSection, y });
    y += secH;

    const auto lfoArea = juce::Rectangle<int> (pad, y, w - pad * 2, 82);
    m_lfoOn.setBounds (lfoArea.getX(), lfoArea.getY(), 40, 16);
    m_lfoTarget.setBounds (lfoArea.getX() + 44, lfoArea.getY(), lfoArea.getWidth() - 44, 16);
    const int lfoColW = (lfoArea.getWidth() - 4) / 2;
    m_lfoRate.setBounds (lfoArea.getX(), lfoArea.getY() + 22, lfoColW, lfoArea.getHeight() - 24);
    m_lfoDepth.setBounds (lfoArea.getX() + lfoColW + 4, lfoArea.getY() + 22, lfoArea.getWidth() - lfoColW - 4, lfoArea.getHeight() - 24);
    y = lfoArea.getBottom() + gap;

    m_sections.add ({ "OUTPUT", kOutputSection, y });
    y += secH;

    const auto outputArea = juce::Rectangle<int> (pad, y, w - pad * 2, 44);
    const int panW = 54;
    m_outLevel.setBounds (outputArea.getX(), outputArea.getY() + 4, outputArea.getWidth() - panW - 6, barH);
    m_outPan.setBounds (outputArea.getRight() - panW, outputArea.getY() + 4, panW, barH);

    const int modeButtonW = 42;
    const int modeY = getHeight() - pad - 18;
    const int modeTotalW = modeButtonW * 2 + 2;
    const int modeX = getWidth() - pad - modeTotalW;
    m_polyBtn.setBounds (modeX, modeY, modeButtonW, 18);
    m_monoBtn.setBounds (modeX + modeButtonW + 2, modeY, modeButtonW, 18);
}

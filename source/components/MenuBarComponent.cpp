#include "MenuBarComponent.h"

MenuBarComponent::MenuBarComponent()
{
}

void MenuBarComponent::resized()
{
    // No child components — nothing to lay out
}

void MenuBarComponent::paint (juce::Graphics& g)
{
    const float w  = static_cast<float> (getWidth());
    const float h  = static_cast<float> (getHeight());
    const float cy = h / 2.0f;

    // ── Background ────────────────────────────────────────────────────────────
    g.setColour (Theme::Colour::surface0);
    g.fillRect (0.0f, 0.0f, w, h);

    // ── Bottom border ─────────────────────────────────────────────────────────
    g.setColour (Theme::Colour::surfaceEdge);
    g.fillRect (0.0f, h - Theme::Stroke::subtle, w, Theme::Stroke::subtle);

    // ── Left side: menu items ─────────────────────────────────────────────────
    {
        const juce::StringArray items { "FILE", "EDIT", "VST BROWSER", "PRESETS", "AUDIO", "VIEW" };
        const auto labelFont = Theme::Font::label();
        g.setFont (labelFont);
        g.setColour (Theme::Colour::inkMid);

        float lx = Theme::Space::lg;
        for (const auto& item : items)
        {
            const float itemW = juce::GlyphArrangement::getStringWidth (labelFont, item);
            g.drawText (item,
                juce::Rectangle<float> (lx, 0.0f, itemW, h),
                juce::Justification::centredLeft,
                false);
            lx += itemW + Theme::Space::lg;
        }
    }

    // ── Right side: status indicators (right → left) ──────────────────────────
    {
        const auto headingFont = Theme::Font::heading();
        const auto microFont   = Theme::Font::micro();

        // Pre-compute widths
        const float transportW  = juce::GlyphArrangement::getStringWidth (headingFont, "1 . 1 . 00");
        const float bpmValW     = juce::GlyphArrangement::getStringWidth (headingFont, "120");
        const float bpmLblW     = juce::GlyphArrangement::getStringWidth (microFont, "BPM");
        const float bpmBlockW   = juce::jmax (bpmValW, bpmLblW);
        const float linkTextW   = juce::GlyphArrangement::getStringWidth (microFont, "LINK");
        const float midiTextW   = juce::GlyphArrangement::getStringWidth (microFont, "MIDI");
        const float pipD        = 10.0f; // larger MIDI pips for visibility
        const float pipGap      = 6.0f;
        const float pipsW       = 4.0f * pipD + 3.0f * pipGap;   // 4 pips
        const float midiBlockW  = pipsW + 6.0f + midiTextW;
        const float linkBlockW  = 6.0f + 4.0f + linkTextW;      // circle + gap + label

        // Cursor starts at right inset
        float rx = w - Theme::Space::lg;

        // 1. Transport position ------------------------------------------------
        g.setFont (headingFont);
        g.setColour (Theme::Colour::inkLight);
        g.drawText ("1 . 1 . 00",
            juce::Rectangle<float> (rx - transportW, 0.0f, transportW, h),
            juce::Justification::centredLeft,
            false);
        rx -= transportW + 4.0f;   // 4px separator

        // 2. BPM value + label ------------------------------------------------
        {
            const float valH      = Theme::Font::sizeHeading;
            const float lblH      = Theme::Font::sizeMicro;
            const float gap       = 2.0f;
            const float totalTxtH = valH + gap + lblH;
            const float startY    = (h - totalTxtH) / 2.0f;
            const float bpmLeft   = rx - bpmBlockW;

            g.setFont (headingFont);
            g.setColour (Theme::Colour::accent);
            g.drawText ("120",
                juce::Rectangle<float> (bpmLeft, startY, bpmBlockW, valH),
                juce::Justification::centred,
                false);

            g.setFont (microFont);
            g.setColour (Theme::Colour::inkGhost);
            g.drawText ("BPM",
                juce::Rectangle<float> (bpmLeft, startY + valH + gap, bpmBlockW, lblH),
                juce::Justification::centred,
                false);
        }
        rx -= bpmBlockW + 8.0f;   // 8px separator

        // 3. Link indicator (circle + "LINK" label) ----------------------------
        {
            const float linkLeft = rx - linkBlockW;

            g.setColour (Theme::Zone::c);
            g.fillEllipse (linkLeft, cy - 3.0f, 6.0f, 6.0f);

            g.setFont (microFont);
            g.setColour (Theme::Colour::inkGhost);
            g.drawText ("LINK",
                juce::Rectangle<float> (linkLeft + 6.0f + 4.0f,
                                        cy - Theme::Font::sizeMicro / 2.0f,
                                        linkTextW,
                                        Theme::Font::sizeMicro),
                juce::Justification::centredLeft,
                false);
        }
        rx -= linkBlockW + 8.0f;   // 8px separator

        // 4. MIDI activity pips + "MIDI" label ---------------------------------
        {
            const float midiLeft = rx - midiBlockW;

            const juce::Colour pipColours[4] {
                Theme::Signal::audio,
                Theme::Signal::midi,
                Theme::Signal::cv,
                Theme::Signal::gate
            };

                for (int i = 0; i < 4; ++i)
                {
                    const float pipX = midiLeft + static_cast<float> (i) * (pipD + pipGap);
                    g.setColour (pipColours[i].withAlpha (0.30f));
                    g.fillEllipse (pipX, cy - pipD/2.0f, pipD, pipD);
                }

            g.setFont (Theme::Font::value());
            g.setColour (Theme::Colour::inkGhost);
            g.drawText ("MIDI",
                juce::Rectangle<float> (midiLeft + pipsW + 6.0f,
                                        cy - Theme::Font::sizeValue / 2.0f,
                                        midiTextW,
                                        Theme::Font::sizeValue),
                juce::Justification::centredLeft,
                false);
        }
        rx -= midiBlockW + 8.0f;   // 8px gap before alert dot

        // 5. Alert dot (inactive state) ----------------------------------------
        {
            constexpr float alertD = 5.0f;
            rx -= alertD;
            g.setColour (Theme::Colour::inactive);
            g.fillEllipse (rx, cy - alertD / 2.0f, alertD, alertD);
        }
    }
}

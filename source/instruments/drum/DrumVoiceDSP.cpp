#include "DrumVoiceDSP.h"

//==============================================================================
void DrumVoiceDSP::prepare (double sampleRate)
{
    m_sampleRate = sampleRate;
    m_active     = false;
    m_toneEnv    = 0.0f;
    m_noiseEnv   = 0.0f;
    m_clickEnv   = 0.0f;
    m_metalEnv   = 0.0f;
    m_attackEnv  = 1.0f;
    m_attackStep = 1.0f;
    m_pinkState  = 0.0f;
    m_noiseLast  = 0.0f;
    m_noiseHP.reset();
    m_noiseLP.reset();
    m_clickHP.reset();
    m_metalHP.reset();
}

//==============================================================================
void DrumVoiceDSP::applyParams (const DrumVoiceParams& p)
{
    m_params = p;
    m_driveAmount = juce::jlimit (0.0f, 1.0f, p.character.drive);
    m_gritAmount  = juce::jlimit (0.0f, 1.0f, p.character.grit);
    m_snapAmount  = juce::jlimit (0.0f, 1.0f, p.character.snap);

    const auto tunedHz = [&] (float hz) -> double
    {
        const auto tuneRatio = std::pow (2.0, static_cast<double> (p.out.tune) / 12.0);
        return static_cast<double> (hz) * tuneRatio;
    };

    // --- Pre-compute decay coefficients ---
    m_toneDecayCoeff      = decayCoeff (p.tone.decay);
    m_tonePitchDecayCoeff = decayCoeff (p.tone.pitchdecay);
    m_noiseDecayCoeff     = decayCoeff (p.noise.decay);
    m_clickDecayCoeff     = decayCoeff (juce::jmax (0.0005f, p.click.decay * (1.0f - 0.35f * m_snapAmount)));
    m_metalDecayCoeff     = decayCoeff (p.metal.decay);

    // --- Filter coefficients ---
    const auto noiseHp = juce::jlimit (20.0f, 20000.0f, p.noise.hpfreq * (1.0f + 0.25f * m_snapAmount));
    const auto noiseLp = juce::jlimit (noiseHp + 50.0f, 20000.0f, p.noise.lpfreq * (1.0f - 0.15f * m_gritAmount));
    const auto metalHp = juce::jlimit (20.0f, 20000.0f, p.metal.hpfreq * (1.0f + 0.15f * m_snapAmount));
    const auto clickHp = juce::jlimit (120.0f, 18000.0f, p.click.pitch * (0.55f + 0.2f * m_snapAmount));

    m_noiseHP.setLP (noiseHp, static_cast<float> (m_sampleRate));
    m_noiseLP.setLP (noiseLp, static_cast<float> (m_sampleRate));
    m_metalHP.setLP (metalHp, static_cast<float> (m_sampleRate));
    m_clickHP.setLP (clickHp, static_cast<float> (m_sampleRate));

    // --- Metal oscillator frequencies ---
    for (int i = 0; i < 8; ++i)
    {
        const auto ratioJitter = 1.0 + static_cast<double> (m_gritAmount) * 0.01 * (i - 3);
        m_metalOscs[i].freq = tunedHz (p.metal.basefreq)
                              * static_cast<double> (p.metal.ratios[static_cast<size_t> (i)])
                              * ratioJitter;
    }

    m_clickPhaseInc = juce::MathConstants<double>::twoPi * tunedHz (p.click.pitch) / m_sampleRate;
    m_attackStep = p.tone.attack <= 0.0001f
                     ? 1.0f
                     : static_cast<float> (1.0 / (m_sampleRate * static_cast<double> (p.tone.attack)));
}

//==============================================================================
void DrumVoiceDSP::trigger (float velocity)
{
    m_velocity = velocity;
    m_active   = true;
    m_attackEnv = m_params.tone.attack <= 0.0001f ? 1.0f : 0.0f;

    // Reset noise filter state to avoid leftover transients
    m_noiseHP.reset();
    m_noiseLP.reset();
    m_clickHP.reset();
    m_metalHP.reset();
    m_pinkState = 0.0f;
    m_noiseLast = 0.0f;

    if (m_params.tone.enable)
    {
        m_tonePhase        = 0.0;
        m_toneEnv          = 1.0f;
        m_tonePitchCurrent = static_cast<double> (m_params.tone.pitch)
                             * static_cast<double> (m_params.tone.pitchstart);
        m_tonePitchTarget  = static_cast<double> (m_params.tone.pitch);
    }

    if (m_params.noise.enable)
        m_noiseEnv = 1.0f;

    if (m_params.click.enable)
    {
        m_clickEnv          = 1.0f;
        m_clickPhase        = 0.0;
        m_clickBurstCount   = 1;     // first burst already counting
        m_clickBurstSamples = 0;
        m_nextBurstAt       = static_cast<int> (m_params.click.burstspacing
                                                * m_sampleRate);
    }

    if (m_params.metal.enable)
    {
        m_metalEnv = 1.0f;
        for (auto& osc : m_metalOscs)
            osc.phase = 0.0;
    }
}

//==============================================================================
float DrumVoiceDSP::renderTone() noexcept
{
    // Exponential pitch fall from start to target
    m_tonePitchCurrent += (m_tonePitchTarget - m_tonePitchCurrent)
                          * (1.0 - m_tonePitchDecayCoeff);

    // Phase accumulator
    const double phaseInc = juce::MathConstants<double>::twoPi
                            * m_tonePitchCurrent / m_sampleRate;
    m_tonePhase += phaseInc;
    if (m_tonePhase >= juce::MathConstants<double>::twoPi)
        m_tonePhase -= juce::MathConstants<double>::twoPi;

    // Waveform
    float wave;
    if (! m_params.tone.triangleWave)
    {
        wave = static_cast<float> (std::sin (m_tonePhase));
    }
    else
    {
        // Triangle: piecewise linear
        const double t = m_tonePhase / juce::MathConstants<double>::twoPi;
        wave = (t < 0.5)
               ? static_cast<float> (t * 4.0 - 1.0)
               : static_cast<float> (3.0 - t * 4.0);
    }

    const float body = juce::jlimit (0.0f, 1.0f, m_params.tone.body);
    wave += std::sin (static_cast<float> (m_tonePhase * 2.0)) * body * 0.26f;
    wave += std::sin (static_cast<float> (m_tonePhase * 3.0 - 0.2)) * body * 0.10f;
    wave = std::tanh (wave * (1.0f + 0.35f * body));

    // Amplitude envelope (exponential decay)
    m_toneEnv *= static_cast<float> (m_toneDecayCoeff);
    return wave * m_toneEnv;
}

//==============================================================================
float DrumVoiceDSP::renderNoise() noexcept
{
    // White noise
    float noise = m_random.nextFloat() * 2.0f - 1.0f;

    // Pink noise tint — 1-pole leaky integrator blend
    if (m_params.noise.color > 0.01f)
    {
        m_pinkState = m_pinkState * 0.99f + noise * 0.01f;
        noise = noise * (1.0f - m_params.noise.color)
              + m_pinkState * m_params.noise.color * 100.0f;  // amplify pink component
    }

    // Band-pass: HP then LP
    noise = m_noiseHP.processHP (noise);
    noise = m_noiseLP.processLP (noise);

    const float crisp = noise - m_noiseLast * (0.65f - 0.25f * m_snapAmount);
    m_noiseLast = noise;
    noise = juce::jmap (m_snapAmount, noise, crisp);
    noise = juce::jmap (m_gritAmount, noise, std::tanh (noise * 1.6f));

    // Amplitude envelope
    m_noiseEnv *= static_cast<float> (m_noiseDecayCoeff);
    return noise * m_noiseEnv;
}

//==============================================================================
float DrumVoiceDSP::renderClick() noexcept
{
    // Burst retrigger logic
    if (m_clickBurstCount < m_params.click.bursts
        && m_clickBurstSamples >= m_nextBurstAt)
    {
        m_clickEnv = 1.0f;
        ++m_clickBurstCount;
        m_nextBurstAt = m_clickBurstSamples
                        + static_cast<int> (m_params.click.burstspacing * m_sampleRate);
    }
    ++m_clickBurstSamples;

    m_clickPhase += m_clickPhaseInc;
    if (m_clickPhase >= juce::MathConstants<double>::twoPi)
        m_clickPhase -= juce::MathConstants<double>::twoPi;

    const float tonal = static_cast<float> (std::sin (m_clickPhase));
    const float noisy = m_random.nextFloat() * 2.0f - 1.0f;
    float click = juce::jmap (juce::jlimit (0.0f, 1.0f, m_params.click.tone), noisy, tonal);
    click = m_clickHP.processHP (click);
    click = std::tanh (click * (1.0f + 1.5f * m_snapAmount));

    m_clickEnv *= static_cast<float> (m_clickDecayCoeff);
    return click * m_clickEnv;
}

//==============================================================================
float DrumVoiceDSP::renderMetal() noexcept
{
    const int numVoices = juce::jlimit (1, 8, m_params.metal.voices);
    float sum = 0.0f;

    for (int i = 0; i < numVoices; ++i)
    {
        m_metalOscs[i].phase += juce::MathConstants<double>::twoPi
                                * m_metalOscs[i].freq / m_sampleRate;
        if (m_metalOscs[i].phase >= juce::MathConstants<double>::twoPi)
            m_metalOscs[i].phase -= juce::MathConstants<double>::twoPi;

        sum += static_cast<float> (std::sin (m_metalOscs[i].phase));
    }
    sum /= static_cast<float> (numVoices);

    // High-pass filter to keep only the bright metallic content
    sum = m_metalHP.processHP (sum);
    sum = juce::jmap (m_snapAmount, sum, std::tanh (sum * 1.4f));

    m_metalEnv *= static_cast<float> (m_metalDecayCoeff);
    return sum * m_metalEnv;
}

float DrumVoiceDSP::applyCharacter (float sample) noexcept
{
    const float drive = 1.0f + m_driveAmount * 3.0f;
    const float gritNoise = (m_random.nextFloat() * 2.0f - 1.0f) * m_gritAmount * 0.015f;
    return std::tanh ((sample + gritNoise) * drive);
}

//==============================================================================
void DrumVoiceDSP::renderBlock (juce::AudioBuffer<float>& buf,
                                 int startSample, int numSamples) noexcept
{
    if (! m_active)
        return;

    const int numCh = juce::jmin (buf.getNumChannels(), 2);
    if (numCh == 0) return;

    const float pan  = m_params.out.pan;
    const float lvl  = m_params.out.level;

    // Velocity scaling: blend between full level and velocity-proportional level
    const float velScale = 1.0f - m_params.out.velocity
                           + m_params.out.velocity * m_velocity;

    for (int i = 0; i < numSamples; ++i)
    {
        // --- Render each enabled layer ---
        float tone  = m_params.tone.enable
                      ? renderTone()  * m_params.tone.level   : 0.0f;
        float noise = m_params.noise.enable
                      ? renderNoise() * m_params.noise.level  : 0.0f;
        float click = m_params.click.enable
                      ? renderClick() * m_params.click.level  : 0.0f;
        float metal = m_params.metal.enable
                      ? renderMetal() * m_params.metal.level  : 0.0f;

        m_attackEnv = juce::jmin (1.0f, m_attackEnv + m_attackStep);

        float sum = (tone + noise + click + metal) * m_attackEnv * velScale * lvl;
        sum = applyCharacter (sum);

        // Pan (constant-power-ish linear pan)
        const float leftGain  = 1.0f - juce::jmax (0.0f,  pan);
        const float rightGain = 1.0f + juce::jmin (0.0f, pan);

        const int si = startSample + i;
        if (numCh >= 1) buf.getWritePointer (0)[si] += sum * leftGain;
        if (numCh >= 2) buf.getWritePointer (1)[si] += sum * rightGain;
    }

    // Update active flag — voice is done when all envelopes are inaudible
    constexpr float kThreshold = 0.00001f;
    m_active = (m_toneEnv  > kThreshold)
            || (m_noiseEnv > kThreshold)
            || (m_metalEnv > kThreshold)
            || (m_clickEnv > kThreshold);
}

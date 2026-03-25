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
    m_pinkState  = 0.0f;
    m_noiseHP.reset();
    m_noiseLP.reset();
    m_metalHP.reset();
}

//==============================================================================
void DrumVoiceDSP::applyParams (const DrumVoiceParams& p)
{
    m_params = p;

    // --- Pre-compute decay coefficients ---
    m_toneDecayCoeff      = decayCoeff (p.tone.decay);
    m_tonePitchDecayCoeff = decayCoeff (p.tone.pitchdecay);
    m_noiseDecayCoeff     = decayCoeff (p.noise.decay);
    m_clickDecayCoeff     = decayCoeff (p.click.decay);
    m_metalDecayCoeff     = decayCoeff (p.metal.decay);

    // --- Filter coefficients ---
    m_noiseHP.setLP (p.noise.hpfreq, static_cast<float> (m_sampleRate));
    m_noiseLP.setLP (p.noise.lpfreq, static_cast<float> (m_sampleRate));
    m_metalHP.setLP (p.metal.hpfreq, static_cast<float> (m_sampleRate));

    // --- Metal oscillator frequencies ---
    // osc[i].freq = basefreq * ratios[i]
    for (int i = 0; i < 8; ++i)
        m_metalOscs[i].freq = static_cast<double> (p.metal.basefreq)
                               * static_cast<double> (p.metal.ratios[i]);
}

//==============================================================================
void DrumVoiceDSP::trigger (float velocity)
{
    m_velocity = velocity;
    m_active   = true;

    // Reset noise filter state to avoid leftover transients
    m_noiseHP.reset();
    m_noiseLP.reset();
    m_metalHP.reset();
    m_pinkState = 0.0f;

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

    // Tonal (using tone phase for a pitched transient) or noise burst
    float click;
    if (m_params.click.tone > 0.5f)
        click = static_cast<float> (std::sin (m_tonePhase * 4.0));
    else
        click = m_random.nextFloat() * 2.0f - 1.0f;

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

    m_metalEnv *= static_cast<float> (m_metalDecayCoeff);
    return sum * m_metalEnv;
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

        float sum = (tone + noise + click + metal) * velScale * lvl;

        // Soft clip to prevent overload when many voices sum together
        sum = std::tanh (sum * 0.8f);

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

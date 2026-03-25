#include "DrumMachineProcessor.h"

//==============================================================================
void DrumMachineProcessor::prepare (double sampleRate, int /*blockSize*/)
{
    m_sampleRate = sampleRate;

    for (int i = 0; i < MAX_VOICES; ++i)
    {
        m_voices[i].prepare (sampleRate);
        if (i < m_numVoices)
            m_voices[i].applyParams (m_params[i]);
    }
}

//==============================================================================
void DrumMachineProcessor::process (juce::AudioBuffer<float>& audio,
                                     const juce::MidiBuffer&   midi) noexcept
{
    applyPendingParams();
    processMidi (midi);

    const int numSamples = audio.getNumSamples();
    for (int i = 0; i < m_numVoices; ++i)
    {
        if (m_muted[i]) continue;
        m_voices[i].renderBlock (audio, 0, numSamples);
    }
}

//==============================================================================
void DrumMachineProcessor::applyPendingParams() noexcept
{
    for (int i = 0; i < m_numVoices; ++i)
    {
        if (m_pending[i].dirty)
        {
            m_params[i]  = m_pending[i].params;
            m_voices[i].applyParams (m_params[i]);
            m_pending[i].dirty = false;
        }
    }
}

void DrumMachineProcessor::processMidi (const juce::MidiBuffer& midi) noexcept
{
    for (const auto& event : midi)
    {
        const auto msg = event.getMessage();

        if (msg.isNoteOn() && msg.getVelocity() > 0)
        {
            const int   note = msg.getNoteNumber();
            const float vel  = msg.getFloatVelocity();

            for (int i = 0; i < m_numVoices; ++i)
            {
                if (m_midiNotes[i] == note && ! m_muted[i])
                {
                    m_voices[i].trigger (vel);
                }
            }
        }
    }
}

//==============================================================================
int DrumMachineProcessor::addVoice (const DrumVoiceParams& params,
                                     const juce::String&   name,
                                     int                   midiNote)
{
    if (m_numVoices >= MAX_VOICES)
        return -1;

    const int idx    = m_numVoices++;
    m_params   [idx] = params;
    m_names    [idx] = name;
    m_midiNotes[idx] = midiNote;
    m_muted    [idx] = false;
    m_pending  [idx].dirty = false;

    m_voices[idx].prepare (m_sampleRate);
    m_voices[idx].applyParams (params);

    return idx;
}

void DrumMachineProcessor::setVoiceParams (int voiceIndex, const DrumVoiceParams& params)
{
    if (voiceIndex < 0 || voiceIndex >= m_numVoices)
        return;

    // Write to pending — will be applied at the start of the next audio block
    m_pending[voiceIndex].params = params;
    m_pending[voiceIndex].dirty  = true;
}

void DrumMachineProcessor::removeVoice (int voiceIndex)
{
    if (voiceIndex < 0 || voiceIndex >= m_numVoices)
        return;

    // Shift remaining voices down
    for (int i = voiceIndex; i < m_numVoices - 1; ++i)
    {
        m_params   [i] = m_params   [i + 1];
        m_names    [i] = m_names    [i + 1];
        m_midiNotes[i] = m_midiNotes[i + 1];
        m_muted    [i] = m_muted    [i + 1];
        m_pending  [i] = m_pending  [i + 1];
        m_voices   [i].prepare   (m_sampleRate);
        m_voices   [i].applyParams (m_params[i]);
    }
    --m_numVoices;
}

void DrumMachineProcessor::loadKit (const DrumKitPatch& kit)
{
    m_numVoices = 0;
    for (auto& entry : kit.voices)
    {
        if (m_numVoices >= MAX_VOICES)
            break;
        addVoice (entry.params, juce::String (entry.name), entry.params.midiNote);
    }
}

//==============================================================================
void DrumMachineProcessor::triggerVoice (int voiceIndex, float velocity) noexcept
{
    if (voiceIndex < 0 || voiceIndex >= m_numVoices)
        return;
    if (m_muted[voiceIndex])
        return;

    m_voices[voiceIndex].trigger (velocity);
}

void DrumMachineProcessor::setVoiceMuted (int index, bool muted) noexcept
{
    if (index >= 0 && index < MAX_VOICES)
        m_muted[index] = muted;
}

bool DrumMachineProcessor::isVoiceMuted (int index) const noexcept
{
    if (index < 0 || index >= MAX_VOICES)
        return false;
    return m_muted[index];
}

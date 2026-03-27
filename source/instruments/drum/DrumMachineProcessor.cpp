#include "DrumMachineProcessor.h"

namespace
{
constexpr juce::uint32 kDrumStateMagic   = 0x4452554Du; // "DRUM"
constexpr juce::uint32 kDrumStateVersion = 3u;

DrumVoiceRole roleFromLegacyVoice (const std::string& name, int midiNote, int index)
{
    const auto upper = juce::String (name).toUpperCase();

    if (upper.contains ("SUB"))                    return DrumVoiceRole::subKick;
    if (upper.contains ("SNARE"))                  return DrumVoiceRole::snare;
    if (upper == "HAT" || upper.contains ("CH")
        || upper.contains ("OH"))                  return DrumVoiceRole::hat;
    if (upper.contains ("CLAP"))                   return DrumVoiceRole::clap;
    if (upper.contains ("CYM"))                    return DrumVoiceRole::cymbal;
    if (upper.contains ("TOM"))                    return DrumVoiceRole::tom;
    if (upper.contains ("PERC 2"))                 return DrumVoiceRole::perc2;
    if (upper.contains ("PERC"))                   return DrumVoiceRole::perc1;
    if (upper.contains ("KICK"))                   return DrumVoiceRole::kick;
    if (midiNote == 35)                            return DrumVoiceRole::subKick;
    if (midiNote == 38)                            return DrumVoiceRole::snare;
    if (midiNote == 42 || midiNote == 46)          return DrumVoiceRole::hat;
    if (midiNote == 56)                            return DrumVoiceRole::perc2;
    if (index == 1)                                return DrumVoiceRole::subKick;
    if (index == 2)                                return DrumVoiceRole::snare;
    if (index == 3)                                return DrumVoiceRole::hat;
    if (index == 5)                                return DrumVoiceRole::perc2;
    return index >= 4 ? DrumVoiceRole::perc1 : DrumVoiceRole::kick;
}
}

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

void DrumMachineProcessor::applyState (const DrumModuleState& state)
{
    m_numVoices = 0;

    for (const auto& voice : state.voices)
    {
        if (m_numVoices >= MAX_VOICES)
            break;

        addVoice (voice.params, juce::String (voice.name), voice.midiNote);
        setVoiceMuted (m_numVoices - 1, voice.muted);
    }
}

DrumModuleState DrumMachineProcessor::captureState() const
{
    DrumModuleState state;
    state.name = "DSP Drum State";
    state.voices.reserve (static_cast<size_t> (m_numVoices));

    for (int i = 0; i < m_numVoices; ++i)
    {
        DrumVoiceState voice;
        voice.name      = m_names[i].toStdString();
        voice.params    = m_params[i];
        voice.midiNote  = m_midiNotes[i];
        voice.muted     = m_muted[i];
        voice.params.midiNote = voice.midiNote;
        state.voices.push_back (voice);
    }

    state.clamp();
    return state;
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

void DrumMachineProcessor::getState (juce::MemoryBlock& destData) const
{
    const auto state = captureState();

    juce::MemoryOutputStream s (destData, false);
    s.writeInt (static_cast<int> (kDrumStateMagic));
    s.writeInt (static_cast<int> (kDrumStateVersion));
    s.writeInt (static_cast<int> (state.voices.size()));

    for (const auto& voice : state.voices)
    {
        s.writeString (juce::String (voice.name));
        s.writeInt    (voice.midiNote);
        s.writeBool   (voice.muted);

        const auto& p = voice.params;
        s.writeBool  (p.tone.enable);
        s.writeFloat (p.tone.pitch);
        s.writeFloat (p.tone.pitchstart);
        s.writeFloat (p.tone.pitchdecay);
        s.writeFloat (p.tone.attack);
        s.writeFloat (p.tone.decay);
        s.writeFloat (p.tone.level);
        s.writeBool  (p.tone.triangleWave);
        s.writeFloat (p.tone.body);

        s.writeBool  (p.noise.enable);
        s.writeFloat (p.noise.decay);
        s.writeFloat (p.noise.level);
        s.writeFloat (p.noise.hpfreq);
        s.writeFloat (p.noise.lpfreq);
        s.writeFloat (p.noise.color);

        s.writeBool  (p.click.enable);
        s.writeFloat (p.click.level);
        s.writeFloat (p.click.decay);
        s.writeFloat (p.click.tone);
        s.writeFloat (p.click.pitch);
        s.writeInt   (p.click.bursts);
        s.writeFloat (p.click.burstspacing);

        s.writeBool  (p.metal.enable);
        s.writeInt   (p.metal.voices);
        s.writeFloat (p.metal.basefreq);
        for (float ratio : p.metal.ratios)
            s.writeFloat (ratio);
        s.writeFloat (p.metal.decay);
        s.writeFloat (p.metal.level);
        s.writeFloat (p.metal.hpfreq);

        s.writeInt   (static_cast<int> (p.role));
        s.writeFloat (p.character.drive);
        s.writeFloat (p.character.grit);
        s.writeFloat (p.character.snap);

        s.writeFloat (p.out.level);
        s.writeFloat (p.out.pan);
        s.writeFloat (p.out.tune);
        s.writeFloat (p.out.velocity);
    }
}

bool DrumMachineProcessor::setState (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes < 12)
        return false;

    juce::MemoryInputStream s (data, static_cast<size_t> (sizeInBytes), false);
    if (static_cast<juce::uint32> (s.readInt()) != kDrumStateMagic)
        return false;
    const auto version = static_cast<juce::uint32> (s.readInt());
    if (version == 0 || version > kDrumStateVersion)
        return false;

    DrumModuleState state;
    const int numVoices = juce::jlimit (0, MAX_VOICES, s.readInt());
    state.voices.reserve (static_cast<size_t> (numVoices));

    for (int i = 0; i < numVoices; ++i)
    {
        DrumVoiceState voice;
        voice.name     = s.readString().toStdString();
        voice.midiNote = s.readInt();
        voice.muted    = s.readBool();

        auto& p = voice.params;
        p.tone.enable       = s.readBool();
        p.tone.pitch        = s.readFloat();
        p.tone.pitchstart   = s.readFloat();
        p.tone.pitchdecay   = s.readFloat();
        if (version >= 3)
            p.tone.attack   = s.readFloat();
        else
            p.tone.attack   = 0.0f;
        p.tone.decay        = s.readFloat();
        p.tone.level        = s.readFloat();
        p.tone.triangleWave = s.readBool();
        if (version >= 2)
            p.tone.body = s.readFloat();

        p.noise.enable = s.readBool();
        p.noise.decay  = s.readFloat();
        p.noise.level  = s.readFloat();
        p.noise.hpfreq = s.readFloat();
        p.noise.lpfreq = s.readFloat();
        p.noise.color  = s.readFloat();

        p.click.enable       = s.readBool();
        p.click.level        = s.readFloat();
        p.click.decay        = s.readFloat();
        p.click.tone         = s.readFloat();
        if (version >= 2)
            p.click.pitch    = s.readFloat();
        p.click.bursts       = s.readInt();
        p.click.burstspacing = s.readFloat();

        p.metal.enable   = s.readBool();
        p.metal.voices   = s.readInt();
        p.metal.basefreq = s.readFloat();
        for (float& ratio : p.metal.ratios)
            ratio = s.readFloat();
        p.metal.decay  = s.readFloat();
        p.metal.level  = s.readFloat();
        p.metal.hpfreq = s.readFloat();

        if (version >= 2)
        {
            p.role            = static_cast<DrumVoiceRole> (s.readInt());
            p.character.drive = s.readFloat();
            p.character.grit  = s.readFloat();
            p.character.snap  = s.readFloat();
        }
        else
        {
            p.role = roleFromLegacyVoice (voice.name, voice.midiNote, i);
            const auto factory = DrumVoiceParams::makeForRole (p.role);
            p.tone.body        = factory.tone.body;
            p.click.pitch      = factory.click.pitch;
            p.character        = factory.character;
        }

        p.out.level    = s.readFloat();
        p.out.pan      = s.readFloat();
        p.out.tune     = s.readFloat();
        p.out.velocity = s.readFloat();
        p.midiNote     = voice.midiNote;

        state.voices.push_back (voice);
    }

    state.clamp();
    applyState (state);
    return true;
}

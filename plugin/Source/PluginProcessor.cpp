#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace TurboTuning
{

// ==========================================================
//  Parameter Layout
// ==========================================================

juce::AudioProcessorValueTreeState::ParameterLayout TurboTuningProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master volume
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("masterVol", 1), "Master Volume",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f));

    // EQ bands
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("eqLow", 1), "EQ Low",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("eqMid", 1), "EQ Mid",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("eqHigh", 1), "EQ High",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("eqPres", 1), "EQ Presence",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

    // Effects
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("reverb", 1), "Reverb",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 25.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("delay", 1), "Delay",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("chorus", 1), "Chorus",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

    // Toggles
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("safeMode", 1), "Safe Mode", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("autoTune", 1), "Auto-Tune", true));

    // Instrument (as integer)
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID ("instrument", 1), "Instrument",
        0, (int)InstrumentType::NumInstruments - 1, 0));

    // Key (index into KEYS array)
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID ("keyIndex", 1), "Key",
        0, 11, 0));

    return { params.begin(), params.end() };
}

// ==========================================================
//  Constructor / Destructor
// ==========================================================

TurboTuningProcessor::TurboTuningProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TurboTuningParams", createParameterLayout())
{
}

TurboTuningProcessor::~TurboTuningProcessor() {}

// ==========================================================
//  Prepare / Release
// ==========================================================

void TurboTuningProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        harmonicaVoices[i].prepare (sampleRate, samplesPerBlock);
        brassVoices[i].prepare (sampleRate, samplesPerBlock);
        stringVoices[i].prepare (sampleRate, samplesPerBlock);
        woodwindVoices[i].prepare (sampleRate, samplesPerBlock);
    }

    effectsChain.prepare (sampleRate, samplesPerBlock);
    safeMode.prepare (sampleRate, samplesPerBlock);
}

void TurboTuningProcessor::releaseResources()
{
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        harmonicaVoices[i].reset();
        brassVoices[i].reset();
        stringVoices[i].reset();
        woodwindVoices[i].reset();
    }

    effectsChain.reset();
    safeMode.reset();
}

// ==========================================================
//  Editor
// ==========================================================

juce::AudioProcessorEditor* TurboTuningProcessor::createEditor()
{
    return new TurboTuningEditor (*this);
}

// ==========================================================
//  Process Block
// ==========================================================

void TurboTuningProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Handle MIDI
    for (const auto metadata : midiMessages)
        handleMidiMessage (metadata.getMessage());

    // Read parameter values
    masterVolume = apvts.getRawParameterValue ("masterVol")->load() / 100.0f;
    setSafeModeEnabled (apvts.getRawParameterValue ("safeMode")->load() > 0.5f);
    setAutoTuneEnabled (apvts.getRawParameterValue ("autoTune")->load() > 0.5f);

    effectsChain.setReverbLevel (apvts.getRawParameterValue ("reverb")->load());
    effectsChain.setDelayLevel (apvts.getRawParameterValue ("delay")->load());
    effectsChain.setChorusLevel (apvts.getRawParameterValue ("chorus")->load());
    effectsChain.setEQ ("low",      apvts.getRawParameterValue ("eqLow")->load());
    effectsChain.setEQ ("mid",      apvts.getRawParameterValue ("eqMid")->load());
    effectsChain.setEQ ("high",     apvts.getRawParameterValue ("eqHigh")->load());
    effectsChain.setEQ ("presence", apvts.getRawParameterValue ("eqPres")->load());

    int keyIdx = (int) apvts.getRawParameterValue ("keyIndex")->load();
    if (keyIdx >= 0 && keyIdx < 12)
        currentKey = MusicTheory::KEYS[(size_t)keyIdx];

    int instIdx = (int) apvts.getRawParameterValue ("instrument")->load();
    currentInstrument = (InstrumentType) instIdx;

    // ── Render voices to mono channel 0 ──
    const int numSamples = buffer.getNumSamples();

    // We render into a mono temp buffer, then copy to all channels
    juce::AudioBuffer<float> monoBuffer (1, numSamples);
    monoBuffer.clear();

    for (int i = 0; i < NUM_VOICES; ++i)
    {
        switch (currentInstrument)
        {
            case InstrumentType::Harmonica:
                if (harmonicaVoices[i].isActive())
                    harmonicaVoices[i].renderBlock (monoBuffer, 0, numSamples);
                break;

            case InstrumentType::Trumpet:
            case InstrumentType::Tuba:
            case InstrumentType::FrenchHorn:
                if (brassVoices[i].isActive())
                    brassVoices[i].renderBlock (monoBuffer, 0, numSamples);
                break;

            case InstrumentType::Guitar:
            case InstrumentType::Harp:
            case InstrumentType::Violin:
                if (stringVoices[i].isActive())
                    stringVoices[i].renderBlock (monoBuffer, 0, numSamples);
                break;

            case InstrumentType::Flute:
            case InstrumentType::Clarinet:
            case InstrumentType::PanFlute:
                if (woodwindVoices[i].isActive())
                    woodwindVoices[i].renderBlock (monoBuffer, 0, numSamples);
                break;

            default:
                break;
        }
    }

    // ── Effects chain (mono) ──
    effectsChain.process (monoBuffer);

    // ── Safe Mode (compressor) ──
    safeMode.process (monoBuffer);

    // ── Apply master volume and copy to stereo ──
    monoBuffer.applyGain (masterVolume);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, monoBuffer, 0, 0, numSamples);
}

// ==========================================================
//  MIDI Handling
// ==========================================================

void TurboTuningProcessor::handleMidiMessage (const juce::MidiMessage& msg)
{
    if (msg.isNoteOn())
    {
        int note = msg.getNoteNumber();
        float vel = msg.getFloatVelocity();

        // MIDI mapping: notes 60..69 = blow holes 1..10
        //               notes 70..79 = draw holes 1..10
        if (note >= 60 && note <= 69)
            playHole (note - 59, true, vel);
        else if (note >= 70 && note <= 79)
            playHole (note - 69, false, vel);
    }
    else if (msg.isNoteOff())
    {
        int note = msg.getNoteNumber();
        if (note >= 60 && note <= 69)
            stopHole (note - 59, true);
        else if (note >= 70 && note <= 79)
            stopHole (note - 69, false);
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        stopAll();
    }
}

// ==========================================================
//  Play / Stop Holes
// ==========================================================

void TurboTuningProcessor::playHole (int hole, bool isBlow, float velocity)
{
    if (hole < 1 || hole > 10) return;

    int midiNote = MusicTheory::getNote (hole, isBlow, currentKey);
    float freq = MusicTheory::midiToFreq (midiNote);

    // Auto-tune correction
    if (autoTuner.isEnabled())
        freq = autoTuner.correctPitch (freq);

    int idx = voiceIndex (hole, isBlow);

    switch (currentInstrument)
    {
        case InstrumentType::Harmonica:
            harmonicaVoices[idx].noteOn (freq, velocity, isBlow);
            break;

        case InstrumentType::Trumpet:
            brassVoices[idx].setType (BrassEngine::Type::Trumpet);
            brassVoices[idx].noteOn (freq, velocity);
            break;
        case InstrumentType::Tuba:
            brassVoices[idx].setType (BrassEngine::Type::Tuba);
            brassVoices[idx].noteOn (freq, velocity);
            break;
        case InstrumentType::FrenchHorn:
            brassVoices[idx].setType (BrassEngine::Type::FrenchHorn);
            brassVoices[idx].noteOn (freq, velocity);
            break;

        case InstrumentType::Guitar:
            stringVoices[idx].setType (StringEngine::Type::Guitar);
            stringVoices[idx].noteOn (freq, velocity);
            break;
        case InstrumentType::Harp:
            stringVoices[idx].setType (StringEngine::Type::Harp);
            stringVoices[idx].noteOn (freq, velocity);
            break;
        case InstrumentType::Violin:
            stringVoices[idx].setType (StringEngine::Type::Violin);
            stringVoices[idx].noteOn (freq, velocity);
            break;

        case InstrumentType::Flute:
            woodwindVoices[idx].setType (WoodwindEngine::Type::Flute);
            woodwindVoices[idx].noteOn (freq, velocity);
            break;
        case InstrumentType::Clarinet:
            woodwindVoices[idx].setType (WoodwindEngine::Type::Clarinet);
            woodwindVoices[idx].noteOn (freq, velocity);
            break;
        case InstrumentType::PanFlute:
            woodwindVoices[idx].setType (WoodwindEngine::Type::PanFlute);
            woodwindVoices[idx].noteOn (freq, velocity);
            break;

        default:
            break;
    }
}

void TurboTuningProcessor::stopHole (int hole, bool isBlow)
{
    if (hole < 1 || hole > 10) return;

    int idx = voiceIndex (hole, isBlow);

    harmonicaVoices[idx].noteOff();
    brassVoices[idx].noteOff();
    stringVoices[idx].noteOff();
    woodwindVoices[idx].noteOff();
}

void TurboTuningProcessor::stopAll()
{
    for (int i = 0; i < NUM_VOICES; ++i)
    {
        harmonicaVoices[i].noteOff();
        brassVoices[i].noteOff();
        stringVoices[i].noteOff();
        woodwindVoices[i].noteOff();
    }
}

// ==========================================================
//  Key / Instrument / Volume
// ==========================================================

void TurboTuningProcessor::setKey (const juce::String& key)
{
    currentKey = key;
    autoTuner.setKey (key);
    stopAll();

    // Sync with APVTS
    for (int i = 0; i < 12; ++i)
    {
        if (MusicTheory::KEYS[(size_t)i] == key)
        {
            if (auto* param = apvts.getParameter ("keyIndex"))
                param->setValueNotifyingHost (param->convertTo0to1 ((float)i));
            break;
        }
    }
}

void TurboTuningProcessor::setInstrument (InstrumentType inst)
{
    currentInstrument = inst;
    stopAll();

    if (auto* param = apvts.getParameter ("instrument"))
        param->setValueNotifyingHost (param->convertTo0to1 ((float)(int)inst));
}

void TurboTuningProcessor::setEQ (const juce::String& band, float value)
{
    effectsChain.setEQ (band, value);
}

void TurboTuningProcessor::setMasterVolume (float v)
{
    if (auto* param = apvts.getParameter ("masterVol"))
        param->setValueNotifyingHost (param->convertTo0to1 (v));
}

// ==========================================================
//  State Save / Load
// ==========================================================

void TurboTuningProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TurboTuningProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr)
    {
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

} // namespace TurboTuning

// ==========================================================
//  JUCE Plugin Entry Point
// ==========================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TurboTuning::TurboTuningProcessor();
}

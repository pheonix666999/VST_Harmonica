#pragma once
#include <JuceHeader.h>
#include "DSP/HarmonicaEngine.h"
#include "DSP/BrassEngine.h"
#include "DSP/StringEngine.h"
#include "DSP/WoodwindEngine.h"
#include "DSP/EffectsChain.h"
#include "DSP/SafeMode.h"
#include "DSP/AutoTuner.h"
#include "Util/MusicTheory.h"

namespace TurboTuning
{

/**
 * TurboTuningProcessor — Main audio processor for the plugin.
 *
 * Manages:
 *   • 10 voices (one per harmonica hole), each with its own engine instance
 *   • Key selection via Circle of Fifths
 *   • Instrument switching (Harmonica/Brass/Strings/Woodwinds)
 *   • EQ, Effects, Safe Mode, Auto-Tune parameters
 *   • MIDI input for hole triggering
 */
class TurboTuningProcessor : public juce::AudioProcessor
{
public:
    TurboTuningProcessor();
    ~TurboTuningProcessor() override;

    // AudioProcessor overrides
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Turbo Tuning"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int /*index*/) override {}
    const juce::String getProgramName (int /*index*/) override { return {}; }
    void changeProgramName (int /*index*/, const juce::String& /*name*/) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ==========================================================
    //  Public API for Editor
    // ==========================================================

    // Key
    void setKey (const juce::String& key);
    juce::String getCurrentKey() const { return currentKey; }

    // Instrument
    void setInstrument (InstrumentType inst);
    InstrumentType getCurrentInstrument() const { return currentInstrument; }

    // Play / stop holes (UI-triggered)
    void playHole (int hole, bool isBlow, float velocity = 0.8f);
    void stopHole (int hole, bool isBlow);
    void stopAll();

    // EQ
    void setEQ (const juce::String& band, float value);

    // Effects
    void setReverbLevel (float v)  { effectsChain.setReverbLevel (v); }
    void setDelayLevel (float v)   { effectsChain.setDelayLevel (v); }
    void setChorusLevel (float v)  { effectsChain.setChorusLevel (v); }

    // Safe Mode
    void setSafeModeEnabled (bool on) { safeMode.setEnabled (on); }
    bool isSafeModeEnabled() const    { return safeMode.isEnabled(); }

    // Auto-Tune
    void setAutoTuneEnabled (bool on) { autoTuner.setEnabled (on); }
    bool isAutoTuneEnabled() const    { return autoTuner.isEnabled(); }

    // Master Volume (0..100)
    void setMasterVolume (float v);

    // Parameter tree for DAW automation
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    // 10 harmonica holes, each can play blow OR draw
    // We use 20 voice slots: [0..9] = blow, [10..19] = draw
    static constexpr int NUM_HOLES = 10;
    static constexpr int NUM_VOICES = 20;

    // One engine instance per voice
    HarmonicaEngine harmonicaVoices[NUM_VOICES];
    BrassEngine     brassVoices[NUM_VOICES];
    StringEngine    stringVoices[NUM_VOICES];
    WoodwindEngine  woodwindVoices[NUM_VOICES];

    // Effects
    EffectsChain effectsChain;
    SafeMode     safeMode;
    AutoTuner    autoTuner;

    // State
    juce::String    currentKey = "C";
    InstrumentType  currentInstrument = InstrumentType::Harmonica;
    float           masterVolume = 0.7f;

    // MIDI mapping: MIDI notes 60..79 map to holes 1..10 (blow/draw)
    void handleMidiMessage (const juce::MidiMessage& msg);

    // Voice index helpers
    int voiceIndex (int hole, bool isBlow) const { return isBlow ? (hole - 1) : (hole - 1 + NUM_HOLES); }

    // Parameter tree
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TurboTuningProcessor)
};

} // namespace TurboTuning

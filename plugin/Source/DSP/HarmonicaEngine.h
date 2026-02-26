#pragma once
#include <JuceHeader.h>
#include "../Util/MusicTheory.h"

namespace TurboTuning
{

/**
 * HarmonicaEngine — Core reed-instrument synthesis.
 *
 * Produces sound using:
 *   • Sawtooth + slightly detuned square for reed buzz
 *   • Sine sub oscillator for body
 *   • Bandpass-filtered noise for breath
 *   • Lowpass filter with blow/draw cutoff variation
 *   • ADSR envelope
 */
class HarmonicaEngine
{
public:
    HarmonicaEngine();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    /** Start a note. velocity 0..1, isBlow selects blow/draw filter character. */
    void noteOn (float frequency, float velocity, bool isBlow);
    void noteOff();

    /** Render next block into the buffer (mono, adds to existing content). */
    void renderBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    bool isActive() const { return active; }

private:
    double sampleRate = 44100.0;
    bool   active = false;

    // Oscillators
    float phase1 = 0.0f;   // sawtooth
    float phase2 = 0.0f;   // square (slightly detuned)
    float phase3 = 0.0f;   // sine sub
    float freq   = 440.0f;

    // Noise
    juce::Random noiseRng;

    // Lowpass filter (IIR biquad)
    juce::dsp::IIR::Filter<float> lpFilter;

    // Envelope (ADSR)
    juce::ADSR envelope;
    juce::ADSR::Parameters blowParams;
    juce::ADSR::Parameters drawParams;

    // Mix gains
    float gainSaw    = 0.28f;
    float gainSquare = 0.12f;
    float gainSub    = 0.22f;
    float gainNoise  = 0.04f;
    float velocity   = 0.8f;
    bool  currentBlow = true;
};

} // namespace TurboTuning

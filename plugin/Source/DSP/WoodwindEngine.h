#pragma once
#include <JuceHeader.h>

namespace TurboTuning
{

/**
 * WoodwindEngine — Flute, Clarinet, Pan Flute synthesis.
 *
 * Flute:     Pure sine + octave harmonic + bandpass-filtered breath noise
 * Clarinet:  Odd-harmonic additive (square + sine partials 3,5,7,9) through lowpass
 * Pan Flute: Sine + triangle octave + wider breath noise
 */
class WoodwindEngine
{
public:
    enum class Type { Flute = 0, Clarinet, PanFlute };

    WoodwindEngine();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void setType (Type t) { type = t; }
    void noteOn (float frequency, float velocity);
    void noteOff();
    void renderBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    bool isActive() const { return active; }

private:
    double sampleRate = 44100.0;
    bool   active = false;
    Type   type = Type::Flute;
    float  velocity = 0.8f;

    // Oscillators (up to 5 for clarinet)
    static constexpr int MAX_OSCS = 5;
    float phases[MAX_OSCS] = {};
    float oscFreqs[MAX_OSCS] = {};
    float oscGains[MAX_OSCS] = {};
    bool  oscIsSquare[MAX_OSCS] = {};
    int   numOscs = 0;

    // Noise
    juce::Random noiseRng;
    float noiseGain = 0.0f;

    // Bandpass filter for noise
    juce::dsp::IIR::Filter<float> noiseBP;
    // Lowpass for clarinet tone
    juce::dsp::IIR::Filter<float> toneLP;
    bool  useToneLP = false;

    // Envelope
    juce::ADSR envelope;
};

} // namespace TurboTuning

#pragma once
#include <JuceHeader.h>

namespace TurboTuning
{

/**
 * BrassEngine — Sawtooth-harmonic additive synthesis with "bloom" filter.
 *
 * Supports Trumpet, Tuba, and French Horn variants.
 * Each has unique harmonic structure, filter cutoff, and attack time.
 */
class BrassEngine
{
public:
    enum class Type { Trumpet = 0, Tuba, FrenchHorn };

    BrassEngine();

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
    Type   type = Type::Trumpet;

    // Up to 7 harmonic oscillators
    static constexpr int MAX_HARMONICS = 7;
    float phases[MAX_HARMONICS] = {};
    float harmonicFreqs[MAX_HARMONICS] = {};
    float harmonicGains[MAX_HARMONICS] = {};
    int   numHarmonics = 0;

    float velocity = 0.8f;

    // "Bloom" filter — opens up during attack
    juce::dsp::IIR::Filter<float> lpFilter;
    float filterTarget = 4500.0f;
    float filterCurrent = 400.0f;
    float filterSmoothCoeff = 0.0f;

    // Envelope
    juce::ADSR envelope;
};

} // namespace TurboTuning

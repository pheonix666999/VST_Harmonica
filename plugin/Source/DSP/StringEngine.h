#pragma once
#include <JuceHeader.h>

namespace TurboTuning
{

/**
 * StringEngine — Triangle / sawtooth oscillators with harmonic partials.
 *
 * Guitar: sharp pluck with moderate decay
 * Harp:   bright pluck with gentle sustain
 * Violin: bowed, slower attack, sustained
 */
class StringEngine
{
public:
    enum class Type { Guitar = 0, Harp, Violin };

    StringEngine();

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
    Type   type = Type::Guitar;
    float  velocity = 0.8f;

    // Fundamental + detuned copy (violin only) + 3 harmonics = up to 5 oscs
    static constexpr int MAX_OSCS = 5;
    float  phases[MAX_OSCS] = {};
    float  oscFreqs[MAX_OSCS] = {};
    float  oscGains[MAX_OSCS] = {};
    bool   oscIsSaw[MAX_OSCS] = {};
    int    numOscs = 0;

    // Filter
    juce::dsp::IIR::Filter<float> lpFilter;
    float filterStart = 5000.0f;
    float filterEnd   = 1500.0f;
    float filterCurrent = 5000.0f;
    float filterSmoothCoeff = 0.0f;
    bool  filterSweeps = false;

    // Envelope
    juce::ADSR envelope;
};

} // namespace TurboTuning

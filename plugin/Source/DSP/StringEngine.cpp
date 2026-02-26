#include "StringEngine.h"

namespace TurboTuning
{

StringEngine::StringEngine() {}

void StringEngine::prepare (double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1;
    lpFilter.prepare (spec);

    envelope.setSampleRate (sr);
}

void StringEngine::reset()
{
    lpFilter.reset();
    envelope.reset();
    std::memset (phases, 0, sizeof (phases));
    active = false;
}

void StringEngine::noteOn (float frequency, float vel)
{
    velocity = vel;
    std::memset (phases, 0, sizeof (phases));

    juce::ADSR::Parameters envParams;
    bool useSaw = (type == Type::Violin);
    filterSweeps = false;

    numOscs = 0;

    // Fundamental
    oscFreqs[numOscs] = frequency;
    oscGains[numOscs] = 0.35f;
    oscIsSaw[numOscs] = useSaw;
    numOscs++;

    // Detuned copy for violin
    if (type == Type::Violin)
    {
        oscFreqs[numOscs] = frequency * 1.002f;
        oscGains[numOscs] = 0.15f;
        oscIsSaw[numOscs] = true;
        numOscs++;
    }

    // Harmonics
    float harmonicMults[3];
    if (type == Type::Harp)
    {
        harmonicMults[0] = 2.0f; harmonicMults[1] = 3.0f; harmonicMults[2] = 5.0f;
    }
    else
    {
        harmonicMults[0] = 2.0f; harmonicMults[1] = 3.0f; harmonicMults[2] = 4.0f;
    }

    for (int i = 0; i < 3; ++i)
    {
        oscFreqs[numOscs] = frequency * harmonicMults[i];
        oscGains[numOscs] = (0.18f / (float)(i + 1));
        oscIsSaw[numOscs] = false; // sine harmonics
        numOscs++;
    }

    // Envelope and filter per type
    switch (type)
    {
        case Type::Harp:
            envParams = { 0.008f, 0.6f, 0.30f, 1.5f };
            filterStart = 6000.0f; filterEnd = 2000.0f;
            filterSweeps = true;
            filterSmoothCoeff = 1.0f - std::exp (-1.0f / (float)(sampleRate * 1.0));
            break;

        case Type::Violin:
            envParams = { 0.12f, 0.1f, 0.90f, 0.15f };
            filterStart = 5500.0f; filterEnd = 5500.0f;
            filterSweeps = false;
            break;

        default: // Guitar
            envParams = { 0.004f, 0.35f, 0.35f, 1.0f };
            filterStart = 5000.0f; filterEnd = 1500.0f;
            filterSweeps = true;
            filterSmoothCoeff = 1.0f - std::exp (-1.0f / (float)(sampleRate * 0.5));
            break;
    }

    filterCurrent = filterStart;
    *lpFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, filterCurrent, 1.0f);
    lpFilter.reset();

    envelope.setParameters (envParams);
    envelope.noteOn();
    active = true;
}

void StringEngine::noteOff()
{
    envelope.noteOff();
}

void StringEngine::renderBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (! active) return;

    auto* out = buffer.getWritePointer (0);

    for (int i = startSample; i < startSample + numSamples; ++i)
    {
        float mixed = 0.0f;

        for (int o = 0; o < numOscs; ++o)
        {
            float sample;
            if (oscIsSaw[o])
                sample = 2.0f * phases[o] - 1.0f; // sawtooth
            else
                sample = std::sin (juce::MathConstants<float>::twoPi * phases[o]); // sine / triangle approx

            mixed += sample * oscGains[o] * velocity;

            float inc = (float)(oscFreqs[o] / sampleRate);
            phases[o] += inc;
            if (phases[o] >= 1.0f) phases[o] -= 1.0f;
        }

        // Filter sweep for plucked instruments
        if (filterSweeps)
        {
            filterCurrent += (filterEnd - filterCurrent) * filterSmoothCoeff;
            *lpFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, juce::jmax (80.0f, filterCurrent), 1.0f);
        }

        float filtered = lpFilter.processSample (mixed);
        float env = envelope.getNextSample();
        out[i] += filtered * env;
    }

    if (! envelope.isActive())
        active = false;
}

} // namespace TurboTuning

#include "WoodwindEngine.h"

namespace TurboTuning
{

WoodwindEngine::WoodwindEngine() {}

void WoodwindEngine::prepare (double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1;
    noiseBP.prepare (spec);
    toneLP.prepare (spec);

    envelope.setSampleRate (sr);
}

void WoodwindEngine::reset()
{
    noiseBP.reset();
    toneLP.reset();
    envelope.reset();
    std::memset (phases, 0, sizeof (phases));
    active = false;
}

void WoodwindEngine::noteOn (float frequency, float vel)
{
    velocity = vel;
    std::memset (phases, 0, sizeof (phases));
    numOscs = 0;
    useToneLP = false;
    noiseGain = 0.0f;

    juce::ADSR::Parameters envParams;

    switch (type)
    {
        case Type::Flute:
        {
            envParams = { 0.05f, 0.1f, 0.85f, 0.10f };

            // Pure sine fundamental
            oscFreqs[numOscs] = frequency;
            oscGains[numOscs] = 0.32f;
            oscIsSquare[numOscs] = false;
            numOscs++;

            // Octave harmonic
            oscFreqs[numOscs] = frequency * 2.0f;
            oscGains[numOscs] = 0.08f;
            oscIsSquare[numOscs] = false;
            numOscs++;

            // Breathy noise
            noiseGain = 0.06f;
            *noiseBP.coefficients = *juce::dsp::IIR::Coefficients<float>::makeBandPass (sampleRate, frequency * 1.5f, 1.5f);
            noiseBP.reset();
            break;
        }

        case Type::Clarinet:
        {
            envParams = { 0.04f, 0.1f, 0.85f, 0.08f };
            useToneLP = true;
            *toneLP.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, 3500.0f, 0.8f);
            toneLP.reset();

            // Odd harmonics: 1, 3, 5, 7, 9
            float oddH[] = { 1.0f, 3.0f, 5.0f, 7.0f, 9.0f };
            for (int i = 0; i < 5; ++i)
            {
                oscFreqs[numOscs] = frequency * oddH[i];
                oscGains[numOscs] = (0.25f / (float)(i + 1));
                oscIsSquare[numOscs] = (i == 0); // first harmonic is square
                numOscs++;
            }
            break;
        }

        case Type::PanFlute:
        {
            envParams = { 0.06f, 0.1f, 0.85f, 0.12f };

            // Sine fundamental
            oscFreqs[numOscs] = frequency;
            oscGains[numOscs] = 0.35f;
            oscIsSquare[numOscs] = false;
            numOscs++;

            // Triangle-ish octave (use sine as approx)
            oscFreqs[numOscs] = frequency * 2.0f;
            oscGains[numOscs] = 0.10f;
            oscIsSquare[numOscs] = false;
            numOscs++;

            // Wider breath noise
            noiseGain = 0.08f;
            *noiseBP.coefficients = *juce::dsp::IIR::Coefficients<float>::makeBandPass (sampleRate, frequency, 0.8f);
            noiseBP.reset();
            break;
        }
    }

    envelope.setParameters (envParams);
    envelope.noteOn();
    active = true;
}

void WoodwindEngine::noteOff()
{
    envelope.noteOff();
}

void WoodwindEngine::renderBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (! active) return;

    auto* out = buffer.getWritePointer (0);

    for (int i = startSample; i < startSample + numSamples; ++i)
    {
        float mixed = 0.0f;

        for (int o = 0; o < numOscs; ++o)
        {
            float sample;
            if (oscIsSquare[o])
                sample = phases[o] < 0.5f ? 1.0f : -1.0f;
            else
                sample = std::sin (juce::MathConstants<float>::twoPi * phases[o]);

            mixed += sample * oscGains[o] * velocity;

            float inc = (float)(oscFreqs[o] / sampleRate);
            phases[o] += inc;
            if (phases[o] >= 1.0f) phases[o] -= 1.0f;
        }

        // Apply tone lowpass (clarinet)
        if (useToneLP)
            mixed = toneLP.processSample (mixed);

        // Add filtered breath noise
        if (noiseGain > 0.0f)
        {
            float noise = noiseRng.nextFloat() * 2.0f - 1.0f;
            float filteredNoise = noiseBP.processSample (noise);
            mixed += filteredNoise * noiseGain * velocity;
        }

        float env = envelope.getNextSample();
        out[i] += mixed * env;
    }

    if (! envelope.isActive())
        active = false;
}

} // namespace TurboTuning

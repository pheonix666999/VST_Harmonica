#include "BrassEngine.h"

namespace TurboTuning
{

BrassEngine::BrassEngine() {}

void BrassEngine::prepare (double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1;
    lpFilter.prepare (spec);

    envelope.setSampleRate (sr);
}

void BrassEngine::reset()
{
    lpFilter.reset();
    envelope.reset();
    std::memset (phases, 0, sizeof (phases));
    active = false;
}

void BrassEngine::noteOn (float frequency, float vel)
{
    velocity = vel;

    float effectiveFreq = frequency;
    juce::ADSR::Parameters envParams;

    switch (type)
    {
        case Type::Tuba:
            effectiveFreq = frequency * 0.5f; // one octave lower
            numHarmonics = 5;
            { float h[] = {1,2,3,4,5};   float g[] = {0.38f,0.28f,0.14f,0.10f,0.05f}; 
              for (int i = 0; i < numHarmonics; ++i) { harmonicFreqs[i] = effectiveFreq * h[i]; harmonicGains[i] = g[i]; } }
            filterTarget = 1800.0f;
            envParams = { 0.10f, 0.15f, 0.76f, 0.12f };
            break;

        case Type::FrenchHorn:
            numHarmonics = 6;
            { float h[] = {1,2,3,4,5,6}; float g[] = {0.30f,0.22f,0.18f,0.12f,0.08f,0.05f};
              for (int i = 0; i < numHarmonics; ++i) { harmonicFreqs[i] = effectiveFreq * h[i]; harmonicGains[i] = g[i]; } }
            filterTarget = 3000.0f;
            envParams = { 0.07f, 0.15f, 0.76f, 0.10f };
            break;

        default: // Trumpet
            numHarmonics = 7;
            { float h[] = {1,2,3,4,5,6,7}; float g[] = {0.24f,0.18f,0.16f,0.13f,0.10f,0.07f,0.04f};
              for (int i = 0; i < numHarmonics; ++i) { harmonicFreqs[i] = effectiveFreq * h[i]; harmonicGains[i] = g[i]; } }
            filterTarget = 4500.0f;
            envParams = { 0.06f, 0.15f, 0.76f, 0.08f };
            break;
    }

    // Start filter at 400 Hz and bloom toward target
    filterCurrent = 400.0f;
    float bloomTimeMs = envParams.attack * 1.5f * 1000.0f;
    filterSmoothCoeff = 1.0f - std::exp (-1.0f / (float)(sampleRate * bloomTimeMs * 0.001));

    *lpFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, filterCurrent, 1.2f);
    lpFilter.reset();

    std::memset (phases, 0, sizeof (phases));

    envelope.setParameters (envParams);
    envelope.noteOn();
    active = true;
}

void BrassEngine::noteOff()
{
    envelope.noteOff();
}

void BrassEngine::renderBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (! active) return;

    auto* out = buffer.getWritePointer (0);

    for (int i = startSample; i < startSample + numSamples; ++i)
    {
        // Mix additive sawtooth harmonics
        float mixed = 0.0f;
        for (int h = 0; h < numHarmonics; ++h)
        {
            float saw = 2.0f * phases[h] - 1.0f;
            mixed += saw * harmonicGains[h] * velocity;

            float inc = (float)(harmonicFreqs[h] / sampleRate);
            phases[h] += inc;
            if (phases[h] >= 1.0f) phases[h] -= 1.0f;
        }

        // Bloom filter — smoothly open toward target
        filterCurrent += (filterTarget - filterCurrent) * filterSmoothCoeff;
        *lpFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, filterCurrent, 1.2f);

        float filtered = lpFilter.processSample (mixed);

        // Apply envelope
        float env = envelope.getNextSample();
        out[i] += filtered * env;
    }

    if (! envelope.isActive())
        active = false;
}

} // namespace TurboTuning

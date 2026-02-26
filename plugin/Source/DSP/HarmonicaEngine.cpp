#include "HarmonicaEngine.h"

namespace TurboTuning
{

HarmonicaEngine::HarmonicaEngine()
{
    // Blow envelope — slightly softer attack
    blowParams.attack  = 0.045f;
    blowParams.decay   = 0.1f;
    blowParams.sustain = 0.85f;
    blowParams.release = 0.08f;

    // Draw envelope — snappier attack
    drawParams.attack  = 0.025f;
    drawParams.decay   = 0.08f;
    drawParams.sustain = 0.90f;
    drawParams.release = 0.08f;
}

void HarmonicaEngine::prepare (double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1;
    lpFilter.prepare (spec);

    envelope.setSampleRate (sr);
}

void HarmonicaEngine::reset()
{
    lpFilter.reset();
    envelope.reset();
    phase1 = phase2 = phase3 = 0.0f;
    active = false;
}

void HarmonicaEngine::noteOn (float frequency, float vel, bool isBlow)
{
    freq = frequency;
    velocity = vel;
    currentBlow = isBlow;

    phase1 = 0.0f;
    phase2 = 0.0f;
    phase3 = 0.0f;

    // Set filter — blow = warmer (2800 Hz), draw = brighter (3800 Hz)
    float cutoff = isBlow ? 2800.0f : 3800.0f;
    *lpFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, cutoff, 2.5f);
    lpFilter.reset();

    // Set envelope
    envelope.setParameters (isBlow ? blowParams : drawParams);
    envelope.noteOn();

    active = true;
}

void HarmonicaEngine::noteOff()
{
    envelope.noteOff();
}

void HarmonicaEngine::renderBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (! active) return;

    auto* out = buffer.getWritePointer (0);

    const float detuneRatio = 1.003f; // slight detune for reed shimmer
    const float phaseInc1 = (float)(freq / sampleRate);
    const float phaseInc2 = (float)((freq * detuneRatio) / sampleRate);
    const float phaseInc3 = (float)(freq / sampleRate);

    for (int i = startSample; i < startSample + numSamples; ++i)
    {
        // === Sawtooth oscillator ===
        float saw = 2.0f * phase1 - 1.0f;

        // === Square oscillator (detuned) ===
        float sqr = phase2 < 0.5f ? 1.0f : -1.0f;

        // === Sine sub ===
        float sub = std::sin (juce::MathConstants<float>::twoPi * phase3);

        // === Breath noise ===
        float noise = noiseRng.nextFloat() * 2.0f - 1.0f;

        // Mix
        float mixed = saw     * gainSaw    * velocity
                    + sqr     * gainSquare * velocity
                    + sub     * gainSub    * velocity
                    + noise   * gainNoise  * velocity;

        // Filter
        float filtered = lpFilter.processSample (mixed);

        // Envelope
        float env = envelope.getNextSample();
        out[i] += filtered * env;

        // Advance phases (wrap 0..1)
        phase1 += phaseInc1; if (phase1 >= 1.0f) phase1 -= 1.0f;
        phase2 += phaseInc2; if (phase2 >= 1.0f) phase2 -= 1.0f;
        phase3 += phaseInc3; if (phase3 >= 1.0f) phase3 -= 1.0f;
    }

    if (! envelope.isActive())
        active = false;
}

} // namespace TurboTuning

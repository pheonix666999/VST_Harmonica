#pragma once
#include <JuceHeader.h>

namespace TurboTuning
{

/**
 * EffectsChain — 4-band EQ + Reverb + Delay + Chorus.
 *
 * Signal path mirrors the JS demo:
 *   input → EQ (low→mid→high→presence) → dry/wet splits:
 *     • Dry path
 *     • Reverb (convolution)
 *     • Delay (feedback)
 *     • Chorus (LFO-modulated dual delay)
 *   → sum → output
 */
class EffectsChain
{
public:
    EffectsChain();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    /** Process a mono buffer in-place. */
    void process (juce::AudioBuffer<float>& buffer);

    // EQ — value 0..100 (50 = flat, maps to -12..+12 dB)
    void setEQ (const juce::String& band, float value);

    // Effects — value 0..100
    void setReverbLevel (float value);
    void setDelayLevel (float value);
    void setChorusLevel (float value);

private:
    double sampleRate = 44100.0;
    int    blockSize  = 512;

    // ── EQ (4 IIR biquads) ──
    juce::dsp::IIR::Filter<float> eqLow;
    juce::dsp::IIR::Filter<float> eqMid;
    juce::dsp::IIR::Filter<float> eqHigh;
    juce::dsp::IIR::Filter<float> eqPresence;

    // ── Reverb ──
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;
    float reverbMix = 0.25f;

    // ── Delay ──
    juce::dsp::DelayLine<float> delayLine { 96000 };
    float delayTime     = 0.35f;   // seconds
    float delayFeedback = 0.35f;
    float delayMix      = 0.0f;
    float delayZ1       = 0.0f;    // feedback sample

    // ── Chorus ──
    juce::dsp::DelayLine<float> chorusDL1 { 4800 };
    juce::dsp::DelayLine<float> chorusDL2 { 4800 };
    float chorusLFOPhase = 0.0f;
    float chorusLFOFreq  = 1.5f;
    float chorusMix      = 0.0f;
};

} // namespace TurboTuning

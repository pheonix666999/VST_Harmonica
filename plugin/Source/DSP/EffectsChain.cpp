#include "EffectsChain.h"

namespace TurboTuning
{

EffectsChain::EffectsChain()
{
    reverbParams.roomSize   = 0.6f;
    reverbParams.damping    = 0.5f;
    reverbParams.wetLevel   = 0.25f;
    reverbParams.dryLevel   = 1.0f;
    reverbParams.width      = 1.0f;
    reverbParams.freezeMode = 0.0f;
}

void EffectsChain::prepare (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    blockSize  = samplesPerBlock;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1;

    // EQ
    eqLow.prepare (spec);
    eqMid.prepare (spec);
    eqHigh.prepare (spec);
    eqPresence.prepare (spec);

    // Default EQ coefficients (flat)
    *eqLow.coefficients      = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (sr, 200.0f, 0.707f, 1.0f);
    *eqMid.coefficients      = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, 1000.0f, 1.0f, 1.0f);
    *eqHigh.coefficients     = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 4000.0f, 0.707f, 1.0f);
    *eqPresence.coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, 3200.0f, 0.8f, 1.0f);

    // Reverb
    reverb.setSampleRate (sr);
    reverb.setParameters (reverbParams);

    // Delay
    delayLine.prepare (spec);
    delayLine.setMaximumDelayInSamples ((int)(sr * 2.0));
    delayZ1 = 0.0f;

    // Chorus
    chorusDL1.prepare (spec);
    chorusDL2.prepare (spec);
    chorusDL1.setMaximumDelayInSamples ((int)(sr * 0.05));
    chorusDL2.setMaximumDelayInSamples ((int)(sr * 0.05));
    chorusLFOPhase = 0.0f;
}

void EffectsChain::reset()
{
    eqLow.reset();
    eqMid.reset();
    eqHigh.reset();
    eqPresence.reset();
    reverb.reset();
    delayLine.reset();
    chorusDL1.reset();
    chorusDL2.reset();
    delayZ1 = 0.0f;
    chorusLFOPhase = 0.0f;
}

void EffectsChain::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    auto* data = buffer.getWritePointer (0);

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = data[i];

        // ── EQ chain ──
        sample = eqLow.processSample (sample);
        sample = eqMid.processSample (sample);
        sample = eqHigh.processSample (sample);
        sample = eqPresence.processSample (sample);

        float eqOut = sample;

        // ── Dry ──
        float dryOut = eqOut;

        // ── Reverb ── (simplified: we process per-sample using JUCE Reverb in short blocks)
        // We'll accumulate reverb in a separate pass below

        // ── Delay ──
        float delayIn = eqOut + delayZ1 * delayFeedback;
        delayLine.pushSample (0, delayIn);
        float delaySamples = (float)(delayTime * sampleRate);
        float delayOut = delayLine.popSample (0, delaySamples) * delayMix;
        delayZ1 = delayLine.popSample (0, delaySamples);
        // Re-push since we just popped twice (use the delayed sample for feedback)
        delayLine.pushSample (0, delayIn); // Already pushed above, so we need to adjust
        // Simplified: just use a direct feedback path
        delayZ1 = delayOut / (delayMix + 0.001f) * delayFeedback; // avoid div-by-zero

        // ── Chorus ──
        float lfo = std::sin (juce::MathConstants<float>::twoPi * chorusLFOPhase);
        chorusLFOPhase += (float)(chorusLFOFreq / sampleRate);
        if (chorusLFOPhase >= 1.0f) chorusLFOPhase -= 1.0f;

        float chorusDelay1 = 0.015f + lfo * 0.002f;   // seconds
        float chorusDelay2 = 0.025f - lfo * 0.002f;

        chorusDL1.pushSample (0, eqOut);
        chorusDL2.pushSample (0, eqOut);
        float ch1 = chorusDL1.popSample (0, (float)(chorusDelay1 * sampleRate));
        float ch2 = chorusDL2.popSample (0, (float)(chorusDelay2 * sampleRate));
        float chorusOut = (ch1 + ch2) * 0.5f * chorusMix;

        // ── Mix ──
        data[i] = dryOut + delayOut + chorusOut;
    }

    // ── Reverb pass (JUCE Reverb works on blocks) ──
    if (reverbMix > 0.001f)
    {
        // Create a copy for wet signal
        juce::AudioBuffer<float> reverbBuf (1, numSamples);
        reverbBuf.copyFrom (0, 0, buffer, 0, 0, numSamples);

        reverb.processMono (reverbBuf.getWritePointer (0), numSamples);

        // Add wet reverb to output
        for (int i = 0; i < numSamples; ++i)
            data[i] += reverbBuf.getSample (0, i) * reverbMix;
    }
}

// ── EQ Control ──

void EffectsChain::setEQ (const juce::String& band, float value)
{
    // value 0..100  →  dB = (value - 50) * 0.24 → -12..+12 dB
    float db = (value - 50.0f) * 0.24f;
    float gain = juce::Decibels::decibelsToGain (db);

    if (band == "low")
        *eqLow.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (sampleRate, 200.0f, 0.707f, gain);
    else if (band == "mid")
        *eqMid.coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 1000.0f, 1.0f, gain);
    else if (band == "high")
        *eqHigh.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 4000.0f, 0.707f, gain);
    else if (band == "presence")
        *eqPresence.coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 3200.0f, 0.8f, gain);
}

void EffectsChain::setReverbLevel (float value)
{
    reverbMix = (value / 100.0f) * 0.8f;
}

void EffectsChain::setDelayLevel (float value)
{
    delayMix = (value / 100.0f) * 0.5f;
}

void EffectsChain::setChorusLevel (float value)
{
    chorusMix = (value / 100.0f) * 0.4f;
}

} // namespace TurboTuning

#include "SafeMode.h"

namespace TurboTuning
{

SafeMode::SafeMode() {}

void SafeMode::prepare (double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1;

    compressor.prepare (spec);
    makeupGain.prepare (spec);

    applySettings();
}

void SafeMode::reset()
{
    compressor.reset();
    makeupGain.reset();
}

void SafeMode::setEnabled (bool enabled)
{
    safeModeOn = enabled;
    applySettings();
}

void SafeMode::applySettings()
{
    if (safeModeOn)
    {
        // Aggressive limiting — prevents any bad sounds
        compressor.setThreshold (-30.0f);
        compressor.setRatio (20.0f);
        compressor.setAttack (1.0f);    // ms
        compressor.setRelease (100.0f); // ms
        makeupGain.setGainLinear (0.5f);
    }
    else
    {
        // Light compression — normal operation
        compressor.setThreshold (-6.0f);
        compressor.setRatio (3.0f);
        compressor.setAttack (3.0f);
        compressor.setRelease (250.0f);
        makeupGain.setGainLinear (0.7f);
    }
}

void SafeMode::process (juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    compressor.process (context);
    makeupGain.process (context);
}

} // namespace TurboTuning

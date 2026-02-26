#pragma once
#include <JuceHeader.h>

namespace TurboTuning
{

/**
 * SafeMode — Dynamics compressor / limiter.
 *
 * Normal mode:  light compression (-6 dB threshold, 3:1 ratio)
 * Safe mode ON: aggressive limiting (-30 dB threshold, 20:1 ratio)
 *   → prevents any harsh or overly loud sounds (kid-friendly)
 */
class SafeMode
{
public:
    SafeMode();

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void setEnabled (bool enabled);
    bool isEnabled() const { return safeModeOn; }

    /** Process mono buffer in-place. */
    void process (juce::AudioBuffer<float>& buffer);

private:
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Gain<float>       makeupGain;
    bool safeModeOn = false;
    double sampleRate = 44100.0;

    void applySettings();
};

} // namespace TurboTuning

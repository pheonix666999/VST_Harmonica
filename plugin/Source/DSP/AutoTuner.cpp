#include "AutoTuner.h"
#include <cmath>

namespace TurboTuning
{

AutoTuner::AutoTuner() {}

float AutoTuner::correctPitch (float inputFreq) const
{
    if (! autoTuneOn || inputFreq <= 0.0f)
        return inputFreq;

    // Convert frequency to MIDI note number (continuous)
    float midiContinuous = 69.0f + 12.0f * std::log2 (inputFreq / 440.0f);

    // Round to nearest semitone
    int nearestMidi = juce::roundToInt (midiContinuous);

    // Convert back to frequency
    return MusicTheory::midiToFreq (nearestMidi);
}

int AutoTuner::correctMidiNote (int midiNote) const
{
    // MIDI is already quantized, this is a no-op
    // but we keep the API for future use with continuous controllers
    return midiNote;
}

} // namespace TurboTuning

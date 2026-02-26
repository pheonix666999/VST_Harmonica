#pragma once
#include <JuceHeader.h>
#include "../Util/MusicTheory.h"

namespace TurboTuning
{

/**
 * AutoTuner — Pitch correction that snaps incoming frequencies
 * to the nearest note in the current key.
 *
 * In the harmonica layout, notes are already quantized to MIDI,
 * so this mainly applies when using continuous wind controllers
 * that might produce off-pitch input.
 */
class AutoTuner
{
public:
    AutoTuner();

    void setEnabled (bool enabled) { autoTuneOn = enabled; }
    bool isEnabled() const { return autoTuneOn; }

    void setKey (const juce::String& key) { currentKey = key; }

    /** Snap a frequency to the nearest chromatic pitch. */
    float correctPitch (float inputFreq) const;

    /** Snap a MIDI note to nearest integer (trivial but consistent with the API). */
    int correctMidiNote (int midiNote) const;

private:
    bool autoTuneOn = true;
    juce::String currentKey = "C";
};

} // namespace TurboTuning

#pragma once
#include <JuceHeader.h>
#include "TurboLookAndFeel.h"
#include "../Util/MusicTheory.h"

namespace TurboTuning
{

/**
 * CircleOfFifths — Interactive key selector widget.
 *
 * Displays 12 keys in a circle with the selected key highlighted.
 * Centre shows the current major key and its relative minor.
 */
class CircleOfFifths : public juce::Component
{
public:
    std::function<void(const juce::String& key)> onKeySelected;

    CircleOfFifths();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;

    void setSelectedKey (const juce::String& key);
    juce::String getSelectedKey() const { return selectedKey; }

private:
    juce::String selectedKey = "C";

    // Pre-calculated button centres
    juce::Point<float> keyCentres[12];
    float keyRadius = 18.0f;

    int getKeyFromPoint (juce::Point<float> pt) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircleOfFifths)
};

} // namespace TurboTuning

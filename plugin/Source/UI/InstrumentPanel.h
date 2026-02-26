#pragma once
#include <JuceHeader.h>
#include "TurboLookAndFeel.h"
#include "../Util/MusicTheory.h"

namespace TurboTuning
{

/**
 * InstrumentPanel — 4-category instrument browser.
 *
 * Categories: Harmonica, Brass, Strings, Woodwinds
 * Each with individual instrument buttons.
 */
class InstrumentPanel : public juce::Component
{
public:
    std::function<void(InstrumentType)> onInstrumentSelected;

    InstrumentPanel();

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setSelectedInstrument (InstrumentType inst);

private:
    InstrumentType selected = InstrumentType::Harmonica;

    static constexpr int NUM_INSTRUMENTS = 10;
    juce::OwnedArray<juce::TextButton> buttons;
    InstrumentType buttonTypes[NUM_INSTRUMENTS] = {};

    void setupButtons();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InstrumentPanel)
};

} // namespace TurboTuning

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/TurboLookAndFeel.h"
#include "UI/HarmonicaView.h"
#include "UI/CircleOfFifths.h"
#include "UI/InstrumentPanel.h"
#include "UI/EffectsPanel.h"

namespace TurboTuning
{

/**
 * TurboTuningEditor — Main plugin GUI.
 *
 * Layout (matches demo):
 *   ┌──────────────────────────────────────────┐
 *   │  Header: Logo | Key Display | Vol Slider │
 *   ├────────────────────┬─────────────────────┤
 *   │  Harmonica         │  Instruments        │
 *   │  (10-hole view)    │  (category browser) │
 *   │                    │                     │
 *   │  Circle of Fifths  │  EQ + Effects       │
 *   │  (key selector)    │  + Toggles          │
 *   ├────────────────────┴─────────────────────┤
 *   │  Footer: Version info | Keyboard hints   │
 *   └──────────────────────────────────────────┘
 */
class TurboTuningEditor : public juce::AudioProcessorEditor
{
public:
    TurboTuningEditor (TurboTuningProcessor&);
    ~TurboTuningEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    TurboTuningProcessor& processor;
    TurboLookAndFeel turboLnF;

    // UI Components
    HarmonicaView   harmonicaView;
    CircleOfFifths  circleOfFifths;
    InstrumentPanel instrumentPanel;
    EffectsPanel    effectsPanel;

    // Header labels
    juce::Label titleLabel;
    juce::Label keyDisplayLabel;
    juce::Label instrumentDisplayLabel;
    juce::Label betaBadge;

    // Footer
    juce::Label footerLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TurboTuningEditor)
};

} // namespace TurboTuning

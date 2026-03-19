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

class TurboTuningEditor : public juce::AudioProcessorEditor
{
public:
    explicit TurboTuningEditor (TurboTuningProcessor& p);
    ~TurboTuningEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Image loadLogo() const;
    void setStatusText (const juce::String& text);
    void refreshFromProcessor();

    TurboTuningProcessor& processor;
    TurboLookAndFeel lookAndFeel;
    juce::Image logoImage;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label quickStartLabel;
    juce::Label statusLabel;

    HarmonicaView harmonicaView;
    CircleOfFifths circleOfFifths;
    InstrumentPanel instrumentPanel;
    EffectsPanel effectsPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TurboTuningEditor)
};

} // namespace TurboTuning

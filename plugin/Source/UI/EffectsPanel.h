#pragma once
#include <JuceHeader.h>
#include "TurboLookAndFeel.h"

namespace TurboTuning
{

/**
 * EffectsPanel — EQ sliders + effect knobs + toggle switches.
 *
 * Layout:
 *   • 4-band EQ vertical sliders (Low, Mid, High, Presence)
 *   • 3 rotary knobs (Reverb, Delay, Chorus)
 *   • 2 toggles (Auto-Tune, Safe Mode)
 */
class EffectsPanel : public juce::Component
{
public:
    EffectsPanel (juce::AudioProcessorValueTreeState& apvts);
    ~EffectsPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    // EQ
    juce::Slider eqLow, eqMid, eqHigh, eqPresence;
    juce::Label  eqLowLabel, eqMidLabel, eqHighLabel, eqPresLabel;

    // Effects knobs
    juce::Slider reverbKnob, delayKnob, chorusKnob;
    juce::Label  reverbLabel, delayLabel, chorusLabel;

    // Toggles
    juce::ToggleButton autoTuneToggle { "AUTO-TUNE" };
    juce::ToggleButton safeModeToggle { "SAFE MODE" };

    // Master volume
    juce::Slider masterVolSlider;
    juce::Label  masterVolLabel;

    // APVTS attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> eqLowAtt, eqMidAtt, eqHighAtt, eqPresAtt;
    std::unique_ptr<SliderAttachment> reverbAtt, delayAtt, chorusAtt;
    std::unique_ptr<SliderAttachment> masterVolAtt;
    std::unique_ptr<ButtonAttachment> autoTuneAtt, safeModeAtt;

    void setupSlider (juce::Slider& s, juce::Label& l, const juce::String& text,
                      juce::Slider::SliderStyle style);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsPanel)
};

} // namespace TurboTuning

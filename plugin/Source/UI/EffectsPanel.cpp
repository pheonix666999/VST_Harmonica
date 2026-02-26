#include "EffectsPanel.h"

namespace TurboTuning
{

EffectsPanel::EffectsPanel (juce::AudioProcessorValueTreeState& apvts)
{
    // ── EQ Sliders ──
    setupSlider (eqLow,      eqLowLabel,  "LOW",   juce::Slider::LinearVertical);
    setupSlider (eqMid,      eqMidLabel,  "MID",   juce::Slider::LinearVertical);
    setupSlider (eqHigh,     eqHighLabel, "HIGH",  juce::Slider::LinearVertical);
    setupSlider (eqPresence, eqPresLabel, "PRES",  juce::Slider::LinearVertical);

    eqLowAtt  = std::make_unique<SliderAttachment> (apvts, "eqLow",  eqLow);
    eqMidAtt  = std::make_unique<SliderAttachment> (apvts, "eqMid",  eqMid);
    eqHighAtt = std::make_unique<SliderAttachment> (apvts, "eqHigh", eqHigh);
    eqPresAtt = std::make_unique<SliderAttachment> (apvts, "eqPres", eqPresence);

    // ── Effect Knobs ──
    setupSlider (reverbKnob, reverbLabel, "REVERB", juce::Slider::RotaryHorizontalVerticalDrag);
    setupSlider (delayKnob,  delayLabel,  "DELAY",  juce::Slider::RotaryHorizontalVerticalDrag);
    setupSlider (chorusKnob, chorusLabel, "CHORUS", juce::Slider::RotaryHorizontalVerticalDrag);

    reverbAtt = std::make_unique<SliderAttachment> (apvts, "reverb", reverbKnob);
    delayAtt  = std::make_unique<SliderAttachment> (apvts, "delay",  delayKnob);
    chorusAtt = std::make_unique<SliderAttachment> (apvts, "chorus", chorusKnob);

    // ── Master Volume ──
    setupSlider (masterVolSlider, masterVolLabel, "VOL", juce::Slider::LinearHorizontal);
    masterVolAtt = std::make_unique<SliderAttachment> (apvts, "masterVol", masterVolSlider);

    // ── Toggles ──
    addAndMakeVisible (autoTuneToggle);
    addAndMakeVisible (safeModeToggle);

    autoTuneAtt = std::make_unique<ButtonAttachment> (apvts, "autoTune", autoTuneToggle);
    safeModeAtt = std::make_unique<ButtonAttachment> (apvts, "safeMode", safeModeToggle);
}

EffectsPanel::~EffectsPanel() {}

void EffectsPanel::setupSlider (juce::Slider& s, juce::Label& l, const juce::String& text,
                                 juce::Slider::SliderStyle style)
{
    s.setSliderStyle (style);
    s.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    s.setRange (0.0, 100.0, 0.1);
    addAndMakeVisible (s);

    l.setText (text, juce::dontSendNotification);
    l.setFont (juce::Font (10.0f, juce::Font::bold));
    l.setColour (juce::Label::textColourId, juce::Colour (TurboLookAndFeel::textSecondary));
    l.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (l);
}

void EffectsPanel::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    // Full panel background
    g.setColour (juce::Colour (TurboLookAndFeel::bgPanel));
    g.fillRoundedRectangle (area, 12.0f);
    g.setColour (juce::Colour (TurboLookAndFeel::border));
    g.drawRoundedRectangle (area, 12.0f, 1.0f);

    // Section divider labels
    auto drawSection = [&] (const juce::String& title, float y) {
        g.setColour (juce::Colour (TurboLookAndFeel::textPrimary));
        g.setFont (juce::Font (13.0f, juce::Font::bold));
        g.drawText (title, 12.0f, y, 200.0f, 20.0f, juce::Justification::centredLeft);
    };

    drawSection ("EQ",       4.0f);
    drawSection ("Effects", 180.0f);
    drawSection ("Controls",310.0f);
}

void EffectsPanel::resized()
{
    // ── EQ Section (top) ──
    float eqTop = 28.0f;
    float eqSliderH = 130.0f;
    float eqW = (float)(getWidth() - 32) / 4.0f;

    auto layoutEQ = [&] (juce::Slider& s, juce::Label& l, int index) {
        float x = 16.0f + index * eqW;
        s.setBounds ((int)(x + eqW * 0.15f), (int)eqTop, (int)(eqW * 0.7f), (int)eqSliderH);
        l.setBounds ((int)x, (int)(eqTop + eqSliderH + 2.0f), (int)eqW, 16);
    };

    layoutEQ (eqLow,      eqLowLabel,  0);
    layoutEQ (eqMid,      eqMidLabel,  1);
    layoutEQ (eqHigh,     eqHighLabel, 2);
    layoutEQ (eqPresence, eqPresLabel, 3);

    // ── Effects Knobs ──
    float fxTop = 204.0f;
    float knobSize = 70.0f;
    float fxW = (float)(getWidth() - 32) / 3.0f;

    auto layoutKnob = [&] (juce::Slider& s, juce::Label& l, int index) {
        float x = 16.0f + index * fxW + (fxW - knobSize) * 0.5f;
        s.setBounds ((int)x, (int)fxTop, (int)knobSize, (int)knobSize);
        l.setBounds ((int)(16.0f + index * fxW), (int)(fxTop + knobSize + 2.0f), (int)fxW, 16);
    };

    layoutKnob (reverbKnob, reverbLabel, 0);
    layoutKnob (delayKnob,  delayLabel,  1);
    layoutKnob (chorusKnob, chorusLabel, 2);

    // ── Controls Section ──
    float ctrlTop = 334.0f;
    int w = getWidth() - 32;
    autoTuneToggle.setBounds (16, (int)ctrlTop, w, 28);
    safeModeToggle.setBounds (16, (int)(ctrlTop + 32.0f), w, 28);

    // Master volume at bottom
    float volTop = ctrlTop + 70.0f;
    masterVolLabel.setBounds (16, (int)volTop, 30, 20);
    masterVolSlider.setBounds (50, (int)volTop, w - 40, 20);
}

} // namespace TurboTuning

#include "PluginEditor.h"

namespace TurboTuning
{

TurboTuningEditor::TurboTuningEditor (TurboTuningProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      effectsPanel (p.getAPVTS())
{
    setLookAndFeel (&turboLnF);
    setSize (900, 660);

    // ── Header ──
    titleLabel.setText ("TURBO TUNING", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (22.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (TurboLookAndFeel::gold));
    addAndMakeVisible (titleLabel);

    betaBadge.setText ("BETA", juce::dontSendNotification);
    betaBadge.setFont (juce::Font (9.0f, juce::Font::bold));
    betaBadge.setColour (juce::Label::textColourId, juce::Colour (TurboLookAndFeel::gold));
    addAndMakeVisible (betaBadge);

    keyDisplayLabel.setText ("Key of " + processor.getCurrentKey(), juce::dontSendNotification);
    keyDisplayLabel.setFont (juce::Font (14.0f, juce::Font::bold));
    keyDisplayLabel.setColour (juce::Label::textColourId, juce::Colour (TurboLookAndFeel::textPrimary));
    keyDisplayLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (keyDisplayLabel);

    instrumentDisplayLabel.setText (instrumentName (processor.getCurrentInstrument()), juce::dontSendNotification);
    instrumentDisplayLabel.setFont (juce::Font (11.0f));
    instrumentDisplayLabel.setColour (juce::Label::textColourId, juce::Colour (TurboLookAndFeel::textSecondary));
    instrumentDisplayLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (instrumentDisplayLabel);

    // ── Main Components ──
    addAndMakeVisible (harmonicaView);
    addAndMakeVisible (circleOfFifths);
    addAndMakeVisible (instrumentPanel);
    addAndMakeVisible (effectsPanel);

    // ── Footer ──
    footerLabel.setText ("Turbo Tuning v0.1.0 Beta  |  Keys 1-0 = Holes  |  Shift = Draw  |  Space = Toggle",
                         juce::dontSendNotification);
    footerLabel.setFont (juce::Font (10.0f));
    footerLabel.setColour (juce::Label::textColourId, juce::Colour (TurboLookAndFeel::textDim));
    footerLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (footerLabel);

    // ── Callbacks ──
    harmonicaView.onHoleEvent = [this] (int hole, bool isBlow, bool noteOn)
    {
        if (noteOn)
        {
            processor.playHole (hole, isBlow);
            int midi = MusicTheory::getNote (hole, isBlow, processor.getCurrentKey());
            harmonicaView.setCurrentNote (MusicTheory::midiToNoteName (midi));
        }
        else
        {
            processor.stopHole (hole, isBlow);
            harmonicaView.setCurrentNote ("");
        }
    };

    circleOfFifths.onKeySelected = [this] (const juce::String& key)
    {
        processor.setKey (key);
        harmonicaView.setCurrentKey (key);
        keyDisplayLabel.setText ("Key of " + key, juce::dontSendNotification);
    };

    instrumentPanel.onInstrumentSelected = [this] (InstrumentType inst)
    {
        processor.setInstrument (inst);
        instrumentDisplayLabel.setText (instrumentName (inst), juce::dontSendNotification);
    };
}

TurboTuningEditor::~TurboTuningEditor()
{
    setLookAndFeel (nullptr);
}

void TurboTuningEditor::paint (juce::Graphics& g)
{
    // ── Background gradient ──
    g.fillAll (juce::Colour (TurboLookAndFeel::bgBody));

    // Subtle radial gradient overlay
    auto center = getLocalBounds().getCentre().toFloat();
    juce::ColourGradient bg (juce::Colour (TurboLookAndFeel::bgApp).withAlpha (0.8f), center.x, center.y,
                              juce::Colour (TurboLookAndFeel::bgBody), 0.0f, 0.0f, true);
    g.setGradientFill (bg);
    g.fillAll();

    // ── Header background ──
    auto headerRect = juce::Rectangle<float> (0, 0, (float)getWidth(), 52.0f);
    g.setColour (juce::Colour (0xE6080C20));
    g.fillRect (headerRect);
    g.setColour (juce::Colour (TurboLookAndFeel::border));
    g.drawLine (0, 52.0f, (float)getWidth(), 52.0f, 1.0f);

    // ── Footer background ──
    float footerY = (float)getHeight() - 30.0f;
    g.setColour (juce::Colour (0xE6080C20));
    g.fillRect (0.0f, footerY, (float)getWidth(), 30.0f);
    g.setColour (juce::Colour (TurboLookAndFeel::border));
    g.drawLine (0, footerY, (float)getWidth(), footerY, 1.0f);
}

void TurboTuningEditor::resized()
{
    auto area = getLocalBounds();

    // Header (52px)
    auto header = area.removeFromTop (52);
    auto headerLeft = header.removeFromLeft (200).reduced (12, 8);
    titleLabel.setBounds (headerLeft.removeFromLeft (160));
    betaBadge.setBounds (headerLeft);

    auto headerCenter = header.reduced (0, 12);
    auto centerArea = headerCenter.withSizeKeepingCentre (300, 28);
    keyDisplayLabel.setBounds (centerArea.removeFromLeft (150));
    instrumentDisplayLabel.setBounds (centerArea);

    // Footer (30px)
    auto footer = area.removeFromBottom (30);
    footerLabel.setBounds (footer);

    // Main content — 2-column layout with 16px gap
    auto main = area.reduced (12, 8);
    int gap = 12;
    int leftW = (int)(main.getWidth() * 0.52f);
    int rightW = main.getWidth() - leftW - gap;

    auto leftCol  = main.removeFromLeft (leftW);
    main.removeFromLeft (gap);
    auto rightCol = main;

    // Left column: Harmonica (60%) + Circle of Fifths (40%)
    int harmoH = (int)(leftCol.getHeight() * 0.55f);
    harmonicaView.setBounds (leftCol.removeFromTop (harmoH).reduced (0, 4));
    circleOfFifths.setBounds (leftCol.reduced (0, 4));

    // Right column: Instruments (45%) + Effects (55%)
    int instH = (int)(rightCol.getHeight() * 0.48f);
    instrumentPanel.setBounds (rightCol.removeFromTop (instH).reduced (0, 4));
    effectsPanel.setBounds (rightCol.reduced (0, 4));
}

} // namespace TurboTuning

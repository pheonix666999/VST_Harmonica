#include "PluginEditor.h"

namespace TurboTuning
{

TurboTuningEditor::TurboTuningEditor (TurboTuningProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
      effectsPanel (p.getAPVTS())
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);
    setSize (1120, 760);
    logoImage = loadLogo();

    titleLabel.setText ("TURBO HARP", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (28.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (TurboLookAndFeel::gold));
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("GarageBand, made very easy.", juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (13.0f, juce::Font::bold));
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (TurboLookAndFeel::textSecondary));
    addAndMakeVisible (subtitleLabel);

    quickStartLabel.setText ("1. Pick a key   2. Pick a sound   3. Tap holes or press 1-0", juce::dontSendNotification);
    quickStartLabel.setFont (juce::Font (12.0f));
    quickStartLabel.setColour (juce::Label::textColourId, juce::Colour (TurboLookAndFeel::textDim));
    addAndMakeVisible (quickStartLabel);

    statusLabel.setFont (juce::Font (12.0f));
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    statusLabel.setColour (juce::Label::textColourId, juce::Colour (TurboLookAndFeel::textSecondary));
    addAndMakeVisible (statusLabel);

    harmonicaView.onHoleEvent = [this] (int hole, bool isBlow, bool noteOn)
    {
        if (noteOn)
        {
            processor.playHole (hole, isBlow);
            const auto midiNote = MusicTheory::getNote (hole, isBlow, processor.getCurrentKey());
            harmonicaView.setCurrentNote (MusicTheory::midiToNoteName (midiNote));
            setStatusText ("Playing " + MusicTheory::midiToNoteName (midiNote));
        }
        else
        {
            processor.stopHole (hole, isBlow);
            harmonicaView.setCurrentNote ({});
            setStatusText ("Ready");
        }

        harmonicaView.setActiveHole (hole, isBlow, noteOn);
    };
    addAndMakeVisible (harmonicaView);

    circleOfFifths.onKeySelected = [this] (const juce::String& key)
    {
        processor.setKey (key);
        harmonicaView.setCurrentKey (key);
        setStatusText ("Key: " + key);
    };
    addAndMakeVisible (circleOfFifths);

    instrumentPanel.onInstrumentSelected = [this] (InstrumentType instrument)
    {
        processor.setInstrument (instrument);
        setStatusText ("Sound: " + instrumentName (instrument));
    };
    addAndMakeVisible (instrumentPanel);

    addAndMakeVisible (effectsPanel);

    refreshFromProcessor();
    setStatusText ("Ready");
}

TurboTuningEditor::~TurboTuningEditor()
{
    setLookAndFeel (nullptr);
}

juce::Image TurboTuningEditor::loadLogo() const
{
    const auto exeFile = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    const juce::File candidates[] = {
        exeFile.getSiblingFile ("logo.png"),
        exeFile.getParentDirectory().getSiblingFile ("Resources").getChildFile ("logo.png"),
        exeFile.getParentDirectory().getSiblingFile ("demo").getChildFile ("logo.png"),
        exeFile.getParentDirectory().getParentDirectory().getSiblingFile ("demo").getChildFile ("logo.png")
    };

    for (const auto& file : candidates)
        if (file.existsAsFile())
            return juce::ImageFileFormat::loadFrom (file);

    return {};
}

void TurboTuningEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient background (juce::Colour (TurboLookAndFeel::bgApp), 0.0f, 0.0f,
                                     juce::Colour (TurboLookAndFeel::bgBody), 0.0f, (float) getHeight(), false);
    g.setGradientFill (background);
    g.fillAll();

    auto bounds = getLocalBounds().reduced (16);
    auto hero = bounds.removeFromTop (108);

    g.setColour (juce::Colour (TurboLookAndFeel::bgPanel));
    g.fillRoundedRectangle (hero.toFloat(), 18.0f);
    g.setColour (juce::Colour (TurboLookAndFeel::border));
    g.drawRoundedRectangle (hero.toFloat().reduced (0.5f), 18.0f, 1.0f);

    if (logoImage.isValid())
    {
        g.drawImageWithin (logoImage, hero.getX() + 18, hero.getY() + 18, 72, 72,
                           juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, false);
    }
    else
    {
        g.setColour (juce::Colour (TurboLookAndFeel::gold).withAlpha (0.15f));
        g.fillRoundedRectangle ((float) hero.getX() + 18.0f, (float) hero.getY() + 18.0f, 72.0f, 72.0f, 14.0f);
    }

    auto content = getLocalBounds().reduced (16).withTrimmedTop (124);
    auto left = content.removeFromLeft ((content.getWidth() * 58) / 100);
    left.removeFromRight (10);
    auto right = content;

    g.setColour (juce::Colour (TurboLookAndFeel::textDim));
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("PLAY", left.withHeight (18).translated (0, -18), juce::Justification::centredLeft);
    g.drawText ("SETUP", right.withHeight (18).translated (0, -18), juce::Justification::centredLeft);
}

void TurboTuningEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    auto hero = area.removeFromTop (108);

    auto textArea = hero.withTrimmedLeft (106).reduced (0, 16);
    titleLabel.setBounds (textArea.removeFromTop (34));
    subtitleLabel.setBounds (textArea.removeFromTop (20));
    quickStartLabel.setBounds (textArea.removeFromTop (18));

    auto footer = area.removeFromBottom (28);
    statusLabel.setBounds (footer);

    auto left = area.removeFromLeft ((area.getWidth() * 58) / 100);
    left.removeFromRight (10);
    auto right = area;

    harmonicaView.setBounds (left.removeFromTop ((left.getHeight() * 62) / 100));
    left.removeFromTop (10);
    circleOfFifths.setBounds (left);

    auto instrumentHeight = juce::jmin (290, right.getHeight() / 2);
    instrumentPanel.setBounds (right.removeFromTop (instrumentHeight));
    right.removeFromTop (10);
    effectsPanel.setBounds (right);
}

void TurboTuningEditor::setStatusText (const juce::String& text)
{
    statusLabel.setText (text, juce::dontSendNotification);
}

void TurboTuningEditor::refreshFromProcessor()
{
    harmonicaView.setCurrentKey (processor.getCurrentKey());
    circleOfFifths.setSelectedKey (processor.getCurrentKey());
    instrumentPanel.setSelectedInstrument (processor.getCurrentInstrument());
}

} // namespace TurboTuning

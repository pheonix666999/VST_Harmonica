#include "InstrumentPanel.h"

namespace TurboTuning
{

InstrumentPanel::InstrumentPanel()
{
    setupButtons();
}

void InstrumentPanel::setupButtons()
{
    struct InstDef { InstrumentType type; juce::String name; };
    InstDef defs[] = {
        { InstrumentType::Harmonica, "Diatonic" },
        { InstrumentType::Trumpet,   "Trumpet" },
        { InstrumentType::Tuba,      "Tuba" },
        { InstrumentType::FrenchHorn,"Fr. Horn" },
        { InstrumentType::Guitar,    "Guitar" },
        { InstrumentType::Harp,      "Harp" },
        { InstrumentType::Violin,    "Violin" },
        { InstrumentType::Flute,     "Flute" },
        { InstrumentType::Clarinet,  "Clarinet" },
        { InstrumentType::PanFlute,  "Pan Flute" },
    };

    for (int i = 0; i < NUM_INSTRUMENTS; ++i)
    {
        auto* btn = new juce::TextButton (defs[i].name);
        btn->setToggleable (true);
        btn->setToggleState (defs[i].type == selected, juce::dontSendNotification);

        auto t = defs[i].type;
        btn->onClick = [this, t] {
            setSelectedInstrument (t);
            if (onInstrumentSelected)
                onInstrumentSelected (t);
        };

        buttonTypes[i] = defs[i].type;
        addAndMakeVisible (btn);
        buttons.add (btn);
    }
}

void InstrumentPanel::setSelectedInstrument (InstrumentType inst)
{
    selected = inst;
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState (buttonTypes[i] == inst, juce::dontSendNotification);
    repaint();
}

void InstrumentPanel::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    // Card background
    g.setColour (juce::Colour (TurboLookAndFeel::bgPanel));
    g.fillRoundedRectangle (area, 12.0f);
    g.setColour (juce::Colour (TurboLookAndFeel::border));
    g.drawRoundedRectangle (area, 12.0f, 1.0f);

    // Title
    g.setColour (juce::Colour (TurboLookAndFeel::textPrimary));
    g.setFont (juce::Font (14.0f, juce::Font::bold));
    g.drawText ("Instruments", area.reduced (12.0f, 0).removeFromTop (28.0f),
                juce::Justification::centredLeft);

    // Category headers
    auto drawCategoryLabel = [&] (const juce::String& label, float y) {
        g.setColour (juce::Colour (TurboLookAndFeel::textDim));
        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.drawText (label, 16.0f, y, 200.0f, 16.0f, juce::Justification::centredLeft);
    };

    float startY = 34.0f;
    float btnH = 28.0f;
    float gap = 4.0f;

    drawCategoryLabel ("HARMONICA", startY);
    drawCategoryLabel ("BRASS",    startY + (btnH + gap) * 1 + 20.0f);
    drawCategoryLabel ("STRINGS",  startY + (btnH + gap) * 4 + 40.0f);
    drawCategoryLabel ("WOODWINDS",startY + (btnH + gap) * 7 + 60.0f);
}

void InstrumentPanel::resized()
{
    float startY = 52.0f;
    float btnH = 28.0f;
    float gap = 4.0f;
    float x = 16.0f;
    float w = (float)getWidth() - 32.0f;

    int categoryStarts[] = { 0, 1, 4, 7 };
    float categoryLabelH = 20.0f;

    float y = startY;
    for (int cat = 0; cat < 4; ++cat)
    {
        int end = (cat < 3) ? categoryStarts[cat + 1] : NUM_INSTRUMENTS;
        for (int i = categoryStarts[cat]; i < end; ++i)
        {
            buttons[i]->setBounds ((int)x, (int)y, (int)w, (int)btnH);
            y += btnH + gap;
        }
        y += categoryLabelH;
    }
}

} // namespace TurboTuning

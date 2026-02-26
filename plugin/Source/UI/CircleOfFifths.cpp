#include "CircleOfFifths.h"

namespace TurboTuning
{

CircleOfFifths::CircleOfFifths()
{
    setSize (280, 280);
}

void CircleOfFifths::setSelectedKey (const juce::String& key)
{
    selectedKey = key;
    repaint();
}

void CircleOfFifths::paint (juce::Graphics& g)
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
    g.drawText ("Circle of Fifths", area.removeFromTop (28.0f).reduced (12.0f, 0), juce::Justification::centredLeft);

    // Circle area
    auto circleArea = getLocalBounds().reduced (16).withTrimmedTop (28).toFloat();
    auto cx = circleArea.getCentreX();
    auto cy = circleArea.getCentreY();
    auto circleRadius = juce::jmin (circleArea.getWidth(), circleArea.getHeight()) * 0.42f;
    keyRadius = circleRadius * 0.18f;

    // Subtle ring
    g.setColour (juce::Colour (TurboLookAndFeel::border));
    g.drawEllipse (cx - circleRadius, cy - circleRadius,
                   circleRadius * 2.0f, circleRadius * 2.0f, 1.0f);

    // Draw 12 key buttons
    for (int i = 0; i < 12; ++i)
    {
        float angle = (float)i * (juce::MathConstants<float>::twoPi / 12.0f)
                      - juce::MathConstants<float>::halfPi;

        float kx = cx + circleRadius * std::cos (angle);
        float ky = cy + circleRadius * std::sin (angle);
        keyCentres[i] = { kx, ky };

        bool isSelected = (MusicTheory::KEYS[(size_t)i] == selectedKey);

        // Button circle
        if (isSelected)
        {
            // Gold glow
            g.setColour (juce::Colour (TurboLookAndFeel::gold).withAlpha (0.3f));
            g.fillEllipse (kx - keyRadius - 3, ky - keyRadius - 3,
                           (keyRadius + 3) * 2, (keyRadius + 3) * 2);

            g.setColour (juce::Colour (TurboLookAndFeel::gold));
            g.fillEllipse (kx - keyRadius, ky - keyRadius, keyRadius * 2, keyRadius * 2);

            g.setColour (juce::Colour (TurboLookAndFeel::bgBody));
            g.setFont (juce::Font (13.0f, juce::Font::bold));
        }
        else
        {
            g.setColour (juce::Colour (TurboLookAndFeel::bgCard));
            g.fillEllipse (kx - keyRadius, ky - keyRadius, keyRadius * 2, keyRadius * 2);
            g.setColour (juce::Colour (TurboLookAndFeel::border));
            g.drawEllipse (kx - keyRadius, ky - keyRadius, keyRadius * 2, keyRadius * 2, 1.0f);

            g.setColour (juce::Colour (TurboLookAndFeel::textSecondary));
            g.setFont (juce::Font (12.0f));
        }

        g.drawText (MusicTheory::KEYS[(size_t)i],
                    juce::Rectangle<float> (kx - keyRadius, ky - keyRadius, keyRadius * 2, keyRadius * 2),
                    juce::Justification::centred);
    }

    // Centre display
    float centreR = circleRadius * 0.30f;

    g.setColour (juce::Colour (TurboLookAndFeel::bgInput));
    g.fillEllipse (cx - centreR, cy - centreR, centreR * 2, centreR * 2);
    g.setColour (juce::Colour (TurboLookAndFeel::border));
    g.drawEllipse (cx - centreR, cy - centreR, centreR * 2, centreR * 2, 1.0f);

    // Major key
    g.setColour (juce::Colour (TurboLookAndFeel::gold));
    g.setFont (juce::Font (20.0f, juce::Font::bold));
    g.drawText (selectedKey,
                juce::Rectangle<float> (cx - centreR, cy - centreR, centreR * 2, centreR * 1.2f),
                juce::Justification::centred);

    // Relative minor
    auto minorIt = MusicTheory::RELATIVE_MINORS.find (selectedKey);
    if (minorIt != MusicTheory::RELATIVE_MINORS.end())
    {
        g.setColour (juce::Colour (TurboLookAndFeel::blue));
        g.setFont (juce::Font (12.0f));
        g.drawText (minorIt->second,
                    juce::Rectangle<float> (cx - centreR, cy - centreR + centreR * 0.9f, centreR * 2, centreR * 1.0f),
                    juce::Justification::centred);
    }
}

int CircleOfFifths::getKeyFromPoint (juce::Point<float> pt) const
{
    for (int i = 0; i < 12; ++i)
    {
        if (pt.getDistanceFrom (keyCentres[i]) <= keyRadius + 4.0f)
            return i;
    }
    return -1;
}

void CircleOfFifths::mouseDown (const juce::MouseEvent& e)
{
    int idx = getKeyFromPoint (e.position);
    if (idx >= 0 && idx < 12)
    {
        selectedKey = MusicTheory::KEYS[(size_t)idx];
        repaint();
        if (onKeySelected)
            onKeySelected (selectedKey);
    }
}

} // namespace TurboTuning

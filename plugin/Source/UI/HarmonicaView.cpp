#include "HarmonicaView.h"

namespace TurboTuning
{

HarmonicaView::HarmonicaView()
{
    setWantsKeyboardFocus (true);

    addAndMakeVisible (blowBtn);
    addAndMakeVisible (drawBtn);

    blowBtn.setToggleable (true);
    drawBtn.setToggleable (true);
    blowBtn.setToggleState (true, juce::dontSendNotification);

    blowBtn.onClick = [this] { setBlowMode (true); };
    drawBtn.onClick = [this] { setBlowMode (false); };

    startTimerHz (30); // for glow animation
}

HarmonicaView::~HarmonicaView()
{
    stopTimer();
}

void HarmonicaView::setBlowMode (bool isBlow)
{
    blowMode = isBlow;
    blowBtn.setToggleState (isBlow, juce::dontSendNotification);
    drawBtn.setToggleState (! isBlow, juce::dontSendNotification);
    repaint();
}

void HarmonicaView::setActiveHole (int hole, bool /*isBlow*/, bool active)
{
    if (hole >= 1 && hole <= 10)
    {
        holeActive[hole - 1] = active;
        repaint();
    }
}

void HarmonicaView::timerCallback()
{
    repaint(); // subtle animation refresh
}

void HarmonicaView::resized()
{
    auto area = getLocalBounds();

    // Blow/Draw buttons at top
    auto btnArea = area.removeFromTop (32).reduced (4, 0);
    auto half = btnArea.getWidth() / 2;
    blowBtn.setBounds (btnArea.removeFromLeft (half).reduced (2, 0));
    drawBtn.setBounds (btnArea.reduced (2, 0));

    // Remaining area is for the harmonica body
    area.removeFromTop (8);
    auto bodyArea = area.reduced (8, 0);

    // Calculate hole rects
    float chromeH = 8.0f;
    float bodyTop = (float)bodyArea.getY() + chromeH;
    float bodyBottom = (float)bodyArea.getBottom() - chromeH - 40.0f; // leave room for note display + labels
    float bodyHeight = bodyBottom - bodyTop;
    float bodyWidth = (float)bodyArea.getWidth();
    float holeGap = 3.0f;
    float holeWidth = (bodyWidth - holeGap * 9.0f) / 10.0f;
    float holeHeight = bodyHeight;

    for (int i = 0; i < 10; ++i)
    {
        float x = (float)bodyArea.getX() + i * (holeWidth + holeGap);
        holeRects[i] = { x, bodyTop, holeWidth, holeHeight };
    }
}

void HarmonicaView::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    // Section card background
    g.setColour (juce::Colour (TurboLookAndFeel::bgPanel));
    g.fillRoundedRectangle (area, 12.0f);
    g.setColour (juce::Colour (TurboLookAndFeel::border));
    g.drawRoundedRectangle (area, 12.0f, 1.0f);

    // Title
    g.setColour (juce::Colour (TurboLookAndFeel::textPrimary));
    g.setFont (juce::Font (14.0f, juce::Font::bold));

    // Chrome trims
    float bodyLeft = holeRects[0].getX();
    float bodyRight = holeRects[9].getRight();
    float bodyWidth = bodyRight - bodyLeft;
    float chromeTopY = holeRects[0].getY() - 8.0f;

    // Top chrome
    juce::ColourGradient topChrome (juce::Colour (0xFF484860), 0, chromeTopY,
                                     juce::Colour (0xFF1A1A2E), 0, chromeTopY + 8.0f, false);
    g.setGradientFill (topChrome);
    g.fillRect (bodyLeft, chromeTopY, bodyWidth, 8.0f);

    // Bottom chrome
    float chromeBottomY = holeRects[0].getBottom();
    juce::ColourGradient botChrome (juce::Colour (0xFF1A1A2E), 0, chromeBottomY,
                                     juce::Colour (0xFF484860), 0, chromeBottomY + 8.0f, false);
    g.setGradientFill (botChrome);
    g.fillRect (bodyLeft, chromeBottomY, bodyWidth, 8.0f);

    // Harmonica background
    g.setColour (juce::Colour (0x4D000000)); // rgba(0,0,0,0.3)
    g.fillRect (bodyLeft, holeRects[0].getY(), bodyWidth, holeRects[0].getHeight());

    // Draw 10 holes
    float time = (float)(juce::Time::getMillisecondCounter() % 1200) / 1200.0f;
    float pulse = 0.5f + 0.5f * std::sin (time * juce::MathConstants<float>::twoPi);

    for (int i = 0; i < 10; ++i)
    {
        auto r = holeRects[i];

        // Hole background
        g.setColour (juce::Colour (0xFF08091A));
        g.fillRoundedRectangle (r, 6.0f);

        // Active glow
        if (holeActive[i])
        {
            auto glowColour = blowMode
                ? juce::Colour (TurboLookAndFeel::gold).withAlpha (0.6f + 0.3f * pulse)
                : juce::Colour (TurboLookAndFeel::blue).withAlpha (0.6f + 0.3f * pulse);

            g.setColour (glowColour);
            g.drawRoundedRectangle (r.expanded (2.0f), 6.0f, 2.5f);

            // Inner glow
            g.setColour (glowColour.withAlpha (0.15f));
            g.fillRoundedRectangle (r, 6.0f);
        }
        else
        {
            g.setColour (juce::Colour (TurboLookAndFeel::border));
            g.drawRoundedRectangle (r, 6.0f, 1.0f);
        }

        // Note label
        int midiNote = MusicTheory::getNote (i + 1, blowMode, currentKey);
        auto noteName = MusicTheory::NOTE_NAMES[(size_t)(midiNote % 12)];
        int octave = midiNote / 12 - 1;

        g.setColour (holeActive[i]
            ? (blowMode ? juce::Colour (TurboLookAndFeel::gold) : juce::Colour (TurboLookAndFeel::blue))
            : juce::Colour (TurboLookAndFeel::textSecondary));
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawText (noteName + juce::String (octave), r, juce::Justification::centred);

        // Hole number below
        g.setColour (juce::Colour (TurboLookAndFeel::textDim));
        g.setFont (juce::Font (10.0f));
        g.drawText (juce::String (i + 1),
                    juce::Rectangle<float> (r.getX(), r.getBottom() + 10.0f, r.getWidth(), 14.0f),
                    juce::Justification::centred);
    }

    // Note display at bottom
    auto noteArea = juce::Rectangle<float> (bodyLeft, holeRects[0].getBottom() + 28.0f,
                                             bodyWidth, 28.0f);
    g.setColour (juce::Colour (TurboLookAndFeel::bgInput));
    g.fillRoundedRectangle (noteArea, 8.0f);
    g.setColour (juce::Colour (TurboLookAndFeel::border));
    g.drawRoundedRectangle (noteArea, 8.0f, 1.0f);

    g.setColour (juce::Colour (TurboLookAndFeel::textDim));
    g.setFont (juce::Font (9.0f));
    g.drawText ("NOTE", noteArea.withWidth (40.0f).withX (noteArea.getX() + 8.0f),
                juce::Justification::centredLeft);

    g.setColour (juce::Colour (TurboLookAndFeel::textPrimary));
    g.setFont (juce::Font (16.0f, juce::Font::bold));
    g.drawText (currentNote.isEmpty() ? juce::String::charToString (0x2014) : currentNote,
                noteArea, juce::Justification::centred);
}

// ── Mouse interaction ──

int HarmonicaView::getHoleFromPoint (juce::Point<float> pt) const
{
    for (int i = 0; i < 10; ++i)
        if (holeRects[i].contains (pt))
            return i + 1;
    return -1;
}

void HarmonicaView::mouseDown (const juce::MouseEvent& e)
{
    int hole = getHoleFromPoint (e.position);
    if (hole >= 1 && hole <= 10)
    {
        holeActive[hole - 1] = true;
        if (onHoleEvent)
            onHoleEvent (hole, blowMode, true);
        repaint();
    }
}

void HarmonicaView::mouseUp (const juce::MouseEvent& e)
{
    int hole = getHoleFromPoint (e.position);
    if (hole >= 1 && hole <= 10)
    {
        holeActive[hole - 1] = false;
        if (onHoleEvent)
            onHoleEvent (hole, blowMode, false);
        repaint();
    }
}

// ── Keyboard interaction ──

bool HarmonicaView::keyPressed (const juce::KeyPress& key)
{
    // Keys 1-0 map to holes 1-10
    const int keyMap[] = { '1','2','3','4','5','6','7','8','9','0' };

    for (int i = 0; i < 10; ++i)
    {
        if (key.getKeyCode() == keyMap[i])
        {
            bool isBlow = ! juce::ModifierKeys::currentModifiers.isShiftDown();
            setBlowMode (isBlow);
            holeActive[i] = true;
            if (onHoleEvent)
                onHoleEvent (i + 1, isBlow, true);
            repaint();
            return true;
        }
    }

    // Space to toggle blow/draw
    if (key.getKeyCode() == juce::KeyPress::spaceKey)
    {
        setBlowMode (! blowMode);
        return true;
    }

    return false;
}

bool HarmonicaView::keyStateChanged (bool isKeyDown)
{
    if (! isKeyDown)
    {
        // Release all holes
        for (int i = 0; i < 10; ++i)
        {
            if (holeActive[i])
            {
                holeActive[i] = false;
                if (onHoleEvent)
                    onHoleEvent (i + 1, blowMode, false);
            }
        }
        repaint();
        return true;
    }
    return false;
}

} // namespace TurboTuning

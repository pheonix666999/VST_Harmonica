#pragma once
#include <JuceHeader.h>
#include "TurboLookAndFeel.h"
#include "../Util/MusicTheory.h"

namespace TurboTuning
{

/**
 * HarmonicaView — Interactive 10-hole harmonica visualisation.
 *
 * Features:
 *   • Chrome trim top/bottom
 *   • 10 clickable holes with note labels
 *   • Blow/Draw toggle buttons
 *   • Active-note glow animation (gold for blow, blue for draw)
 *   • Note display showing currently played note
 */
class HarmonicaView : public juce::Component,
                      public juce::Timer
{
public:
    /** Callback when a hole is played or released. */
    std::function<void(int hole, bool isBlow, bool noteOn)> onHoleEvent;

    HarmonicaView();
    ~HarmonicaView() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setBlowMode (bool isBlow);
    bool isBlowMode() const { return blowMode; }

    void setCurrentKey (const juce::String& key) { currentKey = key; repaint(); }
    void setActiveHole (int hole, bool isBlow, bool active);
    void setCurrentNote (const juce::String& note) { currentNote = note; repaint(); }

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    bool keyPressed (const juce::KeyPress& key) override;
    bool keyStateChanged (bool isKeyDown) override;

private:
    bool blowMode = true;
    juce::String currentKey = "C";
    juce::String currentNote;

    // Active state per hole [0..9]
    bool holeActive[10] = {};

    // UI rectangles for the 10 holes
    juce::Rectangle<float> holeRects[10];

    // Buttons
    juce::TextButton blowBtn { "BLOW" };
    juce::TextButton drawBtn { "DRAW" };

    int getHoleFromPoint (juce::Point<float> pt) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonicaView)
};

} // namespace TurboTuning

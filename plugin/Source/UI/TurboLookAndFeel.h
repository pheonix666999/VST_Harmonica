#pragma once
#include <JuceHeader.h>

namespace TurboTuning
{

/**
 * TurboLookAndFeel — Custom JUCE look and feel matching the demo's dark theme.
 *
 * Design tokens:
 *   • Deep navy backgrounds (#06081a → #0a0d24)
 *   • Gold/amber accents (#E8A832)
 *   • Blue accent (#3A8FD4)
 *   • Glassmorphism panels with blur
 */
class TurboLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // ── Colour Constants ──
    static constexpr juce::uint32 bgBody       = 0xFF06081A;
    static constexpr juce::uint32 bgApp        = 0xFF0A0D24;
    static constexpr juce::uint32 bgPanel      = 0xD00C102A;
    static constexpr juce::uint32 bgCard       = 0xB3101637;
    static constexpr juce::uint32 bgInput      = 0xE6080C20;
    static constexpr juce::uint32 gold         = 0xFFE8A832;
    static constexpr juce::uint32 goldBright   = 0xFFF0C050;
    static constexpr juce::uint32 goldDim      = 0xFFB8841A;
    static constexpr juce::uint32 blue         = 0xFF3A8FD4;
    static constexpr juce::uint32 green        = 0xFF2ECC71;
    static constexpr juce::uint32 red          = 0xFFE74C3C;
    static constexpr juce::uint32 textPrimary  = 0xFFF2F2F8;
    static constexpr juce::uint32 textSecondary= 0xFFA0A0B8;
    static constexpr juce::uint32 textDim      = 0xFF585878;
    static constexpr juce::uint32 border       = 0x33FFFFFF;

    TurboLookAndFeel()
    {
        // Set base colours
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (bgBody));
        setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour (gold));
        setColour (juce::Slider::thumbColourId,               juce::Colour (gold));
        setColour (juce::Slider::trackColourId,               juce::Colour (bgInput));
        setColour (juce::Label::textColourId,                 juce::Colour (textPrimary));
        setColour (juce::TextButton::buttonColourId,          juce::Colour (bgCard));
        setColour (juce::TextButton::textColourOnId,          juce::Colour (gold));
        setColour (juce::TextButton::textColourOffId,         juce::Colour (textSecondary));
        setColour (juce::ToggleButton::textColourId,          juce::Colour (textPrimary));
        setColour (juce::ToggleButton::tickColourId,          juce::Colour (gold));
    }

    // ── Custom rotary slider (knob) ──
    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;

        // Knob body (dark circle)
        g.setColour (juce::Colour (bgInput));
        g.fillEllipse (rx, ry, rw, rw);

        // Border
        g.setColour (juce::Colour (border));
        g.drawEllipse (rx, ry, rw, rw, 1.5f);

        // Arc track (background)
        juce::Path bgArc;
        bgArc.addCentredArc (centreX, centreY, radius - 3.0f, radius - 3.0f,
                             0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (textDim).withAlpha (0.3f));
        g.strokePath (bgArc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Arc track (fill)
        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        juce::Path fillArc;
        fillArc.addCentredArc (centreX, centreY, radius - 3.0f, radius - 3.0f,
                               0.0f, rotaryStartAngle, angle, true);
        g.setColour (juce::Colour (gold));
        g.strokePath (fillArc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Indicator dot
        auto dotRadius = 3.0f;
        auto dotX = centreX + (radius - 8.0f) * std::cos (angle - juce::MathConstants<float>::halfPi);
        auto dotY = centreY + (radius - 8.0f) * std::sin (angle - juce::MathConstants<float>::halfPi);
        g.setColour (juce::Colour (goldBright));
        g.fillEllipse (dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }

    // ── Linear slider ──
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical || style == juce::Slider::LinearHorizontal)
        {
            bool isVertical = (style == juce::Slider::LinearVertical);
            auto trackWidth = 4.0f;

            // Track background
            juce::Rectangle<float> track;
            if (isVertical)
                track = { (float)x + (float)width * 0.5f - trackWidth * 0.5f, (float)y,
                           trackWidth, (float)height };
            else
                track = { (float)x, (float)y + (float)height * 0.5f - trackWidth * 0.5f,
                           (float)width, trackWidth };

            g.setColour (juce::Colour (bgInput));
            g.fillRoundedRectangle (track, 2.0f);

            // Fill
            juce::Rectangle<float> fill;
            if (isVertical)
                fill = { track.getX(), sliderPos, trackWidth, (float)y + (float)height - sliderPos };
            else
                fill = { track.getX(), track.getY(), sliderPos - (float)x, trackWidth };

            g.setColour (juce::Colour (gold));
            g.fillRoundedRectangle (fill, 2.0f);

            // Thumb
            float thumbSize = 12.0f;
            float tx, ty;
            if (isVertical)
            {
                tx = (float)x + (float)width * 0.5f;
                ty = sliderPos;
            }
            else
            {
                tx = sliderPos;
                ty = (float)y + (float)height * 0.5f;
            }

            g.setColour (juce::Colour (goldBright));
            g.fillEllipse (tx - thumbSize * 0.5f, ty - thumbSize * 0.5f, thumbSize, thumbSize);
            g.setColour (juce::Colour (gold));
            g.drawEllipse (tx - thumbSize * 0.5f, ty - thumbSize * 0.5f, thumbSize, thumbSize, 1.5f);
        }
        else
        {
            LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    // ── Button ──
    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        auto cornerSize = 8.0f;

        auto baseColour = backgroundColour;
        if (isButtonDown)
            baseColour = baseColour.brighter (0.2f);
        else if (isMouseOverButton)
            baseColour = baseColour.brighter (0.1f);

        g.setColour (baseColour);
        g.fillRoundedRectangle (bounds, cornerSize);

        g.setColour (juce::Colour (border));
        g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto toggleWidth = 44.0f;
        auto toggleHeight = 22.0f;

        auto toggleBounds = juce::Rectangle<float> (
            bounds.getRight() - toggleWidth - 4.0f,
            bounds.getCentreY() - toggleHeight * 0.5f,
            toggleWidth, toggleHeight);

        // Track
        auto trackColour = button.getToggleState() ? juce::Colour (gold) : juce::Colour (bgInput);
        g.setColour (trackColour);
        g.fillRoundedRectangle (toggleBounds, toggleHeight * 0.5f);

        // Border
        g.setColour (juce::Colour (border));
        g.drawRoundedRectangle (toggleBounds, toggleHeight * 0.5f, 1.0f);

        // Knob
        float knobDiameter = toggleHeight - 4.0f;
        float knobX = button.getToggleState()
                          ? toggleBounds.getRight() - knobDiameter - 2.0f
                          : toggleBounds.getX() + 2.0f;
        float knobY = toggleBounds.getY() + 2.0f;

        g.setColour (juce::Colour (textPrimary));
        g.fillEllipse (knobX, knobY, knobDiameter, knobDiameter);

        // Label text
        g.setColour (juce::Colour (textPrimary));
        g.setFont (juce::Font (13.0f));
        g.drawText (button.getButtonText(),
                    bounds.withRight (toggleBounds.getX() - 4.0f).toNearestInt(),
                    juce::Justification::centredLeft, true);
    }
};

} // namespace TurboTuning

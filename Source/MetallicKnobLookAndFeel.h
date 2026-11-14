#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class MetallicKnobLookAndFeel : public juce::LookAndFeel_V4 {
public:
    MetallicKnobLookAndFeel() {}

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override
    {
        if (! slider.isEnabled())
            g.setOpacity(0.4f); // greyed-out look when locked / bypassed

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f - 6.0f;
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;

        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Outer Glow Arc Track
        juce::Path arcPath;
        arcPath.addCentredArc(centreX, centreY, radius + 3.0f, radius + 3.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0x309370db));
        g.strokePath(arcPath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Active Arc
        juce::Path activeArc;
        activeArc.addCentredArc(centreX, centreY, radius + 3.0f, radius + 3.0f, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colour(0xffc084fc));
        g.strokePath(activeArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Outer Bezel
        g.setColour(juce::Colour(0xff181628));
        g.fillEllipse(rx - 2.0f, ry - 2.0f, rw + 4.0f, rw + 4.0f);

        // Metallic Body Gradient
        juce::ColourGradient metallicGrad(
            juce::Colour(0xffe0e5ff), centreX - radius * 0.5f, centreY - radius * 0.5f,
            juce::Colour(0xff2a2640), centreX + radius * 0.5f, centreY + radius * 0.5f,
            true
        );
        metallicGrad.addColour(0.3, juce::Colour(0xff8c8aa5));
        metallicGrad.addColour(0.7, juce::Colour(0xff12101d));
        g.setGradientFill(metallicGrad);
        g.fillEllipse(rx, ry, rw, rw);

        // Inner Metal Cap
        juce::ColourGradient innerGrad(
            juce::Colour(0xff4a4468), centreX, centreY - radius * 0.6f,
            juce::Colour(0xff151226), centreX, centreY + radius * 0.6f,
            false
        );
        g.setGradientFill(innerGrad);
        g.fillEllipse(rx + 4.0f, ry + 4.0f, rw - 8.0f, rw - 8.0f);

        // Indicator Line
        juce::Path pointerPath;
        auto pointerLength = radius * 0.65f;
        auto pointerThickness = 3.0f;
        pointerPath.addRoundedRectangle(-pointerThickness * 0.5f, -radius + 4.0f, pointerThickness, pointerLength, 1.5f);
        pointerPath.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        
        g.setColour(juce::Colour(0xffe9d5ff));
        g.fillPath(pointerPath);
    }
};

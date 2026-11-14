#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// "Aurora Teal" look: charcoal metal + ice-cyan neon accents.
class MetallicKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static juce::Colour accent()        { return juce::Colour (0xff5fd9ff); }
    static juce::Colour accentBright()  { return juce::Colour (0xffbdf3ff); }
    static juce::Colour panelDark()     { return juce::Colour (0xff14161a); }
    static juce::Colour panelBorder()   { return juce::Colour (0xff454c54); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        if (! slider.isEnabled())
            g.setOpacity (0.45f);

        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const float bodyR = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f - 11.0f;
        const float arcR  = bodyR + 6.0f;

        const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        const float range = rotaryEndAngle - rotaryStartAngle;
        const int numSegs = 34;
        const float gap = (range / (float) numSegs) * 0.38f;

        auto segmentPath = [&](int i) {
            const float t0 = (float) i / (float) numSegs;
            const float t1 = (float) (i + 1) / (float) numSegs;
            juce::Path p;
            p.addCentredArc (cx, cy, arcR, arcR, 0.0f,
                             rotaryStartAngle + range * t0 + gap * 0.5f,
                             rotaryStartAngle + range * t1 - gap * 0.5f, false);
            return p;
        };

        // glow pass behind the active segments
        g.setColour (accent().withAlpha (0.22f));
        for (int i = 0; i < numSegs; ++i)
            if (sliderPosProportional > (float) i / (float) numSegs)
                g.strokePath (segmentPath (i), juce::PathStrokeType (5.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // segment pass
        for (int i = 0; i < numSegs; ++i)
        {
            const bool active = sliderPosProportional > (float) i / (float) numSegs;
            g.setColour (active ? accent() : juce::Colour (0xff333940));
            g.strokePath (segmentPath (i), juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // knob body: brushed metal gradient
        juce::ColourGradient bodyGrad (juce::Colour (0xff4b525a), cx - bodyR * 0.45f, cy - bodyR * 0.55f,
                                       juce::Colour (0xff14161a), cx + bodyR * 0.55f, cy + bodyR * 0.70f, true);
        bodyGrad.addColour (0.45, juce::Colour (0xff272c32));
        g.setGradientFill (bodyGrad);
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);

        g.setColour (juce::Colour (0xff0c0e10));
        g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.5f);

        // inner face
        const float inner = bodyR - 7.0f;
        juce::ColourGradient innerGrad (juce::Colour (0xff3a4047), cx, cy - inner,
                                        juce::Colour (0xff171a1e), cx, cy + inner, false);
        g.setGradientFill (innerGrad);
        g.fillEllipse (cx - inner, cy - inner, inner * 2.0f, inner * 2.0f);

        // top bevel highlight
        juce::Path bevel;
        bevel.addCentredArc (cx, cy, inner - 1.5f, inner - 1.5f, 0.0f,
                             rotaryStartAngle + range * 0.30f, rotaryStartAngle + range * 0.70f, false);
        g.setColour (juce::Colour (0x2effffff));
        g.strokePath (bevel, juce::PathStrokeType (1.5f));

        // indicator: glowing thin line
        juce::Path pointer;
        const float pointerLength = inner - 5.0f;
        pointer.addRoundedRectangle (-1.25f, -inner + 4.0f, 2.5f, pointerLength, 1.25f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (cx, cy));

        g.setColour (accent().withAlpha (0.30f));
        g.strokePath (pointer, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xfff2fbff));
        g.fillPath (pointer);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& /*backgroundColour*/,
                               bool highlighted, bool down) override
    {
        auto b = button.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = button.getClickingTogglesState() && button.getToggleState();
        const float radius = juce::jmin (6.0f, b.getHeight() * 0.35f);

        if (on)
        {
            g.setColour (accent().withAlpha (0.28f));
            g.fillRoundedRectangle (b.expanded (2.0f), radius + 2.0f);
        }

        juce::Colour fill = on ? juce::Colour (0xff5fd9ff) : juce::Colour (0xff23272c);

        if (down)
            fill = fill.brighter (0.15f);
        else if (highlighted && ! on)
            fill = juce::Colour (0xff2c3138);

        g.setColour (fill);
        g.fillRoundedRectangle (b, radius);

        g.setColour (on ? juce::Colour (0xffbdf3ff) : panelBorder());
        g.drawRoundedRectangle (b, radius, 1.0f);
    }
};

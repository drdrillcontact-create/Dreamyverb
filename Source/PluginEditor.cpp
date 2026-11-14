#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    // ---- Aurora Teal palette ----
    constexpr juce::uint32 panelTop    = 0xff2e3238;
    constexpr juce::uint32 panelBottom = 0xff1b1e22;
    constexpr juce::uint32 frameCol    = 0xff454c54;
    constexpr juce::uint32 chipBg      = 0xff14171a;
    constexpr juce::uint32 chipBorder  = 0xff454c54;
    constexpr juce::uint32 labelText   = 0xffccd3da;
    constexpr juce::uint32 valueText   = 0xffe6ebf0;
    constexpr juce::uint32 titleCol    = 0xff7ff0ff;
    constexpr juce::uint32 accentCol   = 0xff5fd9ff;
    constexpr juce::uint32 meterUnlit  = 0xff333940;
    constexpr juce::uint32 meterTeal   = 0xff4de3c1;
    constexpr juce::uint32 meterAmber  = 0xffd9a441;
    constexpr juce::uint32 meterRed    = 0xffff5f6d;
}

DreamyReverbAudioProcessorEditor::DreamyReverbAudioProcessorEditor (DreamyReverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    presetBox.addItemList (juce::StringArray { "CELESTIAL VOID", "CRYSTAL CAVE", "ETHEREAL SHIMMER" }, 1);
    presetBox.setTextWhenNothingSelected ("PRESET: ...");
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (chipBg));
    presetBox.setColour (juce::ComboBox::outlineColourId, juce::Colour (chipBorder));
    presetBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xffcfd6dd));
    presetBox.setColour (juce::ComboBox::arrowColourId, juce::Colour (accentCol));
    presetBox.onChange = [this] { applyPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    auto setupBtn = [this](juce::TextButton& btn) {
        btn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff23272c));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (accentCol));
        btn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffaab3bb));
        btn.setColour (juce::TextButton::textColourOnId, juce::Colour (0xff061014));
        btn.setLookAndFeel (&customKnobLAF);
        addAndMakeVisible (btn);
    };

    setupBtn (bypassButton);
    bypassButton.setClickingTogglesState (true);
    bypassButton.setButtonText ("BYPASS");
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.parameters, "bypass", bypassButton);

    auto setupKnob = [this](juce::Slider& slider, juce::Label& lbl, const juce::String& name, juce::Label& valLbl) {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel (&customKnobLAF);
        addAndMakeVisible (slider);

        lbl.setText (name, juce::dontSendNotification);
        lbl.setJustificationType (juce::Justification::centred);
        lbl.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        lbl.setColour (juce::Label::textColourId, juce::Colour (labelText));
        addAndMakeVisible (lbl);

        valLbl.setJustificationType (juce::Justification::centred);
        valLbl.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::plain)));
        valLbl.setColour (juce::Label::textColourId, juce::Colour (valueText));
        addAndMakeVisible (valLbl);
    };

    setupKnob (decaySlider, decayLabel, "DECAY", decayValLabel);
    setupKnob (sizeSlider, sizeLabel, "SIZE", sizeValLabel);
    setupKnob (dampingSlider, dampingLabel, "DAMPING", dampingValLabel);
    setupKnob (shimmerSlider, shimmerLabel, "SHIMMER", shimmerValLabel);
    setupKnob (mixSlider, mixLabel, "MIX", mixValLabel);

    decaySlider.onValueChange   = [this] { refreshValueLabels(); };
    sizeSlider.onValueChange    = [this] { refreshValueLabels(); };
    dampingSlider.onValueChange = [this] { refreshValueLabels(); };
    shimmerSlider.onValueChange = [this] { refreshValueLabels(); };
    mixSlider.onValueChange     = [this] { refreshValueLabels(); };

    decayAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "decay", decaySlider);
    sizeAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "size", sizeSlider);
    dampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "damping", dampingSlider);
    shimmerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "shimmer", shimmerSlider);
    mixAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.parameters, "mix", mixSlider);

    refreshValueLabels();

    setupBtn (hpfBtn); setupBtn (lpfBtn); setupBtn (depthBtn); setupBtn (rateBtn); setupBtn (lockBtn);

    hpfBtn.setClickingTogglesState (true);
    lpfBtn.setClickingTogglesState (true);
    hpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.parameters, "hpf", hpfBtn);
    lpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.parameters, "lpf", lpfBtn);

    auto styleReadout = [this](juce::Label& l) {
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::plain)));
        l.setColour (juce::Label::textColourId, juce::Colour (valueText));
        addAndMakeVisible (l);
    };
    styleReadout (hpfValLabel);
    styleReadout (lpfValLabel);
    styleReadout (depthValLabel);
    styleReadout (rateValLabel);

    hpfValLabel.setText ("120 Hz", juce::dontSendNotification);
    lpfValLabel.setText ("8.0 kHz", juce::dontSendNotification);

    depthBtn.onClick = [this] {
        static const float steps[] = { 0.0f, 25.0f, 50.0f, 75.0f, 100.0f };
        const float cur = *audioProcessor.parameters.getRawParameterValue ("depth");
        float next = steps[0];
        bool found = false;
        for (float s : steps)
            if (s > cur + 0.01f) { next = s; found = true; break; }
        if (! found) next = steps[0];
        auto* param = audioProcessor.parameters.getParameter ("depth");
        param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (next));
        updateModReadouts();
    };

    rateBtn.onClick = [this] {
        static const float steps[] = { 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
        const float cur = *audioProcessor.parameters.getRawParameterValue ("rate");
        float next = steps[0];
        bool found = false;
        for (float s : steps)
            if (s > cur + 0.01f) { next = s; found = true; break; }
        if (! found) next = steps[0];
        auto* param = audioProcessor.parameters.getParameter ("rate");
        param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (next));
        updateModReadouts();
    };

    lockBtn.setClickingTogglesState (true);
    lockBtn.onClick = [this] {
        paramsLocked = lockBtn.getToggleState();
        updateEnabledState (true);
    };

    updateModReadouts();

    decaySlider.sendLookAndFeelChange();
    sizeSlider.sendLookAndFeelChange();
    dampingSlider.sendLookAndFeelChange();
    shimmerSlider.sendLookAndFeelChange();
    mixSlider.sendLookAndFeelChange();

    setSize (880, 420);
    startTimerHz (30);
}

DreamyReverbAudioProcessorEditor::~DreamyReverbAudioProcessorEditor()
{
    for (auto* s : { &decaySlider, &sizeSlider, &dampingSlider, &shimmerSlider, &mixSlider })
        s->setLookAndFeel (nullptr);

    for (auto* b : { &bypassButton, &hpfBtn, &lpfBtn, &depthBtn, &rateBtn, &lockBtn })
        b->setLookAndFeel (nullptr);
}

void DreamyReverbAudioProcessorEditor::applyPreset (int presetIndex)
{
    struct Preset { float decay, size, damping, shimmer, mix; };
    static const Preset presets[] = {
        { 7.5f,  95.0f,  5500.0f,  8.0f, 45.0f }, // CELESTIAL VOID
        { 3.2f,  55.0f,  9500.0f,  4.0f, 35.0f }, // CRYSTAL CAVE
        { 9.0f, 100.0f, 12000.0f, 12.0f, 60.0f }, // ETHEREAL SHIMMER
    };

    if (presetIndex < 0 || presetIndex >= (int) (sizeof (presets) / sizeof (presets[0])))
        return;

    auto setParam = [this](const char* id, float value) {
        auto* param = audioProcessor.parameters.getParameter (id);
        param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (value));
    };

    setParam ("decay",   presets[presetIndex].decay);
    setParam ("size",    presets[presetIndex].size);
    setParam ("damping", presets[presetIndex].damping);
    setParam ("shimmer", presets[presetIndex].shimmer);
    setParam ("mix",     presets[presetIndex].mix);
}

void DreamyReverbAudioProcessorEditor::refreshValueLabels()
{
    decayValLabel.setText   (juce::String (decaySlider.getValue(), 1) + "s", juce::dontSendNotification);
    sizeValLabel.setText    (juce::String ((int) sizeSlider.getValue()) + "%", juce::dontSendNotification);
    dampingValLabel.setText (juce::String (dampingSlider.getValue() / 1000.0f, 1) + " kHz", juce::dontSendNotification);
    shimmerValLabel.setText ("+" + juce::String ((int) shimmerSlider.getValue()) + " st", juce::dontSendNotification);
    mixValLabel.setText     (juce::String ((int) mixSlider.getValue()) + "%", juce::dontSendNotification);
}

void DreamyReverbAudioProcessorEditor::updateModReadouts()
{
    const int d = (int) std::round (*audioProcessor.parameters.getRawParameterValue ("depth"));
    const float r = *audioProcessor.parameters.getRawParameterValue ("rate");
    depthValLabel.setText (juce::String (d) + "%", juce::dontSendNotification);
    rateValLabel.setText (juce::String (r, 2) + " Hz", juce::dontSendNotification);
}

void DreamyReverbAudioProcessorEditor::updateEnabledState (bool force)
{
    const bool bypassed = *audioProcessor.parameters.getRawParameterValue ("bypass") > 0.5f;
    const bool shouldEnable = ! paramsLocked && ! bypassed;

    bypassButton.setButtonText (bypassed ? "BYPASS ON" : "BYPASS");

    if (force || shouldEnable != lastEnabledState)
    {
        lastEnabledState = shouldEnable;
        for (auto* s : { &decaySlider, &sizeSlider, &dampingSlider, &shimmerSlider, &mixSlider })
            s->setEnabled (shouldEnable);
    }
}

void DreamyReverbAudioProcessorEditor::timerCallback()
{
    wavePhase += 0.05f;
    if (wavePhase > juce::MathConstants<float>::twoPi)
        wavePhase -= juce::MathConstants<float>::twoPi;

    // meters: follow the processor peaks with a smooth decay
    for (int ch = 0; ch < 2; ++ch)
    {
        const float peak = audioProcessor.outputPeaks[ch].load (std::memory_order_relaxed);
        meterLevel[ch] = juce::jmax (peak, meterLevel[ch] - 0.025f);
    }

    repaint (16, 10, 850, 225); // header (meters/title/bypass) + display
    updateEnabledState();
}

void DreamyReverbAudioProcessorEditor::drawLevelMeter (juce::Graphics& g, juce::Rectangle<int> bar, float level)
{
    const int segs = 24;
    const float gap = 1.5f;
    const float segW = (bar.getWidth() - gap * (float) (segs - 1)) / (float) segs;

    float db = level > 1.0e-6f ? 20.0f * std::log10 (level) : -60.0f;
    const float norm = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
    const int lit = (int) std::ceil (norm * (float) segs);

    for (int i = 0; i < segs; ++i)
    {
        const auto r = juce::Rectangle<float> (bar.getX() + i * (segW + gap), (float) bar.getY(), segW, (float) bar.getHeight());
        if (i < lit)
        {
            if (i >= segs - 3)       g.setColour (juce::Colour (meterRed));
            else if (i >= segs - 6)  g.setColour (juce::Colour (meterAmber));
            else                     g.setColour (juce::Colour (meterTeal));
        }
        else
        {
            g.setColour (juce::Colour (meterUnlit));
        }
        g.fillRoundedRectangle (r, 2.0f);
    }
}

void DreamyReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    // ---- brushed charcoal panel ----
    juce::ColourGradient bg (juce::Colour (panelTop), 0.0f, 0.0f, juce::Colour (panelBottom), 0.0f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    g.setColour (juce::Colours::white.withAlpha (0.025f));
    for (int x = 10; x < getWidth() - 8; x += 6)
        g.drawVerticalLine (x, 8.0f, (float) getHeight() - 8.0f);

    // outer frame
    g.setColour (juce::Colour (frameCol));
    g.drawRoundedRectangle (juce::Rectangle<float> (6.0f, 6.0f, (float) getWidth() - 12.0f, (float) getHeight() - 12.0f), 10.0f, 2.0f);

    // ---- title with glow ----
    const auto titleArea = juce::Rectangle<int> (250, 13, 360, 36);
    g.setFont (juce::Font (juce::FontOptions (27.0f, juce::Font::bold)));

    g.setColour (juce::Colour (titleCol).withAlpha (0.16f));
    for (int dx = -2; dx <= 2; ++dx)
        for (int dy = -2; dy <= 2; ++dy)
            if (dx != 0 || dy != 0)
                g.drawText ("DREAMY REVERB", titleArea.translated (dx, dy), juce::Justification::centred, true);

    g.setColour (juce::Colour (titleCol).withAlpha (0.35f));
    g.drawText ("DREAMY REVERB", titleArea, juce::Justification::centred, true);

    g.setColour (juce::Colour (0xffd9fbff));
    g.drawText ("DREAMY REVERB", titleArea, juce::Justification::centred, true);

    // ---- L / R level meters ----
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.setColour (juce::Colour (0xff8b949e));

    auto meterRow = [&](int y, int channel) {
        const auto labelAreaL = juce::Rectangle<int> (24, y, 12, 12);
        const auto bar = juce::Rectangle<int> (40, y, 188, 11);
        const auto labelAreaR = juce::Rectangle<int> (234, y, 12, 12);
        g.drawText (channel == 0 ? "L" : "R", labelAreaL, juce::Justification::centred, true);
        g.drawText (channel == 0 ? "L" : "R", labelAreaR, juce::Justification::centred, true);
        drawLevelMeter (g, bar, meterLevel[channel]);
    };

    meterRow (17, 0);
    meterRow (33, 1);

    // ---- aurora display ----
    const auto d = juce::Rectangle<float> (24.0f, 58.0f, (float) getWidth() - 48.0f, 170.0f);

    g.saveState();
    g.reduceClipRegion (d.toNearestInt());

    juce::ColourGradient sky (juce::Colour (0xff081521), d.getX(), d.getY(),
                              juce::Colour (0xff030a11), d.getX(), d.getBottom(), false);
    g.setGradientFill (sky);
    g.fillRoundedRectangle (d, 10.0f);

    const float horizon = d.getY() + d.getHeight() * 0.70f;
    const float bottom  = d.getBottom();

    // perspective grid floor
    g.setColour (juce::Colour (0xff2f9cc4).withAlpha (0.55f));
    g.drawHorizontalLine ((int) horizon, d.getX(), d.getRight());

    for (int i = 1; i <= 6; ++i)
    {
        const float t = (float) i / 6.0f;
        const float y = horizon + (bottom - horizon) * std::pow (t, 1.7f);
        g.setColour (juce::Colour (0xff2f9cc4).withAlpha (0.10f + 0.14f * t));
        g.drawHorizontalLine ((int) y, d.getX(), d.getRight());
    }

    const float vpX = d.getCentreX();
    const float step = d.getWidth() / 14.0f;
    for (float bx = d.getX(); bx <= d.getRight() + 1.0f; bx += step)
    {
        const float sx = vpX + (bx - vpX) * 0.08f;
        g.setColour (juce::Colour (0xff2f9cc4).withAlpha (0.20f));
        g.drawLine (sx, horizon + 1.0f, bx, bottom, 1.2f);
    }

    // stars (twinkling)
    for (int i = 0; i < 40; ++i)
    {
        const float rx = std::sin ((float) i * 12.9898f) * 43758.5453f;
        const float ry = std::sin ((float) i * 78.233f) * 12543.8567f;
        const float px = d.getX() + (rx - std::floor (rx)) * d.getWidth();
        const float py = d.getY() + (ry - std::floor (ry)) * d.getHeight() * 0.62f;
        const float tw = 0.35f + 0.45f * std::abs (std::sin (wavePhase * 0.7f + (float) i * 1.7f));
        g.setColour (juce::Colours::white.withAlpha (tw));
        const float sz = 1.2f + std::sin ((float) i) * 0.4f;
        g.fillEllipse (px, py, sz, sz);
    }

    // aurora ribbons
    struct Ribbon { juce::uint32 colour; float baseFrac; float ampFrac; float speed; float widthScale; };
    static const Ribbon ribs[] = {
        { 0xff4dffb8, 0.32f, 0.13f, 1.00f, 1.00f },
        { 0xff3fd9ff, 0.47f, 0.16f, 0.80f, 1.10f },
        { 0xffa66cff, 0.24f, 0.10f, 1.30f, 0.75f },
    };

    for (int k = 0; k < 3; ++k)
    {
        const auto& rib = ribs[k];
        juce::Path p;
        bool first = true;
        for (float px = d.getX() - 4.0f; px <= d.getRight() + 4.0f; px += 4.0f)
        {
            const float t = (px - d.getX()) / d.getWidth();
            const float y = d.getY() + d.getHeight() * rib.baseFrac
                          + std::sin (t * 6.0f  + wavePhase * rib.speed + (float) k * 2.0f) * d.getHeight() * rib.ampFrac
                          + std::sin (t * 11.0f - wavePhase * rib.speed * 0.7f + (float) k * 1.3f) * d.getHeight() * rib.ampFrac * 0.45f;
            if (first) { p.startNewSubPath (px, y); first = false; }
            else       { p.lineTo (px, y); }
        }

        const auto col = juce::Colour (rib.colour);
        struct Pass { float w; float a; };
        static const Pass passes[] = { { 26.0f, 0.09f }, { 14.0f, 0.16f }, { 6.0f, 0.45f }, { 2.5f, 0.90f } };
        for (const auto& pass : passes)
        {
            g.setColour (col.withAlpha (pass.a));
            g.strokePath (p, juce::PathStrokeType (pass.w * rib.widthScale, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    g.restoreState();

    g.setColour (juce::Colour (0xff3a4550));
    g.drawRoundedRectangle (d, 10.0f, 1.5f);

    // ---- divider above the bottom bar ----
    g.setColour (juce::Colour (0xff58616a).withAlpha (0.45f));
    g.drawHorizontalLine (359, 24.0f, (float) getWidth() - 24.0f);
    g.setColour (juce::Colour (0xff0e0f11).withAlpha (0.7f));
    g.drawHorizontalLine (361, 24.0f, (float) getWidth() - 24.0f);

    // ---- value chips ----
    auto drawChip = [&](const juce::Rectangle<int>& r) {
        g.setColour (juce::Colour (chipBg));
        g.fillRoundedRectangle (r.toFloat(), 5.0f);
        g.setColour (juce::Colour (chipBorder));
        g.drawRoundedRectangle (r.toFloat(), 5.0f, 1.0f);
    };

    for (const auto* r : { &decayValBox, &sizeValBox, &dampingValBox, &shimmerValBox, &mixValBox,
                           &hpfValBox, &lpfValBox, &depthValBox, &rateValBox })
        drawChip (*r);
}

void DreamyReverbAudioProcessorEditor::resized()
{
    presetBox.setBounds (624, 17, 140, 26);
    bypassButton.setBounds (774, 17, 92, 26);

    // knobs: 5 columns of 164 px
    const int knobY = 242;
    const int knobSize = 80;
    const int colW = 164;
    const int colX0 = 30;

    struct Group { juce::Slider* s; juce::Label* val; juce::Label* name; juce::Rectangle<int>* box; };
    const Group groups[] = {
        { &decaySlider,   &decayValLabel,   &decayLabel,   &decayValBox },
        { &sizeSlider,    &sizeValLabel,    &sizeLabel,    &sizeValBox },
        { &dampingSlider, &dampingValLabel, &dampingLabel, &dampingValBox },
        { &shimmerSlider, &shimmerValLabel, &shimmerLabel, &shimmerValBox },
        { &mixSlider,     &mixValLabel,     &mixLabel,     &mixValBox },
    };

    for (int i = 0; i < 5; ++i)
    {
        const int colX = colX0 + i * colW;
        groups[i].s->setBounds (colX, knobY, knobSize, knobSize);
        *groups[i].box = juce::Rectangle<int> (colX + 86, knobY + 28, 74, 24);
        groups[i].val->setBounds (*groups[i].box);
        groups[i].name->setBounds (colX - 8, knobY + knobSize + 6, 96, 16);
    }

    // bottom bar
    const int btnY = 374;
    hpfBtn.setBounds     (40,  btnY, 56, 28);
    hpfValBox = juce::Rectangle<int> (100, btnY + 3, 86, 22);
    hpfValLabel.setBounds  (hpfValBox);

    lpfBtn.setBounds     (210, btnY, 56, 28);
    lpfValBox = juce::Rectangle<int> (270, btnY + 3, 86, 22);
    lpfValLabel.setBounds  (lpfValBox);

    depthBtn.setBounds   (400, btnY, 74, 28);
    depthValBox = juce::Rectangle<int> (478, btnY + 3, 64, 22);
    depthValLabel.setBounds  (depthValBox);

    rateBtn.setBounds    (570, btnY, 64, 28);
    rateValBox = juce::Rectangle<int> (638, btnY + 3, 70, 22);
    rateValLabel.setBounds  (rateValBox);

    lockBtn.setBounds    (740, btnY, 80, 28);
}

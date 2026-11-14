#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

DreamyReverbAudioProcessorEditor::DreamyReverbAudioProcessorEditor(DreamyReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {

    presetBox.addItem("CELESTIAL VOID", 1);
    presetBox.addItem("CRYSTAL CAVE", 2);
    presetBox.addItem("ETHEREAL SHIMMER", 3);
    presetBox.setTextWhenNothingSelected("PRESETS");
    presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1f1b2e));
    presetBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff4a3e6d));
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    presetBox.onChange = [this] { applyPreset(presetBox.getSelectedId() - 1); };
    addAndMakeVisible(presetBox);

    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff60a5fa));
    addAndMakeVisible(bypassButton);

    auto setupKnob = [this](juce::Slider& slider, juce::Label& lbl, const juce::String& name, juce::Label& valLbl) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel(&customKnobLAF);
        addAndMakeVisible(slider);

        lbl.setText(name, juce::dontSendNotification);
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        lbl.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
        addAndMakeVisible(lbl);

        valLbl.setJustificationType(juce::Justification::centred);
        valLbl.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::plain)));
        valLbl.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
        addAndMakeVisible(valLbl);
    };

    setupKnob(decaySlider, decayLabel, "DECAY", decayValLabel);
    setupKnob(sizeSlider, sizeLabel, "SIZE", sizeValLabel);
    setupKnob(dampingSlider, dampingLabel, "DAMPING", dampingValLabel);
    setupKnob(shimmerSlider, shimmerLabel, "SHIMMER", shimmerValLabel);
    setupKnob(mixSlider, mixLabel, "MIX", mixValLabel);

    decaySlider.onValueChange = [this]() { refreshValueLabels(); };
    sizeSlider.onValueChange = [this]() { refreshValueLabels(); };
    dampingSlider.onValueChange = [this]() { refreshValueLabels(); };
    shimmerSlider.onValueChange = [this]() { refreshValueLabels(); };
    mixSlider.onValueChange = [this]() { refreshValueLabels(); };

    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "decay", decaySlider);
    sizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "size", sizeSlider);
    dampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "damping", dampingSlider);
    shimmerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "shimmer", shimmerSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "mix", mixSlider);

    refreshValueLabels();

    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "bypass", bypassButton);

    auto setupBtn = [this](juce::TextButton& btn) {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e1b2e));
        btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4c1d95));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcbd5e1));
        btn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        addAndMakeVisible(btn);
    };

    setupBtn(hpfBtn); setupBtn(lpfBtn); setupBtn(depthBtn); setupBtn(rateBtn); setupBtn(lockBtn);

    hpfBtn.setClickingTogglesState(true);
    lpfBtn.setClickingTogglesState(true);
    hpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "hpf", hpfBtn);
    lpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.parameters, "lpf", lpfBtn);

    depthBtn.onClick = [this] {
        static const float steps[] = { 0.0f, 25.0f, 50.0f, 75.0f, 100.0f };
        const float cur = *audioProcessor.parameters.getRawParameterValue("depth");
        float next = steps[0];
        bool found = false;
        for (float s : steps) {
            if (s > cur + 0.01f) { next = s; found = true; break; }
        }
        if (!found) next = steps[0];
        auto* param = audioProcessor.parameters.getParameter("depth");
        param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1(next));
        updateDepthButtonText();
    };

    rateBtn.onClick = [this] {
        static const float steps[] = { 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
        const float cur = *audioProcessor.parameters.getRawParameterValue("rate");
        float next = steps[0];
        bool found = false;
        for (float s : steps) {
            if (s > cur + 0.01f) { next = s; found = true; break; }
        }
        if (!found) next = steps[0];
        auto* param = audioProcessor.parameters.getParameter("rate");
        param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1(next));
        updateRateButtonText();
    };

    lockBtn.setClickingTogglesState(true);
    lockBtn.onClick = [this] {
        paramsLocked = lockBtn.getToggleState();
        updateEnabledState(true);
    };

    updateDepthButtonText();
    updateRateButtonText();

    decaySlider.sendLookAndFeelChange();
    sizeSlider.sendLookAndFeelChange();
    dampingSlider.sendLookAndFeelChange();
    shimmerSlider.sendLookAndFeelChange();
    mixSlider.sendLookAndFeelChange();

    setSize(800, 450);
    startTimerHz(30);
}

DreamyReverbAudioProcessorEditor::~DreamyReverbAudioProcessorEditor() {
    decaySlider.setLookAndFeel(nullptr);
    sizeSlider.setLookAndFeel(nullptr);
    dampingSlider.setLookAndFeel(nullptr);
    shimmerSlider.setLookAndFeel(nullptr);
    mixSlider.setLookAndFeel(nullptr);
}

void DreamyReverbAudioProcessorEditor::applyPreset(int presetIndex) {
    struct Preset { float decay, size, damping, shimmer, mix; };
    static const Preset presets[] = {
        { 7.5f,  95.0f,  5500.0f,  8.0f, 45.0f }, // CELESTIAL VOID
        { 3.2f,  55.0f,  9500.0f,  4.0f, 35.0f }, // CRYSTAL CAVE
        { 9.0f, 100.0f, 12000.0f, 12.0f, 60.0f }, // ETHEREAL SHIMMER
    };

    if (presetIndex < 0 || presetIndex >= (int)(sizeof(presets) / sizeof(presets[0])))
        return;

    auto setParam = [this](const char* id, float value) {
        auto* param = audioProcessor.parameters.getParameter(id);
        param->setValueNotifyingHost(param->getNormalisableRange().convertTo0to1(value));
    };

    setParam("decay",   presets[presetIndex].decay);
    setParam("size",    presets[presetIndex].size);
    setParam("damping", presets[presetIndex].damping);
    setParam("shimmer", presets[presetIndex].shimmer);
    setParam("mix",     presets[presetIndex].mix);
}

void DreamyReverbAudioProcessorEditor::refreshValueLabels() {
    decayValLabel.setText(juce::String(decaySlider.getValue(), 1) + "s", juce::dontSendNotification);
    sizeValLabel.setText(juce::String((int)sizeSlider.getValue()) + "%", juce::dontSendNotification);
    dampingValLabel.setText(juce::String(dampingSlider.getValue() / 1000.0f, 1) + "kHz", juce::dontSendNotification);
    shimmerValLabel.setText("+" + juce::String((int)shimmerSlider.getValue()) + " st", juce::dontSendNotification);
    mixValLabel.setText(juce::String((int)mixSlider.getValue()) + "%", juce::dontSendNotification);
}

void DreamyReverbAudioProcessorEditor::updateDepthButtonText() {
    const int v = (int)std::round(*audioProcessor.parameters.getRawParameterValue("depth"));
    depthBtn.setButtonText("DPT " + juce::String(v) + "%");
}

void DreamyReverbAudioProcessorEditor::updateRateButtonText() {
    const float v = *audioProcessor.parameters.getRawParameterValue("rate");
    rateBtn.setButtonText("RATE " + juce::String(v, 2));
}

void DreamyReverbAudioProcessorEditor::updateEnabledState(bool force) {
    const bool bypassed = *audioProcessor.parameters.getRawParameterValue("bypass") > 0.5f;
    const bool shouldEnable = !paramsLocked && !bypassed;

    if (force || shouldEnable != lastEnabledState) {
        lastEnabledState = shouldEnable;
        decaySlider.setEnabled(shouldEnable);
        sizeSlider.setEnabled(shouldEnable);
        dampingSlider.setEnabled(shouldEnable);
        shimmerSlider.setEnabled(shouldEnable);
        mixSlider.setEnabled(shouldEnable);
    }
}

void DreamyReverbAudioProcessorEditor::timerCallback() {
    wavePhase += 0.05f;
    if (wavePhase > juce::MathConstants<float>::twoPi)
        wavePhase -= juce::MathConstants<float>::twoPi;

    repaint(40, 55, getWidth() - 80, 150);
    updateEnabledState();
}

void DreamyReverbAudioProcessorEditor::paint(juce::Graphics& g) {
    juce::ColourGradient bgGrad(juce::Colour(0xff1a1728), 0.0f, 0.0f, juce::Colour(0xff090810), 0.0f, (float)getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillAll();

    // Outer Bezel Frame
    g.setColour(juce::Colour(0xff3b3554));
    g.drawRect(getLocalBounds(), 2);

    // Title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(22.0f, juce::Font::bold)));
    g.drawText("DREAMY REVERB", 0, 15, getWidth(), 30, juce::Justification::centred, true);

    // Dynamic Waveform Display Screen
    auto displayArea = juce::Rectangle<float>(40.0f, 55.0f, (float)getWidth() - 80.0f, 150.0f);
    juce::ColourGradient dispGrad(juce::Colour(0xff120e24), displayArea.getX(), displayArea.getY(), juce::Colour(0xff080612), displayArea.getX(), displayArea.getBottom(), false);
    g.setGradientFill(dispGrad);
    g.fillRoundedRectangle(displayArea, 8.0f);
    g.setColour(juce::Colour(0xff382e5c));
    g.drawRoundedRectangle(displayArea, 8.0f, 1.5f);

    // Glowing Reverb Wave Animation
    juce::Path wavePath;
    float startX = displayArea.getX();
    float centerY = displayArea.getCentreY();
    float width = displayArea.getWidth();

    wavePath.startNewSubPath(startX, centerY);
    for (float x = 0; x < width; x += 3.0f) {
        float y = std::sin(x * 0.02f + wavePhase) * 20.0f * std::cos(x * 0.008f - wavePhase * 0.5f);
        wavePath.lineTo(startX + x, centerY + y);
    }

    g.setColour(juce::Colour(0xaac084fc));
    g.strokePath(wavePath, juce::PathStrokeType(2.5f));

    // Particle Stars
    g.setColour(juce::Colour(0xffe9d5ff));
    for (int i = 0; i < 15; ++i) {
        float px = startX + (float)((i * 53) % (int)width);
        float py = displayArea.getY() + 20.0f + (float)((i * 37) % 110);
        float size = 1.0f + std::sin(wavePhase + i) * 1.5f;
        g.fillEllipse(px, py, size, size);
    }
}

void DreamyReverbAudioProcessorEditor::resized() {
    presetBox.setBounds(getWidth() - 250, 15, 140, 24);
    bypassButton.setBounds(getWidth() - 100, 15, 80, 24);

    int knobY = 225;
    int knobWidth = 85;
    int knobHeight = 85;
    int spacing = (getWidth() - (knobWidth * 5)) / 6;

    int x = spacing;

    auto placeKnobGroup = [&](juce::Slider& slider, juce::Label& valLbl, juce::Label& lbl) {
        valLbl.setBounds(x - 10, knobY - 20, knobWidth + 20, 18);
        slider.setBounds(x, knobY, knobWidth, knobHeight);
        lbl.setBounds(x - 10, knobY + knobHeight + 4, knobWidth + 20, 18);
        x += knobWidth + spacing;
    };

    placeKnobGroup(decaySlider, decayValLabel, decayLabel);
    placeKnobGroup(sizeSlider, sizeValLabel, sizeLabel);
    placeKnobGroup(dampingSlider, dampingValLabel, dampingLabel);
    placeKnobGroup(shimmerSlider, shimmerValLabel, shimmerLabel);
    placeKnobGroup(mixSlider, mixValLabel, mixLabel);

    // Bottom Filter & Modulation control bar
    int btnY = 375;
    hpfBtn.setBounds(150, btnY, 52, 22);
    lpfBtn.setBounds(207, btnY, 52, 22);
    depthBtn.setBounds(330, btnY, 80, 22);
    rateBtn.setBounds(415, btnY, 85, 22);
    lockBtn.setBounds(600, btnY, 60, 22);
}

#include "PluginProcessor.h"
#include "PluginEditor.h"

DreamyReverbAudioProcessorEditor::DreamyReverbAudioProcessorEditor(DreamyReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {

    presetBox.addItem("CELESTIAL VOID", 1);
    presetBox.addItem("CRYSTAL CAVE", 2);
    presetBox.addItem("ETHEREAL SHIMMER", 3);
    presetBox.setSelectedId(1);
    presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff1f1b2e));
    presetBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff4a3e6d));
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
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
        lbl.setFont(juce::Font(12.0f, juce::Font::bold));
        lbl.setColour(juce::Label::textColourId, juce::Colour(0xffcbd5e1));
        addAndMakeVisible(lbl);

        valLbl.setJustificationType(juce::Justification::centred);
        valLbl.setFont(juce::Font(11.0f, juce::Font::plain));
        valLbl.setColour(juce::Label::textColourId, juce::Colour(0xff94a3b8));
        addAndMakeVisible(valLbl);
    };

    setupKnob(decaySlider, decayLabel, "DECAY", decayValLabel);
    setupKnob(sizeSlider, sizeLabel, "SIZE", sizeValLabel);
    setupKnob(dampingSlider, dampingLabel, "DAMPING", dampingValLabel);
    setupKnob(shimmerSlider, shimmerLabel, "SHIMMER", shimmerValLabel);
    setupKnob(mixSlider, mixLabel, "MIX", mixValLabel);

    decaySlider.onValueChange = [this]() { decayValLabel.setText(juce::String(decaySlider.getValue(), 1) + "s", juce::dontSendNotification); };
    sizeSlider.onValueChange = [this]() { sizeValLabel.setText(juce::String((int)sizeSlider.getValue()) + "%", juce::dontSendNotification); };
    dampingSlider.onValueChange = [this]() { dampingValLabel.setText(juce::String(dampingSlider.getValue() / 1000.0f, 1) + "kHz", juce::dontSendNotification); };
    shimmerSlider.onValueChange = [this]() { shimmerValLabel.setText("+" + juce::String((int)shimmerSlider.getValue()) + " st", juce::dontSendNotification); };
    mixSlider.onValueChange = [this]() { mixValLabel.setText("Wet " + juce::String((int)mixSlider.getValue()) + "% Wet", juce::dontSendNotification); };

    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "decay", decaySlider);
    sizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "size", sizeSlider);
    dampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "damping", dampingSlider);
    shimmerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "shimmer", shimmerSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.parameters, "mix", mixSlider);

    decaySlider.sendLookAndFeelChange();
    sizeSlider.sendLookAndFeelChange();
    dampingSlider.sendLookAndFeelChange();
    shimmerSlider.sendLookAndFeelChange();
    mixSlider.sendLookAndFeelChange();

    auto setupBtn = [this](juce::TextButton& btn) {
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e1b2e));
        btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4c1d95));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcbd5e1));
        addAndMakeVisible(btn);
    };

    setupBtn(hpfBtn); setupBtn(lpfBtn); setupBtn(depthBtn); setupBtn(rateBtn); setupBtn(lockBtn);

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

void DreamyReverbAudioProcessorEditor::timerCallback() {
    wavePhase += 0.05f;
    repaint(50, 60, 700, 160);
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
    g.setFont(juce::Font(22.0f, juce::Font::bold));
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
    hpfBtn.setBounds(180, btnY, 50, 22);
    lpfBtn.setBounds(235, btnY, 50, 22);
    depthBtn.setBounds(370, btnY, 55, 22);
    rateBtn.setBounds(430, btnY, 55, 22);
    lockBtn.setBounds(570, btnY, 55, 22);
}

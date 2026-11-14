#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "MetallicKnobLookAndFeel.h"

class DreamyReverbAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    DreamyReverbAudioProcessorEditor (DreamyReverbAudioProcessor&);
    ~DreamyReverbAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void applyPreset (int presetIndex);
    void refreshValueLabels();
    void updateModReadouts();
    void updateEnabledState (bool force = false);
    void drawLevelMeter (juce::Graphics& g, juce::Rectangle<int> bar, float level);

    DreamyReverbAudioProcessor& audioProcessor;

    MetallicKnobLookAndFeel customKnobLAF;

    juce::ComboBox presetBox;
    juce::TextButton bypassButton { "BYPASS" };

    juce::Slider decaySlider;
    juce::Slider sizeSlider;
    juce::Slider dampingSlider;
    juce::Slider shimmerSlider;
    juce::Slider mixSlider;

    juce::Label decayLabel, decayValLabel;
    juce::Label sizeLabel, sizeValLabel;
    juce::Label dampingLabel, dampingValLabel;
    juce::Label shimmerLabel, shimmerValLabel;
    juce::Label mixLabel, mixValLabel;

    juce::TextButton hpfBtn { "HPF" };
    juce::TextButton lpfBtn { "LPF" };
    juce::TextButton depthBtn { "DEPTH" };
    juce::TextButton rateBtn { "RATE" };
    juce::TextButton lockBtn { "LOCK" };

    juce::Label hpfValLabel, lpfValLabel, depthValLabel, rateValLabel;

    // value-chip rectangles drawn in paint()
    juce::Rectangle<int> decayValBox, sizeValBox, dampingValBox, shimmerValBox, mixValBox;
    juce::Rectangle<int> hpfValBox, lpfValBox, depthValBox, rateValBox;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dampingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shimmerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hpfAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lpfAttachment;

    float wavePhase = 0.0f;
    float meterLevel[2] = { 0.0f, 0.0f };
    bool paramsLocked = false;
    bool lastEnabledState = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DreamyReverbAudioProcessorEditor)
};

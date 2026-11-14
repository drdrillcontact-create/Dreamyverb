#include "PluginProcessor.h"
#include "PluginEditor.h"

DreamyReverbAudioProcessor::DreamyReverbAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout()) {}

DreamyReverbAudioProcessor::~DreamyReverbAudioProcessor() {}

const juce::String DreamyReverbAudioProcessor::getName() const { return JucePlugin_Name; }
bool DreamyReverbAudioProcessor::acceptsMidi() const { return false; }
bool DreamyReverbAudioProcessor::producesMidi() const { return false; }
bool DreamyReverbAudioProcessor::isMidiEffect() const { return false; }
double DreamyReverbAudioProcessor::getTailLengthSeconds() const {
    return (double)(*parameters.getRawParameterValue("decay")) + 1.0;
}

int DreamyReverbAudioProcessor::getNumPrograms() { return 1; }
int DreamyReverbAudioProcessor::getCurrentProgram() { return 0; }
void DreamyReverbAudioProcessor::setCurrentProgram(int) {}
const juce::String DreamyReverbAudioProcessor::getProgramName(int) { return {}; }
void DreamyReverbAudioProcessor::changeProgramName(int, const juce::String&) {}

juce::AudioProcessorValueTreeState::ParameterLayout DreamyReverbAudioProcessor::createParameterLayout() {
    using P = juce::ParameterID;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        P{"decay", 1}, "Decay",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 4.2f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        P{"size", 1}, "Size",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 85.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        P{"damping", 1}, "Damping",
        juce::NormalisableRange<float>(100.0f, 20000.0f, 1.0f, 0.3f), 4500.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        P{"shimmer", 1}, "Shimmer",
        juce::NormalisableRange<float>(0.0f, 12.0f, 1.0f), 6.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        P{"mix", 1}, "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 40.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        P{"depth", 1}, "Mod Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 25.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        P{"rate", 1}, "Mod Rate",
        juce::NormalisableRange<float>(0.05f, 8.0f, 0.01f, 0.5f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        P{"hpf", 1}, "High-Pass Filter", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        P{"lpf", 1}, "Low-Pass Filter", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        P{"bypass", 1}, "Bypass", false));

    return { params.begin(), params.end() };
}

void DreamyReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    engine.prepare(sampleRate, samplesPerBlock, 2);
    wasBypassed = false;
}

void DreamyReverbAudioProcessor::releaseResources() {}

bool DreamyReverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void DreamyReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const bool bypassed = *parameters.getRawParameterValue("bypass") > 0.5f;
    if (bypassed) {
        if (!wasBypassed) {
            engine.reset();
            wasBypassed = true;
        }
        return;
    }
    wasBypassed = false;

    ReverbEngine::Parameters p;
    p.decay   = *parameters.getRawParameterValue("decay");
    p.size    = *parameters.getRawParameterValue("size");
    p.damping = *parameters.getRawParameterValue("damping");
    p.shimmer = *parameters.getRawParameterValue("shimmer");
    p.mix     = *parameters.getRawParameterValue("mix");
    p.depth   = *parameters.getRawParameterValue("depth");
    p.rate    = *parameters.getRawParameterValue("rate");
    p.hpf     = *parameters.getRawParameterValue("hpf") > 0.5f;
    p.lpf     = *parameters.getRawParameterValue("lpf") > 0.5f;

    engine.process(buffer, p);
}

bool DreamyReverbAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* DreamyReverbAudioProcessor::createEditor() { return new DreamyReverbAudioProcessorEditor(*this); }

void DreamyReverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DreamyReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new DreamyReverbAudioProcessor();
}

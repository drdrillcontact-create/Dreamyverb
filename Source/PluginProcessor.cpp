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
double DreamyReverbAudioProcessor::getTailLengthSeconds() const { return 5.0; }

int DreamyReverbAudioProcessor::getNumPrograms() { return 1; }
int DreamyReverbAudioProcessor::getCurrentProgram() { return 0; }
void DreamyReverbAudioProcessor::setCurrentProgram(int) {}
const juce::String DreamyReverbAudioProcessor::getProgramName(int) { return {}; }
void DreamyReverbAudioProcessor::changeProgramName(int, const juce::String&) {}

juce::AudioProcessorValueTreeState::ParameterLayout DreamyReverbAudioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("decay", "Decay", juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 4.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("size", "Size", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 85.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("damping", "Damping", juce::NormalisableRange<float>(100.0f, 20000.0f, 1.0f, 0.3f), 4500.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("shimmer", "Shimmer", juce::NormalisableRange<float>(0.0f, 24.0f, 1.0f), 12.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 40.0f));

    return { params.begin(), params.end() };
}

void DreamyReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;
    delayLines.resize(2);
    for (auto& channel : delayLines) {
        channel.resize(static_cast<int>(sampleRate * 4.0), 0.0f);
    }
    delayIndices.assign(2, 0);
    filterStates.assign(2, 0.0f);
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

    float decay = *parameters.getRawParameterValue("decay") / 10.0f * 0.95f;
    float damping = *parameters.getRawParameterValue("damping");
    float shimmer = *parameters.getRawParameterValue("shimmer") / 24.0f;
    float mix = *parameters.getRawParameterValue("mix") / 100.0f;

    int numSamples = buffer.getNumSamples();
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        auto& dLine = delayLines[channel];
        int& dIndex = delayIndices[channel];
        float& fState = filterStates.getUnchecked(channel);

        float dampingCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * damping / static_cast<float>(currentSampleRate));

        for (int i = 0; i < numSamples; ++i) {
            float in = channelData[i];
            int delayLength = static_cast<int>(currentSampleRate * 0.15);
            int readIndex = (dIndex - delayLength + dLine.size()) % dLine.size();
            float delayed = dLine[readIndex];

            fState = delayed * (1.0f - dampingCoeff) + fState * dampingCoeff;
            float wet = fState * decay + (in * 0.5f);

            dLine[dIndex] = in + wet * decay * (1.0f + shimmer * 0.15f * std::sin(i * 0.02f));
            dIndex = (dIndex + 1) % dLine.size();

            channelData[i] = in * (1.0f - mix) + wet * mix;
        }
    }
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

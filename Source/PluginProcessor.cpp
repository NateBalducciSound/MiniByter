/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
MiniByterAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "bitDepth", "Bit Depth",
        juce::NormalisableRange<float>(1.0f, 16.0f, 1.0f), 8.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "sampleRate", "Sample Rate",
        juce::NormalisableRange<float>(1000.0f, 44100.0f, 1.0f), 44100.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "wetDry", "Wet/Dry",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "bypass", "Bypass", false));

    return layout;
}

MiniByterAudioProcessor::MiniByterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       audioValueTree(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

MiniByterAudioProcessor::~MiniByterAudioProcessor()
{
}

//==============================================================================
const juce::String MiniByterAudioProcessor::getName() const { return JucePlugin_Name; }

bool MiniByterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MiniByterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MiniByterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MiniByterAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int    MiniByterAudioProcessor::getNumPrograms()             { return 1; }
int    MiniByterAudioProcessor::getCurrentProgram()          { return 0; }
void   MiniByterAudioProcessor::setCurrentProgram (int)      {}
const juce::String MiniByterAudioProcessor::getProgramName (int) { return {}; }
void   MiniByterAudioProcessor::changeProgramName (int, const juce::String&) {}

//==============================================================================
void MiniByterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    heldSample.fill(0.0f);
    counter.fill(0.0f);
}

void MiniByterAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MiniByterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif
    return true;
  #endif
}
#endif

void MiniByterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    bool  bypass    = audioValueTree.getRawParameterValue("bypass")->load();
    float bitDepth  = audioValueTree.getRawParameterValue("bitDepth")->load();
    float targetSR  = audioValueTree.getRawParameterValue("sampleRate")->load();
    float wetDry    = audioValueTree.getRawParameterValue("wetDry")->load();

    if (bypass) return;

    float steps    = std::pow(2.0f, bitDepth);
    float stepSize = 2.0f / steps;
    float holdTime = (float)sampleRate / targetSR;

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            float dry = channelData[n];

            // sample rate reduction
            counter[channel] += 1.0f;
            if (counter[channel] >= holdTime)
            {
                counter[channel] -= holdTime;
                heldSample[channel] = channelData[n];
            }
            float wet = heldSample[channel];

            // bit depth reduction
            wet = stepSize * std::round(wet / stepSize);

            // wet/dry mix
            channelData[n] = dry + wetDry * (wet - dry);
        }
    }
}

//==============================================================================
bool MiniByterAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* MiniByterAudioProcessor::createEditor()
{
    return new MiniByterAudioProcessorEditor (*this);
}

void MiniByterAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}
void MiniByterAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MiniByterAudioProcessor();
}

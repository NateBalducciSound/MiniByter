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

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "pitchDepth", "Pitch Depth",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f), 0.0f));

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

    // Hardcoded envelope times: 1ms attack, 80ms release
    envAttackCoeff  = std::exp(-1.0f / (float)(0.001 * sampleRate));
    envReleaseCoeff = std::exp(-1.0f / (float)(0.080 * sampleRate));

    for (auto& buf : pitchBuffer)
        buf.fill(0.0f);
    pitchWritePos.fill(0);
    // Start read pointer 2048 samples behind write so we have headroom
    pitchReadPos[0] = (float)(PITCH_BUF_SIZE - 2048);
    pitchReadPos[1] = (float)(PITCH_BUF_SIZE - 2048);
    envelope.fill(0.0f);
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

    bool  bypass     = audioValueTree.getRawParameterValue("bypass")->load();
    float bitDepth   = audioValueTree.getRawParameterValue("bitDepth")->load();
    float targetSR   = audioValueTree.getRawParameterValue("sampleRate")->load();
    float wetDry     = audioValueTree.getRawParameterValue("wetDry")->load();
    float pitchDepth = audioValueTree.getRawParameterValue("pitchDepth")->load();

    if (bypass) return;

    float steps    = std::pow(2.0f, bitDepth);
    float stepSize = 2.0f / steps;
    float holdTime = (float)sampleRate / targetSR;

    for (int ch = 0; ch < totalNumInputChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);

        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            float dry = channelData[n];

            // --- Pitch modulation via amplitude-driven resampling ---
            // Write input into circular pitch buffer
            pitchBuffer[ch][pitchWritePos[ch]] = channelData[n];
            pitchWritePos[ch] = (pitchWritePos[ch] + 1) % PITCH_BUF_SIZE;

            // Envelope follower: fast attack, slow release to track transient peaks
            float level = std::abs(channelData[n]);
            if (level > envelope[ch])
                envelope[ch] = envAttackCoeff  * envelope[ch] + (1.0f - envAttackCoeff)  * level;
            else
                envelope[ch] = envReleaseCoeff * envelope[ch] + (1.0f - envReleaseCoeff) * level;

            // Map envelope → pitch shift: peaks drive pitch up by up to pitchDepth semitones
            float semitones = pitchDepth * envelope[ch];
            float rate      = std::pow(2.0f, semitones / 12.0f);

            // Advance read pointer at modulated rate, then interpolate
            pitchReadPos[ch] += rate;
            if (pitchReadPos[ch] >= PITCH_BUF_SIZE)
                pitchReadPos[ch] -= PITCH_BUF_SIZE;

            int   idx0    = (int)pitchReadPos[ch];
            int   idx1    = (idx0 + 1) % PITCH_BUF_SIZE;
            float frac    = pitchReadPos[ch] - (float)idx0;
            float pitched = pitchBuffer[ch][idx0] * (1.0f - frac)
                          + pitchBuffer[ch][idx1] * frac;

            // --- Sample rate reduction ---
            counter[ch] += 1.0f;
            if (counter[ch] >= holdTime)
            {
                counter[ch] -= holdTime;
                heldSample[ch] = pitched;
            }
            float wet = heldSample[ch];

            // --- Bit depth reduction ---
            wet = stepSize * std::round(wet / stepSize);

            // --- Wet/dry mix ---
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

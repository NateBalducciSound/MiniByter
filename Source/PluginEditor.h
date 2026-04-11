/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class MiniByterAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    MiniByterAudioProcessorEditor (MiniByterAudioProcessor&);
    ~MiniByterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MiniByterAudioProcessor& audioProcessor;

    juce::Slider bitDepthKnob;
    juce::Slider sampleRateKnob;
    juce::Slider wetDryKnob;
    juce::ToggleButton bypassButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bitDepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sampleRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetDryAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiniByterAudioProcessorEditor)
};

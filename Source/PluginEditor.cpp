/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MiniByterAudioProcessorEditor::MiniByterAudioProcessorEditor (MiniByterAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    auto makeRotary = [](juce::Slider& s) {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    };

    makeRotary(bitDepthKnob);
    makeRotary(sampleRateKnob);
    makeRotary(wetDryKnob);

    bypassButton.setButtonText("Bypass");

    auto& vt = audioProcessor.audioValueTree;

    bitDepthAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(vt, "bitDepth",   bitDepthKnob);
    sampleRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(vt, "sampleRate", sampleRateKnob);
    wetDryAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(vt, "wetDry",     wetDryKnob);
    bypassAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(vt, "bypass",     bypassButton);

    addAndMakeVisible(bitDepthKnob);
    addAndMakeVisible(sampleRateKnob);
    addAndMakeVisible(wetDryKnob);
    addAndMakeVisible(bypassButton);

    setSize(400, 180);
}

MiniByterAudioProcessorEditor::~MiniByterAudioProcessorEditor()
{
}

//==============================================================================
void MiniByterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(15.0f));
    g.drawFittedText("MiniByter", getLocalBounds().removeFromTop(20), juce::Justification::centred, 1);
}

void MiniByterAudioProcessorEditor::resized()
{
    bitDepthKnob.setBounds  (20,  40, 90, 110);
    sampleRateKnob.setBounds(130, 40, 90, 110);
    wetDryKnob.setBounds    (240, 40, 90, 110);
    bypassButton.setBounds  (340, 70, 45,  30);
}

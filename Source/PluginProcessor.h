/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

struct ChainSettings
{
    float peak1Freq { 0 }, peak1GainInDecibels { 0 }, peak1Quality { 1.f },
    peak2Freq { 0 }, peak2GainInDecibels { 0 }, peak2Quality { 1.f },
    span { 0 }, balance { 0 }, outputGain { 0.f };
    
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;

using Gain = juce::dsp::Gain<float>;

using MonoChain = juce::dsp::ProcessorChain<Filter, Filter, Gain>;

enum ChainPositions
{
    Peak1,
    Peak2,
    outputGain
};

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(Coefficients& old, const Coefficients& replacemets);

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);
Coefficients makePeakFilter2(const ChainSettings& chainSettings, double sampleRate);

//==============================================================================
/**
*/
class SimpleDualFilterAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleDualFilterAudioProcessor();
    ~SimpleDualFilterAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()};

private:
    MonoChain leftChain, rightChain;
    
    void updatePeakFilter(const ChainSettings& chainSettings);
    
    void updateFilters();
    void updateGain();
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleDualFilterAudioProcessor)
};

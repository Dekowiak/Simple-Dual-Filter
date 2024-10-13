/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleDualFilterAudioProcessor::SimpleDualFilterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleDualFilterAudioProcessor::~SimpleDualFilterAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleDualFilterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleDualFilterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleDualFilterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleDualFilterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleDualFilterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleDualFilterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleDualFilterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleDualFilterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleDualFilterAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleDualFilterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleDualFilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    juce::dsp::ProcessSpec spec;
    
    spec.maximumBlockSize = samplesPerBlock;
    
    spec.numChannels = 1;
    
    spec.sampleRate = sampleRate;
    
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    updateFilters();
    updateGain();
    
}

void SimpleDualFilterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleDualFilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleDualFilterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    
    // Get current settings, including output gain
    ChainSettings chainSettings = getChainSettings(apvts);
    
    updateFilters();
    updateGain();

    juce::dsp::AudioBlock<float> block(buffer);
    
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    // Process the filters for left and right channels
    leftChain.process(leftContext);
    rightChain.process(rightContext);
    
    // Apply output gain to both channels after processing the filters
        float outputGain = juce::Decibels::decibelsToGain(chainSettings.outputGain); // Convert dB to gain
        buffer.applyGain(outputGain); // Apply gain to the entire buffer

}

//==============================================================================
bool SimpleDualFilterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleDualFilterAudioProcessor::createEditor()
{
    return new SimpleDualFilterAudioProcessorEditor (*this);
//    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleDualFilterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void SimpleDualFilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if( tree.isValid() )
    {
        apvts.replaceState(tree);
        updateFilters();
        updateGain();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;
    
    settings.peak1Freq = apvts.getRawParameterValue("Peak1 Freq")->load();
    settings.peak1GainInDecibels = apvts.getRawParameterValue("Peak1 Gain")->load();
    settings.peak1Quality = apvts.getRawParameterValue("Peak1 Quality")->load();
//    settings.peak2Freq = apvts.getRawParameterValue("Peak2 Freq")->load();
//    settings.peak2GainInDecibels = apvts.getRawParameterValue("Peak2 Gain")->load();
//    settings.peak2Quality = apvts.getRawParameterValue("Peak2 Quality")->load();
    settings.span = apvts.getRawParameterValue("Span")->load();
    settings.balance = apvts.getRawParameterValue("Balance")->load();
    settings.outputGain = apvts.getRawParameterValue("Output Gain")->load();

    return settings;
}

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                               chainSettings.peak1Freq,
                                                               chainSettings.peak1Quality,
                                                               juce::Decibels::decibelsToGain(chainSettings.peak1GainInDecibels - chainSettings.balance));
}

Coefficients makePeakFilter2(const ChainSettings& chainSettings, double sampleRate)
{
    // Span spaces the second filter based on a percentage of the first frequency
    double spanFactor = 1.0 + (chainSettings.span / 2.0);
    
    // Ensure the frequency does not exceed Nyquist or fall below a certain minimum
    double peak2Freq = juce::jlimit(20.0, sampleRate / 2.0, chainSettings.peak1Freq * spanFactor);

    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                               peak2Freq,
                                                               chainSettings.peak1Quality,
                                                               juce::Decibels::decibelsToGain(chainSettings.peak1GainInDecibels + chainSettings.balance));
}


void SimpleDualFilterAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings)
{
    auto peak1Coefficients = makePeakFilter(chainSettings, getSampleRate());

    auto peak2Coefficients = makePeakFilter2(chainSettings, getSampleRate());

    updateCoefficients(leftChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);

    updateCoefficients(leftChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);
}

void updateCoefficients(Coefficients &old, const Coefficients &replacements)
{
    *old = *replacements;
}

void SimpleDualFilterAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);
    updatePeakFilter(chainSettings);
}

void SimpleDualFilterAudioProcessor::updateGain()
{
    auto chainSettings = getChainSettings(apvts);
    auto gainCoefficient = juce::Decibels::decibelsToGain(chainSettings.outputGain);
    leftChain.get<ChainPositions::outputGain>().setGainLinear(gainCoefficient);
    rightChain.get<ChainPositions::outputGain>().setGainLinear(gainCoefficient);
}

juce::AudioProcessorValueTreeState::ParameterLayout SimpleDualFilterAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Freq",
                                                         "Peak1 Freq",
                                                         juce::NormalisableRange<float>(20.f, 10000.f, 1.f, 0.25f), 20.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Gain",
                                                         "Peak1 Gain",
                                                         juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Quality",
                                                         "Peak1 Quality",
                                                         juce::NormalisableRange<float>(0.1f, 10.f, 0.1f, 0.25f), 1.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Span",
                                                         "Span",
                                                         juce::NormalisableRange<float>(0.f, 10.f, 0.01f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Balance",
                                                         "Balance",
                                                         juce::NormalisableRange<float>(-12.f, 12.f, 0.1f, 1.f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("Output Gain",
                                                         "Output Gain",
                                                         juce::NormalisableRange<float>(-60.f, 0.f, 0.1f, 0.25f), 0.f));

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleDualFilterAudioProcessor();
}

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// To do : better Colour names, adjust skew factor for freq parameter 

// Features for another Version of this PlugIn: multimode Filters with changeable characteristics and possibly morphing between the different filters.


struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider (juce::Graphics&,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override;
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix, const juce::String& labelName) : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox),
    param(&rap),
    suffix(unitSuffix),
    labelName(labelName)
    {
        setLookAndFeel(&lnf);
    }

    ~RotarySliderWithLabels()    {
        setLookAndFeel(nullptr);
    }

    struct LabelPos
    {
        float pos; // 0.0 -> 1.0 for slider position (fraction)
        juce::String label;
    };

    juce::Array<LabelPos> labels;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;
private:
    LookAndFeel lnf;

    juce::RangedAudioParameter* param;
    juce::String suffix;
    juce::String labelName;   // Static label for the parameter name
};

struct ResponseCurveComponent : juce::Component,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
    ResponseCurveComponent(SimpleDualFilterAudioProcessor&);
    ~ResponseCurveComponent();
    
    void parameterValueChanged (int parameterIndex, float newValue) override;
    
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override { }
    
    void timerCallback() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
private:
    SimpleDualFilterAudioProcessor& audioProcessor;
    
    juce::Atomic<bool> parametersChanged { false };
    
    MonoChain monoChain;
    
    void updateChain();
    
    juce::Image background;
    
    juce::Rectangle<int> getRenderArea();
    
    juce::Rectangle<int> getAnalysisArea();
};

//==============================================================================
/**
*/
class SimpleDualFilterAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleDualFilterAudioProcessorEditor (SimpleDualFilterAudioProcessor&);
    ~SimpleDualFilterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleDualFilterAudioProcessor& audioProcessor;
    
    RotarySliderWithLabels freqSlider,
    gainSlider,
    qualitySlider,
    spanSlider,
    balanceSlider,
    outputGainSlider;
    
    ResponseCurveComponent responseCurveComponent;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    Attachment freqSliderAttachment,
    gainSliderAttachment,
    qualitySliderAttachment,
    spanSliderAttachment,
    balanceSliderAttachment,
    outputGainSliderAttachment;
    
    std::vector<juce::Component*> getComps();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleDualFilterAudioProcessorEditor)
};

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


struct ThemeColours
{
    juce::Colour background_colour                      = juce::Colour(0xFF0C0C0C);
    juce::Colour value_arc_colour                       = juce::Colour(0xFF323E3E);
    juce::Colour thumb_colour                           = juce::Colour(0xFFBCD8DB);
    juce::Colour dial_colour                            = juce::Colour(0xFFBCD8DB);
    juce::Colour value_colour                           = juce::Colour(0xFFFF7751);
    juce::Colour label_colour                           = juce::Colour(0xFFF0F9CC);
    juce::Colour big_label_colour                       = juce::Colour(0xFFBCD8DB);
    juce::Colour big_label_background_colour            = juce::Colour(0xFF323E3E);
    juce::Colour responsegrid_outline_colour            = juce::Colour(0xFF323E3E);
    juce::Colour responsecurve_colour                   = juce::Colour(0xFFFF7751);
    juce::Colour responsegrid_colour                    = juce::Colour(0xFF323E3E);
    juce::Colour responsegrid_highlight_colour          = juce::Colour(0xFF40515B);
    juce::Colour responsegrid_label_colour              = juce::Colour(0xFF4B7076);
    juce::Colour responsegrid_label_highlight_colour    = juce::Colour(0xFF4B7076);
    juce::Colour light_line_colour                      = juce::Colour(0xFF4B7076);
    juce::Colour dark_line_colour                       = juce::Colour(0xFF323E3E);
    juce::Colour light_point_colour                     = juce::Colour(0xFFBCD8DB);
    juce::Colour dark_point_colour                      = juce::Colour(0xFF2A2A2A);
};

// Create an istance of the colour theme
ThemeColours theme;

constexpr float designRatio = 15.0f / 8.0f;
constexpr float designWidth = 1500.f;
constexpr float designHeight = designWidth / designRatio;

void drawCustomLine(juce::Graphics &g,
                     float startX,
                     float y,
                     float midX,
                     float endX,
                     float width,
                     bool lightDark)
{
    // Draws a line with a light and a dark part and points on the startpoint, endpoint
    // and the point where the line switches from light to dark
    
    // First part of the line
    if ( lightDark )
    {
        g.setColour(theme.light_line_colour);
    }
    else {
        g.setColour(theme.dark_line_colour);
    }
    
    g.fillRoundedRectangle(startX, y, midX - startX + width / 2, width,  width / 2);

    // Second part of the line
    if ( lightDark )
    {
        g.setColour(theme.dark_line_colour);
    }
    else {
        g.setColour(theme.light_line_colour);
    }
    
    g.fillRoundedRectangle(midX, y, endX - midX + width / 2, width,  width / 2);
    
    // Line points
    g.setColour(theme.light_point_colour);
    g.fillEllipse(startX, y, width, width);
    
    g.fillEllipse(midX, y, width, width);
    
    g.fillEllipse(endX, y, width, width);
}

void LookAndFeel::drawRotarySlider(juce::Graphics &g,
                                   int x,
                                   int y,
                                   int width,
                                   int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider & slider)
{
    using namespace juce;
    
    Font font;
    font.setTypefaceName("Rubik");
    font.setBold(false);
    g.setFont(font);
    
    auto bounds = Rectangle<int>(x, y, width, height).toFloat();

    auto scaleFactor = bounds.getWidth() / 250.f;

    auto dialBounds = Rectangle<int>(x, y, width, height).toFloat().reduced(scaleFactor * 20);

    auto radius = jmin(dialBounds.getWidth(), dialBounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = jmin(scaleFactor * 4.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;
    
    // Shift the whole dial down by this amount
    auto yShift = scaleFactor * 50;

    // Value Arc
    if (slider.isEnabled())
    {
        Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(),
            bounds.getCentreY() + yShift,
            arcRadius,
            arcRadius,
            0.0f,
            rotaryStartAngle,
            toAngle,
            true);
        
        g.setColour(theme.value_arc_colour);
        g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // Dial marker
    if( auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
        {
            auto center = dialBounds.getCentre();
    
            Path p;
    
            Rectangle<float> r;
            r.setLeft(center.getX() - scaleFactor * 1);
            r.setRight(center.getX() + scaleFactor * 1);
            r.setTop(dialBounds.getY() + yShift + 2);
            r.setBottom(center.getY() + yShift - 60 * scaleFactor);
    
            p.addRoundedRectangle(r, 1.f);
    
            auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
    
            p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY() + yShift));
            
            g.setColour(theme.dial_colour);
            g.fillPath(p);
            
            // Parameter value displayed in the center of the slider
            g.setFont(30.f * scaleFactor);
            auto text = rswl->getDisplayString();

            GlyphArrangement glyphArrangement;
            auto strWidth = glyphArrangement.getStringWidth(g.getCurrentFont(), text);

            r.setSize(strWidth + (scaleFactor * 4), rswl->getTextHeight() + (scaleFactor * 2));
            r.setCentre(bounds.getCentreX(), bounds.getCentreY() + yShift);
    
            g.setColour(theme.value_colour);
            g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
        }
    
    // Thumb
    auto thumbWidth = lineW * 1.1f;
    Point<float> thumbPoint(bounds.getCentreX() + arcRadius * std::cos(toAngle - MathConstants<float>::halfPi),
        bounds.getCentreY()+ yShift + arcRadius * std::sin(toAngle - MathConstants<float>::halfPi));

    g.setColour(theme.thumb_colour);
    g.fillEllipse(Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
}

void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    
    auto sliderBounds = getSliderBounds();
    
    // Reduces the bounds to where the component should be drawn.
    // The whole space for the component was made bigger, so nothing drawn in here gets cut off.
    auto innerBounds = getLocalBounds().reduced(20);

    
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();
    
    
    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng,
                                      endAng,
                                      *this);

    float scaleFactor = float(innerBounds.getWidth() / 250.f);
    
    float pointSize = scaleFactor * 3.5f;
    
    float dist = float(innerBounds.getWidth() / 5.f);
    
    auto offset = pointSize / 2.f;
    
    auto x = innerBounds.getX();
    
    auto y = innerBounds.getY();
    
    
    
    // Draw the big label background
    Rectangle<float> labelBackground;
    
    labelBackground.setBounds(x - offset, y + 0.5 * dist - offset, dist * 3 + offset * 2, dist);
    
    g.setColour(theme.big_label_background_colour);
    g.fillRoundedRectangle(labelBackground, pointSize);

    
    // Draw the big label
    Font font;
    font.setTypefaceName("Rubik");
    font.setBold(false);
    g.setFont(font);
    g.setColour(theme.big_label_colour);
    g.setFont(50.0f * scaleFactor);
    
    // Convert Rectange<float> to Rectangle<int> for the drawFittedText function
    Rectangle<int> intLabelBounds = labelBackground.toNearestInt();

    // Draw the label text
    g.drawFittedText(labelName, intLabelBounds, Justification::centred, 1);
    
    // Draw the top line
    drawCustomLine(g, x - offset, y + 2 * dist - offset, x + dist * 3.5, x + dist * 5, pointSize, false);
    
    // Draw the bottom line
    drawCustomLine(g, x - offset, y + 7 * dist - offset, x + dist * 3.5, x + dist * 5, pointSize, true);
    
    // Draw the min an max labels
    g.setColour(theme.label_colour);
    g.setFont(24.0f * scaleFactor);
    
    auto leftLabel = labels[0].label;
    
    // Using GlyphArrangement to calculate text width
    GlyphArrangement glyphArrangementLeft;
    glyphArrangementLeft.addFittedText(g.getCurrentFont(), leftLabel, 0, 0, 0, 1.0f, Justification::centred, 1.0f);
    float leftLabelWidth = glyphArrangementLeft.getStringWidth(g.getCurrentFont(), leftLabel);
    
    // Draw the label
    g.drawFittedText(leftLabel, x, y + dist * 6.3, leftLabelWidth, 20.0f * scaleFactor, Justification::left, 1);
    
    auto rightLabel = labels[1].label;
    
    // Using GlyphArrangement to calculate text width
    GlyphArrangement glyphArrangementRight;
    glyphArrangementRight.addFittedText(g.getCurrentFont(), rightLabel, 0, 0, 0, 1.0f, Justification::centred, 1.0f);
    float rightLabelWidth = glyphArrangementRight.getStringWidth(g.getCurrentFont(), rightLabel);
    
    // Draw the label
    g.drawFittedText(rightLabel, x + dist * 5 - rightLabelWidth, y + dist * 6.3 , rightLabelWidth, 20.0f * scaleFactor, Justification::right, 1);
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    // Reduces the bounds to where the component should be drawn.
    // The whole space for the component was made bigger, so nothing drawn in here gets cut off.
    auto bounds = getLocalBounds().reduced(20);
    
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), bounds.getCentreY());
    
    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    // Add a k in the String if the parameter value is over 999.f
//    if( auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param) )
//        return juce::String(getValue());
    
    juce::String str;
    bool addK = false;
    
    if( auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param) )
    {
        float val = getValue();
        
        if( val > 999.f )
        {
            val /= 1000.f;
            addK = true;
        }
        
        str = juce::String(val, (addK ? 2 :0));
    }
    else
    {
        jassertfalse;
    }
    
    if( suffix.isNotEmpty() )
    {
        str << " ";
        if( addK )
            str<< "k";
        
        str << suffix;
    }
    
    return str;
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleDualFilterAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->addListener(this);
    }
    
    updateChain();
    
    startTimer(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if( parametersChanged.compareAndSetBool(false, true) )
    {
        // update monochain
        updateChain();
        
        // signal repaint
        repaint();
    }
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peak1Coefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
    auto peak2Coefficients = makePeakFilter2(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);
}

void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    
    g.drawImage(background, getLocalBounds().toFloat());
    
    auto responseArea = getAnalysisArea();
    
    auto w = responseArea.getWidth();
    
    float scaleFactor = float(w / 1050.f);
    
    auto& peak1 = monoChain.get<ChainPositions::Peak1>();
    auto& peak2 = monoChain.get<ChainPositions::Peak2>();
    
    auto sampleRate = audioProcessor.getSampleRate();
    
    std::vector<double> mags;
    
    mags.resize(w);
    
    for( int i = 0; i < w; ++i )
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        
        mag *= peak1.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        mag *= peak2.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        mags[i] = Decibels::gainToDecibels(mag);
    }
    
    // Response Curve
    Path responseCurve;
    
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    
    for( size_t i = 1; i < mags.size(); ++i )
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
    
    // Draw responsegrid outline
    g.setColour(theme.responsegrid_outline_colour);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 1.f * scaleFactor, 3.f * scaleFactor);

    // Draw responsecurve
    g.setColour(theme.responsecurve_colour);
    g.strokePath(responseCurve, PathStrokeType(2.f * scaleFactor));
}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    
    background = Image(Image::PixelFormat::ARGB, getWidth(), getHeight(), true);
    
    Graphics g(background);
    
    Font font;
    font.setTypefaceName("Rubik");
    font.setBold(false);
    g.setFont(font);
    
    Array<float> freqs
    {
        20, 50, 100,
        200, 500, 1000,
        2000, 5000, 10000,
        20000
    };
    
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    float scaleFactor = float(width / 1050.f);
    
    Array<float> xs;
    for( auto f : freqs )
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }
    
    g.setColour(theme.responsegrid_colour);
    for( auto x : xs )
    {
        juce::Rectangle<float> rect(x, top, 2.f * scaleFactor, bottom - top);
        g.fillRect(rect);
    }
    
    Array<float> gain
    {
        -24, -12, 0, 12, 24
    };
    
    for( auto gDb : gain )
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gDb == 0.f ? theme.responsegrid_highlight_colour : theme.responsegrid_colour );
        juce::Rectangle<float> rect(left, y, right - left, 2.f * scaleFactor);
        g.fillRect(rect);
    }
    
    g.setColour(theme.responsegrid_label_colour);
    auto fontHeight = 20 * scaleFactor;
    g.setFont(fontHeight);
    
    for( int i = 0; i < freqs.size(); ++i)
    {
        auto f = freqs[i];
        auto x = xs[i];
        
        bool addK = false;
        String str;
        if( f > 999.f )
        {
            addK = true;
            f /= 1000.f;
        }
        
        str << f;
        if( addK )
            str << "k";
        str << "Hz";
        
        GlyphArrangement glyphArrangement;
        auto textWidth = glyphArrangement.getStringWidth(g.getCurrentFont(), str);
        
        auto y = 55 * scaleFactor;

        // Draw frequency labels
        if( i == 0 )
        {
            g.drawFittedText(str, x, y, textWidth, fontHeight, juce::Justification::left, 1);
        }
        else if ( i == freqs.size() - 1 )
        {
            g.drawFittedText(str, x - textWidth, y, textWidth, fontHeight, juce::Justification::right, 1);
        }
        else
        {
            g.drawFittedText(str, x - textWidth / 2, y, textWidth, fontHeight, juce::Justification::centred, 1);
        }
        
    }
    
    for( auto gDb : gain )
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        
        String str;
        if( gDb > 0 )
            str << "+";
        str << gDb;
        
        GlyphArrangement glyphArrangement;
        auto textWidth = glyphArrangement.getStringWidth(g.getCurrentFont(), str);
        
        g.setColour(gDb == 0.f ? theme.responsegrid_label_highlight_colour : theme.responsegrid_label_colour );
        
        // Draw dB labels on the right
        if ( gDb == 24.f )
        {
            g.drawFittedText(str, getWidth() - textWidth - scaleFactor * 13, y, textWidth, fontHeight, juce::Justification::right, 1);
            
            str.clear();
            str << (gDb -24.f);
            
            textWidth = glyphArrangement.getStringWidth(g.getCurrentFont(), str);
            // Draw dB labels on the left
            g.drawFittedText(str, scaleFactor * 13, y, textWidth, fontHeight, juce::Justification::right, 1);
            
            
        }
        else if ( gDb == -24.f )
        {
            g.drawFittedText(str, getWidth() - textWidth - scaleFactor * 13, y - fontHeight, textWidth, fontHeight, juce::Justification::right, 1);
            
            str.clear();
            str << (gDb -24.f);
            
            textWidth = glyphArrangement.getStringWidth(g.getCurrentFont(), str);
            g.drawFittedText(str, scaleFactor * 13, y - fontHeight, textWidth, fontHeight, juce::Justification::right, 1);
        }
        else
        {
            g.drawFittedText(str, getWidth() - textWidth - scaleFactor * 13, y - fontHeight / 2, textWidth, fontHeight, juce::Justification::right, 1);
            
            str.clear();
            str << (gDb -24.f);
            
            textWidth = glyphArrangement.getStringWidth(g.getCurrentFont(), str);
            g.drawFittedText(str, scaleFactor * 13, y - fontHeight / 2, textWidth, fontHeight, juce::Justification::right, 1);
        }
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();
    
    float scaleFactor = float(bounds.getWidth() / 1150.0f);
    
    bounds.removeFromTop(80 * scaleFactor);
    bounds.removeFromBottom(20 * scaleFactor);
    bounds.removeFromLeft(50 * scaleFactor);
    bounds.removeFromRight(50 * scaleFactor);
    
    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    
    return bounds;
}
//==============================================================================
SimpleDualFilterAudioProcessorEditor::SimpleDualFilterAudioProcessorEditor (SimpleDualFilterAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),

freqSlider(*audioProcessor.apvts.getParameter("Peak1 Freq"), "Hz", "FREQ"),
gainSlider(*audioProcessor.apvts.getParameter("Peak1 Gain"), "dB", "P GAIN"),
qualitySlider(*audioProcessor.apvts.getParameter("Peak1 Quality"), "", "QUAL"),
spanSlider(*audioProcessor.apvts.getParameter("Span"), "", "SPAN"),
balanceSlider(*audioProcessor.apvts.getParameter("Balance"), "", "BAL"),
outputGainSlider(*audioProcessor.apvts.getParameter("Output Gain"), "dB", "OUT G"),

responseCurveComponent(audioProcessor),
freqSliderAttachment(audioProcessor.apvts, "Peak1 Freq", freqSlider),
gainSliderAttachment(audioProcessor.apvts, "Peak1 Gain", gainSlider),
qualitySliderAttachment(audioProcessor.apvts, "Peak1 Quality", qualitySlider),
spanSliderAttachment(audioProcessor.apvts, "Span", spanSlider),
balanceSliderAttachment(audioProcessor.apvts, "Balance", balanceSlider),
outputGainSliderAttachment(audioProcessor.apvts, "Output Gain", outputGainSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    // Min Max labels fot the sliders
    freqSlider.labels.add({0.f, "20Hz"});
    freqSlider.labels.add({1.f, "10kHz"});
    
    gainSlider.labels.add({0.f, "-24dB"});
    gainSlider.labels.add({1.f, "+24dB"});
    
    qualitySlider.labels.add({0.f, "0.1"});
    qualitySlider.labels.add({1.f, "10.0"});

    spanSlider.labels.add({0.f, "0.0"});
    spanSlider.labels.add({1.f, "10.0"});

    balanceSlider.labels.add({0.f, "P1"});
    balanceSlider.labels.add({1.f, "P2"});

    outputGainSlider.labels.add({0.f, "-60dB"});
    outputGainSlider.labels.add({1.f, "0dB"});
    
    
    for( auto* comp : getComps() )
    {
        addAndMakeVisible(comp);
    }
    
    // Enable resizing
    setResizable(true, true);
    
    setResizeLimits(designWidth / 2, designHeight / 2, designWidth * 2, designHeight * 2);
    
    getConstrainer()->setFixedAspectRatio(designRatio);
    
    // Set the initial size of the plugin window
    setSize (designWidth, designHeight);

}

SimpleDualFilterAudioProcessorEditor::~SimpleDualFilterAudioProcessorEditor()
{
}

//==============================================================================
void SimpleDualFilterAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    
    g.fillAll(theme.background_colour);
    
    auto bounds = getLocalBounds();
    
    // Size of the points
    float pointSize = bounds.toFloat().getWidth() / 1500.f  * 3.5f;
    
    // Distance between grid points
    float dist = float(bounds.getWidth()) / 30.0f;
    
    // Generate a border
    auto border = bounds.getWidth() / 60;
    bounds.reduce(border, border);
    
    // Calculate the offste to center the points
    float offset = border - pointSize / 2;
    
    // Draw the grid of points
    g.setColour(theme.dark_point_colour);
    
    for (int i = 0; i <= 30; ++i)
    {
        for (int j = 0; j <= 16; j++)
        {
            g.fillEllipse(i * dist + offset, j * dist + offset, pointSize, pointSize);
        }
    }
    
    // Draw the line at the top
    drawCustomLine(g, 0 + offset, 0 + offset, dist * 5, dist * 29.5, pointSize, true);
}

void SimpleDualFilterAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Generate a border
    auto border = bounds.getWidth() / 60;
    bounds.reduce(border, border);
    
    using namespace juce;
    
    // Using a grid to layout the individual components
    Grid grid;
    
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;
    
    grid.templateRows = { Track (Fr (7)), Track (Fr (1)), Track (Fr (7)) };
    grid.templateColumns = { Track (Fr (5)), Track (Fr (1)), Track (Fr (5)), Track (Fr (1)), Track (Fr (5)), Track (Fr (1)) , Track (Fr (5)), Track (Fr (1)), Track (Fr (5)) };
    
    // The negative margin for the grid items creates an overlap.
    // This overlap is reduced in the individual components.
    // This method allows for drawing on the edge of a component without cutting of anything
    grid.items = { GridItem (responseCurveComponent).withArea(1, 1, 3, 8), GridItem ().withArea(1, 8, 2, 8),
        GridItem (outputGainSlider).withArea(1, 9, 2, 9).withMargin(juce::GridItem::Margin(-20, -20, -20, -20)),
        GridItem ().withArea(2, 9, 2, 9),
        GridItem (gainSlider).withArea(3, 1, 3, 1).withMargin(juce::GridItem::Margin(-20, -20, -20, -20)),
        GridItem ().withArea(3, 2, 3, 2),
        GridItem (qualitySlider).withArea(3, 3, 3, 3).withMargin(juce::GridItem::Margin(-20, -20, -20, -20)),
        GridItem ().withArea(3, 4, 3, 4),
        GridItem (freqSlider).withArea(3, 5, 3, 5).withMargin(juce::GridItem::Margin(-20, -20, -20, -20)),
        GridItem ().withArea(3, 6, 3, 6),
        GridItem (spanSlider).withArea(3, 7, 3, 7).withMargin(juce::GridItem::Margin(-20, -20, -20, -20)),
        GridItem ().withArea(3, 8, 3, 8),
        GridItem (balanceSlider).withArea(3, 9, 3, 9).withMargin(juce::GridItem::Margin(-20, -20, -20, -20))
    };
    
    grid.performLayout(bounds);
}

std::vector<juce::Component*> SimpleDualFilterAudioProcessorEditor::getComps()
{
    return {
        &freqSlider,
        &gainSlider,
        &qualitySlider,
        &spanSlider,
        &balanceSlider,
        &responseCurveComponent,
        &outputGainSlider
    };
}

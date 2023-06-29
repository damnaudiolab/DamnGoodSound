/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

  name:             DamnGoodSound

  dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_plugin_client, 
                    juce_audio_processors, juce_audio_utils, juce_core, juce_data_structures, juce_dsp, 
                    juce_events, juce_graphics, juce_gui_basics
  exporters:        VS2022

  moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1

  type:             AudioProcessor
  mainClass:        PluginAudioProcessor

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once
#include <JuceHeader.h>
#include "CustomLookAndFeel.h"
#include "ProcessorHeader.h"

const static std::string VSTID = "DamnGoodSound";

//==============================================================================
class PluginAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    PluginAudioProcessor()
        : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo())
            .withOutput("Output", juce::AudioChannelSet::stereo())
        ), parameters(*this, nullptr, Identifier(VSTID),
            {
                std::make_unique<AudioParameterFloat>(
                    "amount",
                    "Amount",
                    NormalisableRange<float>(0.0f, 100.0f, 0.01f),
                    0.0f
                ),
                std::make_unique<AudioParameterFloat>(
                    "crossFreq",
                    "CrossFreq",
                    NormalisableRange<float>(200.0f, 8000.0f, 0.01f, 0.5f),
                    200.0f
                ),
            }
            )
    {
        amount = parameters.getRawParameterValue("amount");
    }

    ~PluginAudioProcessor() override
    {
    }

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        // Use this method as the place to do any pre-playback
        // initialisation that you need..
        spec.maximumBlockSize = (uint32) samplesPerBlock;
        spec.numChannels = 2;
        spec.sampleRate = sampleRate;

        filterBuffer.setSize(4, samplesPerBlock, false, false, true);

        lowBand.prepare(spec);
        highBand.prepare(spec);
        lowShaper.prepare(spec);
        highShaper.prepare(spec);
        calcShaper.prepare(spec);
    }

    void releaseResources() override
    {
        // When playback stops, you can use this as an opportunity to free up any
        // spare memory, etc.
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;
        auto totalNumInputChannels = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();

        const auto numSamples = buffer.getNumSamples();
        const auto numChannels = buffer.getNumChannels();

        // In case we have more outputs than inputs, this code clears any output
        // channels that didn't contain input data, (because these aren't
        // guaranteed to be empty - they may contain garbage).
        // This is here to avoid people getting screaming feedback
        // when they first compile a plugin, but obviously you don't need to keep
        // this code if your algorithm always overwrites all the output channels.
        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
            buffer.clear(i, 0, buffer.getNumSamples());

        auto mainBlock = dsp::AudioBlock<float>(buffer);
        auto multiBandBlock = dsp::AudioBlock<float>(filterBuffer).getSubBlock(0, (size_t) numSamples);

        auto lowBlock = multiBandBlock.getSubsetChannelBlock(0, (size_t) numChannels);
        auto highBlock = multiBandBlock.getSubsetChannelBlock(2, (size_t) numChannels);

        auto preGain = powf(2, (*amount / 100.0f - 1.0f) * 4);
        auto postGain = 1.0f / preGain;

        //mainBlock.multiplyBy(preGain);

        lowBlock.copyFrom(mainBlock);
        highBlock.copyFrom(mainBlock);

        //ainBlock.multiplyBy(postGain);

        auto lowBandContext = dsp::ProcessContextReplacing<float> (lowBlock);

        auto testContext = dsp::ProcessContextReplacing<float> (mainBlock);
        lowShaper.process(testContext);

        // This is the place where you'd normally do the guts of your plugin's
        // audio processing...
        // Make sure to reset the state if your inner loop is processing
        // the samples and the outer loop is handling the channels.
        // Alternatively, you can process the samples with the channels
        // interleaved by keeping the same state.
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* inputBuffer = buffer.getReadPointer(channel);
            auto* outputBuffer = buffer.getWritePointer(channel);

            for (auto i = 0; i < numSamples; i++)
            {
                auto x = inputBuffer[i];
                x *= preGain;
                outputBuffer[i] = x * postGain;
            }
        }
    }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override {
        return new PluginAudioProcessorEditor(*this, parameters);
    }
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return VSTID; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override
    {
        // You should use this method to store your parameters in the memory block.
        // You could do that either as raw data, or use the XML or ValueTree classes
        // as intermediaries to make it easy to save and load complex data.
        auto state = parameters.copyState();
        std::unique_ptr<XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        // You should use this method to restore your parameters from this memory block,
        // whose contents will have been created by the getStateInformation() call.
        std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

        if (xmlState.get() != nullptr)
            if (xmlState->hasTagName(parameters.state.getType()))
                parameters.replaceState(ValueTree::fromXml(*xmlState));
    }

    //==============================================================================
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        // This is the place where you check if the layout is supported.
        // In this template code we only support mono or stereo.
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
            && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

        // This checks if the input layout matches the output layout
        if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
            return false;

        return true;
    }

private:
    class PluginAudioProcessorEditor : public AudioProcessorEditor
    {
    public:
        PluginAudioProcessorEditor(PluginAudioProcessor& p,
            AudioProcessorValueTreeState& vts)
            : AudioProcessorEditor(&p), audioProcessor(p), valueTreeState(vts)
        {

            logo = Drawable::createFromImageData(BinaryData::logo_svg, BinaryData::logo_svgSize);
            //addAndMakeVisible(logo.get());
            logo->setTransformToFit(headerArea.reduced(20).toFloat(), RectanglePlacement::centred);

            setSize(width, height);
            
            amountSliderAttachment.reset(new SliderAttachment(valueTreeState, "amount", amountSlider)); // 追加
            addAndMakeVisible(amountSlider);

            amountSlider.setBounds(bodyArea.reduced(20));
        }

        ~PluginAudioProcessorEditor() override {}

        void paint(Graphics& g) override
        {
            g.fillAll(customLookAndFeel.colourPalette[CustomLookAndFeel::grey]);
            g.setColour(customLookAndFeel.colourPalette[CustomLookAndFeel::black]);
            g.fillRect(headerArea);
        }

        void resized() override
        {
        }

    private:

        typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;

        CustomLookAndFeel customLookAndFeel;

        int width = 400;
        int height = 400;

        PluginAudioProcessor& audioProcessor;

        AudioProcessorValueTreeState& valueTreeState;

        std::unique_ptr<Drawable> logo;

        Rectangle<int> bodyArea{0, 0, width, height};

        Rectangle<int> headerArea{0, 0, width, height};

        Slider amountSlider;
        std::unique_ptr<SliderAttachment> amountSliderAttachment;


        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAudioProcessorEditor)
    };

    AudioProcessorValueTreeState parameters;
    
    float shaperFunc(float x)
    {
        return x + (1 - fabs(x)) * x;
    }

    using Filter = dsp::LinkwitzRileyFilter<float>;
    Filter lowBand, highBand;

    using Shaper = SquaredShaper<float>;
    Shaper lowShaper, highShaper, calcShaper;

    dsp::ProcessSpec spec;

    AudioBuffer<float> filterBuffer;

    std::atomic<float>* amount = nullptr;
    std::atomic<float>* color = nullptr;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAudioProcessor)
};

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), treeState(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    treeState.addParameterListener(this, "mix");
    treeState.addParameterListener(this, "time");
    treeState.addParameterListener(this, "regen");
    treeState.addParameterListener(this, "mod");
}

PluginProcessor::~PluginProcessor()
{
    treeState.removeParameterListener(this, "mix");
    treeState.removeParameterListener(this, "time");
    treeState.removeParameterListener(this, "regen");
    treeState.removeParameterListener(this, "mod");
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector <std::unique_ptr<juce::RangedAudioParameter>> parameters;

    auto pMix = std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.0, 24.0, 0.0);
    auto pTime = std::make_unique<juce::AudioParameterFloat>("time", "Time", 0.0, 24.0, 0.0);
    auto pRegen = std::make_unique<juce::AudioParameterFloat>("regen", "Regen", 0.0, 24.0, 0.0);
    auto pMod = std::make_unique<juce::AudioParameterBool>("mod", "Mod", false);

    params.push_back(std::move(pMix));
    params.push_back(std::move(pTime));
    params.push_back(std::move(pRegen));
    params.push_back(std::move(pMod));

    return { params.begin(), params.end() }
}

void PluginProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "mix") {
        mix = newValue;
    }

    if (parameterID == "time") {
        time = newValue;
    }

    if (parameterID == "regen") {
        regen = newValue;
    }

    if (parameterID == "mod") {
        mod = newValue; // float -> bool
    }
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::process (juce::dsp::ProcessContextReplacing<float> context)
{
    int delayTimeInSamples = *parameters.getRawParameterValue("time") * delayLineSampleRate;

    delayLine.setDelay(delayTimeInSamples);
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused (sampleRate, samplesPerBlock);

    mix = juce::Decibels::decibelsToGain(static_cast<float>(*treeState.getRawParameterValue("mix")));
    time = static_cast<float>(*treeState.getRawParameterValue("time"));
    regen = static_cast<float>(*treeState.getRawParameterValue("regen"));
    mod = *treeState.getRawParameterValue("mod");

    juce::dsp::ProcessSpec spec;

    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();

    int DELAY_RATE = 24000;

    delayLine.reset();
    delayLine.prepare(spec);
    delayLine.setDelay(DELAY_RATE);

    delayLineSampleRate = sampleRate;
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
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

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block (buffer);

    // process(juce::dsp::ProcessContextReplacing<float> (buffer));

    // TODO: Move to process method:
    
    // Handle time
    int delayTimeInSamples = *parameters.getRawParameterValue("time") * delayLineSampleRate;

    delayLine.setDelay(delayTimeInSamples);

    for (int channel; channel < totalNumInputChannels; ++channel)
    {
        auto* inSamples = buffer.getReadPointer(channel);
        auto* outSamples = buffer.getWritePointer(channel);

        for (int sampleIndex; sampleIndex < buffer.getNumSamples(); sampleIndex++)
        {
            float delayedSample = delayLine.popSample(channel);

            // Handle regen
            float inDelay = inSamples[sampleIndex] + (*parameters.getRawParameterValue("regen") * delayedSample);
            
            delayLine.pushSample(channel, inDelay);
        
            // Handle mix
            delayedSample *= *parameters.getRawParameterValue("mix");

            outSamples[sampleIndex] = inSamples[sampleIndex] + delayedSample;
        }
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    // return new PluginEditor (*this);

    // TODO: UI
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    
    // Called whenever you hit save

    juce::MemoryOutputStream stream(destData, false);
    treeState.state.writeToStream(stream);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    // Called whenever you load the live set

    auto tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));

    if (tree.isValid()) {
        // Could get invalid if you have two versions of this plugin and you're making changes
        // to one. It can do weird stuff.

        treeState.state = tree;

        mix = juce::Decibels::decibelsToGain(static_cast<float>(*treeState.getRawParameterValue("mix")));
        time = static_cast<float>(*treeState.getRawParameterValue("time"));
        regen = static_cast<float>(*treeState.getRawParameterValue("regen"));
        mod = *treeState.getRawParameterValue("mod");
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}

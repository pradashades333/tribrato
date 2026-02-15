#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
TribratProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int r = 1; r <= 2; ++r)
    {
        auto id = [&] (const juce::String& n) { return rowParam (r, n); };
        auto nm = [&] (const juce::String& n) { return "Row " + juce::String (r) + " " + n; };

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id ("trigger"), 1 }, nm ("Trigger"), false));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id ("mode"), 1 }, nm ("Mode"),
            juce::StringArray { "Momentary", "Latch" }, 1));  // default = Latch

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id ("onset"), 1 }, nm ("Onset"),
            juce::NormalisableRange<float> (10.0f, 2000.0f, 1.0f, 0.4f),
            200.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id ("rate"), 1 }, nm ("Rate"),
            juce::NormalisableRange<float> (0.5f, 15.0f, 0.01f, 0.7f),
            5.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id ("pitch"), 1 }, nm ("Pitch"),
            juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f),
            50.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id ("amplitude"), 1 }, nm ("Amplitude"),
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
            0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id ("formant"), 1 }, nm ("Formant"),
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
            0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { id ("variation"), 1 }, nm ("Variation"),
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
            0.0f));
    }

    return { params.begin(), params.end() };
}

//==============================================================================
TribratProcessor::TribratProcessor()
    : AudioProcessor (BusesProperties()
            .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

//==============================================================================
void TribratProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine1.prepare (sampleRate, samplesPerBlock);
    engine2.prepare (sampleRate, samplesPerBlock);
}

void TribratProcessor::releaseResources()
{
    engine1.reset();
    engine2.reset();
}

//==============================================================================
void TribratProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (auto ch = getTotalNumInputChannels();
         ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    auto readParams = [&] (int row) -> VibratoEngine::Params
    {
        VibratoEngine::Params out;
        out.triggered  = apvts.getRawParameterValue (rowParam (row, "trigger"))  ->load() > 0.5f;
        out.onsetMs    = apvts.getRawParameterValue (rowParam (row, "onset"))    ->load();
        out.rateHz     = apvts.getRawParameterValue (rowParam (row, "rate"))     ->load();
        out.pitchCents = apvts.getRawParameterValue (rowParam (row, "pitch"))    ->load();
        out.amplitude  = apvts.getRawParameterValue (rowParam (row, "amplitude"))->load();
        out.formant    = apvts.getRawParameterValue (rowParam (row, "formant"))  ->load();
        out.variation  = apvts.getRawParameterValue (rowParam (row, "variation"))->load();
        return out;
    };

    engine1.process (buffer, readParams (1));   // Row 1 first
    engine2.process (buffer, readParams (2));   // Row 2 in series
}

//==============================================================================
juce::AudioProcessorEditor* TribratProcessor::createEditor()
{
    return new TribratEditor (*this);
}

//==============================================================================
void TribratProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TribratProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TribratProcessor();
}

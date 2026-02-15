#pragma once
#include <JuceHeader.h>
#include "VibratoEngine.h"

//==============================================================================
class TribratProcessor : public juce::AudioProcessor
{
public:
    TribratProcessor();
    ~TribratProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Tribrato"; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()    override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;

    static juce::String rowParam (int row, const juce::String& name)
    {
        return "row" + juce::String (row) + "_" + name;
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    VibratoEngine engine1, engine2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TribratProcessor)
};

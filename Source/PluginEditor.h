#pragma once
#include "PluginProcessor.h"

//==============================================================================
// Custom LookAndFeel for dark 3D knobs with blue glow arc
//==============================================================================
class TribratLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TribratLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;

private:
    juce::Image knobShadowImg;
};

//==============================================================================
// Trigger button drawn with on/off PNG images
//==============================================================================
class ImageTriggerButton : public juce::Component, public juce::Timer
{
public:
    ImageTriggerButton (juce::RangedAudioParameter& trigParam,
                        juce::RangedAudioParameter& modeParam,
                        int rowNumber);

    void paint     (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void timerCallback() override;

private:
    juce::RangedAudioParameter& triggerParam;
    juce::RangedAudioParameter& modeParam;
    juce::Image onImage, offImage;
    bool currentState = false;
};

//==============================================================================
// Momentary / Latch toggle drawn with left/right PNG images
//==============================================================================
class ImageToggle : public juce::Component, public juce::Timer
{
public:
    ImageToggle (juce::RangedAudioParameter& param, int rowNumber);

    void paint     (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void timerCallback() override;

private:
    juce::RangedAudioParameter& modeParam;
    juce::Image leftImage, rightImage;
    bool isRight = true;
};

//==============================================================================
// One row: toggle + trigger + 6 knobs
//==============================================================================
class RowComponent : public juce::Component, public juce::Timer
{
public:
    RowComponent (TribratProcessor& proc, int rowNumber);
    void resized() override;
    void timerCallback() override;

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;

    int row;
    ImageTriggerButton triggerButton;
    ImageToggle        modeToggle;

    struct KnobGroup
    {
        juce::Slider slider;
        juce::Label  nameLabel, valueLabel;
        std::unique_ptr<SA> attachment;
    };

    KnobGroup knobs[6];
    juce::Label triggerLabel;
    juce::Label momentaryLabel, latchLabel, modeLabel;
};

//==============================================================================
class TribratEditor : public juce::AudioProcessorEditor
{
public:
    explicit TribratEditor (TribratProcessor&);
    ~TribratEditor() override;

    void paint   (juce::Graphics&) override;
    void resized() override;

private:
    TribratProcessor&  processor;
    TribratLookAndFeel lnf;
    RowComponent       row1, row2;
    juce::Label        titleLabel, footerLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TribratEditor)
};

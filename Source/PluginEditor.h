#pragma once
#include "PluginProcessor.h"

//==============================================================================
// Custom LookAndFeel for the new knob artwork
//==============================================================================
class TribratLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TribratLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    void drawLabel (juce::Graphics&, juce::Label&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;

private:
    juce::Image knobImg;
};

//==============================================================================
// Trigger button drawn with on/off PNG images
//==============================================================================
class ImageTriggerButton : public juce::Component, public juce::Timer
{
public:
    ImageTriggerButton (juce::RangedAudioParameter& trigParam,
                        juce::RangedAudioParameter& modeParam);

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
// Momentary / Latch toggle drawn with the supplied PNG strip
//==============================================================================
class ImageToggle : public juce::Component, public juce::Timer
{
public:
    explicit ImageToggle (juce::RangedAudioParameter& param);

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
    void setLabelScale (float scale);

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
    juce::Label modeLabel;
    float labelScale = 1.0f;
};

//==============================================================================
class FooterImageButton : public juce::Component
{
public:
    FooterImageButton() = default;

    void setImages (juce::Image off, juce::Image hover, juce::Image on,
                    bool shouldToggle, bool initialState);

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit  (const juce::MouseEvent&) override;
    void mouseUp    (const juce::MouseEvent&) override;

private:
    juce::Image offImage, hoverImage, onImage;
    bool toggleable = false;
    bool toggled = false;
    bool hovered = false;
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
    RowComponent       row1, row2, row3;
    FooterImageButton  undoButton, redoButton, tipButton, settingsButton, enableButton;
    juce::Image        bgImg, dividerImg;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TribratEditor)
};

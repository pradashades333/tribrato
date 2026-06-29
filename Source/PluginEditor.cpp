#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
juce::Image loadImg (const void* data, int size)
{
    return juce::ImageCache::getFromMemory (data, size);
}

constexpr float kReferenceWidth  = 1112.0f;
constexpr float kReferenceHeight = 1415.0f;
constexpr float kKnobStartAngleDegrees = 230.0f;
constexpr float kKnobEndAngleDegrees = 490.0f;
constexpr float kKnobAssetAngleDegrees = 230.0f;

juce::Rectangle<int> scaleRect (juce::Rectangle<float> area, float x, float y, float w, float h)
{
    const auto sx = area.getWidth()  / kReferenceWidth;
    const auto sy = area.getHeight() / kReferenceHeight;

    return juce::Rectangle<int> (
        juce::roundToInt (area.getX() + x * sx),
        juce::roundToInt (area.getY() + y * sy),
        juce::roundToInt (w * sx),
        juce::roundToInt (h * sy));
}

void styleLabel (juce::Label& label, const juce::String& text, juce::Component& parent,
                 float size, juce::Colour colour)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (size));
    label.setColour (juce::Label::textColourId, colour);
    parent.addAndMakeVisible (label);
}

void setParameterValue (juce::RangedAudioParameter& parameter, float value)
{
    parameter.beginChangeGesture();
    parameter.setValueNotifyingHost (value);
    parameter.endChangeGesture();
}
}

//==============================================================================
TribratLookAndFeel::TribratLookAndFeel()
{
    knobImg = loadImg (BinaryData::knob1_png, BinaryData::knob1_pngSize);

    setColour (juce::Label::textColourId,               juce::Colour (0xffb6bac1));
    setColour (juce::Slider::textBoxTextColourId,       juce::Colour (0xffb6bac1));
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

juce::Label* TribratLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
    label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    label->setFont (juce::FontOptions (10.0f));
    return label;
}

void TribratLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    if (label.isBeingEdited())
        return;

    g.setFont (label.getFont());
    g.setColour (juce::Colour (0xaa000000));
    g.drawText (label.getText(), label.getLocalBounds().translated (0, 1),
                label.getJustificationType(), false);
    g.setColour (label.findColour (juce::Label::textColourId));
    g.drawText (label.getText(), label.getLocalBounds(),
                label.getJustificationType(), false);
}

void TribratLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                           int x, int y, int width, int height,
                                           float sliderPos, float startAngle, float endAngle,
                                           juce::Slider&)
{
    using namespace juce;
    juce::ignoreUnused (startAngle, endAngle);

    auto bounds = Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const auto centre = bounds.getCentre();
    const auto knobStartAngle = juce::degreesToRadians (kKnobStartAngleDegrees);
    const auto knobEndAngle = juce::degreesToRadians (kKnobEndAngleDegrees);
    const auto toAngle = knobStartAngle + sliderPos * (knobEndAngle - knobStartAngle);
    const auto radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto arcRadius = radius * 1.12f;
    const auto arcThickness = jmax (2.0f, radius * 0.08f);

    Path trackArc;
    trackArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            knobStartAngle, knobEndAngle, true);
    g.setColour (Colour (0x32000000));
    g.strokePath (trackArc, PathStrokeType (arcThickness, PathStrokeType::curved, PathStrokeType::rounded));

    Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            knobStartAngle, toAngle, true);
    g.setColour (Colour (0xff59efe0).withAlpha (0.30f));
    g.strokePath (valueArc, PathStrokeType (arcThickness * 2.4f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colour (0xff59efe0));
    g.strokePath (valueArc, PathStrokeType (arcThickness, PathStrokeType::curved, PathStrokeType::rounded));

    g.setColour (Colour (0x50000000));
    g.drawEllipse (bounds.expanded (1.0f), 1.2f);

    if (! knobImg.isNull())
    {
        const auto rotation = toAngle - juce::degreesToRadians (kKnobAssetAngleDegrees);
        const auto scaleX = bounds.getWidth()  / (float) knobImg.getWidth();
        const auto scaleY = bounds.getHeight() / (float) knobImg.getHeight();

        auto transform = AffineTransform::translation (-(float) knobImg.getWidth() * 0.5f,
                                                       -(float) knobImg.getHeight() * 0.5f)
                            .followedBy (AffineTransform::scale (scaleX, scaleY))
                            .followedBy (AffineTransform::rotation (rotation))
                            .followedBy (AffineTransform::translation (centre.x, centre.y));

        g.drawImageTransformed (knobImg, transform, false);
    }
}

//==============================================================================
ImageTriggerButton::ImageTriggerButton (juce::RangedAudioParameter& tp,
                                        juce::RangedAudioParameter& mp)
    : triggerParam (tp), modeParam (mp)
{
    onImage  = loadImg (BinaryData::trigg_on_png, BinaryData::trigg_on_pngSize);
    offImage = loadImg (BinaryData::trigg_off_png, BinaryData::trigg_off_pngSize);
    startTimerHz (30);
}

void ImageTriggerButton::paint (juce::Graphics& g)
{
    auto& image = currentState ? onImage : offImage;
    if (! image.isNull())
        g.drawImage (image, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
}

void ImageTriggerButton::mouseDown (const juce::MouseEvent&)
{
    const bool isLatch = modeParam.getValue() > 0.5f;

    if (isLatch)
    {
        const auto nextValue = triggerParam.getValue() > 0.5f ? 0.0f : 1.0f;
        setParameterValue (triggerParam, nextValue);
    }
    else
    {
        triggerParam.beginChangeGesture();
        triggerParam.setValueNotifyingHost (1.0f);
    }
}

void ImageTriggerButton::mouseUp (const juce::MouseEvent&)
{
    if (modeParam.getValue() <= 0.5f)
    {
        triggerParam.setValueNotifyingHost (0.0f);
        triggerParam.endChangeGesture();
    }
}

void ImageTriggerButton::timerCallback()
{
    const bool nextState = triggerParam.getValue() > 0.5f;
    if (nextState != currentState)
    {
        currentState = nextState;
        repaint();
    }
}

//==============================================================================
ImageToggle::ImageToggle (juce::RangedAudioParameter& parameter)
    : modeParam (parameter)
{
    leftImage  = loadImg (BinaryData::momentary_on_png, BinaryData::momentary_on_pngSize);
    rightImage = loadImg (BinaryData::latch_on_png, BinaryData::latch_on_pngSize);
    startTimerHz (30);
}

void ImageToggle::paint (juce::Graphics& g)
{
    auto& image = isRight ? rightImage : leftImage;
    if (! image.isNull())
        g.drawImage (image, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
}

void ImageToggle::mouseDown (const juce::MouseEvent&)
{
    const auto nextValue = modeParam.getValue() > 0.5f ? 0.0f : 1.0f;
    setParameterValue (modeParam, nextValue);
}

void ImageToggle::timerCallback()
{
    const bool nextState = modeParam.getValue() > 0.5f;
    if (nextState != isRight)
    {
        isRight = nextState;
        repaint();
    }
}

//==============================================================================
RowComponent::RowComponent (TribratProcessor& proc, int rowNumber)
    : row (rowNumber),
      triggerButton (*proc.apvts.getParameter (proc.rowParam (row, "trigger")),
                     *proc.apvts.getParameter (proc.rowParam (row, "mode"))),
      modeToggle (*proc.apvts.getParameter (proc.rowParam (row, "mode")))
{
    addAndMakeVisible (triggerButton);
    addAndMakeVisible (modeToggle);

    styleLabel (modeLabel, "MODE", *this, 17.0f, juce::Colour (0xffa2a8af));
    styleLabel (triggerLabel, "TRIGGER", *this, 18.0f, juce::Colour (0xff45d7cb));

    static const char* names[] = { "ONSET RATE", "RATE", "PITCH", "AMPLITUDE", "FORMANT", "VARIATION" };
    static const char* suffixes[] = { "onset", "rate", "pitch", "amplitude", "formant", "variation" };

    for (int i = 0; i < 6; ++i)
    {
        auto& knob = knobs[i];
        knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        knob.slider.setRotaryParameters (juce::degreesToRadians (kKnobStartAngleDegrees),
                                         juce::degreesToRadians (kKnobEndAngleDegrees), true);
        addAndMakeVisible (knob.slider);

        styleLabel (knob.nameLabel, names[i], *this, 18.0f, juce::Colour (0xffa2a8af));
        styleLabel (knob.valueLabel, "", *this, 18.0f, juce::Colour (0xffc2c8ce));

        knob.attachment = std::make_unique<SA> (proc.apvts, proc.rowParam (row, suffixes[i]), knob.slider);
    }

    startTimerHz (15);
}

void RowComponent::setLabelScale (float scale)
{
    labelScale = scale;
    modeLabel.setFont (juce::FontOptions (17.0f * scale));
    triggerLabel.setFont (juce::FontOptions (18.0f * scale));

    for (auto& knob : knobs)
    {
        knob.nameLabel.setFont (juce::FontOptions (18.0f * scale));
        knob.valueLabel.setFont (juce::FontOptions (18.0f * scale));
    }
}

void RowComponent::timerCallback()
{
    for (int i = 0; i < 6; ++i)
    {
        const auto value = knobs[i].slider.getValue();
        const auto text = i == 1 ? "(" + juce::String (value, 1) + ")"
                                 : "(" + juce::String (juce::roundToInt (value)) + ")";

        if (knobs[i].valueLabel.getText() != text)
            knobs[i].valueLabel.setText (text, juce::dontSendNotification);
    }
}

void RowComponent::resized()
{
    auto area = getLocalBounds().toFloat();
    const auto w = area.getWidth();
    const auto h = area.getHeight();

    const auto toggleW = w * 0.36f;
    const auto toggleH = h * 0.18f;
    modeToggle.setBounds (juce::roundToInt ((w - toggleW) * 0.5f),
                          juce::roundToInt (h * 0.02f),
                          juce::roundToInt (toggleW),
                          juce::roundToInt (toggleH));

    modeLabel.setBounds (juce::roundToInt ((w - 80.0f * labelScale) * 0.5f),
                         juce::roundToInt (h * 0.24f),
                         juce::roundToInt (80.0f * labelScale),
                         juce::roundToInt (18.0f * labelScale));

    const auto knobAreaTop = h * 0.38f;
    const auto knobSize = juce::jmin (h * 0.34f, w * 0.105f);
    const auto labelTop = knobAreaTop + knobSize + h * 0.06f;
    const auto startX = w * 0.02f;
    const auto step = (w - startX * 2.0f) / 7.0f;

    triggerButton.setBounds (juce::roundToInt (startX + (step - knobSize) * 0.5f),
                             juce::roundToInt (knobAreaTop),
                             juce::roundToInt (knobSize),
                             juce::roundToInt (knobSize));

    triggerLabel.setBounds (juce::roundToInt (startX - step * 0.06f),
                            juce::roundToInt (labelTop),
                            juce::roundToInt (step * 1.12f),
                            juce::roundToInt (22.0f * labelScale));

    for (int i = 0; i < 6; ++i)
    {
        const auto x = startX + step * (float) (i + 1);
        auto& knob = knobs[i];

        knob.slider.setBounds (juce::roundToInt (x + (step - knobSize) * 0.5f),
                               juce::roundToInt (knobAreaTop),
                               juce::roundToInt (knobSize),
                               juce::roundToInt (knobSize));

        knob.nameLabel.setBounds (juce::roundToInt (x - step * 0.08f),
                                  juce::roundToInt (labelTop),
                                  juce::roundToInt (step * 1.16f),
                                  juce::roundToInt (22.0f * labelScale));

        knob.valueLabel.setBounds (juce::roundToInt (x - step * 0.08f),
                                   juce::roundToInt (labelTop + 21.0f * labelScale),
                                   juce::roundToInt (step * 1.16f),
                                   juce::roundToInt (22.0f * labelScale));
    }
}

//==============================================================================
void FooterImageButton::setImages (juce::Image off, juce::Image hover, juce::Image on,
                                   bool shouldToggle, bool initialState)
{
    offImage = std::move (off);
    hoverImage = std::move (hover);
    onImage = std::move (on);
    toggleable = shouldToggle;
    toggled = initialState;
    repaint();
}

void FooterImageButton::paint (juce::Graphics& g)
{
    const auto& image = toggleable && toggled && ! onImage.isNull()
                            ? onImage
                            : (hovered && ! hoverImage.isNull() ? hoverImage : offImage);

    if (! image.isNull())
        g.drawImage (image, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
}

void FooterImageButton::mouseEnter (const juce::MouseEvent&)
{
    hovered = true;
    repaint();
}

void FooterImageButton::mouseExit (const juce::MouseEvent&)
{
    hovered = false;
    repaint();
}

void FooterImageButton::mouseUp (const juce::MouseEvent&)
{
    if (toggleable)
        toggled = ! toggled;

    repaint();
}

//==============================================================================
TribratEditor::TribratEditor (TribratProcessor& p)
    : AudioProcessorEditor (p), processor (p),
      row1 (p, 1), row2 (p, 2), row3 (p, 3)
{
    setLookAndFeel (&lnf);

    bgImg = loadImg (BinaryData::Full_bg2_png, BinaryData::Full_bg2_pngSize);
    dividerImg = loadImg (BinaryData::divider_line_png, BinaryData::divider_line_pngSize);

    addAndMakeVisible (row1);
    addAndMakeVisible (row2);
    addAndMakeVisible (row3);

    undoButton.setImages (loadImg (BinaryData::undo_off_png, BinaryData::undo_off_pngSize),
                          loadImg (BinaryData::undo_hover_png, BinaryData::undo_hover_pngSize),
                          loadImg (BinaryData::undo_on_png, BinaryData::undo_on_pngSize),
                          false, false);
    redoButton.setImages (loadImg (BinaryData::redo_off_png, BinaryData::redo_off_pngSize),
                          loadImg (BinaryData::redo_hover_png, BinaryData::redo_hover_pngSize),
                          loadImg (BinaryData::redo_on_png, BinaryData::redo_on_pngSize),
                          false, false);
    tipButton.setImages (loadImg (BinaryData::tip_off_png, BinaryData::tip_off_pngSize),
                         loadImg (BinaryData::tip_hover_png, BinaryData::tip_hover_pngSize),
                         loadImg (BinaryData::tip_on_png, BinaryData::tip_on_pngSize),
                         true, true);
    settingsButton.setImages (loadImg (BinaryData::sett_off_png, BinaryData::sett_off_pngSize),
                              loadImg (BinaryData::sett_hover_png, BinaryData::sett_hover_pngSize),
                              juce::Image(), false, false);
    enableButton.setImages (loadImg (BinaryData::enable_off_png, BinaryData::enable_off_pngSize),
                            loadImg (BinaryData::enable_hover_png, BinaryData::enable_hover_pngSize),
                            loadImg (BinaryData::enable_on_png, BinaryData::enable_on_pngSize),
                            true, true);

    addAndMakeVisible (undoButton);
    addAndMakeVisible (redoButton);
    addAndMakeVisible (tipButton);
    addAndMakeVisible (settingsButton);
    addAndMakeVisible (enableButton);

    const auto fallbackWidth = 557;
    const auto fallbackHeight = 706;
    setSize (bgImg.isNull() ? fallbackWidth : bgImg.getWidth() / 2,
             bgImg.isNull() ? fallbackHeight : bgImg.getHeight() / 2);
}

TribratEditor::~TribratEditor()
{
    setLookAndFeel (nullptr);
}

void TribratEditor::paint (juce::Graphics& g)
{
    if (! bgImg.isNull())
        g.drawImage (bgImg, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
    else
        g.fillAll (juce::Colour (0xff171c20));

    if (! dividerImg.isNull())
    {
        auto area = getLocalBounds().toFloat();
        g.drawImage (dividerImg, scaleRect (area, 52.0f, 516.0f, 1008.0f, 5.0f).toFloat(),
                     juce::RectanglePlacement::stretchToFit);
        g.drawImage (dividerImg, scaleRect (area, 52.0f, 919.0f, 1008.0f, 5.0f).toFloat(),
                     juce::RectanglePlacement::stretchToFit);
    }
}

void TribratEditor::resized()
{
    auto area = getLocalBounds().toFloat();
    const auto labelScale = area.getWidth() / kReferenceWidth;

    row1.setLabelScale (labelScale);
    row2.setLabelScale (labelScale);
    row3.setLabelScale (labelScale);

    row1.setBounds (scaleRect (area, 54.0f, 150.0f, 1002.0f, 252.0f));
    row2.setBounds (scaleRect (area, 54.0f, 553.0f, 1002.0f, 252.0f));
    row3.setBounds (scaleRect (area, 54.0f, 955.0f, 1002.0f, 252.0f));

    const auto buttonSize = juce::roundToInt (area.getWidth() * 0.043f);
    const auto gap = juce::roundToInt (buttonSize * 0.42f);
    auto x = getWidth() - juce::roundToInt (area.getWidth() * 0.03f) - buttonSize * 5 - gap * 4;
    const auto y = getHeight() - juce::roundToInt (area.getHeight() * 0.058f) - buttonSize / 2;

    undoButton.setBounds (x, y, buttonSize, buttonSize);
    x += buttonSize + gap;
    redoButton.setBounds (x, y, buttonSize, buttonSize);
    x += buttonSize + gap;
    tipButton.setBounds (x, y, buttonSize, buttonSize);
    x += buttonSize + gap;
    settingsButton.setBounds (x, y, buttonSize, buttonSize);
    x += buttonSize + gap;
    enableButton.setBounds (x, y, buttonSize, buttonSize);
}

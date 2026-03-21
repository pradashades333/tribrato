#include "PluginEditor.h"
#include <BinaryData.h>

static juce::Image loadImg (const void* data, int size)
{
    return juce::ImageCache::getFromMemory (data, size);
}

//==============================================================================
//  TribratLookAndFeel
//==============================================================================
TribratLookAndFeel::TribratLookAndFeel()
{
    knobShadowImg    = loadImg (BinaryData::knob_shadow_png,    BinaryData::knob_shadow_pngSize);
    knobImg          = loadImg (BinaryData::KNOB_NOBG_png,      BinaryData::KNOB_NOBG_pngSize);
    knobHighlightImg = loadImg (BinaryData::knob_highlight_png, BinaryData::knob_highlight_pngSize);

    setColour (juce::Label::textColourId,            juce::Colour (0xff8b95a1));
    setColour (juce::Slider::textBoxTextColourId,     juce::Colour (0xff8b95a1));
    setColour (juce::Slider::textBoxOutlineColourId,  juce::Colours::transparentBlack);
}

juce::Label* TribratLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = juce::LookAndFeel_V4::createSliderTextBox (s);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId,    juce::Colours::transparentBlack);
    l->setFont (juce::FontOptions (10.0f));
    return l;
}

void TribratLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour (juce::Label::backgroundColourId));

    if (! label.isBeingEdited())
    {
        auto font = label.getFont();
        auto text = label.getText();
        auto just = label.getJustificationType();
        auto b    = label.getLocalBounds();

        g.setFont (font);

        // Dark drop-shadow 1px below
        g.setColour (juce::Colour (0x80000000));
        g.drawText (text, b.translated (0, 1), just, false);

        // Bright text
        g.setColour (juce::Colour (0xffc8d4e0));
        g.drawText (text, b, just, false);
    }
}

void TribratLookAndFeel::drawRotarySlider (juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos, float startAngle, float endAngle,
    juce::Slider&)
{
    using namespace juce;
    auto bounds = Rectangle<float> ((float) x, (float) y,
                                    (float) width, (float) height).reduced (2.0f);
    float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    float cx = bounds.getCentreX();
    float cy = bounds.getCentreY();

    float toAngle = startAngle + sliderPos * (endAngle - startAngle);

    // 1 — Drop shadow: soft radial gradient (PNG has white bg, use gradient instead)
    {
        float sz = radius * 1.6f;
        ColourGradient shadowGrad (Colour (0x70000000), cx, cy + sz * 0.15f,
                                   Colour (0x00000000), cx + sz, cy + sz * 0.15f, true);
        g.setGradientFill (shadowGrad);
        g.fillEllipse (cx - sz, cy - sz * 0.7f, sz * 2.0f, sz * 2.0f);
    }

    // 2 — Blue highlight ring (static, not rotated) drawn UNDER the knob body
    if (! knobHighlightImg.isNull())
    {
        float sz = radius * 2.5f;
        g.drawImage (knobHighlightImg,
                     { cx - sz * 0.5f, cy - sz * 0.5f, sz, sz },
                     RectanglePlacement::stretchToFit);
    }

    // 3 — Knob body, rotated to show value (indicator dot at top = 12 o'clock)
    if (! knobImg.isNull())
    {
        float imgSize = radius * 2.0f;
        float scaleX  = imgSize / (float) knobImg.getWidth();
        float scaleY  = imgSize / (float) knobImg.getHeight();

        auto xf = AffineTransform::translation (-(float) knobImg.getWidth()  * 0.5f,
                                                -(float) knobImg.getHeight() * 0.5f)
                    .followedBy (AffineTransform::scale (scaleX, scaleY))
                    .followedBy (AffineTransform::rotation (toAngle))
                    .followedBy (AffineTransform::translation (cx, cy));

        g.drawImageTransformed (knobImg, xf, false);
    }
}

//==============================================================================
//  ImageTriggerButton
//==============================================================================
ImageTriggerButton::ImageTriggerButton (juce::RangedAudioParameter& tp,
                                        juce::RangedAudioParameter& mp,
                                        int rowNumber)
    : triggerParam (tp), modeParam (mp)
{
    if (rowNumber == 1)
    {
        onImage  = loadImg (BinaryData::trigger1_on_png,  BinaryData::trigger1_on_pngSize);
        offImage = loadImg (BinaryData::trigger1_off_png, BinaryData::trigger1_off_pngSize);
    }
    else if (rowNumber == 3)
    {
        onImage  = loadImg (BinaryData::trigger3_on_png,  BinaryData::trigger3_on_pngSize);
        offImage = loadImg (BinaryData::trigger3_off_png, BinaryData::trigger3_off_pngSize);
    }
    else
    {
        onImage  = loadImg (BinaryData::trigger2_on_png,  BinaryData::trigger2_on_pngSize);
        offImage = loadImg (BinaryData::trigger2_off_png, BinaryData::trigger2_off_pngSize);
    }
    startTimerHz (30);
}

void ImageTriggerButton::paint (juce::Graphics& g)
{
    auto& img = currentState ? onImage : offImage;
    if (! img.isNull())
        g.drawImage (img, getLocalBounds().toFloat(),
                     juce::RectanglePlacement::centred);
}

void ImageTriggerButton::mouseDown (const juce::MouseEvent&)
{
    bool isLatch = modeParam.getValue() > 0.5f;
    if (isLatch)
    {
        bool on = triggerParam.getValue() > 0.5f;
        triggerParam.setValueNotifyingHost (on ? 0.0f : 1.0f);
    }
    else
    {
        triggerParam.setValueNotifyingHost (1.0f);
    }
}

void ImageTriggerButton::mouseUp (const juce::MouseEvent&)
{
    if (modeParam.getValue() <= 0.5f)          // Momentary
        triggerParam.setValueNotifyingHost (0.0f);
}

void ImageTriggerButton::timerCallback()
{
    bool on = triggerParam.getValue() > 0.5f;
    if (on != currentState) { currentState = on; repaint(); }
}

//==============================================================================
//  ImageToggle
//==============================================================================
ImageToggle::ImageToggle (juce::RangedAudioParameter& p, int rowNumber)
    : modeParam (p)
{
    if (rowNumber == 1)
    {
        leftImage  = loadImg (BinaryData::toggle1_left_png,  BinaryData::toggle1_left_pngSize);
        rightImage = loadImg (BinaryData::toggle1_right_png, BinaryData::toggle1_right_pngSize);
    }
    else if (rowNumber == 3)
    {
        leftImage  = loadImg (BinaryData::toggle3_left_png,  BinaryData::toggle3_left_pngSize);
        rightImage = loadImg (BinaryData::toggle3_right_png, BinaryData::toggle3_right_pngSize);
    }
    else
    {
        leftImage  = loadImg (BinaryData::toggle2_left_png,  BinaryData::toggle2_left_pngSize);
        rightImage = loadImg (BinaryData::toggle2_right_png, BinaryData::toggle2_right_pngSize);
    }
    startTimerHz (30);
}

void ImageToggle::paint (juce::Graphics& g)
{
    auto& img = isRight ? rightImage : leftImage;
    if (! img.isNull())
        g.drawImage (img, getLocalBounds().toFloat(),
                     juce::RectanglePlacement::stretchToFit);
}

void ImageToggle::mouseDown (const juce::MouseEvent&)
{
    bool cur = modeParam.getValue() > 0.5f;
    modeParam.setValueNotifyingHost (cur ? 0.0f : 1.0f);
}

void ImageToggle::timerCallback()
{
    bool r = modeParam.getValue() > 0.5f;
    if (r != isRight) { isRight = r; repaint(); }
}

//==============================================================================
//  RowComponent
//==============================================================================
static void styleLabel (juce::Label& l, const juce::String& text,
                        juce::Component& parent, float size = 9.0f)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (juce::FontOptions (size));
    l.setColour (juce::Label::textColourId, juce::Colour (0xff8b95a1));
    parent.addAndMakeVisible (l);
}

RowComponent::RowComponent (TribratProcessor& proc, int rowNumber)
    : row (rowNumber),
      triggerButton (*proc.apvts.getParameter (proc.rowParam (row, "trigger")),
                     *proc.apvts.getParameter (proc.rowParam (row, "mode")),
                     row),
      modeToggle (*proc.apvts.getParameter (proc.rowParam (row, "mode")), row)
{
    addAndMakeVisible (triggerButton);
    addAndMakeVisible (modeToggle);

    styleLabel (momentaryLabel, "MOMENTARY", *this, 9.0f);
    styleLabel (latchLabel,     "LATCH",     *this, 9.0f);
    styleLabel (modeLabel,      "MODE",      *this, 9.0f);
    styleLabel (triggerLabel,   "TRIGGER",   *this, 9.0f);

    static const char* names[]   = { "ONSET RATE", "RATE", "PITCH",
                                     "AMPLITUDE",  "FORMANT", "VARIATION" };
    static const char* suffixes[] = { "onset", "rate", "pitch",
                                      "amplitude", "formant", "variation" };

    for (int i = 0; i < 6; ++i)
    {
        auto& k = knobs[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        addAndMakeVisible (k.slider);

        styleLabel (k.nameLabel,  names[i], *this, 9.0f);
        styleLabel (k.valueLabel, "",       *this, 9.0f);

        k.attachment = std::make_unique<SA> (
            proc.apvts, proc.rowParam (row, suffixes[i]), k.slider);
    }

    startTimerHz (15);
}

void RowComponent::timerCallback()
{
    for (int i = 0; i < 6; ++i)
    {
        double v = knobs[i].slider.getValue();
        juce::String t;
        if (i == 1)  t = "(" + juce::String (v, 1) + ")";   // Rate → 1 decimal
        else         t = "(" + juce::String ((int) v) + ")";
        if (knobs[i].valueLabel.getText() != t)
            knobs[i].valueLabel.setText (t, juce::dontSendNotification);
    }
}

void RowComponent::resized()
{
    auto area = getLocalBounds();
    int  w    = area.getWidth();

    // ---- Controls geometry ----
    int numCols  = 7;
    int colW     = 52;       // tight cluster for 500px width
    int knobSize = 46;
    int trigSize = knobSize; // same size as a single knob
    int toggleW  = 56, toggleH = 26;   // narrower proportions

    // Spread vertically to fill taller 850px rows (~241px each)
    int toggleY  = 30;
    int ctrlY    = toggleY + toggleH + 35;

    // ---- Toggle section (centred horizontally) ----
    int startX  = (w - numCols * colW) / 2;
    int toggleX = startX + (numCols * colW - toggleW) / 2;
    modeToggle.setBounds (toggleX, toggleY, toggleW, toggleH);

    // Labels 2px from toggle edges (closer than before)
    momentaryLabel.setBounds (toggleX - 72, toggleY + 5, 70, 16);
    latchLabel.setBounds     (toggleX + toggleW + 2, toggleY + 5, 44, 16);
    modeLabel.setBounds      (toggleX, toggleY + toggleH + 1, toggleW, 13);

    // ---- Column 0 — trigger button ----
    int col0 = startX;
    triggerButton.setBounds (col0 + (colW - trigSize) / 2, ctrlY - (trigSize - knobSize) / 2,
                             trigSize, trigSize);
    triggerLabel.setBounds  (col0, ctrlY + knobSize + 4, colW, 13);

    // ---- Columns 1-6 — knobs ----
    for (int i = 0; i < 6; ++i)
    {
        int cx = startX + (i + 1) * colW;
        knobs[i].slider.setBounds    (cx + (colW - knobSize) / 2, ctrlY, knobSize, knobSize);
        knobs[i].nameLabel.setBounds (cx - 2, ctrlY + knobSize + 4, colW + 4, 13);
        knobs[i].valueLabel.setBounds(cx, ctrlY + knobSize + 17, colW, 12);
    }
}

//==============================================================================
//  TribratEditor
//==============================================================================
TribratEditor::TribratEditor (TribratProcessor& p)
    : AudioProcessorEditor (p), processor (p),
      row1 (p, 1), row2 (p, 2), row3 (p, 3)
{
    setLookAndFeel (&lnf);

    // Background image — title, footer, and separator lines are all baked in
    bgImg = loadImg (BinaryData::background_jpeg, BinaryData::background_jpegSize);

    titleLabel.setVisible (false);
    addAndMakeVisible (titleLabel);

    footerLabel.setVisible (false);   // baked into background.jpeg
    addAndMakeVisible (footerLabel);

    addAndMakeVisible (row1);
    addAndMakeVisible (row2);
    addAndMakeVisible (row3);

    setSize (500, 850);
}

TribratEditor::~TribratEditor()
{
    setLookAndFeel (nullptr);
}

void TribratEditor::paint (juce::Graphics& g)
{
    // Draw background.jpeg — title, lines, and footer are all baked in
    if (! bgImg.isNull())
        g.drawImage (bgImg, getLocalBounds().toFloat(),
                     juce::RectanglePlacement::stretchToFit);
    else
    {
        g.fillAll (juce::Colour (0xff1e2227));
    }
}

void TribratEditor::resized()
{
    auto area = getLocalBounds();
    titleLabel.setBounds  (area.removeFromTop    (72));
    footerLabel.setBounds (area.removeFromBottom (55));

    int rowH = area.getHeight() / 3;
    row1.setBounds (area.removeFromTop (rowH));
    row2.setBounds (area.removeFromTop (rowH));
    row3.setBounds (area);
}

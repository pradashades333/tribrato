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
    knobShadowImg = loadImg (BinaryData::knob_shadow_png,
                             BinaryData::knob_shadow_pngSize);

    setColour (juce::Label::textColourId,       juce::Colour (0xff7a7a88));
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xff7a7a88));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

juce::Label* TribratLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = juce::LookAndFeel_V4::createSliderTextBox (s);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId,    juce::Colours::transparentBlack);
    l->setFont (juce::FontOptions (10.0f));
    return l;
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

    // 1 — Shadow image behind knob
    if (! knobShadowImg.isNull())
    {
        float sz = radius * 2.8f;
        g.drawImage (knobShadowImg,
                     { cx - sz * 0.5f, cy - sz * 0.42f, sz, sz },
                     RectanglePlacement::stretchToFit);
    }

    // 2 — Background arc (dark track)
    float arcR = radius * 0.88f;
    {
        Path bg;
        bg.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (Colour (0xff1a1a22));
        g.strokePath (bg, PathStrokeType (3.5f, PathStrokeType::curved,
                                          PathStrokeType::rounded));
    }

    // 3 — Blue value arc with glow
    float toAngle = startAngle + sliderPos * (endAngle - startAngle);
    if (sliderPos > 0.002f)
    {
        Path arc;
        arc.addCentredArc (cx, cy, arcR, arcR, 0.0f, startAngle, toAngle, true);

        g.setColour (Colour (0x204090cc));
        g.strokePath (arc, PathStrokeType (8.0f, PathStrokeType::curved,
                                           PathStrokeType::rounded));
        g.setColour (Colour (0x504a90d0));
        g.strokePath (arc, PathStrokeType (5.0f, PathStrokeType::curved,
                                           PathStrokeType::rounded));
        g.setColour (Colour (0xff4a95d5));
        g.strokePath (arc, PathStrokeType (2.5f, PathStrokeType::curved,
                                           PathStrokeType::rounded));
    }

    // 4 — Knob body
    float bodyR = radius * 0.62f;

    // outer rim
    g.setColour (Colour (0xff1a1a22));
    g.fillEllipse (cx - bodyR - 2, cy - bodyR - 2, (bodyR + 2) * 2, (bodyR + 2) * 2);

    // gradient fill
    {
        ColourGradient grad (Colour (0xff4a4a54), cx - bodyR * 0.3f, cy - bodyR * 0.5f,
                             Colour (0xff28282e), cx + bodyR * 0.3f, cy + bodyR * 0.6f,
                             true);
        g.setGradientFill (grad);
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2, bodyR * 2);
    }

    // inner bevel
    g.setColour (Colour (0xff353540));
    g.drawEllipse (cx - bodyR + 1, cy - bodyR + 1, (bodyR - 1) * 2, (bodyR - 1) * 2, 0.5f);

    // 5 — Rotating cross indicator
    {
        auto xf = AffineTransform::rotation (toAngle, cx, cy);
        float len = bodyR * 0.42f;
        g.setColour (Colour (0xff505058));

        Path v; v.startNewSubPath (cx, cy - len); v.lineTo (cx, cy + len);
        g.strokePath (v, PathStrokeType (1.5f), xf);

        Path h; h.startNewSubPath (cx - len, cy); h.lineTo (cx + len, cy);
        g.strokePath (h, PathStrokeType (1.5f), xf);
    }

    // 6 — Position dot
    {
        float d = bodyR * 0.72f;
        float dx = cx + d * std::cos (toAngle - MathConstants<float>::halfPi);
        float dy = cy + d * std::sin (toAngle - MathConstants<float>::halfPi);
        g.setColour (Colour (0xff6a6a78));
        g.fillEllipse (dx - 2, dy - 2, 4, 4);
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
                     juce::RectanglePlacement::centred);
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
    l.setColour (juce::Label::textColourId, juce::Colour (0xff6a6a78));
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

    // ---- Toggle section (centred at top) ----
    int toggleW = 90, toggleH = 30;
    int toggleX = (w - toggleW) / 2;
    int toggleY = 2;
    modeToggle.setBounds (toggleX, toggleY, toggleW, toggleH);

    momentaryLabel.setBounds (toggleX - 82, toggleY + 7, 78, 16);
    latchLabel.setBounds     (toggleX + toggleW + 4, toggleY + 7, 50, 16);
    modeLabel.setBounds      (toggleX, toggleY + toggleH, toggleW, 14);

    // ---- Controls row ----
    int numCols  = 7;
    int colW     = 66;
    int startX   = (w - numCols * colW) / 2;
    int ctrlY    = toggleY + toggleH + 18;
    int knobSize = 52;
    int trigSize  = 48;

    // Column 0 — trigger button
    int col0 = startX;
    triggerButton.setBounds (col0 + (colW - trigSize) / 2, ctrlY, trigSize, trigSize);
    triggerLabel.setBounds  (col0, ctrlY + knobSize + 2, colW, 13);

    // Columns 1-6 — knobs
    for (int i = 0; i < 6; ++i)
    {
        int cx = startX + (i + 1) * colW;
        knobs[i].slider.setBounds    (cx + (colW - knobSize) / 2, ctrlY, knobSize, knobSize);
        knobs[i].nameLabel.setBounds (cx - 2, ctrlY + knobSize + 2, colW + 4, 13);
        knobs[i].valueLabel.setBounds(cx, ctrlY + knobSize + 14, colW, 13);
    }
}

//==============================================================================
//  TribratEditor
//==============================================================================
TribratEditor::TribratEditor (TribratProcessor& p)
    : AudioProcessorEditor (p), processor (p),
      row1 (p, 1), row2 (p, 2)
{
    setLookAndFeel (&lnf);

    titleLabel.setText ("TRIBRATO", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffccccdd));
    addAndMakeVisible (titleLabel);

    footerLabel.setText ("Aramis - LASTLVL Technology",
                         juce::dontSendNotification);
    footerLabel.setJustificationType (juce::Justification::centred);
    footerLabel.setFont (juce::FontOptions (9.0f));
    footerLabel.setColour (juce::Label::textColourId, juce::Colour (0xff505058));
    addAndMakeVisible (footerLabel);

    addAndMakeVisible (row1);
    addAndMakeVisible (row2);

    setSize (520, 410);
}

TribratEditor::~TribratEditor()
{
    setLookAndFeel (nullptr);
}

void TribratEditor::paint (juce::Graphics& g)
{
    // Dark background gradient
    juce::ColourGradient bg (juce::Colour (0xff323238), 0, 0,
                             juce::Colour (0xff262630), 0, (float) getHeight(),
                             false);
    g.setGradientFill (bg);
    g.fillAll();

    // Separator between the two rows
    int midY = row1.getBottom();
    g.setColour (juce::Colour (0xff3a3a42));
    g.drawHorizontalLine (midY, 15.0f, (float) (getWidth() - 15));
}

void TribratEditor::resized()
{
    auto area = getLocalBounds();
    titleLabel.setBounds  (area.removeFromTop (38));
    footerLabel.setBounds (area.removeFromBottom (22));

    int rowH = area.getHeight() / 2;
    row1.setBounds (area.removeFromTop (rowH));
    row2.setBounds (area);
}

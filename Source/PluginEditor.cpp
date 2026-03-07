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
    knobImg = loadImg (BinaryData::KNOB_NOBG_png, BinaryData::KNOB_NOBG_pngSize);

    setColour (juce::Label::textColourId,        juce::Colour (0xff8b95a1));
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xff8b95a1));
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

    float toAngle = startAngle + sliderPos * (endAngle - startAngle);

    // 1 — Shadow under knob
    if (! knobShadowImg.isNull())
    {
        float sz = radius * 2.8f;
        g.drawImage (knobShadowImg,
                     { cx - sz * 0.5f, cy - sz * 0.45f, sz, sz },
                     RectanglePlacement::stretchToFit);
    }

    // 2 — Knob image, rotated around its centre
    //     The image indicator sits at 12 o'clock; toAngle rotates it to the
    //     correct position (JUCE convention: 0 = up, CW positive).
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

    // 3 — Blue glow arc drawn on top of the knob's dark outer ring
    float arcR = radius * 0.88f;
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
    using namespace juce;
    auto  b      = getLocalBounds().toFloat().reduced (3.0f);
    float corner = 4.0f;

    // Outer recessed housing: dark box, always present
    g.setColour (Colour (0xff111418));
    g.fillRoundedRectangle (b, corner);

    // Inset top shadow — makes housing look carved in
    ColourGradient insetShadow (Colour (0xcc000000), b.getCentreX(), b.getY(),
                                Colour (0x00000000), b.getCentreX(), b.getY() + 7.0f, false);
    g.setGradientFill (insetShadow);
    g.fillRoundedRectangle (b, corner);

    // Inner button — fills most of the housing, always visible
    auto inner = b.reduced (5.0f);
    float innerCorner = 3.0f;

    if (currentState)
    {
        // box-shadow: 0px 0px 10px #2a81ff
        g.setColour (Colour (0x402a81ff));
        g.fillRoundedRectangle (inner.expanded (7.0f), innerCorner + 4.0f);
        g.setColour (Colour (0x602a81ff));
        g.fillRoundedRectangle (inner.expanded (3.5f), innerCorner + 2.0f);

        // Active: linear-gradient(180deg, #4da3ff, #0055ff)
        ColourGradient activeFill (Colour (0xff4da3ff), inner.getX(), inner.getY(),
                                   Colour (0xff0055ff), inner.getX(), inner.getBottom(), false);
        g.setGradientFill (activeFill);
        g.fillRoundedRectangle (inner, innerCorner);

        // White bloom inset
        ColourGradient bloom (Colour (0x80ffffff), inner.getCentreX(), inner.getY(),
                              Colour (0x00ffffff), inner.getCentreX(), inner.getCentreY(), false);
        g.setGradientFill (bloom);
        g.fillRoundedRectangle (inner, innerCorner);
    }
    else
    {
        // OFF inner button: dark metallic raised square
        ColourGradient offFill (Colour (0xff2a3040), inner.getX(), inner.getY(),
                                Colour (0xff181e28), inner.getX(), inner.getBottom(), false);
        g.setGradientFill (offFill);
        g.fillRoundedRectangle (inner, innerCorner);

        // Subtle top highlight to give it a raised feel
        g.setColour (Colour (0x1affffff));
        g.drawLine (inner.getX() + innerCorner, inner.getY() + 0.5f,
                    inner.getRight() - innerCorner, inner.getY() + 0.5f, 1.0f);
    }
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
    using namespace juce;
    auto  b  = getLocalBounds().toFloat();
    float bW = b.getWidth();
    float bH = b.getHeight();

    // Track: wide pill matching reference (~70px wide, 14px tall)
    float trackH      = 14.0f;
    float trackW      = 70.0f;
    float trackX      = (bW - trackW) * 0.5f;
    float trackY      = (bH - trackH) * 0.5f;
    float trackCorner = 50.0f;   // border-radius: 50px — pill ends

    Rectangle<float> track (trackX, trackY, trackW, trackH);

    // Carved-in fill: #0a0b0d
    g.setColour (Colour (0xff0a0b0d));
    g.fillRoundedRectangle (track, trackCorner);

    // inset 0px 2px 5px rgba(0,0,0,0.9) — heavy shadow from top
    ColourGradient insetShadow (Colour (0xe6000000), trackX, trackY,
                                Colour (0x00000000), trackX, trackY + 6.0f, false);
    g.setGradientFill (insetShadow);
    g.fillRoundedRectangle (track, trackCorner);

    // Thumb: slides within 48px track, overflows 2px vertically
    float thumbW      = 18.0f;
    float thumbH      = trackH + 4.0f;
    float thumbCorner = 4.0f;
    float thumbY      = trackY - 2.0f;
    float thumbX      = isRight ? trackX + trackW - thumbW - 3.0f
                                : trackX + 3.0f;

    Rectangle<float> thumb (thumbX, thumbY, thumbW, thumbH);

    if (isRight)
    {
        // Blue glow drop shadow
        g.setColour (Colour (0x602a81ff));
        g.fillRoundedRectangle (thumb.expanded (3.0f), thumbCorner + 2.0f);

        // Blue gradient: #4da3ff → #0055ff
        ColourGradient thumbFill (Colour (0xff4da3ff), thumbX, thumbY,
                                  Colour (0xff0055ff), thumbX, thumbY + thumbH, false);
        g.setGradientFill (thumbFill);
        g.fillRoundedRectangle (thumb, thumbCorner);
    }
    else
    {
        // Inactive: dark drop shadow + grey metallic
        g.setColour (Colour (0x40000000));
        g.fillRoundedRectangle (thumb.expanded (2.0f), thumbCorner + 1.0f);

        ColourGradient thumbFill (Colour (0xff6a7080), thumbX, thumbY,
                                  Colour (0xff3a4050), thumbX, thumbY + thumbH, false);
        g.setGradientFill (thumbFill);
        g.fillRoundedRectangle (thumb, thumbCorner);
    }
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
    int trigSize = 52;       // large — matches reference (fills the column)
    int toggleW  = 70, toggleH = 26;

    // Spread vertically to fill taller 850px rows (~241px each)
    int toggleY  = 30;
    int ctrlY    = toggleY + toggleH + 35;

    // ---- Toggle section (centred horizontally) ----
    int startX  = (w - numCols * colW) / 2;
    int toggleX = startX + (numCols * colW - toggleW) / 2;
    modeToggle.setBounds (toggleX, toggleY, toggleW, toggleH);

    momentaryLabel.setBounds (toggleX - 90, toggleY + 5, 86, 16);
    latchLabel.setBounds     (toggleX + toggleW + 4, toggleY + 5, 52, 16);
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

    // Title drawn engraved in paint() — keep label invisible but in layout
    titleLabel.setVisible (false);
    addAndMakeVisible (titleLabel);

    // Footer — correct UTF-8 encoding for ™
    footerLabel.setText (juce::String::fromUTF8 ("Aramis - LASTLVL Technology\xe2\x84\xa2"),
                         juce::dontSendNotification);
    footerLabel.setJustificationType (juce::Justification::centred);
    footerLabel.setFont (juce::FontOptions (9.0f));
    footerLabel.setColour (juce::Label::textColourId,       juce::Colour (0xff606878));
    footerLabel.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
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
    using namespace juce;
    float W = (float) getWidth();
    float H = (float) getHeight();

    // 1 — Brushed metal background: #3a414a → #1e2227 (top to bottom)
    ColourGradient bg (Colour (0xff3a414a), 0.0f, 0.0f,
                       Colour (0xff1e2227), 0.0f, H, false);
    g.setGradientFill (bg);
    g.fillAll();

    // Dark border framing the whole plugin
    g.setColour (Colour (0xff0d1014));
    g.drawRect (getLocalBounds(), 2);

    // 2 — TRIBRATO title: bright white, matching reference
    Font titleFont (FontOptions (juce::Font::getDefaultMonospacedFontName(), 28.0f, Font::bold));
    g.setFont (titleFont);

    Rectangle<float> titleArea (0.0f, 12.0f, W, 58.0f);

    // Subtle dark drop-shadow 1px below for depth
    g.setColour (Colour (0x80000000));
    g.drawText ("TRIBRATO", titleArea.translated (0.0f, 1.0f),
                Justification::centred, false);

    // Bright white title text
    g.setColour (Colour (0xffd8e0e8));
    g.drawText ("TRIBRATO", titleArea,
                Justification::centred, false);

    // 3 — Separator lines between the 3 rows
    int titleH  = 72;
    int footerH = 55;
    int rowH    = (getHeight() - titleH - footerH) / 3;
    g.setColour (Colour (0xff2a3040));
    g.drawHorizontalLine (titleH + rowH,     2.0f, W - 2.0f);
    g.drawHorizontalLine (titleH + rowH * 2, 2.0f, W - 2.0f);
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

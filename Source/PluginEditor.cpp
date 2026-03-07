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

    setColour (juce::Label::textColourId,        juce::Colour (0xffb0b0bc));
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffb0b0bc));
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
    auto  b      = getLocalBounds().toFloat().reduced (4.0f);
    float corner = 5.0f;

    if (currentState)
    {
        // Neon-blue outer glow layers
        g.setColour (Colour (0x184a95d5));
        g.fillRoundedRectangle (b.expanded (7.0f), corner + 3.5f);
        g.setColour (Colour (0x384a95d5));
        g.fillRoundedRectangle (b.expanded (3.5f), corner + 1.5f);

        // Dark-blue inner fill
        g.setColour (Colour (0xff0d2540));
        g.fillRoundedRectangle (b, corner);

        // Bright neon border
        g.setColour (Colour (0xff4a95d5));
        g.drawRoundedRectangle (b.reduced (1.0f), corner - 0.5f, 1.5f);
    }
    else
    {
        // Off — dark rounded square, subtle rim
        g.setColour (Colour (0xff1e1e28));
        g.fillRoundedRectangle (b, corner);
        g.setColour (Colour (0xff484858));
        g.drawRoundedRectangle (b.reduced (0.5f), corner - 0.5f, 1.0f);
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
    auto  b         = getLocalBounds().toFloat();
    float bW        = b.getWidth();
    float bH        = b.getHeight();

    // Compact track: just wide enough for the square thumb to slide
    float thumbSide = bH * 0.68f;
    float pad       = 2.0f;
    float trackH    = thumbSide + pad * 2.0f;
    float trackW    = thumbSide * 2.2f;
    float trackX    = (bW - trackW) * 0.5f;
    float trackY    = (bH - trackH) * 0.5f;
    float corner    = trackH * 0.38f;
    float thumbCorner = 3.5f;

    // Track
    Rectangle<float> track (trackX, trackY, trackW, trackH);
    g.setColour (Colour (0xff1a1a24));
    g.fillRoundedRectangle (track, corner);
    g.setColour (Colour (0xff3a3a4a));
    g.drawRoundedRectangle (track, corner, 1.0f);

    // Thumb — rounded square that fills the track height
    float thumbX = isRight ? trackX + trackW - thumbSide - pad
                            : trackX + pad;
    float thumbY = trackY + pad;
    Rectangle<float> thumb (thumbX, thumbY, thumbSide, thumbSide);

    if (isRight)
    {
        // Neon-blue glow behind thumb
        g.setColour (Colour (0x404a95d5));
        g.fillRoundedRectangle (thumb.expanded (3.5f), thumbCorner + 2.0f);
        // Neon-blue thumb
        g.setColour (Colour (0xff4a95d5));
        g.fillRoundedRectangle (thumb, thumbCorner);
    }
    else
    {
        g.setColour (Colour (0xff9090a0));
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
    l.setColour (juce::Label::textColourId, juce::Colour (0xffb0b0bc));
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
    int colW     = 66;       // tighter horizontal spacing
    int knobSize = 58;
    int trigSize = 44;
    int toggleW  = 90, toggleH = 26;

    // Top padding + gap so content is balanced in each ~197px row
    int toggleY  = 18;
    int ctrlY    = toggleY + toggleH + 20;

    // ---- Toggle section (centred horizontally) ----
    int startX  = (w - numCols * colW) / 2;
    int toggleX = startX + (numCols * colW - toggleW) / 2;
    modeToggle.setBounds (toggleX, toggleY, toggleW, toggleH);

    momentaryLabel.setBounds (toggleX - 90, toggleY + 6, 86, 16);
    latchLabel.setBounds     (toggleX + toggleW + 4, toggleY + 6, 52, 16);
    modeLabel.setBounds      (toggleX, toggleY + toggleH + 1, toggleW, 13);

    // ---- Column 0 — trigger button ----
    int col0 = startX;
    triggerButton.setBounds (col0 + (colW - trigSize) / 2, ctrlY + (knobSize - trigSize) / 2,
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

    // Title — clean engraved look, no heavy drop shadow
    titleLabel.setText ("TRIBRATO", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId,       juce::Colour (0xffc4c8d0));
    titleLabel.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (titleLabel);

    footerLabel.setText ("Aramis - LASTLVL Technology\xe2\x84\xa2",
                         juce::dontSendNotification);
    footerLabel.setJustificationType (juce::Justification::centred);
    footerLabel.setFont (juce::FontOptions (9.0f));
    footerLabel.setColour (juce::Label::textColourId,       juce::Colour (0xff606070));
    footerLabel.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (footerLabel);

    addAndMakeVisible (row1);
    addAndMakeVisible (row2);
    addAndMakeVisible (row3);

    setSize (714, 700);
}

TribratEditor::~TribratEditor()
{
    setLookAndFeel (nullptr);
}

void TribratEditor::paint (juce::Graphics& g)
{
    // Continuous dark-metallic panel — no separator lines
    juce::ColourGradient bg (juce::Colour (0xff2c343d), 0.0f, 0.0f,
                              juce::Colour (0xff1a2028), 0.0f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    // Subtle centre radial sheen for metallic depth
    juce::ColourGradient shine (juce::Colour (0x0cffffff),
                                 (float) getWidth() * 0.5f, (float) getHeight() * 0.2f,
                                 juce::Colours::transparentWhite,
                                 (float) getWidth() * 0.5f, (float) getHeight() * 0.8f, true);
    g.setGradientFill (shine);
    g.fillAll();
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

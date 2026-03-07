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
    auto  b      = getLocalBounds().toFloat().reduced (3.0f);
    float corner = 6.0f;

    // Outer raised frame: gradient bevel (light top-left, dark bottom-right)
    ColourGradient frame (Colour (0xff4a5a6a), b.getX(), b.getY(),
                          Colour (0xff1a2530), b.getRight(), b.getBottom(), false);
    g.setGradientFill (frame);
    g.fillRoundedRectangle (b, corner);

    // Inner recessed area (inset by 3px from the frame)
    auto inner = b.reduced (3.0f);
    float innerCorner = corner - 1.5f;

    if (currentState)
    {
        // Wide neon-blue outer glow behind the whole button
        g.setColour (Colour (0x204a95d5));
        g.fillRoundedRectangle (b.expanded (6.0f), corner + 3.0f);
        g.setColour (Colour (0x384a95d5));
        g.fillRoundedRectangle (b.expanded (2.5f), corner + 1.0f);

        // Redraw frame (glow may have overdrawn it)
        g.setGradientFill (frame);
        g.fillRoundedRectangle (b, corner);

        // Recessed inner: dark blue fill
        ColourGradient innerFill (Colour (0xff0a1f35), inner.getX(), inner.getY(),
                                  Colour (0xff0d2845), inner.getX(), inner.getBottom(), false);
        g.setGradientFill (innerFill);
        g.fillRoundedRectangle (inner, innerCorner);

        // Neon border inside the recess
        g.setColour (Colour (0xff4a95d5));
        g.drawRoundedRectangle (inner.reduced (0.5f), innerCorner - 0.5f, 1.5f);
    }
    else
    {
        // Recessed inner: very dark, almost black
        ColourGradient innerFill (Colour (0xff16202a), inner.getX(), inner.getY(),
                                  Colour (0xff0e161e), inner.getX(), inner.getBottom(), false);
        g.setGradientFill (innerFill);
        g.fillRoundedRectangle (inner, innerCorner);

        // Subtle inset rim
        g.setColour (Colour (0xff0a1018));
        g.drawRoundedRectangle (inner.reduced (0.5f), innerCorner - 0.5f, 1.0f);
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

    // Thin inset pill track (~12px tall, 40px wide)
    float trackH    = 12.0f;
    float trackW    = 42.0f;
    float trackX    = (bW - trackW) * 0.5f;
    float trackY    = (bH - trackH) * 0.5f;
    float trackCorner = trackH * 0.5f;   // fully rounded pill

    Rectangle<float> track (trackX, trackY, trackW, trackH);

    // Inset track: dark fill + darker stroke to make it look recessed
    g.setColour (Colour (0xff111820));
    g.fillRoundedRectangle (track, trackCorner);
    g.setColour (Colour (0xff090e14));
    g.drawRoundedRectangle (track.reduced (0.5f), trackCorner, 1.0f);

    // Sleek rectangular thumb: wider than tall, small corner radius
    float thumbH   = trackH - 4.0f;               // 8px tall
    float thumbW   = thumbH * 1.5f;               // 12px wide
    float thumbCorner = 2.0f;
    float thumbY   = trackY + (trackH - thumbH) * 0.5f;
    float thumbX   = isRight ? trackX + trackW - thumbW - 3.0f
                              : trackX + 3.0f;

    Rectangle<float> thumb (thumbX, thumbY, thumbW, thumbH);

    if (isRight)
    {
        // Subtle neon glow
        g.setColour (Colour (0x504a95d5));
        g.fillRoundedRectangle (thumb.expanded (2.5f), thumbCorner + 1.5f);
        // Bright metallic-blue thumb
        ColourGradient thumbFill (Colour (0xff6ab0e8), thumbX, thumbY,
                                  Colour (0xff2a6aaa), thumbX, thumbY + thumbH, false);
        g.setGradientFill (thumbFill);
        g.fillRoundedRectangle (thumb, thumbCorner);
    }
    else
    {
        // Inactive: muted grey metallic thumb
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

    momentaryLabel.setBounds (toggleX - 90, toggleY + 5, 86, 16);
    latchLabel.setBounds     (toggleX + toggleW + 4, toggleY + 5, 52, 16);
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

    setSize (714, 700);
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

    // 1 — Dark metallic gradient (lighter at top, darker at bottom)
    ColourGradient bg (Colour (0xff2e3840), 0.0f, 0.0f,
                       Colour (0xff141c24), 0.0f, H, false);
    bg.addColour (0.5, Colour (0xff222c36));
    g.setGradientFill (bg);
    g.fillAll();

    // 2 — Subtle horizontal brushed-metal sheen bands
    ColourGradient sheen (Colour (0x12ffffff), 0.0f, H * 0.05f,
                          Colour (0x00ffffff), 0.0f, H * 0.30f, false);
    g.setGradientFill (sheen);
    g.fillAll();

    // 3 — Vignette: dark edges, lighter centre
    ColourGradient vignette (Colour (0x00000000), W * 0.5f, H * 0.5f,
                             Colour (0x60000000), 0.0f, 0.0f, true);
    g.setGradientFill (vignette);
    g.fillAll();

    // 4 — Engraved TRIBRATO title
    //     Light catch 1 px below, then dark groove on top
    Font titleFont (FontOptions (juce::Font::getDefaultMonospacedFontName(), 28.0f, Font::bold));
    g.setFont (titleFont);

    Rectangle<float> titleArea (0.0f, 12.0f, W, 58.0f);

    // Light catch (shadow below groove — gives raised-letter illusion)
    g.setColour (Colour (0xff4a5e70));
    g.drawText ("TRIBRATO", titleArea.translated (0.0f, 1.0f),
                Justification::centred, false);

    // Dark groove (the engraving itself)
    g.setColour (Colour (0xff0e1820));
    g.drawText ("TRIBRATO", titleArea,
                Justification::centred, false);
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

#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

//==============================================================================
//  Posizioni sulla grafica, in pixel di Resources/panel.jpg (2514 x 283)
//==============================================================================
namespace Layout
{
    // centri delle manopole: EXP GATE, COMP RATIO, ATTACK, RELEASE, INPUT, OUTPUT
    const float knobX[2][6] = { {  427.0f,  665.0f,  788.0f,  908.0f, 1040.0f, 1165.0f },
                                { 1455.0f, 1565.0f, 1668.0f, 1760.0f, 1854.0f, 1968.0f } };
    const float knobY      = 166.0f;
    const float knobBox    = 96.0f;
    const float knobR      = 32.0f;   // raggio della manopola; scale e tacche sono nella grafica

    // finestre dei LED GAIN REDUCTION (da sinistra: 12 6 4 2 1 dB)
    const float ledX[2][5] = { {  249.0f,  296.0f,  342.0f,  388.0f,  435.0f },
                               { 1305.0f, 1344.0f, 1383.0f, 1422.0f, 1461.0f } };
    const float ledY = 48.0f, ledW = 27.0f, ledH = 13.0f;
    const float ledThreshold[5] = { 12.0f, 6.0f, 4.0f, 2.0f, 1.0f };

    // LED quadrati: detect limit e gate
    const Rectangle<float> detectLed[2] { { 194.0f, 45.0f, 25.0f, 23.0f }, { 1259.0f, 45.0f, 24.0f, 23.0f } };
    const Rectangle<float> gateLed[2]   { { 511.0f, 45.0f, 25.0f, 23.0f }, { 1507.0f, 45.0f, 24.0f, 23.0f } };

    // levette
    const Rectangle<float> compSwitch[2] { { 272.0f, 115.0f, 29.0f, 107.0f }, { 1322.0f, 115.0f, 29.0f, 107.0f } };
    const Rectangle<float> linkSwitch    { 2066.0f, 115.0f, 29.0f, 107.0f };
    const Rectangle<float> voiceSwitch   { 530.0f, 120.0f, 29.0f, 100.0f };   // ORIGINAL / MOD

    // tasto POWER / bypass (le immagini sono 51 x 51 in unita' di layout)
    const Rectangle<float> powerButton   { 2342.0f - 25.5f, 66.0f - 25.5f, 51.0f, 51.0f };
    const Rectangle<float> autoGainButton { 2205.0f - 25.5f, 66.0f - 25.5f, 51.0f, 51.0f };
}

static const float kStart = MathConstants<float>::pi * 1.25f;
static const float kEnd   = MathConstants<float>::pi * 2.75f;

//==============================================================================
void DualisLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float startAngle, float endAngle, Slider&)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float s  = b.getWidth() / Layout::knobBox;     // pixel della grafica -> pixel a schermo
    const float r  = Layout::knobR * s;
    const float cx = b.getCentreX(), cy = b.getCentreY();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float sx = std::sin (angle), sy = -std::cos (angle);

    // corpo della manopola: immagine con luce, zigrinatura e ombra (il raggio e' il 40% del lato)
    if (knobImage.isValid())
    {
        const float side = r / 0.40f;
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        g.drawImage (knobImage, Rectangle<float> (cx - side * 0.5f, cy - side * 0.5f, side, side), RectanglePlacement::stretchToFit);
    }
    else
    {
        g.setColour (Colour (0xff1c1c1e));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    }

    // indice dipinto sul cappello, dritto dal centro
    Path bar;
    bar.startNewSubPath (cx + sx * r * 0.12f, cy + sy * r * 0.12f);
    bar.lineTo (cx + sx * r * 0.70f, cy + sy * r * 0.70f);

    g.setColour (Colour (0xff141414).withAlpha (0.8f));
    g.strokePath (bar, PathStrokeType (r * 0.19f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colour (0xffe2dccc));
    g.strokePath (bar, PathStrokeType (r * 0.13f, PathStrokeType::curved, PathStrokeType::rounded));
}

//==============================================================================
void SlideSwitch::paintButton (Graphics& g, bool highlighted, bool)
{
    auto b = getLocalBounds().toFloat();
    const float w = b.getWidth();

    // fessura
    g.setColour (Colour (0xff0d0d0d));
    g.fillRoundedRectangle (b, w * 0.08f);
    g.setColour (Colour (0xff2c2c2b));
    g.drawRoundedRectangle (b.reduced (0.5f), w * 0.08f, 1.0f);

    // levetta: su = spento, giu' = acceso
    const bool on = onIsUp ? ! getToggleState() : getToggleState();   // on = levetta in basso
    auto lever = b.reduced (w * 0.12f, 0.0f).withHeight (b.getHeight() * 0.44f);
    lever.setY (on ? b.getBottom() - lever.getHeight() - w * 0.14f : b.getY() + w * 0.14f);

    ColourGradient cg (Colour (0xff4a4a47), lever.getX(), lever.getY(), Colour (0xff1e1e1c), lever.getX(), lever.getBottom(), false);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (lever, w * 0.06f);

    // righe in rilievo
    for (int i = 0; i < 6; ++i)
    {
        const float yy = lever.getY() + lever.getHeight() * (0.22f + 0.1f * (float) i);
        g.setColour (Colour (0xff8a8a85).withAlpha (0.75f));
        g.drawLine (lever.getX() + w * 0.08f, yy, lever.getRight() - w * 0.08f, yy, 1.2f);
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawLine (lever.getX() + w * 0.08f, yy + 1.4f, lever.getRight() - w * 0.08f, yy + 1.4f, 1.0f);
    }

    if (highlighted)
    {
        g.setColour (Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (lever, w * 0.06f);
    }
}

//==============================================================================
DualisAudioProcessorEditor::DualisAudioProcessorEditor (DualisAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    static const char* ids[numKnobs] = { "gate", "ratio", "attack", "release", "input", "output" };

    for (int c = 0; c < 2; ++c)
    {
        for (int k = 0; k < numKnobs; ++k)
        {
            auto& s = knobs[c][k];
            s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
            s.setRotaryParameters (kStart, kEnd, true);
            s.setPopupDisplayEnabled (true, true, this);
            s.setMouseDragSensitivity (220);
            addAndMakeVisible (s);

            const auto id = DualisAudioProcessor::pid (c, ids[k]);
            knobAtt[c][k] = std::make_unique<SliderAtt> (proc.apvts, id, s);

            if (auto* param = proc.apvts.getParameter (id))
                s.setDoubleClickReturnValue (true, (double) param->convertFrom0to1 (param->getDefaultValue()));
        }

        addAndMakeVisible (compSwitch[c]);
        compAtt[c] = std::make_unique<ButtonAtt> (proc.apvts, DualisAudioProcessor::pid (c, "comp"), compSwitch[c]);
    }

    voiceSwitch.setTooltip ("ORIGINAL / MOD");
    addAndMakeVisible (voiceSwitch);
    voiceAtt = std::make_unique<ButtonAtt> (proc.apvts, "voice", voiceSwitch);

    linkSwitch.onIsUp = true;
    addAndMakeVisible (linkSwitch);
    linkAtt = std::make_unique<ButtonAtt> (proc.apvts, "link", linkSwitch);

    powerButton.imgOn  = ImageCache::getFromMemory (BinaryData::power_on_png,  BinaryData::power_on_pngSize);
    powerButton.imgOff = ImageCache::getFromMemory (BinaryData::power_off_png, BinaryData::power_off_pngSize);
    powerButton.setTooltip ("POWER / BYPASS");
    addAndMakeVisible (powerButton);
    bypassAtt = std::make_unique<ButtonAtt> (proc.apvts, "bypass", powerButton);

    autoGainButton.imgOn  = ImageCache::getFromMemory (BinaryData::autogain_on_png,  BinaryData::autogain_on_pngSize);
    autoGainButton.imgOff = ImageCache::getFromMemory (BinaryData::autogain_off_png, BinaryData::autogain_off_pngSize);
    autoGainButton.litWhenToggled = true;
    autoGainButton.setTooltip ("AUTO GAIN");
    addAndMakeVisible (autoGainButton);
    autoGainAtt = std::make_unique<ButtonAtt> (proc.apvts, "autogain", autoGainButton);

    panel = ImageCache::getFromMemory (BinaryData::panel_jpg, BinaryData::panel_jpgSize);
    lnf.knobImage = ImageCache::getFromMemory (BinaryData::knob_png, BinaryData::knob_pngSize);

    const int w0 = 1600;
    const int h0 = roundToInt ((float) w0 * (float) panelH / (float) panelW);

    setResizable (true, true);
    setResizeLimits (1100, roundToInt (1100.0f * panelH / panelW), panelW, panelH);
    if (auto* cons = getConstrainer())
        cons->setFixedAspectRatio ((double) panelW / (double) panelH);
    setSize (w0, h0);

    startTimerHz (30);
}

DualisAudioProcessorEditor::~DualisAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DualisAudioProcessorEditor::mirrorLinkedControls()
{
    static const char* ids[] = { "comp", "gate", "ratio", "attack", "release", "input", "output" };

    for (auto* id : ids)
    {
        auto* src = proc.apvts.getParameter (DualisAudioProcessor::pid (0, id));
        auto* dst = proc.apvts.getParameter (DualisAudioProcessor::pid (1, id));
        if (src == nullptr || dst == nullptr)
            continue;

        const float v = src->getValue();
        if (std::abs (dst->getValue() - v) > 1.0e-5f)
        {
            dst->beginChangeGesture();
            dst->setValueNotifyingHost (v);
            dst->endChangeGesture();
        }
    }
}

Rectangle<float> DualisAudioProcessorEditor::toScreen (Rectangle<float> r) const
{
    const float s = scale();
    return { r.getX() * s, r.getY() * s, r.getWidth() * s, r.getHeight() * s };
}

//==============================================================================
void DualisAudioProcessorEditor::timerCallback()
{
    const bool linkOn = proc.apvts.getRawParameterValue ("link")->load() > 0.5f;

    // LINK: le manopole e la levetta COMP del CH2 seguono il CH1
    if (linkOn)
        mirrorLinkedControls();

    // CH2 oscurato solo su traccia mono; con LINK si muove insieme al CH1 ma non si tocca
    const bool dim = proc.monoActive.load();
    const bool locked = dim || linkOn;
    if (dim != ch2Dimmed || locked != ch2Locked)
    {
        ch2Dimmed = dim;
        ch2Locked = locked;
        for (auto& kn : knobs[1])
        {
            kn.setEnabled (! locked);
            kn.setAlpha (dim ? 0.35f : 1.0f);
        }
        compSwitch[1].setEnabled (! locked);
        compSwitch[1].setAlpha (dim ? 0.5f : 1.0f);
        repaint();
    }

    for (int c = 0; c < 2; ++c)
    {
        dispGr[c]   = jmax (proc.meterGr[c].load(),   dispGr[c]   * 0.82f);
        dispGate[c] = jmax (proc.meterGate[c].load(), dispGate[c] * 0.82f);

        // ridisegna solo la fila dei LED del canale
        const float x0 = Layout::detectLed[c].getX() - 12.0f;
        const float x1 = Layout::gateLed[c].getRight() + 12.0f;
        repaint (toScreen ({ x0, 30.0f, x1 - x0, 52.0f }).getSmallestIntegerContainer());
    }
}

void DualisAudioProcessorEditor::resized()
{
    for (int c = 0; c < 2; ++c)
    {
        for (int k = 0; k < numKnobs; ++k)
        {
            const float d = Layout::knobBox;
            knobs[c][k].setBounds (toScreen ({ Layout::knobX[c][k] - d * 0.5f, Layout::knobY - d * 0.5f, d, d }).toNearestInt());
        }

        compSwitch[c].setBounds (toScreen (Layout::compSwitch[c]).toNearestInt());
    }

    linkSwitch.setBounds (toScreen (Layout::linkSwitch).toNearestInt());
    voiceSwitch.setBounds (toScreen (Layout::voiceSwitch).toNearestInt());
    powerButton.setBounds (toScreen (Layout::powerButton).toNearestInt());
    autoGainButton.setBounds (toScreen (Layout::autoGainButton).toNearestInt());
}

//==============================================================================
static void drawLit (Graphics& g, Rectangle<float> r, Colour col)
{
    g.setColour (col.withAlpha (0.30f));
    g.fillRoundedRectangle (r.expanded (r.getHeight() * 0.5f), r.getHeight() * 0.6f);

    ColourGradient cg (col.brighter (0.6f), r.getCentreX(), r.getCentreY(), col.darker (0.4f), r.getX(), r.getY(), true);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (r, r.getHeight() * 0.15f);
}

void DualisAudioProcessorEditor::paint (Graphics& g)
{
    g.setImageResamplingQuality (Graphics::highResamplingQuality);

    if (panel.isValid())
        g.drawImage (panel, getLocalBounds().toFloat(), RectanglePlacement::stretchToFit);
    else
        g.fillAll (Colour (0xff1c1c1e));

    // CH2 inattivo: velo scuro sulla sezione
    if (ch2Dimmed)
    {
        g.setColour (Colours::black.withAlpha (0.38f));
        g.fillRect (toScreen ({ 1238.0f, 8.0f, 812.0f, 265.0f }));
    }

    const Colour red   { 0xffff3b2f };
    const Colour green { 0xff3dff6e };

    for (int c = 0; c < 2; ++c)
    {
        for (int j = 0; j < 5; ++j)
            if (dispGr[c] >= Layout::ledThreshold[j])
                drawLit (g, toScreen ({ Layout::ledX[c][j], Layout::ledY, Layout::ledW, Layout::ledH }), red);

        if (dispGr[c] >= 20.0f)
            drawLit (g, toScreen (Layout::detectLed[c].reduced (3.0f)), red);

        if (dispGate[c] >= 1.0f)
            drawLit (g, toScreen (Layout::gateLed[c].reduced (3.0f)), green);
    }
}

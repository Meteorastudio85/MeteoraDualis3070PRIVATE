#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
// Le manopole sono quelle della foto: qui si disegna solo l'indice che ruota
class DualisLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Image knobImage;   // corpo della manopola (Resources/knob.png), l'indice e' disegnato sopra

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
};

// Levetta a slitta verticale (su = OUT / DUAL MONO, giu' = IN / STEREO)
class SlideSwitch : public juce::ToggleButton
{
public:
    SlideSwitch() { setClickingTogglesState (true); }
    bool onIsUp = false;   // LINK: levetta su = LINK, giu' = DUAL MONO
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

// Tasto con LED. POWER: acceso = plugin attivo (toggle = parametro "bypass", quindi invertito).
// AUTO GAIN: acceso = auto gain attivo.
class PowerButton : public juce::ToggleButton
{
public:
    PowerButton() { setClickingTogglesState (true); }
    juce::Image imgOn, imgOff;
    bool litWhenToggled = false;
    void paintButton (juce::Graphics& g, bool, bool) override
    {
        const bool lit = litWhenToggled ? getToggleState() : ! getToggleState();
        const auto& img = lit ? imgOn : imgOff;
        if (img.isValid())
        {
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (img, getLocalBounds().toFloat(), juce::RectanglePlacement::centred);
        }
    }
};

//==============================================================================
class DualisAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit DualisAudioProcessorEditor (DualisAudioProcessor&);
    ~DualisAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // dimensioni della grafica in unita' di layout (Resources/panel.jpg e' a doppia risoluzione)
    static constexpr int panelW = 2514;
    static constexpr int panelH = 283;

private:
    void timerCallback() override;

    enum KnobId { kGate = 0, kRatio, kAttack, kRelease, kInput, kOutput, numKnobs };

    float scale() const { return (float) getWidth() / (float) panelW; }
    juce::Rectangle<float> toScreen (juce::Rectangle<float> r) const;

    DualisAudioProcessor& proc;
    DualisLookAndFeel lnf;

    juce::Slider knobs[2][numKnobs];
    SlideSwitch compSwitch[2];
    SlideSwitch linkSwitch;
    SlideSwitch voiceSwitch;     // su = ORIGINAL, giu' = MOD
    PowerButton powerButton;
    PowerButton autoGainButton;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAtt> knobAtt[2][numKnobs];
    std::unique_ptr<ButtonAtt> compAtt[2], linkAtt, bypassAtt, autoGainAtt, voiceAtt;

    void mirrorLinkedControls();

    juce::Image panel;

    float dispGr[2]   { 0.0f, 0.0f };
    float dispGate[2] { 0.0f, 0.0f };
    bool  ch2Dimmed = false;
    bool  ch2Locked = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DualisAudioProcessorEditor)
};

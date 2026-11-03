#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "DualisModel.h"

class DualisAudioProcessor : public juce::AudioProcessor
{
public:
    DualisAudioProcessor();
    ~DualisAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String pid (int ch, const char* name) { return "c" + juce::String (ch + 1) + "_" + name; }

    juce::AudioProcessorValueTreeState apvts;

    // per i meter dell'interfaccia
    std::atomic<float> meterGr[2]    { 0.0f, 0.0f };
    std::atomic<float> meterGate[2]  { 0.0f, 0.0f };

    // stato mostrato dall'interfaccia
    std::atomic<bool> linkActive { false };   // LINK attivo su una traccia stereo
    std::atomic<bool> monoActive { false };   // traccia mono: lavora solo il CH1

private:
    struct ChannelParams
    {
        std::atomic<float>* comp    = nullptr;
        std::atomic<float>* gate    = nullptr;
        std::atomic<float>* ratio   = nullptr;
        std::atomic<float>* attack  = nullptr;
        std::atomic<float>* release = nullptr;
        std::atomic<float>* input   = nullptr;
        std::atomic<float>* output  = nullptr;
    };

    ChannelParams params[2];
    std::atomic<float>* linkParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* autoGainParam = nullptr;
    std::atomic<float>* voiceParam = nullptr;
    juce::SmoothedValue<float> modMix;     // 0 = ORIGINAL, 1 = MOD

    bool  monoDetected = false;            // L e R uguali (traccia mono passata come stereo)
    double monoTime = 0.0, stereoTime = 0.0; // isteresi del rilevamento mono (secondi)

    // AUTO GAIN: livello RMS lento di ingresso e uscita, guadagno di compensazione
    float agIn[2]   { 0.0f, 0.0f };
    float agOut[2]  { 0.0f, 0.0f };
    float agGain[2] { 1.0f, 1.0f };
    juce::SmoothedValue<float> autoAmt;    // 0 = auto gain spento, 1 = acceso

    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> wetMix;      // 1 = attivo, 0 = bypass (dissolvenza senza click)

    Dualis::Channel chan[2];
    juce::SmoothedValue<float> inGain[2], outGain[2];
    double fs = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DualisAudioProcessor)
};

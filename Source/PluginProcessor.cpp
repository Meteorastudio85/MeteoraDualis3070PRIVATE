#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstring>

using namespace juce;

namespace
{
    String ratioText (float pos)
    {
        const float s = Dualis::ratioSlope (pos);
        if (s >= 0.999f) return String (CharPointer_UTF8 ("\xe2\x88\x9e:1"));
        return String (1.0f / (1.0f - s), 1) + ":1";
    }

    String attackText (float pos)
    {
        const float ms = Dualis::attackMs (pos);
        return String (ms, ms < 1.0f ? 2 : 1) + " ms";
    }

    String releaseText (float pos)
    {
        const float s = Dualis::releaseS (pos);
        return s < 1.0f ? String (roundToInt (s * 1000.0f)) + " ms" : String (s, 2) + " s";
    }

    String dbText (float pos)
    {
        const float db = Dualis::knobToDb (pos);
        return (db > 0.0f ? "+" : "") + String (db, 1) + " dB";
    }

    String gateText (float pos)
    {
        if (pos < 0.05f) return "Off";
        return String (Dualis::gateThreshold (pos), 0) + " dBFS";
    }
}

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout DualisAudioProcessor::createLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    for (int c = 0; c < 2; ++c)
    {
        const String ch = "CH" + String (c + 1) + " ";

        layout.add (std::make_unique<AudioParameterBool> (ParameterID { pid (c, "comp"), 1 }, ch + "Comp", true,
            AudioParameterBoolAttributes().withStringFromValueFunction ([] (bool v, int) { return v ? String ("In") : String ("Out"); })));

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid (c, "gate"), 1 }, ch + "Exp Gate",
            NormalisableRange<float> (0.0f, 10.0f), 0.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return gateText (v); })));

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid (c, "ratio"), 1 }, ch + "Comp Ratio",
            NormalisableRange<float> (0.0f, 5.0f), 3.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return ratioText (v); })));

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid (c, "attack"), 1 }, ch + "Attack",
            NormalisableRange<float> (0.0f, 5.0f), 5.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return attackText (v); })));

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid (c, "release"), 1 }, ch + "Release",
            NormalisableRange<float> (0.0f, 5.0f), Dualis::releaseDefaultPos(),
            AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return releaseText (v); })));

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid (c, "input"), 1 }, ch + "Input",
            NormalisableRange<float> (0.0f, 10.0f), 5.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return dbText (v); })));

        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { pid (c, "output"), 1 }, ch + "Output",
            NormalisableRange<float> (0.0f, 10.0f), 5.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return dbText (v); })));
    }

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 }, "Bypass", false,
        AudioParameterBoolAttributes().withStringFromValueFunction ([] (bool v, int) { return v ? String ("Bypass") : String ("On"); })));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "voice", 1 }, "Original / Mod", true,
        AudioParameterBoolAttributes().withStringFromValueFunction ([] (bool v, int) { return v ? String ("Mod") : String ("Original"); })));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "autogain", 1 }, "Auto Gain", false,
        AudioParameterBoolAttributes().withStringFromValueFunction ([] (bool v, int) { return v ? String ("On") : String ("Off"); })));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "link", 1 }, "Link", false,
        AudioParameterBoolAttributes().withStringFromValueFunction ([] (bool v, int) { return v ? String ("Stereo") : String ("Dual Mono"); })));

    return layout;
}

//==============================================================================
DualisAudioProcessor::DualisAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DUALIS", createLayout())
{
    for (int c = 0; c < 2; ++c)
    {
        params[c].comp    = apvts.getRawParameterValue (pid (c, "comp"));
        params[c].gate    = apvts.getRawParameterValue (pid (c, "gate"));
        params[c].ratio   = apvts.getRawParameterValue (pid (c, "ratio"));
        params[c].attack  = apvts.getRawParameterValue (pid (c, "attack"));
        params[c].release = apvts.getRawParameterValue (pid (c, "release"));
        params[c].input   = apvts.getRawParameterValue (pid (c, "input"));
        params[c].output  = apvts.getRawParameterValue (pid (c, "output"));
    }

    linkParam   = apvts.getRawParameterValue ("link");
    bypassParam = apvts.getRawParameterValue ("bypass");
    autoGainParam = apvts.getRawParameterValue ("autogain");
    voiceParam = apvts.getRawParameterValue ("voice");
}

bool DualisAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void DualisAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;

    dryBuffer.setSize (2, jmax (1, samplesPerBlock));
    wetMix.reset (fs, 0.02);
    wetMix.setCurrentAndTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);
    autoAmt.reset (fs, 0.05);
    autoAmt.setCurrentAndTargetValue (autoGainParam->load() > 0.5f ? 1.0f : 0.0f);
    for (int c = 0; c < 2; ++c) { agIn[c] = agOut[c] = 0.0f; agGain[c] = 1.0f; }
    monoTime = stereoTime = 0.0;
    modMix.reset (fs, 0.03);
    modMix.setCurrentAndTargetValue (voiceParam->load() > 0.5f ? 1.0f : 0.0f);

    for (int c = 0; c < 2; ++c)
    {
        chan[c].prepare (Dualis::cal[c], fs);

        inGain[c].reset (fs, 0.02);
        outGain[c].reset (fs, 0.02);
        inGain[c].setCurrentAndTargetValue (Decibels::decibelsToGain (Dualis::knobToDb (params[c].input->load())));
        outGain[c].setCurrentAndTargetValue (Decibels::decibelsToGain (Dualis::knobToDb (params[c].output->load())));

        meterGr[c] = 0.0f;
        meterGate[c] = 0.0f;
    }
}

//==============================================================================
void DualisAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const int numSamples = buffer.getNumSamples();
    int numCh = jmin (2, numIn, numOut);
    if (numCh == 0 || numSamples == 0)
        return;

    // traccia mono: lavora solo il CH1.
    // Vale anche quando la DAW passa una traccia mono come stereo: se L e R sono uguali
    // (entro -60 dB) e' mono. Nel silenzio (DAW ferma) lo stato resta quello di prima,
    // cosi' l'interfaccia non cambia quando fermi la riproduzione.
    if (numCh == 2)
    {
        const float* l = buffer.getReadPointer (0);
        const float* r = buffer.getReadPointer (1);
        double eSum = 0.0, eDiff = 0.0;
        for (int i = 0; i < numSamples; ++i)
        {
            const double a = (double) l[i], b = (double) r[i];
            eSum  += (a + b) * (a + b);
            eDiff += (a - b) * (a - b);
        }

        // isteresi: per diventare mono servono 50 ms di L = R, per tornare stereo
        // serve almeno 1 s di segnale chiaramente diverso tra L e R. Nel silenzio
        // (o con differenze ambigue) lo stato resta quello di prima.
        const double blockTime = (double) numSamples / fs;
        if (eSum > 1.0e-9 * (double) numSamples)
        {
            const double ratio = eDiff / eSum;
            if (ratio < 1.0e-6)        { monoTime += blockTime;   stereoTime = 0.0; }   // uguali entro -60 dB
            else if (ratio > 1.0e-3)   { stereoTime += blockTime; monoTime = 0.0; }    // diversi oltre -30 dB

            if (! monoDetected && monoTime >= 0.05)   monoDetected = true;
            if (monoDetected && stereoTime >= 1.0)    monoDetected = false;
        }
    }
    else
    {
        monoDetected = false;
    }

    const bool monoSource = numCh == 1 || monoDetected;
    if (monoSource)
        numCh = 1;

    // copia del segnale originale per il bypass morbido
    const int outCh = jmin (2, numOut);
    if (dryBuffer.getNumSamples() < numSamples || dryBuffer.getNumChannels() < outCh)
        dryBuffer.setSize (jmax (2, outCh), numSamples, false, false, true);
    for (int ch = 0; ch < outCh; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, jmin (ch, numIn - 1), 0, numSamples);

    wetMix.setTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);
    autoAmt.setTargetValue (autoGainParam->load() > 0.5f ? 1.0f : 0.0f);
    modMix.setTargetValue (voiceParam->load() > 0.5f ? 1.0f : 0.0f);
    // AUTO GAIN: confronta il livello RMS di ingresso e di uscita (media ~0,8 s)
    const float kAgRms  = 1.0f - std::exp (-1.0f / (0.8f * (float) fs));
    const float kAgGain = 1.0f - std::exp (-1.0f / (0.3f * (float) fs));

    // LINK: in stereo comanda il CH1 (manopole, curva e detector comune)
    const bool link = numCh == 2 && linkParam->load() > 0.5f;
    linkActive.store (link);
    monoActive.store (monoSource);

    const float sr = (float) fs;

    struct Coeffs
    {
        bool  comp = true;
        float slope = 0.0f, threshold = 0.0f, knee = 1.0f;
        float kA = 0.0f, kR = 0.0f;
        bool  gateOn = false;
        float gateThr = -100.0f;
        float inDb = 0.0f, outDb = 0.0f;
    } k[2];

    for (int c = 0; c < 2; ++c)
    {
        const int pc = link ? 0 : c;           // canale da cui leggere manopole e taratura
        const auto& cal = Dualis::cal[pc];
        auto& p = params[pc];

        k[c].comp      = p.comp->load() > 0.5f;
        k[c].slope     = jlimit (0.0f, 1.0f, Dualis::ratioSlope (p.ratio->load()) * cal.slopeScale);
        k[c].threshold = cal.thresholdDb;
        k[c].knee      = jmax (0.1f, cal.kneeDb);

        const float tA = cal.attackTau  * Dualis::attackMs (p.attack->load()) / Dualis::capturedAttackMs;
        const float tR = cal.releaseTau * Dualis::releaseS (p.release->load()) / Dualis::capturedReleaseS;
        k[c].kA = 1.0f - std::exp (-1.0f / (jmax (1.0e-5f, tA) * sr));
        k[c].kR = 1.0f - std::exp (-1.0f / (jmax (1.0e-4f, tR) * sr));

        const float g = p.gate->load();
        k[c].gateOn  = g >= 0.05f;
        k[c].gateThr = Dualis::gateThreshold (g);

        k[c].inDb  = Dualis::knobToDb (p.input->load());
        k[c].outDb = Dualis::knobToDb (p.output->load());

        inGain[c].setTargetValue (Decibels::decibelsToGain (k[c].inDb));
        outGain[c].setTargetValue (Decibels::decibelsToGain (k[c].outDb));
    }

    const float k0       = 1.0f - std::exp (-1.0f / (Dualis::rmsPreTau * sr));
    const float dcR      = 1.0f - (MathConstants<float>::twoPi * 5.0f / sr);
    const float kSmooth  = 1.0f - std::exp (-1.0f / (0.0005f * sr));
    const float gDetRel  = std::exp (-1.0f / (Dualis::gateDetectorRel * sr));
    const float gOpen    = 1.0f - std::exp (-1.0f / (Dualis::gateOpenTau * sr));
    const float gRelStep = Dualis::gateReleaseDbS / sr;
    const int   gHold    = (int) std::round (Dualis::gateHoldS * sr);

    float* data[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    float blockGr[2]   = { 0.0f, 0.0f };
    float blockGate[2] = { 0.0f, 0.0f };

    // detector RMS (pre-media + carica/scarica asimmetriche) -> riduzione in dB
    auto compDetector = [&] (Dualis::Channel& s, const Coeffs& q, float sq)
    {
        s.ms0 += k0 * (sq - s.ms0);
        s.ms  += (s.ms0 > s.ms ? q.kA : q.kR) * (s.ms0 - s.ms);
        if (! q.comp)
            return 0.0f;
        const float levelDb = 10.0f * std::log10 (2.0f * s.ms + 1.0e-20f);
        return Dualis::gainReduction (levelDb, q.threshold, q.slope, q.knee);
    };

    // EXP GATE: detector di picco -> curva misurata -> apertura / hold / chiusura
    auto gateDetector = [&] (Dualis::Channel& s, const Coeffs& q, float a)
    {
        s.gatePeak = a > s.gatePeak ? a : s.gatePeak * gDetRel;

        if (! q.gateOn)
        {
            s.gateAtt = 0.0f;
            s.gateHold = 0;
            return 0.0f;
        }

        const float gl = 20.0f * std::log10 (s.gatePeak + 1.0e-20f);
        const float target = Dualis::gateAttenuation (gl, q.gateThr);

        if (target <= s.gateAtt)
        {
            s.gateAtt += gOpen * (target - s.gateAtt);
            s.gateHold = gHold;
        }
        else if (s.gateHold > 0)
        {
            --s.gateHold;
        }
        else
        {
            s.gateAtt = jmin (target, s.gateAtt + gRelStep);
        }

        return s.gateAtt;
    };

    for (int i = 0; i < numSamples; ++i)
    {
        float x[2]  = { 0.0f, 0.0f };
        float gr[2] = { 0.0f, 0.0f };
        float ga[2] = { 0.0f, 0.0f };

        // INPUT + colore dello stadio d'ingresso (ogni lato ha il suo colore)
        float raw[2] = { 0.0f, 0.0f };
        for (int c = 0; c < numCh; ++c)
        {
            raw[c] = data[c][i];
            x[c] = chan[c].saturate (raw[c] * inGain[c].getNextValue(), dcR);
        }

        if (link)
        {
            // un solo detector, comandato dal CH1, che ascolta entrambi i lati
            const float sq = jmax (x[0] * x[0], x[1] * x[1]);
            const float pk = jmax (std::abs (x[0]), std::abs (x[1]));
            gr[0] = gr[1] = compDetector (chan[0], k[0], sq);
            ga[0] = ga[1] = gateDetector (chan[0], k[0], pk);
        }
        else
        {
            for (int c = 0; c < numCh; ++c)
            {
                gr[c] = compDetector (chan[c], k[c], x[c] * x[c]);
                ga[c] = gateDetector (chan[c], k[c], std::abs (x[c]));
            }
        }

        const float amt = autoAmt.getNextValue();
        const float mod = modMix.getNextValue();

        for (int c = 0; c < numCh; ++c)
        {
            auto& s = chan[c];
            s.grSmooth += kSmooth * ((gr[c] + ga[c]) - s.grSmooth);

            const float v = x[c] * Decibels::decibelsToGain (-s.grSmooth, -200.0f);

            // ORIGINAL: come misurato sul 3070
            const float yOrig = s.shelfOriginal (v);

            // MOD: armoniche del VCA che crescono con la compressione, +1,8 dB a 70 Hz, acuti ammorbiditi
            const float yMod = s.shelf (s.lowBoost (s.vcaColour (v, gr[c], dcR)));

            float y = yOrig + (yMod - yOrig) * mod;   // dissolvenza di 30 ms tra le due versioni
            data[c][i] = y * outGain[c].getNextValue();

            blockGr[c]   = jmax (blockGr[c], gr[c]);
            blockGate[c] = jmax (blockGate[c], ga[c]);
        }

        // AUTO GAIN: l'uscita esce con lo stesso volume (RMS) dell'ingresso.
        // Con LINK si usa un solo guadagno per i due lati, cosi' l'immagine non si sposta.
        for (int c = 0; c < numCh; ++c)
        {
            agIn[c]  += kAgRms * (raw[c] * raw[c] - agIn[c]);
            agOut[c] += kAgRms * (data[c][i] * data[c][i] - agOut[c]);
        }

        for (int c = 0; c < numCh; ++c)
        {
            const float eIn  = link ? agIn[0]  + agIn[1]  : agIn[c];
            const float eOut = link ? agOut[0] + agOut[1] : agOut[c];

            // nel silenzio (sotto circa -70 dBFS) il guadagno resta fermo
            if (eIn > 1.0e-7f && eOut > 1.0e-12f)
            {
                const float target = jlimit (0.0631f, 15.85f, std::sqrt (eIn / eOut));   // +-24 dB
                agGain[c] += kAgGain * (target - agGain[c]);
            }

            const float g = 1.0f + (agGain[c] - 1.0f) * amt;
            data[c][i] *= g;
        }
    }

    // traccia mono passata come stereo: anche il lato destro riceve il CH1
    if (monoSource && getTotalNumOutputChannels() > 1 && buffer.getNumChannels() > 1)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    // bypass: dissolvenza di 20 ms tra elaborato e originale
    const bool fullyOn = ! wetMix.isSmoothing() && wetMix.getTargetValue() > 0.5f;
    if (! fullyOn)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float w = wetMix.getNextValue();
            for (int ch = 0; ch < outCh; ++ch)
            {
                float* o = buffer.getWritePointer (ch);
                o[i] = o[i] * w + dryBuffer.getSample (ch, i) * (1.0f - w);
            }
        }
    }

    const bool bypassed = wetMix.getTargetValue() < 0.5f;

    for (int c = 0; c < 2; ++c)
    {
        const bool active = ! bypassed && (c < numCh || (link && c == 1));
        meterGr[c].store (active ? blockGr[c] : 0.0f);
        meterGate[c].store (active ? blockGate[c] : 0.0f);
    }
}

//==============================================================================
void DualisAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DualisAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessorEditor* DualisAudioProcessor::createEditor()
{
    return new DualisAudioProcessorEditor (*this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DualisAudioProcessor();
}

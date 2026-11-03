#pragma once

#include <cmath>
#include <algorithm>

// =============================================================================
//  Modello del Fostex 3070, calibrato sulle catture Scriba Sweep (novembre 2026)
//
//  Tutti i numeri misurati stanno qui: se rifai una cattura basta aggiornarli.
// =============================================================================
namespace Dualis
{
    struct ChannelCal
    {
        float thresholdDb;  // soglia misurata (dBFS) con INPUT a 0 dB
        float kneeDb;       // ginocchio misurato
        float slopeScale;   // pendenza reale / pendenza nominale della manopola RATIO
        float attackTau;    // costante del detector RMS (s) alla posizione catturata
        float releaseTau;   // costante del detector RMS (s) alla posizione catturata
        float shelfZeroHz;  // shelving acuti del 3070 (1o ordine, adattato in digitale a 48 kHz): zero
        float shelfPoleHz;  //                                        polo  (~ +7 dB)
        float a2;           // 2a armonica (misurata a -18 dBFS, al netto della scheda)
        float a3;           // 3a armonica
        float lowHz;        // piccolo boost sui bassi (campana)
        float lowDb;
        float lowQ;
        float origZeroHz;   // shelving ORIGINAL, come misurato sul 3070
        float origPoleHz;
    };

    //  CH1: soglia -23 dBFS, 6:1 effettivo con manopola su 8, knee 6 dB, H2 -59.8 / H3 -63.6 dB
    //  CH2: soglia -21 dBFS, 8:1 effettivo con manopola su 8, knee 4 dB, H2 -72.3 / H3 -80.6 dB
    //  Tempi: modello RMS adattato agli inviluppi misurati (errore 0,26 / 0,30 dB RMS)
    //
    //  Acuti: lo shelving misurato era +6,4 dB a 15 kHz (zero 3455 Hz / polo 7082 Hz).
    //  MOD (su richiesta): +0,7 dB a 5 kHz, +1,1 dB a 10 kHz, +1,2 dB a 15 kHz
    //  e una campana di +2,7 dB a 70 Hz (CH1) / 74 Hz (CH2).
    //  La levetta ORIGINAL / MOD sceglie tra il suono misurato (ORIGINAL: shelving originale,
    //  niente campana, niente colore del VCA) e questa versione modificata (MOD).
    inline constexpr ChannelCal cal[2] =
    {
        { -23.0f, 6.0f, 0.9524f, 0.0711f, 0.0344f, 4172.0f, 4783.0f, 0.01616f, 0.1668f, 70.0f, 2.7f, 0.75f, 3455.0f, 7082.0f },
        { -21.0f, 4.0f, 1.0000f, 0.0565f, 0.0365f, 4172.0f, 4783.0f, 0.00386f, 0.0235f, 74.0f, 2.7f, 0.75f, 3509.0f, 7057.0f }
    };

    // Colore del VCA: un filo di 2a e 3a armonica che cresce con la compressione,
    // come nei VCA analogici. Da' corpo e profondita' quando il compressore lavora.
    constexpr float vcaH2PerDb  = 0.0060f;   // circa -50 dB di 2a armonica con 10 dB di riduzione a -20 dBFS
    constexpr float vcaH3PerDb  = 0.0250f;
    constexpr float vcaMaxGrDb  = 20.0f;

    // posizione delle manopole durante la cattura DYN (lette dalle foto)
    constexpr float capturedAttackMs  = 20.0f;
    constexpr float capturedReleaseS  = 0.07f;

    // pre-media del detector RMS (filtra il ripple sulle basse)
    constexpr float rmsPreTau = 0.008f;

    // ------------------------------------------------------------------------
    //  EXP GATE, dalle catture DYN con la manopola su 7 e su 4 (2 novembre 2026)
    //
    //  Sopra soglia il gate e' aperto; sotto c'e' un expander morbido per ~12 dB
    //  e poi il gate si chiude di colpo con un range di ~28 dB.
    //  Tempi misurati: apertura ~6 ms, hold ~200 ms, chiusura lineare ~25 dB/s.
    // ------------------------------------------------------------------------
    constexpr int   gateTablePoints = 11;
    // livello rispetto alla soglia (dB) -> attenuazione (dB)
    inline constexpr float gateTableX[gateTablePoints] = {   2.0f,  0.0f, -2.0f, -4.0f, -6.0f, -8.0f, -10.0f, -12.0f, -14.0f, -16.0f, -18.0f };
    inline constexpr float gateTableA[gateTablePoints] = {   0.0f,  0.1f,  0.9f,  2.3f,  3.5f,  4.5f,   5.3f,   8.1f,  22.9f,  26.8f,  28.0f };

    constexpr float gateThresholdAt7 = -38.0f;   // dBFS (INPUT a 0 dB), dove il gate e' tutto aperto
    constexpr float gateDbPerStep    = 22.0f / 3.0f; // misurato: su 4 la soglia e' a -60 dBFS, su 7 a -38
    constexpr float gateOpenTau      = 0.006f;   // s
    constexpr float gateHoldS        = 0.200f;   // s
    constexpr float gateReleaseDbS   = 25.0f;    // dB/s
    constexpr float gateDetectorRel  = 0.015f;   // s, rilascio del detector di picco

    inline float gateThreshold (float knob) noexcept   { return gateThresholdAt7 + (knob - 7.0f) * gateDbPerStep; }

    inline float gateAttenuation (float levelDb, float thresholdDb) noexcept
    {
        const float x = levelDb - thresholdDb;
        if (x >= gateTableX[0]) return 0.0f;
        if (x <= gateTableX[gateTablePoints - 1]) return gateTableA[gateTablePoints - 1];

        int i = 0;
        while (i < gateTablePoints - 2 && x < gateTableX[i + 1])
            ++i;

        const float f = (gateTableX[i] - x) / (gateTableX[i] - gateTableX[i + 1]);
        return gateTableA[i] + f * (gateTableA[i + 1] - gateTableA[i]);
    }

    // ------------------------------------------------------------------------
    //  Scale delle manopole, identiche alla serigrafia del pannello
    // ------------------------------------------------------------------------
    constexpr int numMarks = 6;
    inline constexpr float ratioSlopeMarks[numMarks] = { 0.0f, 0.5f, 0.75f, 0.875f, 0.95f, 1.0f }; // 1 2 4 8 20 inf
    inline constexpr float attackMarksMs [numMarks] = { 0.2f, 1.0f, 3.0f, 5.0f, 10.0f, 20.0f };
    inline constexpr float releaseMarksS [numMarks] = { 0.05f, 0.1f, 0.3f, 0.5f, 1.0f, 2.0f };

    inline float interpLinear (const float* m, float pos) noexcept
    {
        pos = std::clamp (pos, 0.0f, (float) (numMarks - 1));
        const int i = std::min ((int) pos, numMarks - 2);
        const float f = pos - (float) i;
        return m[i] + f * (m[i + 1] - m[i]);
    }

    inline float interpLog (const float* m, float pos) noexcept
    {
        pos = std::clamp (pos, 0.0f, (float) (numMarks - 1));
        const int i = std::min ((int) pos, numMarks - 2);
        const float f = pos - (float) i;
        return m[i] * std::pow (m[i + 1] / m[i], f);
    }

    inline float ratioSlope (float pos) noexcept       { return interpLinear (ratioSlopeMarks, pos); }
    inline float attackMs (float pos) noexcept         { return interpLog (attackMarksMs, pos); }
    inline float releaseS (float pos) noexcept         { return interpLog (releaseMarksS, pos); }
    inline float knobToDb (float pos) noexcept         { return (pos - 5.0f) * 4.0f; }   // 0..10, 5 = 0 dB

    // posizione di default della manopola RELEASE (0,07 s tra .05 e .1)
    inline float releaseDefaultPos() noexcept
    {
        return std::log (capturedReleaseS / releaseMarksS[0]) / std::log (releaseMarksS[1] / releaseMarksS[0]);
    }

    // curva statica con ginocchio morbido, restituisce la riduzione in dB (>= 0)
    inline float gainReduction (float levelDb, float thresholdDb, float slope, float kneeDb) noexcept
    {
        const float o = levelDb - thresholdDb;
        if (2.0f * o <= -kneeDb) return 0.0f;
        if (2.0f * o >= kneeDb)  return slope * o;
        const float t = o + 0.5f * kneeDb;
        return slope * t * t / (2.0f * kneeDb);
    }

    // ------------------------------------------------------------------------
    //  Stato di un canale
    // ------------------------------------------------------------------------
    struct Channel
    {
        // saturazione
        float a2 = 0.0f, a3 = 0.0f, xm = 10.0f, ym = 10.0f;
        float dcX = 0.0f, dcY = 0.0f;

        // detector RMS
        float ms0 = 0.0f, ms = 0.0f;

        // expander/gate
        float gatePeak = 0.0f;
        float gateAtt  = 0.0f;   // attenuazione attuale (dB)
        int   gateHold = 0;      // campioni di hold rimasti

        // guadagno (dB di riduzione) livellato
        float grSmooth = 0.0f;

        // shelving acuti
        float b0 = 1.0f, b1 = 0.0f, a1 = 0.0f, sx = 0.0f, sy = 0.0f;

        // shelving ORIGINAL (misurato)
        float ob0 = 1.0f, ob1 = 0.0f, oa1 = 0.0f, osx = 0.0f, osy = 0.0f;

        // campana sui bassi (biquad)
        float lb0 = 1.0f, lb1 = 0.0f, lb2 = 0.0f, la1 = 0.0f, la2 = 0.0f;
        float lz1 = 0.0f, lz2 = 0.0f;

        // blocco della continua dopo il VCA
        float dc2X = 0.0f, dc2Y = 0.0f;

        void prepare (const ChannelCal& c, double fs)
        {
            a2 = c.a2;
            a3 = c.a3;
            xm = a3 > 1.0e-6f ? 1.0f / std::sqrt (3.0f * a3) : 10.0f;
            ym = xm + a2 * xm * xm - a3 * xm * xm * xm;

            // shelf di 1o ordine (bilineare con prewarp): H(s) = (1 + s/wz) / (1 + s/wp)
            const double k  = 2.0 * fs;
            const double pi = 3.14159265358979323846;
            auto shelfCoeffs = [&] (float zHz, float pHz, float& cb0, float& cb1, float& ca1)
            {
                const double wz = k * std::tan (pi * std::min ((double) zHz, 0.45 * fs) / fs);
                const double wp = k * std::tan (pi * std::min ((double) pHz, 0.47 * fs) / fs);
                const double nb0 = 1.0 + k / wz, nb1 = 1.0 - k / wz;
                const double na0 = 1.0 + k / wp, na1 = 1.0 - k / wp;
                cb0 = (float) (nb0 / na0);
                cb1 = (float) (nb1 / na0);
                ca1 = (float) (na1 / na0);
            };
            shelfCoeffs (c.shelfZeroHz, c.shelfPoleHz, b0, b1, a1);
            shelfCoeffs (c.origZeroHz,  c.origPoleHz,  ob0, ob1, oa1);

            // campana RBJ
            {
                const double A  = std::pow (10.0, (double) c.lowDb / 40.0);
                const double w0 = 2.0 * pi * (double) c.lowHz / fs;
                const double al = std::sin (w0) / (2.0 * (double) c.lowQ);
                const double cs = std::cos (w0);
                const double a0 = 1.0 + al / A;
                lb0 = (float) ((1.0 + al * A) / a0);
                lb1 = (float) ((-2.0 * cs) / a0);
                lb2 = (float) ((1.0 - al * A) / a0);
                la1 = (float) ((-2.0 * cs) / a0);
                la2 = (float) ((1.0 - al / A) / a0);
            }

            reset();
        }

        void reset()
        {
            dcX = dcY = 0.0f;
            ms0 = ms = 1.0e-9f;
            gatePeak = 0.0f;
            gateAtt  = 0.0f;
            gateHold = 0;
            grSmooth = 0.0f;
            sx = sy = 0.0f;
            osx = osy = 0.0f;
            lz1 = lz2 = 0.0f;
            dc2X = dc2Y = 0.0f;
        }

        // stadio d'ingresso: 2a e 3a armonica + blocco della continua
        inline float saturate (float x, float dcR) noexcept
        {
            float y;
            if (x > xm)       y = ym;
            else if (x < -xm) y = -xm + a2 * xm * xm + a3 * xm * xm * xm;
            else              y = x + a2 * x * x - a3 * x * x * x;

            const float o = y - dcX + dcR * dcY;
            dcX = y;
            dcY = o;
            return o;
        }

        // colore del VCA proporzionale alla riduzione (dB) + blocco della continua
        inline float vcaColour (float x, float grDb, float dcR) noexcept
        {
            const float g = std::clamp (grDb, 0.0f, vcaMaxGrDb);
            const float k2 = vcaH2PerDb * g, k3 = vcaH3PerDb * g;
            const float xc = std::clamp (x, -1.5f, 1.5f);
            const float y = x + k2 * xc * xc - k3 * xc * xc * xc;
            const float o = y - dc2X + dcR * dc2Y;
            dc2X = y;
            dc2Y = o;
            return o;
        }

        inline float lowBoost (float x) noexcept
        {
            const float y = lb0 * x + lz1;
            lz1 = lb1 * x - la1 * y + lz2;
            lz2 = lb2 * x - la2 * y;
            return y;
        }

        inline float shelfOriginal (float x) noexcept
        {
            const float y = ob0 * x + ob1 * osx - oa1 * osy;
            osx = x;
            osy = y;
            return y;
        }

        inline float shelf (float x) noexcept
        {
            const float y = b0 * x + b1 * sx - a1 * sy;
            sx = x;
            sy = y;
            return y;
        }
    };
}

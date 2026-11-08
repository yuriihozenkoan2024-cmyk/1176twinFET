// TwinFET — FET limiting amplifier engine (framework independent, header only)
//
//  Two revisions:  BLACK (low-noise "LN" generation)  and  BLUE (early, FET-preamp generation)
//  Three modes:
//    0  FAT     — linear (feed-forward, deterministic) compression + density/"inflation" stage
//    1  CLEAN   — linear, accurate compression. Black: no colouration at all.
//                 Blue: high-grade saturation (transformers + class-A stage, oversampled)
//    2  ANALOG  — circuit-informed nonlinear model: feedback sidechain taken after the FET,
//                 rectifier/hold-cap ripple, program dependent release, FET distortion,
//                 preamp, class-A line amp, input/output transformers, all-buttons behaviour.
//
//  Levels: 1.0 = 0 dBFS, a sine with -18 dBFS peak = 0 VU.
#pragma once

#include "DspUtil.h"
#include "Halfband.h"

#include <array>
#include <cstring>
#include <vector>

namespace twinfet
{

enum Mode     { modeFat = 0, modeClean = 1, modeAnalog = 2 };
enum Revision { revBlack = 0, revBlue = 1 };
enum Ratio    { ratio4 = 0, ratio8, ratio12, ratio20, ratioAll };

struct Params
{
    float inputDb  = 0.0f;   // drive into the fixed threshold
    float outputDb = 0.0f;   // make-up (in ANALOG it also drives the output amplifier)
    float attack   = 3.0f;   // 1..7, 7 = fastest (20 us), 1 = slowest (800 us)
    float release  = 5.0f;   // 1..7, 7 = fastest (50 ms), 1 = slowest (1.1 s)
    int   ratio    = ratio4;
    int   mode     = modeAnalog;
    int   revision = revBlack;
    float mix      = 1.0f;   // 0..1 dry/wet, phase-aligned
    float scHpfHz  = 0.0f;   // sidechain high-pass, <= 20 Hz means off
    bool  link     = true;   // stereo link
    bool  compOff  = false;  // "attack off": colour only, no gain reduction
    bool  noise    = false;  // analog noise floor + hum (ANALOG mode only)
};

//==============================================================================
/** All voicing constants live here so the sound can be tuned in one place. */
struct Voicing
{
    // ---- static curve (per ratio button): threshold dBFS, ratio, knee width dB
    // spacing between the ratios from bench measurements of a 1176LN (onset of compression vs. ratio)
    static constexpr double thresholdDb[5] = { -18.0, -13.6, -12.0, -9.8, -20.0 };
    static constexpr double ratioValue[5]  = {   4.0,   8.0,  12.0,  20.0,  16.0 };
    static constexpr double kneeDb[5]      = {  12.0,   9.0,   7.0,   6.0,   4.0 };

    // ---- ALL buttons
    static constexpr double allOverStartDb = 8.0;   // over-compression begins this far above threshold
    static constexpr double allOverAmount  = 0.16;  // extra dB of GR per dB (FAT/CLEAN)
    static constexpr double allLagAmount   = 4.0;   // attack is (1+lag)x slower from rest (ANALOG, FAT)
    static constexpr double allDriftAmount = 0.45;  // ANALOG: GR grows by this fraction of the slow state

    // ---- detector holds (feed-forward modes)
    static constexpr double holdMsFat   = 5.0;
    static constexpr double holdMsClean = 20.0;

    // ---- analog calibration (time constants vs. the published 20us..800us / 50ms..1.1s)
    static constexpr double analogAttackCal  = 0.60;
    static constexpr double analogReleaseCal = 0.50;
    static constexpr double ffReleaseCal     = 0.75;

    // ---- level of the signal at the FET that equals the 4:1 threshold
    static constexpr double fetRefLevel = 0.125893; // -18 dBFS
};

//==============================================================================
class Engine
{
public:
    //==========================================================================
    void prepare (double sampleRate, int maxBlockSize, int numChannels)
    {
        fs        = sampleRate;
        nCh       = std::max (1, std::min (2, numChannels));
        osFactor  = oversamplingFactorFor (osChoice, fs);
        fsOs      = fs * osFactor;
        (void) maxBlockSize;

        for (auto& o : os)
            o.prepare (fs, osFactor, chunk);
        for (auto& b : osBuf)
            b.assign ((size_t) (chunk * osFactor), 0.0f);

        smIn.setTime (0.02, fsOs);
        smOut.setTime (0.02, fsOs);
        smMix.setTime (0.02, fsOs);

        fadeLen = std::max (1, (int) (0.030 * fsOs));
        latency = computeLatency();

        configured = false;
        reset();
        setParams (params);
    }

    void reset()
    {
        if (fading)                 // finish a pending topology change instantly
        {
            active = 1 - active;
            fading = false;
        }
        for (auto& o : os) o.reset();
        for (auto& p : paths) resetPath (p);
        smIn.reset ((float) dbToGain (params.inputDb));
        smOut.reset ((float) dbToGain (params.outputDb));
        smMix.reset (params.mix);
        fading = false;
        fadePos = 0;
        grMeter = 0.0f;
    }

    int getLatencySamples() const noexcept { return latency; }
    int getOversamplingFactor() const noexcept { return osFactor; }

    /** 0 = auto (4x up to 50 kHz, 2x up to 100 kHz, then 1x), 1..5 = 1x, 2x, 4x, 8x, 16x.
        Takes effect at the next prepare() (it reallocates, so not on the audio thread). */
    void setOversamplingChoice (int choice) noexcept { osChoice = std::max (0, std::min (5, choice)); }
    int getOversamplingChoice() const noexcept { return osChoice; }

    static int oversamplingFactorFor (int choice, double sampleRate) noexcept
    {
        if (choice <= 0)
            return sampleRate <= 50000.0 ? 4 : (sampleRate <= 100000.0 ? 2 : 1);
        return 1 << (std::min (5, choice) - 1);
    }

    void setParams (const Params& p)
    {
        params = p;
        smIn.target  = (float) dbToGain (p.inputDb);
        smOut.target = (float) dbToGain (p.outputDb);
        smMix.target = std::min (1.0f, std::max (0.0f, p.mix));

        if (! configured)
        {
            buildConfig (p, paths[0].cfg);
            applyConfig (paths[0]);
            active = 0;
            configured = true;
            smIn.reset (smIn.target);
            smOut.reset (smOut.target);
            smMix.reset (smMix.target);
            return;
        }

        Config next;
        buildConfig (p, next);

        if (fading)
        {
            // keep the running fade intact; continuous settings go to both paths,
            // a pending topology change is picked up after the fade has finished
            updateContinuous (paths[(size_t) active], p);
            updateContinuous (paths[(size_t) (1 - active)], p);
            return;
        }

        Path& cur = paths[(size_t) active];
        if (needsCrossfade (cur.cfg, next))
        {
            Path& nxt = paths[(size_t) (1 - active)];
            nxt = cur;                       // copy every state -> continuity where possible
            const int oldMode = cur.cfg.mode;
            nxt.cfg = next;
            applyConfig (nxt);
            if (oldMode != next.mode)
                convertDetector (nxt, oldMode, next.mode);
            if (next.compOff)
            {
                // no gain reduction from here on: start the detectors from rest when it comes back
                nxt.ffLink.reset();
                nxt.anLink.reset();
                for (auto& ch : nxt.ch) { ch.ff.reset(); ch.an.reset(); }
            }
            fading  = true;
            fadePos = 0;
        }
        else
        {
            updateContinuous (cur, p);
        }
    }

    /** In-place processing of up to two channels. */
    void process (float* const* channels, int numChannels, int numSamples) noexcept
    {
        const int nc = std::max (1, std::min (nCh, numChannels));
        float grSum = 0.0f;
        int grCount = 0;
        double inAbs = 0.0, outAbs = 0.0;

        for (int start = 0; start < numSamples; start += chunk)
        {
            const int len   = std::min (chunk, numSamples - start);
            const int lenOs = len * osFactor;

            for (int c = 0; c < nc; ++c)
            {
                const float* src = channels[c] + start;
                for (int i = 0; i < len; ++i) inAbs += std::fabs (src[i]);
                os[(size_t) c].upsample (src, osBuf[(size_t) c].data(), len);
            }

            float* b0 = osBuf[0].data();
            float* b1 = nc > 1 ? osBuf[1].data() : nullptr;

            for (int i = 0; i < lenOs; ++i)
            {
                const float gIn  = smIn.next();
                const float gOut = smOut.next();
                const float mix  = smMix.next();

                const float x0 = b0[i];
                const float x1 = b1 != nullptr ? b1[i] : 0.0f;
                float y0 = 0.0f, y1 = 0.0f;

                if (! fading)
                {
                    processFrame (paths[(size_t) active], x0, x1, nc, gIn, gOut, y0, y1);
                }
                else
                {
                    float a0, a1, n0, n1;
                    processFrame (paths[(size_t) active], x0, x1, nc, gIn, gOut, a0, a1);
                    processFrame (paths[(size_t) (1 - active)], x0, x1, nc, gIn, gOut, n0, n1);
                    const float w = (float) fadePos / (float) fadeLen;
                    y0 = a0 + (n0 - a0) * w;
                    y1 = a1 + (n1 - a1) * w;
                    if (++fadePos >= fadeLen)
                    {
                        fading = false;
                        active = 1 - active;
                    }
                }

                b0[i] = x0 + (y0 - x0) * mix;
                if (b1 != nullptr) b1[i] = x1 + (y1 - x1) * mix;

                if ((i & 7) == 0)
                {
                    grSum += pathGrDb (paths[(size_t) active]);
                    ++grCount;
                }
                if (grTrace != nullptr && (i % osFactor) == osFactor - 1)
                    grTrace->push_back (pathGrDb (paths[(size_t) active]));
                if (grTraceOs != nullptr)
                    grTraceOs->push_back (pathGrDb (paths[(size_t) active]));
            }

            for (int c = 0; c < nc; ++c)
            {
                float* dst = channels[c] + start;
                os[(size_t) c].downsample (osBuf[(size_t) c].data(), dst, len);
                for (int i = 0; i < len; ++i) outAbs += std::fabs (dst[i]);
            }
        }

        if (grCount > 0) grMeter = grSum / (float) grCount;
        const double n = (double) std::max (1, numSamples * nc);
        inMeanAbs  = (float) (inAbs / n);
        outMeanAbs = (float) (outAbs / n);
    }

    // ---- meters (read after process) -----------------------------------------
    float getGainReductionDb() const noexcept { return grMeter; }
    float getInputMeanAbs() const noexcept    { return inMeanAbs; }
    float getOutputMeanAbs() const noexcept   { return outMeanAbs; }

    /** Test hooks: if set, the GR (dB) of the active path is appended once per
        base-rate sample / once per oversampled sample. Leave null in the plugin. */
    std::vector<float>* grTrace = nullptr;
    std::vector<float>* grTraceOs = nullptr;

private:
    //==========================================================================
    struct Config
    {
        int mode = modeAnalog, revision = revBlack, ratio = ratio4;
        bool compOff = false, link = true, noise = false;

        // static curve
        double T = -18.0, R = 4.0, W = 12.0, Wfb = 4.8;
        float ffLowLin = 0.1f;     // below this peak level the FF target is zero
        double anKneeLin = 0.1;    // ANALOG: FET output level where the knee starts (linear)
        bool all = false;

        // ballistics
        double aAttack = 0.1;      // FF: one-pole coefficient (dB domain)
        double aImplicit = 0.1;    // ANALOG: dt / tau for implicit attack
        double relMul = 0.999;     // release multiplier per sample
        double aTrans = 0.0001;    // slow-state charge
        double slowMul = 0.9999;   // slow-state discharge multiplier
        double ks = 0.5;           // program dependent release depth
        int holdSub = 0;           // FF hold sub-block length in samples (8 sub-blocks)

        // sidechain HPF
        bool scHpf = false;
        double scHpfHz = 0.0;
    };

    struct FFDet
    {
        double G1 = 0.0, G2 = 0.0;
        float ring[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        float ringMax = 0.0f, cur = 0.0f;
        int ringPos = 0, cnt = 0;

        void reset() { *this = FFDet(); }
    };

    struct AnaDet
    {
        double V = 0.0, S = 0.0;   // hold capacitor and slow "memory" capacitor (control volts, normalised)
        void reset() { V = S = 0.0; }
    };

    struct Chan
    {
        FFDet ff;
        AnaDet an;
        Biquad scHpf;

        Transformer tIn, tOut;
        Biquad hpIn, lpIn, hpOut, lpOut, shelfLo, shelfHi;
        DcBlock dc1, dc2, dc3;

        void reset()
        {
            ff.reset(); an.reset(); scHpf.reset();
            tIn.reset(); tOut.reset();
            hpIn.reset(); lpIn.reset(); hpOut.reset(); lpOut.reset(); shelfLo.reset(); shelfHi.reset();
            dc1.reset(); dc2.reset(); dc3.reset();
        }
    };

    struct Colour
    {
        // FAT
        double fatRef = 0.25, fatInvRef = 4.0, fatE0 = 0.2, fatE1 = 0.3, fatE2 = 0.04;
        bool fatStage = false;
        ClassAStage fatAmp;
        // CLEAN (blue)
        ClassAStage premium;
        // ANALOG
        double fetD2 = 0.006, fetD3 = 0.003;
        ClassAStage preAmp, lineAmp;
        double noiseRms = 0.0, humAmp = 0.0;
    };

    struct Path
    {
        Config cfg;
        Colour col;
        std::array<Chan, 2> ch;
        FFDet ffLink;
        AnaDet anLink;
        Rng rng;
        double humC = 1.0, humS = 0.0, humRc = 1.0, humRs = 0.0;
        float gr = 0.0f;      // FAT / CLEAN: gain reduction in dB
        double grV = 0.0;     // ANALOG: control voltage, GR = 20 log10 (1 + V)
    };

    static float pathGrDb (const Path& p) noexcept
    {
        return p.cfg.mode == modeAnalog ? (float) gainToDb (1.0 + p.grV) : p.gr;
    }

    //==========================================================================
    static double attackSeconds (double knob)  // 1..7 -> 800us..20us (geometric)
    {
        knob = std::min (7.0, std::max (1.0, knob));
        return 800.0e-6 * std::pow (20.0 / 800.0, (knob - 1.0) / 6.0);
    }

    static double releaseSeconds (double knob) // 1..7 -> 1.1s..50ms (geometric)
    {
        knob = std::min (7.0, std::max (1.0, knob));
        return 1.1 * std::pow (0.05 / 1.1, (knob - 1.0) / 6.0);
    }

    void buildConfig (const Params& p, Config& c) const
    {
        c.mode     = std::min (2, std::max (0, p.mode));
        c.revision = std::min (1, std::max (0, p.revision));
        c.ratio    = std::min (4, std::max (0, p.ratio));
        c.compOff  = p.compOff;
        c.link     = p.link;
        c.noise    = p.noise;

        c.T   = Voicing::thresholdDb[c.ratio];
        c.R   = Voicing::ratioValue[c.ratio];
        c.W   = Voicing::kneeDb[c.ratio];
        c.Wfb = 2.0 * c.W / (1.0 + c.R);
        c.all = c.ratio == ratioAll;
        c.ffLowLin = (float) dbToGain (c.T - 0.5 * c.W - 0.5);
        c.anKneeLin = dbToGain (c.T - 0.5 * c.Wfb);

        const bool blue = c.revision == revBlue;
        const double ta = attackSeconds (p.attack);
        const double tr = releaseSeconds (p.release);

        if (c.mode == modeAnalog)
        {
            // feedback loop: the effective attack is ~R times faster than the cap time constant
            const double ratioSlow = 1.0 + 0.05 * (c.R - 4.0);   // lower ratios keep the fastest attack
            const double taModel = ta * Voicing::analogAttackCal * c.R * ratioSlow * (blue ? 1.15 : 1.0);
            c.aImplicit = 1.0 / (taModel * fsOs);
            const double trModel = tr * Voicing::analogReleaseCal * (blue ? 0.92 : 1.0);
            c.relMul = std::exp (-1.0 / (trModel * fsOs));
            c.ks = blue ? 0.28 : 0.32;
            const double tTrans = blue ? 0.45 : 0.60;
            const double tSlow  = std::max (0.30, 2.0 * tr);
            c.aTrans  = 1.0 - std::exp (-1.0 / (tTrans * fsOs));
            c.slowMul = std::exp (-1.0 / (tSlow * fsOs));
            c.holdSub = 0;
        }
        else
        {
            c.aAttack = 1.0 - std::exp (-1.0 / (ta * fsOs));
            c.relMul  = std::exp (-1.0 / (tr * Voicing::ffReleaseCal * fsOs));
            c.ks = c.mode == modeClean ? 0.35 : (blue ? 0.45 : 0.50);
            const double tTrans = 0.50;
            const double tSlow  = std::max (0.30, 2.0 * tr);
            c.aTrans  = 1.0 - std::exp (-1.0 / (tTrans * fsOs));
            c.slowMul = std::exp (-1.0 / (tSlow * fsOs));
            const double holdMs = c.mode == modeClean ? Voicing::holdMsClean : Voicing::holdMsFat;
            // never hold longer than ~40% of the release time
            const double hold = std::min (holdMs * 1.0e-3, 0.4 * tr * Voicing::ffReleaseCal);
            c.holdSub = std::max (1, (int) (hold * fsOs / 8.0));
        }

        c.scHpf   = p.scHpfHz > 20.5f;
        c.scHpfHz = p.scHpfHz;
    }

    static bool needsCrossfade (const Config& a, const Config& b)
    {
        return a.mode != b.mode || a.revision != b.revision || a.compOff != b.compOff
            || a.noise != b.noise || a.link != b.link;
    }

    /** Settings that can change without a crossfade (ratio, times, sidechain filter).
        The path keeps its own topology (mode, revision, switches). */
    void updateContinuous (Path& p, const Params& prm) const
    {
        Params own = prm;
        own.mode     = p.cfg.mode;
        own.revision = p.cfg.revision;
        own.compOff  = p.cfg.compOff;
        own.link     = p.cfg.link;
        own.noise    = p.cfg.noise;
        Config next;
        buildConfig (own, next);
        const bool hpfChanged = p.cfg.scHpf != next.scHpf || std::fabs (p.cfg.scHpfHz - next.scHpfHz) > 0.01;
        p.cfg = next;
        if (hpfChanged && next.scHpf)
            for (auto& ch : p.ch)
                ch.scHpf.setHighpass (next.scHpfHz, 0.7071, fsOs);
    }

    /** Configure all colour stages of a path for its mode / revision. */
    void applyConfig (Path& p) const
    {
        const Config& c = p.cfg;
        Colour& k = p.col;
        const bool blue = c.revision == revBlue;

        for (auto& ch : p.ch)
        {
            if (c.scHpf) ch.scHpf.setHighpass (c.scHpfHz, 0.7071, fsOs);
            ch.dc1.set (2.0, fsOs);
            ch.dc2.set (2.0, fsOs);
            ch.dc3.set (2.0, fsOs);
            ch.shelfLo.setIdentity();
            ch.shelfHi.setIdentity();
            ch.hpIn.setIdentity(); ch.lpIn.setIdentity();
            ch.hpOut.setIdentity(); ch.lpOut.setIdentity();
        }

        if (c.mode == modeFat)
        {
            k.fatRef    = 0.25;
            k.fatInvRef = 1.0 / k.fatRef;
            k.fatE0     = blue ? 0.30 : 0.20;
            k.fatE1     = blue ? 0.45 : 0.30;
            k.fatE2     = blue ? 0.060 : 0.025;
            k.fatStage  = blue;
            k.fatAmp.set (0.85, 0.10);
            for (auto& ch : p.ch)
            {
                ch.tOut.set (30.0, fsOs, blue ? 0.85 : 1.15);
                ch.shelfLo.setLowShelf (85.0, blue ? 0.9 : 0.5, fsOs);
                ch.shelfHi.setHighShelf (7000.0, blue ? 0.35 : 0.0, fsOs);
            }
        }
        else if (c.mode == modeClean)
        {
            k.premium.set (0.72, 0.117);
            for (auto& ch : p.ch)
            {
                ch.tIn.set (30.0, fsOs, 2.4);
                ch.tOut.set (25.0, fsOs, 1.7);
            }
        }
        else // ANALOG
        {
            // Calibrated against the published LN figures: < 0.5 % THD with limiting (50 Hz..15 kHz, 1.1 s
            // release), +22 dBm out (= 0 dBFS here) at <= 0.5 % THD, S/N > 81 dB with the input at threshold.
            // BLUE (the earlier generation) is deliberately dirtier and noisier.
            k.fetD2 = blue ? 0.011 : 0.0034;
            k.fetD3 = blue ? 0.004 : 0.0018;
            // Black: low-noise bipolar preamp / Blue: FET preamp, hotter -> earlier, 2nd-heavy saturation
            k.preAmp.set  (blue ? 0.85 : 0.36, blue ? 0.085 : 0.030);
            k.lineAmp.set (blue ? 0.42 : 0.21, blue ? 0.080 : 0.030);
            k.noiseRms = c.noise ? dbToGain (blue ? -89.0 : -97.5) : 0.0;
            k.humAmp   = c.noise ? dbToGain (blue ? -101.0 : -110.0) : 0.0;
            for (auto& ch : p.ch)
            {
                ch.tIn.set (30.0, fsOs, blue ? 2.0 : 2.6);
                ch.hpIn.setHighpass (blue ? 7.0 : 6.0, 0.55, fsOs);   // overall response about -1 dB at 20 Hz
                ch.lpIn.setLowpass (std::min (60000.0, 0.45 * fsOs), 0.65, fsOs);
                ch.tOut.set (25.0, fsOs, blue ? 1.30 : 3.00);
                ch.hpOut.setHighpass (5.0, 0.55, fsOs);
                ch.lpOut.setLowpass (std::min (blue ? 38000.0 : 50000.0, 0.45 * fsOs), blue ? 0.65 : 0.60, fsOs);
            }
            const double w = 2.0 * kPi * 50.0 / fsOs;
            p.humRc = std::cos (w);
            p.humRs = std::sin (w);
        }
    }

    void resetPath (Path& p)
    {
        for (auto& ch : p.ch) ch.reset();
        p.ffLink.reset();
        p.anLink.reset();
        p.humC = 1.0; p.humS = 0.0;
        p.gr = 0.0f;
        p.grV = 0.0;
    }

    /** Keep the gain reduction continuous when switching between detector types. */
    static void convertDetector (Path& p, int fromMode, int toMode)
    {
        const bool fromFF = fromMode != modeAnalog, toFF = toMode != modeAnalog;
        if (fromFF == toFF) return;

        auto ffToAn = [] (const FFDet& f, AnaDet& a)
        {
            a.V = dbToGain (f.G1) - 1.0;
            a.S = dbToGain (f.G2) - 1.0;
        };
        auto anToFF = [] (const AnaDet& a, FFDet& f)
        {
            f.reset();
            f.G1 = gainToDb (1.0 + a.V);
            f.G2 = gainToDb (1.0 + a.S);
        };

        if (fromFF)
        {
            ffToAn (p.ffLink, p.anLink);
            for (auto& ch : p.ch) ffToAn (ch.ff, ch.an);
        }
        else
        {
            anToFF (p.anLink, p.ffLink);
            for (auto& ch : p.ch) anToFF (ch.an, ch.ff);
        }
        // fresh colour states for the new topology (masked by the crossfade)
        for (auto& ch : p.ch)
        {
            ch.tIn.reset(); ch.tOut.reset();
            ch.hpIn.reset(); ch.lpIn.reset(); ch.hpOut.reset(); ch.lpOut.reset();
            ch.shelfLo.reset(); ch.shelfHi.reset();
            ch.dc1.reset(); ch.dc2.reset(); ch.dc3.reset();
        }
    }

    //==========================================================================
    // Feed-forward static curve: gain reduction (dB) for an input level (dB)
    static double staticGr (const Config& c, double L) noexcept
    {
        double k, s;
        softKnee (L - c.T, c.W, k, s);
        double gr = (1.0 - 1.0 / c.R) * k;
        if (c.all)
        {
            double k2, s2;
            softKnee (L - c.T - Voicing::allOverStartDb, 6.0, k2, s2);
            gr += Voicing::allOverAmount * k2;
        }
        return gr;
    }

    /** Feed-forward detector step: peak level -> held target -> attack / program dependent release. */
    static double ffStep (const Config& c, FFDet& d, float level, bool punchLag) noexcept
    {
        // target from instantaneous peak
        float target = 0.0f;
        if (level > c.ffLowLin)
            target = (float) staticGr (c, gainToDb (level));

        // sliding maximum over ~8 sub-blocks (hold): removes ripple at low frequencies
        d.cur = std::max (d.cur, target);
        if (++d.cnt >= c.holdSub)
        {
            d.ring[d.ringPos] = d.cur;
            d.ringPos = (d.ringPos + 1) & 7;
            d.cur = 0.0f;
            d.cnt = 0;
            float m = 0.0f;
            for (float r : d.ring) m = std::max (m, r);
            d.ringMax = m;
        }
        const double held = std::max (d.cur, d.ringMax);

        if (held > d.G1)
        {
            double a = c.aAttack;
            if (punchLag)   // ALL buttons: attack lags when starting from rest
                a /= 1.0 + Voicing::allLagAmount * std::exp (-d.G1 * 0.5);
            d.G1 += a * (held - d.G1);
        }
        else
        {
            const double floor = std::max (held, d.G2);
            d.G1 = floor + (d.G1 - floor) * c.relMul;
        }

        const double slowTarget = c.ks * d.G1;
        if (slowTarget > d.G2) d.G2 += c.aTrans * (slowTarget - d.G2);
        else                   d.G2 *= c.slowMul;
        return d.G1;
    }

    // Feedback law (ANALOG): wanted GR (dB) from the post-FET level (dB), plus its slope
    static void feedbackLaw (const Config& c, double Lo, double slowDb, double& F, double& dF) noexcept
    {
        double k, s;
        softKnee (Lo - c.T, c.Wfb, k, s);
        F  = (c.R - 1.0) * k;
        dF = (c.R - 1.0) * s;
        if (c.all)
            F += Voicing::allDriftAmount * slowDb * std::min (1.0, s * 2.0);
    }

    /** Analog detector step. Lx = instantaneous rectified sidechain level (dB) *before* the FET.
        The FET output level is Lx - GR(V), so the loop is solved implicitly (unconditionally stable). */
    static double anStep (const Config& c, AnaDet& d, double level) noexcept
    {
        bool attack = false;
        double Lx = 0.0, slowDb = 0.0;

        // cheap linear-domain test first: is the FET output above the start of the knee?
        double G0 = 0.0;
        if (level > c.anKneeLin * (1.0 + d.V))
        {
            Lx = gainToDb (level);
            slowDb = c.all ? gainToDb (1.0 + d.S) : 0.0;
            G0 = gainToDb (1.0 + d.V);
            double F, dF;
            feedbackLaw (c, Lx - G0, slowDb, F, dF);
            attack = F > G0;
        }

        if (attack)
        {
            // Charging is solved in the dB domain: the loop then has a constant effective
            // time constant (~ tau / R) whatever the size of the transient, so slow attack
            // settings really let the front of a hit through, like the hardware.
            double a = c.aImplicit;
            if (c.all)
                a /= 1.0 + Voicing::allLagAmount * std::exp (-d.V * 2.0);

            double G = G0;
            for (int it = 0; it < 4; ++it)
            {
                double F, dF;
                feedbackLaw (c, Lx - G, slowDb, F, dF);
                const double f  = (1.0 + a) * G - G0 - a * F;
                const double fd = 1.0 + a + a * dF;
                const double nG = std::max (G0, G - f / fd);
                if (std::fabs (nG - G) < 1.0e-7) { G = nG; break; }
                G = nG;
            }
            d.V = dbToGain (G) - 1.0;
        }
        else
        {
            // hold cap discharges through the release pot towards the slow cap
            if (d.V > d.S) d.V = d.S + (d.V - d.S) * c.relMul;
        }

        const double slowTarget = c.ks * d.V;
        if (slowTarget > d.S) d.S += c.aTrans * (slowTarget - d.S);
        else                  d.S *= c.slowMul;

        return d.V;
    }

    //==========================================================================
    void processFrame (Path& p, float x0, float x1, int nc, float gIn, float gOut, float& y0, float& y1) noexcept
    {
        switch (p.cfg.mode)
        {
            case modeAnalog: processAnalog (p, x0, x1, nc, gIn, gOut, y0, y1); break;
            default:         processLinear (p, x0, x1, nc, gIn, gOut, y0, y1); break;
        }
    }

    void processLinear (Path& p, float x0, float x1, int nc, float gIn, float gOut, float& y0, float& y1) noexcept
    {
        const Config& c = p.cfg;
        const Colour& k = p.col;
        const bool blue  = c.revision == revBlue;
        const bool clean = c.mode == modeClean;

        double u[2] = { x0, x1 };
        double sc[2] = { 0.0, 0.0 };
        for (int ch = 0; ch < nc; ++ch)
        {
            Chan& s = p.ch[(size_t) ch];
            if (clean && blue) u[ch] = s.tIn.process (u[ch]);   // input transformer (before the input pot)
            u[ch] *= gIn;
            sc[ch] = c.scHpf ? s.scHpf.process (u[ch]) : u[ch];
        }

        double G[2] = { 0.0, 0.0 };
        const bool lag = c.all && c.mode == modeFat;
        if (! c.compOff)
        {
            if (c.link || nc == 1)
            {
                float lvl = (float) std::fabs (sc[0]);
                if (nc > 1) lvl = std::max (lvl, (float) std::fabs (sc[1]));
                G[0] = G[1] = ffStep (c, p.ffLink, lvl, lag);
            }
            else
            {
                for (int ch = 0; ch < nc; ++ch)
                    G[ch] = ffStep (c, p.ch[(size_t) ch].ff, (float) std::fabs (sc[ch]), lag);
            }
        }

        double out[2] = { 0.0, 0.0 };
        for (int ch = 0; ch < nc; ++ch)
        {
            Chan& s = p.ch[(size_t) ch];
            double v = G[ch] > 0.0 ? u[ch] * dbToGain (-G[ch]) : u[ch];

            if (c.mode == modeFat)
            {
                // density / "inflation": lifts low and mid amplitudes, leaves peaks, adds odd+even harmonics.
                // The harder it is driven (more gain reduction), the more it inflates.
                // ALL buttons hits the stage 6 dB harder.
                const double drive = c.all ? 2.0 : 1.0;
                const double e  = k.fatE0 + k.fatE1 * std::min (1.0, G[ch] / 20.0);
                const double vd = v * drive;
                const double uu = vd * k.fatInvRef;
                const double g2 = std::exp (-0.5 * uu * uu);
                double w = vd + k.fatRef * g2 * (0.5 * e * uu + k.fatE2 * (1.0 + e) * uu * uu);
                w /= 1.0 + 0.303 * e;             // unity gain around -12 dBFS: body comes up, peaks go down
                if (k.fatStage || c.all) w = k.fatAmp.process (w);
                v = s.dc1.process (w / drive);
                v = s.tOut.process (v);
                v = s.shelfLo.process (v);
                v = s.shelfHi.process (v);
            }
            else if (blue) // CLEAN + BLUE: premium saturation
            {
                v = k.premium.process (v);
                v = s.dc1.process (v);
                v = s.tOut.process (v);
            }
            out[ch] = v * gOut;
        }

        y0 = (float) out[0];
        y1 = (float) out[1];
        p.gr = (float) std::max (G[0], nc > 1 ? G[1] : 0.0);
    }

    void processAnalog (Path& p, float x0, float x1, int nc, float gIn, float gOut, float& y0, float& y1) noexcept
    {
        const Config& c = p.cfg;
        const Colour& k = p.col;

        double u[2] = { x0, x1 };
        double sc[2] = { 0.0, 0.0 };
        for (int ch = 0; ch < nc; ++ch)
        {
            Chan& s = p.ch[(size_t) ch];
            double t = s.tIn.process (u[ch]);   // input transformer sees the raw line level
            t = s.hpIn.process (t);
            t = s.lpIn.process (t);
            u[ch] = t * gIn;                    // input pot
            sc[ch] = c.scHpf ? s.scHpf.process (u[ch]) : u[ch];
        }

        double V[2] = { 0.0, 0.0 };
        if (! c.compOff)
        {
            if (c.link || nc == 1)
            {
                double lvl = std::fabs (sc[0]);
                if (nc > 1) lvl = std::max (lvl, std::fabs (sc[1]));
                V[0] = V[1] = anStep (c, p.anLink, lvl);
            }
            else
            {
                for (int ch = 0; ch < nc; ++ch)
                    V[ch] = anStep (c, p.ch[(size_t) ch].an, std::fabs (sc[ch]));
            }
        }

        // hum oscillator (only advanced when used)
        double hum = 0.0;
        if (k.humAmp > 0.0)
        {
            const double nc_ = p.humC * p.humRc - p.humS * p.humRs;
            const double ns_ = p.humS * p.humRc + p.humC * p.humRs;
            const double norm = 1.5 - 0.5 * (nc_ * nc_ + ns_ * ns_);
            p.humC = nc_ * norm;
            p.humS = ns_ * norm;
            const double s2 = 2.0 * p.humS * p.humC;
            const double s3 = p.humS * (3.0 - 4.0 * p.humS * p.humS);
            hum = k.humAmp * (p.humS + 0.45 * s2 + 0.3 * s3);
        }

        // ALL: bias points shift and the FET works far less linearly (the famous grit)
        const double fetAmt = c.all ? (c.revision == revBlue ? 5.0 : 8.0) : 1.0;
        const double invRef = 1.0 / Voicing::fetRefLevel;
        double out[2] = { 0.0, 0.0 };
        for (int ch = 0; ch < nc; ++ch)
        {
            Chan& s = p.ch[(size_t) ch];
            const double g = 1.0 / (1.0 + V[ch]);         // FET shunt attenuator
            double v = u[ch] * g;

            // residual FET nonlinearity: grows with gain reduction and with the signal across the FET
            const double n = v * invRef;
            const double gr = 1.0 - g;
            const double blueExtra = c.revision == revBlue ? (1.0 + gr) : 1.0;
            v -= gr * fetAmt * blueExtra * v * (k.fetD2 * n + k.fetD3 * n * n);
            v = s.dc1.process (v);

            v = k.preAmp.process (v);                      // preamp after the FET
            v = s.dc2.process (v);
            if (k.noiseRms > 0.0)
                v += k.noiseRms * (double) p.rng.gauss() + hum;

            v *= gOut;                                     // output pot drives the line amplifier
            v = k.lineAmp.process (v);
            v = s.dc3.process (v);
            v = s.tOut.process (v);
            v = s.lpOut.process (v);
            v = s.hpOut.process (v);
            out[ch] = v;
        }

        y0 = (float) out[0];
        y1 = (float) out[1];
        p.grV = std::max (V[0], nc > 1 ? V[1] : 0.0);   // converted to dB only when read
    }

    //==========================================================================
    int computeLatency()
    {
        if (osFactor == 1) return 0;
        const int N = 4096;
        Oversampler t;
        t.prepare (fs, osFactor, N);
        std::vector<float> imp ((size_t) N, 0.0f), up ((size_t) (N * osFactor)), dn ((size_t) N);
        imp[0] = 1.0f;
        t.upsample (imp.data(), up.data(), N);
        t.downsample (up.data(), dn.data(), N);
        // group delay around 1 kHz
        auto phase = [&] (double f)
        {
            double re = 0.0, im = 0.0;
            for (int n = 0; n < N; ++n)
            {
                re += dn[(size_t) n] * std::cos (2.0 * kPi * f * n / fs);
                im -= dn[(size_t) n] * std::sin (2.0 * kPi * f * n / fs);
            }
            return std::atan2 (im, re);
        };
        const double f = 1000.0, df = 20.0;
        double dp = phase (f + df) - phase (f - df);
        while (dp > kPi)  dp -= 2.0 * kPi;
        while (dp < -kPi) dp += 2.0 * kPi;
        const double gd = -dp / (2.0 * kPi * 2.0 * df / fs);
        return (int) std::lround (gd);
    }

    //==========================================================================
    static constexpr int chunk = 256;

    double fs = 48000.0, fsOs = 192000.0;
    int osFactor = 4, osChoice = 0, nCh = 2, latency = 0;
    std::array<Oversampler, 2> os;
    std::array<std::vector<float>, 2> osBuf;

    Params params;
    Smoother smIn, smOut, smMix;

    std::array<Path, 2> paths;
    int active = 0;
    bool configured = false, fading = false;
    int fadePos = 0, fadeLen = 1;

    float grMeter = 0.0f, inMeanAbs = 0.0f, outMeanAbs = 0.0f;
};

} // namespace twinfet

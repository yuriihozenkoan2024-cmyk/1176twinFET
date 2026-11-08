// TwinFET measurement harness: runs the DSP engine offline and writes CSV/raw data
// for tests/analyze.py.   Build:  g++ -O2 -std=c++17 measure.cpp -o measure
#include "TwinFetEngine.h"   // found via the include path (dsp/ or the project root)

#include <chrono>
#include <complex>
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#if defined(__SSE__) || defined(__x86_64__)
 #include <xmmintrin.h>
 #include <pmmintrin.h>
#endif

using namespace twinfet;

static std::string outDir = "results";
static const char* modeName[] = { "FAT", "CLEAN", "ANALOG" };
static const char* revName[]  = { "BLACK", "BLUE" };
static const char* ratioName[] = { "4", "8", "12", "20", "ALL" };

//------------------------------------------------------------------------------
static void runEngine (Engine& e, std::vector<float>& L, std::vector<float>* R = nullptr, int block = 512)
{
    const int n = (int) L.size();
    for (int s = 0; s < n; s += block)
    {
        const int len = std::min (block, n - s);
        float* chans[2] = { L.data() + s, R != nullptr ? R->data() + s : nullptr };
        e.process (chans, R != nullptr ? 2 : 1, len);
    }
}

static std::vector<float> sine (double f, double ampDb, double fs, int n, double phase = 0.0)
{
    std::vector<float> x ((size_t) n);
    const double a = dbToGain (ampDb);
    for (int i = 0; i < n; ++i) x[(size_t) i] = (float) (a * std::sin (2.0 * kPi * f * i / fs + phase));
    return x;
}

static std::vector<double> bhWindow (int n)
{
    std::vector<double> w ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        const double t = 2.0 * kPi * i / (n - 1);
        w[(size_t) i] = 0.35875 - 0.48829 * std::cos (t) + 0.14128 * std::cos (2 * t) - 0.01168 * std::cos (3 * t);
    }
    return w;
}

/** Windowed single-bin DFT amplitude (peak). */
static double toneAmp (const std::vector<float>& x, size_t start, int n, double f, double fs, const std::vector<double>& w)
{
    std::complex<double> acc = 0.0;
    double ws = 0.0;
    const double dw = -2.0 * kPi * f / fs;
    std::complex<double> rot (std::cos (dw), std::sin (dw)), ph (1.0, 0.0);
    for (int i = 0; i < n; ++i)
    {
        acc += w[(size_t) i] * (double) x[start + (size_t) i] * ph;
        ph *= rot;
        if ((i & 1023) == 0) ph /= std::abs (ph);
        ws += w[(size_t) i];
    }
    return 2.0 * std::abs (acc) / ws;
}

static double binFreq (double f, double fs, int n) { return std::round (f * n / fs) * fs / n; }

static FILE* openCsv (const std::string& name, const char* header)
{
    FILE* f = std::fopen ((outDir + "/" + name).c_str(), "w");
    if (f == nullptr) { std::perror (name.c_str()); std::exit (1); }
    std::fprintf (f, "%s\n", header);
    return f;
}

static Params baseParams (int mode, int rev, int ratio)
{
    Params p;
    p.mode = mode; p.revision = rev; p.ratio = ratio;
    p.inputDb = 0.0f; p.outputDb = 0.0f; p.attack = 4.0f; p.release = 4.0f;
    p.mix = 1.0f; p.link = true; p.compOff = false; p.noise = false;
    return p;
}

//------------------------------------------------------------------------------
// 1. Static input/output curves at 1 kHz
static void testStatic (double fs)
{
    FILE* f = openCsv ("static.csv", "mode,rev,ratio,in_db,out_db,gr_db");
    const int n = (int) (0.7 * fs), seg = 8192;
    const double f0 = binFreq (1000.0, fs, seg);
    auto w = bhWindow (seg);
    for (int mode = 0; mode < 3; ++mode)
        for (int rev = 0; rev < 2; ++rev)
            for (int ratio = 0; ratio < 5; ++ratio)
                for (int lvl = -40; lvl <= 6; ++lvl)
                {
                    Engine e;
                    e.prepare (fs, 512, 1);
                    e.setParams (baseParams (mode, rev, ratio));
                    auto x = sine (f0, lvl, fs, n);
                    runEngine (e, x);
                    const double a = toneAmp (x, (size_t) (n - seg), seg, f0, fs, w);
                    std::fprintf (f, "%d,%d,%d,%d,%.4f,%.4f\n", mode, rev, ratio, lvl, gainToDb (a), e.getGainReductionDb());
                }
    std::fclose (f);
}

//------------------------------------------------------------------------------
// 2. Attack / release timing and program dependent release (GR traces)
static void testTiming (double fs)
{
    FILE* fs_ = openCsv ("timing_summary.csv", "mode,rev,which,knob,spec_ms,meas_ms,gr_peak");
    FILE* ft  = openCsv ("timing_traces.csv", "mode,rev,which,knob,t_ms,gr_db");
    const double hot = -18.0 + 13.33; // ~10 dB of GR at 4:1
    for (int mode = 0; mode < 3; ++mode)
        for (int rev = 0; rev < 2; ++rev)
        {
            // attack: release fixed (4), attack knob varies
            for (int knob : { 1, 3, 5, 7 })
            {
                Params p = baseParams (mode, rev, ratio4);
                p.attack = (float) knob; p.release = 4.0f;
                Engine e; e.prepare (fs, 512, 1); e.setParams (p);
                // square wave (|x| is constant) so the waveform does not mask the detector time constant
                const int pre = (int) (0.2 * fs), on = (int) (0.05 * fs);
                std::vector<float> x ((size_t) (pre + on), 0.0f);
                const float aLo = (float) dbToGain (-40.0), aHi = (float) dbToGain (hot);
                for (int i = 0; i < pre + on; ++i)
                {
                    const float sgn = ((i / 12) % 2 == 0) ? 1.0f : -1.0f;   // 2 kHz square at 48 kHz
                    x[(size_t) i] = sgn * (i < pre ? aLo : aHi);
                }
                std::vector<float> gr;
                e.grTraceOs = &gr;
                runEngine (e, x, nullptr, 64);
                const double fo = fs * e.getOversamplingFactor();
                const size_t preOs = (size_t) pre * (size_t) e.getOversamplingFactor()
                                   + (size_t) e.getLatencySamples() * (size_t) e.getOversamplingFactor() / 2;
                double gmax = 0.0;
                for (size_t i = preOs; i < gr.size(); ++i) gmax = std::max (gmax, (double) gr[i]);
                // settled value: median of the last 5 ms
                std::vector<float> tail (gr.end() - (long) (0.005 * fo), gr.end());
                std::nth_element (tail.begin(), tail.begin() + (long) tail.size() / 2, tail.end());
                const double finalGr = tail[tail.size() / 2];
                // onset: first sample where GR moves
                size_t onset = preOs;
                for (size_t i = preOs - (size_t) (0.001 * fo); i < gr.size(); ++i)
                    if (gr[i] > 0.02) { onset = i - 1; break; }
                double tHit = -1.0;
                for (size_t i = onset; i < gr.size(); ++i)
                    if (gr[i] >= 0.632 * finalGr) { tHit = (double) (i - onset) / fo * 1000.0; break; }
                const double spec = 800.0e-3 * std::pow (20.0 / 800.0, (knob - 1) / 6.0);
                std::fprintf (fs_, "%d,%d,attack,%d,%.4f,%.4f,%.3f\n", mode, rev, knob, spec, tHit, gmax);
                for (size_t i = onset - (size_t) (0.0002 * fo); i < std::min (gr.size(), onset + (size_t) (0.004 * fo)); ++i)
                    std::fprintf (ft, "%d,%d,attack,%d,%.5f,%.4f\n", mode, rev, knob, ((double) i - (double) onset) / fo * 1000.0, gr[i]);
            }
            // release: short (100 ms) and long (2 s) bursts
            for (int burst = 0; burst < 2; ++burst)
                for (int knob : { 1, 4, 7 })
                {
                    Params p = baseParams (mode, rev, ratio4);
                    p.attack = 4.0f; p.release = (float) knob;
                    Engine e; e.prepare (fs, 512, 1); e.setParams (p);
                    const int pre = (int) (0.1 * fs), on = (int) ((burst == 0 ? 0.1 : 2.0) * fs), post = (int) (3.0 * fs);
                    std::vector<float> x ((size_t) (pre + on + post), 0.0f);
                    auto lo1 = sine (1000.0, -40.0, fs, pre);
                    auto hi  = sine (1000.0, hot, fs, on, 2.0 * kPi * 1000.0 * pre / fs);
                    auto lo2 = sine (1000.0, -40.0, fs, post, 2.0 * kPi * 1000.0 * (pre + on) / fs);
                    std::copy (lo1.begin(), lo1.end(), x.begin());
                    std::copy (hi.begin(), hi.end(), x.begin() + pre);
                    std::copy (lo2.begin(), lo2.end(), x.begin() + pre + on);
                    std::vector<float> gr;
                    e.grTrace = &gr;
                    runEngine (e, x, nullptr, 64);
                    const size_t t0 = (size_t) (pre + on);
                    const double g0 = gr[t0 - 1];
                    double tHit = -1.0;
                    for (size_t i = t0; i < gr.size(); ++i)
                        if (gr[i] <= 0.368 * g0) { tHit = (double) (i - t0) / fs * 1000.0; break; }
                    const double spec = 1100.0 * std::pow (50.0 / 1100.0, (knob - 1) / 6.0);
                    const char* which = burst == 0 ? "release_short" : "release_long";
                    std::fprintf (fs_, "%d,%d,%s,%d,%.4f,%.4f,%.3f\n", mode, rev, which, knob, spec, tHit, g0);
                    for (size_t i = t0 - (size_t) (0.05 * fs); i < gr.size(); i += 48)
                        std::fprintf (ft, "%d,%d,%s,%d,%.4f,%.4f\n", mode, rev, which, knob, ((double) i - (double) t0) / fs * 1000.0, gr[i]);
                }
        }
    std::fclose (fs_);
    std::fclose (ft);
}

//------------------------------------------------------------------------------
// 3. Harmonic distortion
static void thdCase (FILE* f, double fs, const Params& p, double freq, double lvl, const char* tag)
{
    const int seg = 32768;
    const double f0 = binFreq (freq, fs, seg);
    const int n = (int) (1.2 * fs) + seg;
    Engine e; e.prepare (fs, 512, 1); e.setParams (p);
    auto x = sine (f0, lvl, fs, n);
    runEngine (e, x);
    auto w = bhWindow (seg);
    const size_t st = (size_t) (n - seg);
    const double a1 = toneAmp (x, st, seg, f0, fs, w);
    double h[10] = { 0 };
    double sum = 0.0;
    for (int k = 2; k <= 9; ++k)
    {
        if (k * f0 >= 0.5 * fs) break;
        h[k] = toneAmp (x, st, seg, k * f0, fs, w);
        sum += h[k] * h[k];
    }
    const double thd = std::sqrt (sum) / a1;
    std::fprintf (f, "%s,%d,%d,%d,%.1f,%.1f,%.2f,%.3f,%.6f,%.2f,%.2f,%.2f,%.2f,%.2f\n", tag, p.mode, p.revision, (int) p.compOff, freq, lvl,
                  gainToDb (a1), e.getGainReductionDb(), thd * 100.0,
                  gainToDb (h[2] / a1), gainToDb (h[3] / a1), gainToDb (h[4] / a1), gainToDb (h[5] / a1), gainToDb (h[7] / a1));
}

static void testThd (double fs)
{
    FILE* f = openCsv ("thd.csv", "tag,mode,rev,compoff,freq,in_db,out_db,gr_db,thd_pct,h2_db,h3_db,h4_db,h5_db,h7_db");
    for (int mode = 0; mode < 3; ++mode)
        for (int rev = 0; rev < 2; ++rev)
        {
            for (double lvl : { -30.0, -24.0, -18.0, -12.0, -6.0, 0.0 })
            {
                Params p = baseParams (mode, rev, ratio4);
                p.attack = 3.0f; p.release = 5.0f;
                thdCase (f, fs, p, 1000.0, lvl, "level1k");
                Params q = p; q.compOff = true;
                thdCase (f, fs, q, 1000.0, lvl, "colour1k");
            }
            // low-frequency gain-modulation distortion vs speed (about 9 dB GR)
            for (int fast = 0; fast < 2; ++fast)
                for (double fr : { 40.0, 60.0, 100.0, 200.0 })
                {
                    Params p = baseParams (mode, rev, ratio4);
                    p.attack = fast ? 7.0f : 3.0f;
                    p.release = fast ? 7.0f : 3.0f;
                    thdCase (f, fs, p, fr, -6.0, fast ? "lf_fast" : "lf_slow");
                }
            // ALL buttons
            {
                Params p = baseParams (mode, rev, ratioAll);
                p.attack = 3.0f; p.release = 5.0f;
                thdCase (f, fs, p, 1000.0, -6.0, "all1k");
                thdCase (f, fs, p, 100.0, -6.0, "all100");
            }
        }
    std::fclose (f);
}

//------------------------------------------------------------------------------
// 4. Aliasing: hot 9.1 kHz tone, look for non-harmonic components
static void testAlias (double fs)
{
    FILE* f = openCsv ("alias.csv", "mode,rev,fund_db,worst_alias_dbc,worst_alias_hz");
    const int seg = 65536;
    const double f0 = binFreq (9100.0, fs, seg);
    for (int mode = 0; mode < 3; ++mode)
        for (int rev = 0; rev < 2; ++rev)
        {
            Params p = baseParams (mode, rev, ratio4);
            p.inputDb = 12.0f; p.attack = 3.0f; p.release = 4.0f;
            if (mode == modeAnalog) p.outputDb = 6.0f;  // push the line amp too
            Engine e; e.prepare (fs, 512, 1); e.setParams (p);
            const int n = (int) (0.6 * fs) + seg;
            auto x = sine (f0, -3.0, fs, n);
            runEngine (e, x);
            // dump the analysed segment for the spectrum plot
            {
                std::string nm = outDir + "/alias_" + std::to_string (mode) + "_" + std::to_string (rev) + ".f32";
                FILE* r = std::fopen (nm.c_str(), "wb");
                std::fwrite (x.data() + (n - seg), sizeof (float), (size_t) seg, r);
                std::fclose (r);
            }
            auto w = bhWindow (seg);
            const size_t st = (size_t) (n - seg);
            const double a1 = toneAmp (x, st, seg, f0, fs, w);
            // scan a grid of bins away from harmonics
            double worst = 0.0, worstHz = 0.0;
            const double binHz = fs / seg;
            for (int b = 20; b < seg / 2 - 20; b += 7)
            {
                const double fr = b * binHz;
                bool nearHarm = false;
                for (int k = 1; k * f0 < 0.5 * fs; ++k)
                    if (std::fabs (fr - k * f0) < 40.0 * binHz) nearHarm = true;
                if (nearHarm || fr < 30.0) continue;
                const double a = toneAmp (x, st, seg, fr, fs, w);
                if (a > worst) { worst = a; worstHz = fr; }
            }
            std::fprintf (f, "%d,%d,%.2f,%.2f,%.1f\n", mode, rev, gainToDb (a1), gainToDb (worst / a1), worstHz);
        }
    std::fclose (f);
}

//------------------------------------------------------------------------------
// 4b. Oversampling choices: aliasing, latency and CPU for 1x..16x
static double worstAliasDbc (std::vector<float>& x, int n, int seg, double f0, double fs)
{
    auto w = bhWindow (seg);
    const size_t st = (size_t) (n - seg);
    const double a1 = toneAmp (x, st, seg, f0, fs, w);
    double worst = 0.0;
    const double binHz = fs / seg;
    for (int b = 20; b < seg / 2 - 20; b += 7)
    {
        const double fr = b * binHz;
        bool nearHarm = false;
        for (int k = 1; k * f0 < 0.5 * fs; ++k)
            if (std::fabs (fr - k * f0) < 40.0 * binHz) nearHarm = true;
        if (nearHarm || fr < 30.0) continue;
        worst = std::max (worst, toneAmp (x, st, seg, fr, fs, w));
    }
    return gainToDb (worst / a1);
}

static void testOversampling (double fs)
{
    FILE* f = openCsv ("oversampling.csv", "choice,factor,mode,rev,latency_samples,worst_alias_dbc,cpu_percent_stereo");
    const int seg = 32768;
    const double f0 = binFreq (9100.0, fs, seg);
    for (int choice = 1; choice <= 5; ++choice)
        for (int mode : { (int) modeFat, (int) modeAnalog })
        {
            const int rev = revBlue;
            Params p = baseParams (mode, rev, ratio4);
            p.inputDb = 12.0f; p.attack = 3.0f; p.release = 4.0f;
            if (mode == modeAnalog) p.outputDb = 6.0f;
            Engine e; e.setOversamplingChoice (choice); e.prepare (fs, 512, 1); e.setParams (p);
            const int n = (int) (0.5 * fs) + seg;
            auto x = sine (f0, -3.0, fs, n);
            runEngine (e, x);
            const double alias = worstAliasDbc (x, n, seg, f0, fs);

            Engine c; c.setOversamplingChoice (choice); c.prepare (fs, 512, 2); c.setParams (p);
            const int m = (int) (5.0 * fs);
            std::mt19937 rng (3);
            std::normal_distribution<float> nd (0.0f, 0.15f);
            std::vector<float> L ((size_t) m), R ((size_t) m);
            for (int i = 0; i < m; ++i) { L[(size_t) i] = nd (rng); R[(size_t) i] = nd (rng); }
            const auto t0 = std::chrono::high_resolution_clock::now();
            runEngine (c, L, &R, 256);
            const auto t1 = std::chrono::high_resolution_clock::now();
            const double cpu = 100.0 * std::chrono::duration<double> (t1 - t0).count() / 5.0;
            std::fprintf (f, "%d,%d,%d,%d,%d,%.1f,%.2f\n", choice, e.getOversamplingFactor(), mode, rev, e.getLatencySamples(), alias, cpu);
        }
    std::fclose (f);
}

//------------------------------------------------------------------------------
// 5. Small-signal frequency response (impulse at -60 dBFS)
static void testFreq (double fs)
{
    for (int mode = 0; mode < 3; ++mode)
        for (int rev = 0; rev < 2; ++rev)
        {
            Params p = baseParams (mode, rev, ratio4);
            p.compOff = true;
            Engine e; e.prepare (fs, 512, 1); e.setParams (p);
            // settle with silence first
            std::vector<float> s ((size_t) (0.5 * fs), 0.0f);
            runEngine (e, s);
            const int n = 1 << 16;
            std::vector<float> x ((size_t) n, 0.0f);
            x[0] = 1.0e-3f;
            runEngine (e, x);
            for (auto& v : x) v *= 1000.0f;
            std::string nm = outDir + "/ir_" + std::to_string (mode) + "_" + std::to_string (rev) + ".f32";
            FILE* r = std::fopen (nm.c_str(), "wb");
            std::fwrite (x.data(), sizeof (float), (size_t) n, r);
            std::fclose (r);
        }
}

//------------------------------------------------------------------------------
// 6. CPU
static void testCpu (double fs)
{
    FILE* f = openCsv ("cpu.csv", "mode,rev,seconds_audio,seconds_cpu,percent_of_one_core");
    const int n = (int) (20.0 * fs);
    std::mt19937 rng (1);
    std::normal_distribution<float> nd (0.0f, 0.15f);
    std::vector<float> L0 ((size_t) n), R0 ((size_t) n);
    for (int i = 0; i < n; ++i) { L0[(size_t) i] = nd (rng); R0[(size_t) i] = nd (rng); }
    for (int mode = 0; mode < 3; ++mode)
        for (int rev = 0; rev < 2; ++rev)
        {
            Params p = baseParams (mode, rev, ratio4);
            p.inputDb = 6.0f;
            Engine e; e.prepare (fs, 512, 2); e.setParams (p);
            auto L = L0, R = R0;
            const auto t0 = std::chrono::high_resolution_clock::now();
            runEngine (e, L, &R, 256);
            const auto t1 = std::chrono::high_resolution_clock::now();
            const double sec = std::chrono::duration<double> (t1 - t0).count();
            std::fprintf (f, "%d,%d,%.1f,%.4f,%.3f\n", mode, rev, 20.0, sec, 100.0 * sec / 20.0);
        }
    std::fclose (f);
}

//------------------------------------------------------------------------------
// 7. Robustness: random automation, random block sizes, silence, extreme levels
static void testRobust (double fs)
{
    FILE* f = openCsv ("robust.csv", "check,value,ok");
    std::mt19937 rng (7);
    std::uniform_real_distribution<float> u01 (0.0f, 1.0f);
    Engine e; e.prepare (fs, 4096, 2);
    Params p = baseParams (modeAnalog, revBlack, ratio4);
    e.setParams (p);
    const int total = (int) (60.0 * fs);
    std::vector<float> L (4096), R (4096);
    bool bad = false;
    double maxAbs = 0.0;
    int pos = 0;
    while (pos < total)
    {
        const int len = 1 + (int) (u01 (rng) * 2047.0f);
        // randomise parameters now and then
        if (u01 (rng) < 0.3f)
        {
            p.mode = (int) (u01 (rng) * 3.0f) % 3;
            p.revision = u01 (rng) < 0.5f ? 0 : 1;
            p.ratio = (int) (u01 (rng) * 5.0f) % 5;
            p.inputDb = -24.0f + 60.0f * u01 (rng);
            p.outputDb = -24.0f + 48.0f * u01 (rng);
            p.attack = 1.0f + 6.0f * u01 (rng);
            p.release = 1.0f + 6.0f * u01 (rng);
            p.mix = u01 (rng);
            p.scHpfHz = u01 (rng) < 0.5f ? 0.0f : 20.0f + 480.0f * u01 (rng);
            p.link = u01 (rng) < 0.5f;
            p.compOff = u01 (rng) < 0.15f;
            p.noise = u01 (rng) < 0.2f;
            e.setParams (p);
        }
        const int kind = (pos / (int) (2.0 * fs)) % 4; // noise / silence / extreme / low sine
        for (int i = 0; i < len; ++i)
        {
            float a = 0.0f, b = 0.0f;
            if (kind == 0) { a = (u01 (rng) - 0.5f) * 0.6f; b = (u01 (rng) - 0.5f) * 0.6f; }
            else if (kind == 2) { a = (u01 (rng) - 0.5f) * 20.0f; b = (u01 (rng) - 0.5f) * 20.0f; }
            else if (kind == 3) { a = 0.5f * std::sin (2.0f * 3.14159265f * 40.0f * (float) (pos + i) / (float) fs); b = -a; }
            L[(size_t) i] = a; R[(size_t) i] = b;
        }
        float* ch[2] = { L.data(), R.data() };
        e.process (ch, 2, len);
        for (int i = 0; i < len; ++i)
        {
            if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) bad = true;
            maxAbs = std::max (maxAbs, (double) std::fabs (L[(size_t) i]));
        }
        pos += len;
    }
    std::fprintf (f, "finite_output_random_automation,%d,%d\n", bad ? 0 : 1, bad ? 0 : 1);
    std::fprintf (f, "max_abs_output_with_+20dB_noise_in_and_+60dB_gain,%.3f,%d\n", maxAbs, std::isfinite (maxAbs) ? 1 : 0);

    // silence after loud material -> must decay to digital silence-ish (noise off)
    for (int mode = 0; mode < 3; ++mode)
        for (int rev = 0; rev < 2; ++rev)
        {
            Engine s; s.prepare (fs, 512, 1);
            Params q = baseParams (mode, rev, ratio4); q.inputDb = 20.0f;
            s.setParams (q);
            auto x = sine (100.0, 0.0, fs, (int) fs);
            runEngine (s, x);
            std::vector<float> z ((size_t) (3.0 * fs), 0.0f);
            runEngine (s, z);
            double rms = 0.0;
            for (size_t i = z.size() - 4800; i < z.size(); ++i) rms += (double) z[i] * z[i];
            rms = std::sqrt (rms / 4800.0);
            std::fprintf (f, "silence_tail_dbfs_mode%d_rev%d,%.1f,%d\n", mode, rev, gainToDb (rms + 1e-30), rms < 1e-6 ? 1 : 0);
        }

    // DC at output for a strongly asymmetric drive (should be blocked)
    for (int mode = 0; mode < 3; ++mode)
    {
        Engine s; s.prepare (fs, 512, 1);
        Params q = baseParams (mode, revBlue, ratio4); q.inputDb = 12.0f; q.outputDb = 6.0f;
        s.setParams (q);
        auto x = sine (220.0, -3.0, fs, (int) (4.0 * fs));
        runEngine (s, x);
        double dc = 0.0;
        const int seg = (int) fs; // integrate full cycles
        for (size_t i = x.size() - (size_t) seg; i < x.size(); ++i) dc += x[i];
        dc /= seg;
        std::fprintf (f, "output_dc_mode%d_blue,%.2e,%d\n", mode, dc, std::fabs (dc) < 1e-3 ? 1 : 0);
    }

    // link check: loud L, quiet R
    for (int link = 0; link < 2; ++link)
    {
        Engine s; s.prepare (fs, 512, 2);
        Params q = baseParams (modeClean, revBlack, ratio4); q.link = link == 1;
        s.setParams (q);
        auto a = sine (binFreq (1000.0, fs, 8192), -3.0, fs, (int) fs);
        auto b = sine (binFreq (1000.0, fs, 8192), -40.0, fs, (int) fs);
        runEngine (s, a, &b);
        auto w = bhWindow (8192);
        const double outR = gainToDb (toneAmp (b, b.size() - 8192, 8192, binFreq (1000.0, fs, 8192), fs, w));
        std::fprintf (f, "right_channel_level_link%d_dbfs,%.2f,%d\n", link, outR, link ? (outR < -45.0 ? 1 : 0) : (std::fabs (outR + 40.0) < 0.3 ? 1 : 0));
    }

    // transparency: CLEAN BLACK, compression off -> magnitude must be flat
    {
        Engine s; s.prepare (fs, 512, 1);
        Params q = baseParams (modeClean, revBlack, ratio4); q.compOff = true;
        s.setParams (q);
        double worst = 0.0;
        for (double fr : { 20.0, 50.0, 100.0, 1000.0, 5000.0, 10000.0, 15000.0, 18000.0, 20000.0 })
        {
            Engine t = s;
            t.reset();
            const int seg = 16384;
            const double f0 = binFreq (fr, fs, seg);
            auto x = sine (f0, -12.0, fs, (int) (0.5 * fs) + seg);
            runEngine (t, x);
            auto w = bhWindow (seg);
            const double a = toneAmp (x, x.size() - seg, seg, f0, fs, w);
            worst = std::max (worst, std::fabs (gainToDb (a) + 12.0));
        }
        std::fprintf (f, "clean_black_compoff_max_magnitude_error_db,%.6f,%d\n", worst, worst < 0.01 ? 1 : 0);
    }

    // crossfade clicks: switch mode/revision on a steady 150 Hz tone
    {
        Engine s; s.prepare (fs, 512, 1);
        Params q = baseParams (modeClean, revBlack, ratio4); q.inputDb = 6.0f;
        s.setParams (q);
        const int n = (int) (6.0 * fs);
        auto x = sine (150.0, -6.0, fs, n);
        std::vector<float> y = x;
        double maxJump = 0.0, base = 0.0;
        int count = 0;
        for (int st = 0; st < n; st += 256)
        {
            const int len = std::min (256, n - st);
            if (st > (int) fs && (st % (int) (0.25 * fs)) < 256)
            {
                q.mode = (q.mode + 1) % 3;
                if (q.mode == 0) q.revision = 1 - q.revision;
                s.setParams (q);
                ++count;
            }
            float* ch[1] = { y.data() + st };
            s.process (ch, 1, len);
        }
        // second difference as click detector; compare to the theoretical level for the tone
        for (int i = (int) fs; i < n - 1; ++i)
        {
            const double d2 = std::fabs (y[(size_t) i + 1] - 2.0 * y[(size_t) i] + y[(size_t) i - 1]);
            maxJump = std::max (maxJump, d2);
        }
        base = dbToGain (-6.0) * std::pow (2.0 * kPi * 150.0 / fs, 2.0);
        std::fprintf (f, "switches,%d,1\n", count);
        std::fprintf (f, "crossfade_max_second_difference_vs_clean_tone,%.3f,%d\n", maxJump / base, maxJump / base < 40.0 ? 1 : 0);
    }
    std::fclose (f);
}

//------------------------------------------------------------------------------
static int countFailures (const std::string& csv)
{
    FILE* f = std::fopen ((outDir + "/" + csv).c_str(), "r");
    if (f == nullptr) return 1;
    char line[512];
    int fails = 0;
    bool header = true;
    while (std::fgets (line, sizeof (line), f) != nullptr)
    {
        if (header) { header = false; continue; }
        std::string l (line);
        while (! l.empty() && (l.back() == '\n' || l.back() == '\r')) l.pop_back();
        const auto comma = l.find_last_of (',');
        if (comma != std::string::npos && l.substr (comma + 1) == "0")
        {
            std::printf ("FAILED: %s\n", l.c_str());
            ++fails;
        }
    }
    std::fclose (f);
    return fails;
}

int main (int argc, char** argv)
{
#if defined(__SSE__) || defined(__x86_64__)
    _MM_SET_FLUSH_ZERO_MODE (_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE (_MM_DENORMALS_ZERO_ON);
#endif
    std::string what = argc > 1 ? argv[1] : "all";
    if (argc > 2) outDir = argv[2];
    const double fs = 48000.0;
    if (what == "static" || what == "all") testStatic (fs);
    if (what == "timing" || what == "all") testTiming (fs);
    if (what == "thd" || what == "all")    testThd (fs);
    if (what == "alias" || what == "all")  testAlias (fs);
    if (what == "os" || what == "all")     testOversampling (fs);
    if (what == "freq" || what == "all")   testFreq (fs);
    if (what == "cpu" || what == "all")    testCpu (fs);
    int failures = 0;
    if (what == "robust" || what == "all")
    {
        testRobust (fs);
        failures = countFailures ("robust.csv");
    }
    std::printf ("done: %s (%d failed checks)\n", what.c_str(), failures);
    (void) modeName; (void) revName; (void) ratioName;
    return failures == 0 ? 0 : 1;
}

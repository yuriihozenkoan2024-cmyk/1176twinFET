// TwinFET — polyphase IIR half-band oversampling (header only).
//
// Each 2x stage is a half-band elliptic-type lowpass built from two chains of
// first-order allpass sections running at the low rate:
//     H(z) = 0.5 * [ A0(z^2) + z^-1 * A1(z^2) ]
// The coefficient design follows the classic method used in Laurent de Soras'
// HIIR library (re-implemented here). It gives very low latency (a few samples)
// and a practically flat passband; the phase is not linear, which is why the
// engine mixes dry and wet signals inside the oversampled domain, so that both
// see exactly the same filters.
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace twinfet
{

class HalfbandDesigner
{
public:
    /** attenuationDb: stopband rejection. transition: (stopband - passband) / highRate, in ]0, 0.5[ */
    static std::vector<double> design (double attenuationDb, double transition)
    {
        double k = 0.0, q = 0.0;
        transitionParam (transition, k, q);
        const int order = computeOrder (attenuationDb, q);
        const int n     = (order - 1) / 2;
        std::vector<double> c ((size_t) n);
        for (int i = 0; i < n; ++i)
            c[(size_t) i] = computeCoef (i, k, q, order);
        return c;
    }

private:
    static constexpr double pi = 3.14159265358979323846;

    static void transitionParam (double transition, double& k, double& q)
    {
        k = std::tan ((1.0 - transition * 2.0) * pi / 4.0);
        k *= k;
        const double kksqrt = std::pow (1.0 - k * k, 0.25);
        const double e  = 0.5 * (1.0 - kksqrt) / (1.0 + kksqrt);
        const double e2 = e * e;
        const double e4 = e2 * e2;
        q = e * (1.0 + e4 * (2.0 + e4 * (15.0 + 150.0 * e4)));
    }

    static int computeOrder (double attenuationDb, double q)
    {
        const double attnP2 = std::pow (10.0, -attenuationDb / 10.0);
        const double a      = attnP2 / (1.0 - attnP2);
        int order = (int) std::ceil (std::log (a * a / 16.0) / std::log (q));
        if ((order & 1) == 0) ++order;
        if (order == 1) order = 3;
        return order;
    }

    static double accNum (double q, int order, int c)
    {
        int i = 0, j = 1;
        double acc = 0.0, term = 0.0;
        do
        {
            term = std::pow (q, (double) (i * (i + 1)));
            term *= std::sin ((double) ((i * 2 + 1) * c) * pi / (double) order) * (double) j;
            acc += term;
            j = -j;
            ++i;
        } while (std::fabs (term) > 1.0e-100 && i < 1000);
        return acc;
    }

    static double accDen (double q, int order, int c)
    {
        int i = 1, j = -1;
        double acc = 0.0, term = 0.0;
        do
        {
            term = std::pow (q, (double) (i * i));
            term *= std::cos ((double) (i * 2 * c) * pi / (double) order) * (double) j;
            acc += term;
            j = -j;
            ++i;
        } while (std::fabs (term) > 1.0e-100 && i < 1000);
        return acc;
    }

    static double computeCoef (int index, double k, double q, int order)
    {
        const int c       = index + 1;
        const double num  = accNum (q, order, c) * std::pow (q, 0.25);
        const double den  = accDen (q, order, c) + 0.5;
        const double ww   = num / den;
        const double wwsq = ww * ww;
        const double x    = std::sqrt ((1.0 - wwsq * k) * (1.0 - wwsq / k)) / (1.0 + wwsq);
        return (1.0 - x) / (1.0 + x);
    }
};

//==============================================================================
/** A chain of first-order allpass sections  y = c (x - y1) + x1. */
class AllpassChain
{
public:
    void init (const std::vector<double>& coefs)
    {
        c.assign (coefs.begin(), coefs.end());
        x1.assign (c.size(), 0.0);
        y1.assign (c.size(), 0.0);
    }

    void reset()
    {
        std::fill (x1.begin(), x1.end(), 0.0);
        std::fill (y1.begin(), y1.end(), 0.0);
    }

    inline double process (double x) noexcept
    {
        const size_t n = c.size();
        for (size_t i = 0; i < n; ++i)
        {
            const double y = c[i] * (x - y1[i]) + x1[i];
            x1[i] = x;
            y1[i] = y;
            x = y;
        }
        return x;
    }

private:
    std::vector<double> c, x1, y1;
};

//==============================================================================
/** One 2x stage with independent states for the up and down directions. */
class Halfband2x
{
public:
    void init (const std::vector<double>& coefs)
    {
        std::vector<double> even, odd;
        for (size_t i = 0; i < coefs.size(); ++i)
            (i % 2 == 0 ? even : odd).push_back (coefs[i]);
        upA.init (even); upB.init (odd);
        dnA.init (even); dnB.init (odd);
    }

    void reset() { upA.reset(); upB.reset(); dnA.reset(); dnB.reset(); }

    /** One low-rate sample in, two high-rate samples out (in time order). */
    inline void up (double x, double& out0, double& out1) noexcept
    {
        out0 = upA.process (x);
        out1 = upB.process (x);
    }

    /** Two high-rate samples in (in time order), one low-rate sample out. */
    inline double down (double in0, double in1) noexcept
    {
        return 0.5 * (dnA.process (in1) + dnB.process (in0));
    }

private:
    AllpassChain upA, upB, dnA, dnB;
};

//==============================================================================
/** 1x / 2x / 4x oversampler for one channel. */
class Oversampler
{
public:
    static constexpr int maxStages = 4;   // up to 16x

    /** factor is rounded up to a power of two (1..16); maxBlock = most low-rate samples per call. */
    void prepare (double baseRate, int factor, int maxBlock = 256)
    {
        osFactor = 1;
        stages = 0;
        while (osFactor < factor && stages < maxStages) { osFactor *= 2; ++stages; }

        // every stage keeps 0..20 kHz (or 0.45 fs) flat and rejects images by >100 dB;
        // the later stages run at higher rates, so their transition bands are wide and cheap
        const double pass = std::min (20000.0, 0.45 * baseRate);
        for (int k = 0; k < stages; ++k)
        {
            const double stageRate = baseRate * (double) (1 << k);
            st[(size_t) k].init (HalfbandDesigner::design (110.0, std::max (0.01, 0.5 - pass / stageRate)));
        }
        for (int k = 0; k < maxStages; ++k)
            tmp[(size_t) k].assign (k < stages - 1 ? (size_t) maxBlock << (k + 1) : 0, 0.0f);
        reset();
    }

    void reset() { for (auto& s : st) s.reset(); }

    int factor() const noexcept { return osFactor; }

    /** in: n low-rate samples; out: n * factor high-rate samples. */
    void upsample (const float* in, float* out, int n) noexcept
    {
        if (stages == 0) { std::copy (in, in + n, out); return; }
        const float* src = in;
        int len = n;
        for (int k = 0; k < stages; ++k)
        {
            float* dst = (k == stages - 1) ? out : tmp[(size_t) k].data();
            auto& f = st[(size_t) k];
            for (int i = 0; i < len; ++i)
            {
                double a, b;
                f.up (src[i], a, b);
                dst[2 * i] = (float) a;
                dst[2 * i + 1] = (float) b;
            }
            src = dst;
            len *= 2;
        }
    }

    /** in: n * factor high-rate samples; out: n low-rate samples. */
    void downsample (const float* in, float* out, int n) noexcept
    {
        if (stages == 0) { std::copy (in, in + n, out); return; }
        const float* src = in;
        int len = n << stages;
        for (int k = stages - 1; k >= 0; --k)
        {
            float* dst = (k == 0) ? out : tmp[(size_t) (k - 1)].data();
            auto& f = st[(size_t) k];
            const int outLen = len / 2;
            for (int i = 0; i < outLen; ++i)
                dst[i] = (float) f.down (src[2 * i], src[2 * i + 1]);
            src = dst;
            len = outLen;
        }
    }

private:
    int osFactor = 1, stages = 0;
    std::array<Halfband2x, maxStages> st;
    std::array<std::vector<float>, maxStages> tmp;
};

} // namespace twinfet

// TwinFET — small DSP building blocks (framework independent, header only)
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace twinfet
{

constexpr double kPi      = 3.14159265358979323846;
constexpr double kLn10_20 = 0.11512925464970228420; // ln(10) / 20
constexpr double k20_Ln10 = 8.68588963806503655302; // 20 / ln(10)

inline double dbToGain (double db) noexcept { return std::exp (db * kLn10_20); }
inline double gainToDb (double g) noexcept  { return k20_Ln10 * std::log (std::max (g, 1.0e-30)); }

//==============================================================================
/** One-pole parameter smoother (exponential). */
struct Smoother
{
    float coef = 0.0f, value = 0.0f, target = 0.0f;

    void setTime (double seconds, double fs) noexcept { coef = (float) std::exp (-1.0 / (seconds * fs)); }
    void reset (float v) noexcept                     { value = target = v; }
    inline float next() noexcept                      { value = target + (value - target) * coef; return value; }
};

//==============================================================================
/** Biquad in transposed direct form II, double precision (low corner
    frequencies at high oversampled rates need the extra precision). */
struct Biquad
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double z1 = 0.0, z2 = 0.0;

    void reset() noexcept { z1 = z2 = 0.0; }

    inline double process (double x) noexcept
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void setIdentity() noexcept { b0 = 1.0; b1 = b2 = a1 = a2 = 0.0; }

    void setLowpass (double fc, double q, double fs) noexcept
    {
        fc = std::min (fc, 0.49 * fs);
        const double w = 2.0 * kPi * fc / fs, c = std::cos (w), s = std::sin (w), al = s / (2.0 * q);
        const double a0 = 1.0 + al;
        b0 = (1.0 - c) * 0.5 / a0; b1 = (1.0 - c) / a0; b2 = b0;
        a1 = -2.0 * c / a0;        a2 = (1.0 - al) / a0;
    }

    void setHighpass (double fc, double q, double fs) noexcept
    {
        const double w = 2.0 * kPi * fc / fs, c = std::cos (w), s = std::sin (w), al = s / (2.0 * q);
        const double a0 = 1.0 + al;
        b0 = (1.0 + c) * 0.5 / a0; b1 = -(1.0 + c) / a0; b2 = b0;
        a1 = -2.0 * c / a0;         a2 = (1.0 - al) / a0;
    }

    void setPeak (double fc, double q, double gainDb, double fs) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * fc / fs, c = std::cos (w), s = std::sin (w), al = s / (2.0 * q);
        const double a0 = 1.0 + al / A;
        b0 = (1.0 + al * A) / a0; b1 = -2.0 * c / a0; b2 = (1.0 - al * A) / a0;
        a1 = -2.0 * c / a0;       a2 = (1.0 - al / A) / a0;
    }

    void setLowShelf (double fc, double gainDb, double fs) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * fc / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2.0 * std::sqrt (2.0); // shelf slope S = 1
        const double sa = 2.0 * std::sqrt (A) * al;
        const double a0 = (A + 1.0) + (A - 1.0) * c + sa;
        b0 = A * ((A + 1.0) - (A - 1.0) * c + sa) / a0;
        b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * c) / a0;
        b2 = A * ((A + 1.0) - (A - 1.0) * c - sa) / a0;
        a1 = -2.0 * ((A - 1.0) + (A + 1.0) * c) / a0;
        a2 = ((A + 1.0) + (A - 1.0) * c - sa) / a0;
    }

    void setHighShelf (double fc, double gainDb, double fs) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * fc / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2.0 * std::sqrt (2.0);
        const double sa = 2.0 * std::sqrt (A) * al;
        const double a0 = (A + 1.0) - (A - 1.0) * c + sa;
        b0 = A * ((A + 1.0) + (A - 1.0) * c + sa) / a0;
        b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * c) / a0;
        b2 = A * ((A + 1.0) + (A - 1.0) * c - sa) / a0;
        a1 = 2.0 * ((A - 1.0) - (A + 1.0) * c) / a0;
        a2 = ((A + 1.0) - (A - 1.0) * c - sa) / a0;
    }
};

//==============================================================================
/** First-order DC blocker (the coupling capacitor of a real stage). */
struct DcBlock
{
    double R = 0.9999, x1 = 0.0, y1 = 0.0;

    void set (double fc, double fs) noexcept { R = std::exp (-2.0 * kPi * fc / fs); }
    void reset() noexcept                    { x1 = y1 = 0.0; }

    inline double process (double x) noexcept
    {
        const double y = x - x1 + R * y1;
        x1 = x;
        y1 = y;
        return y;
    }
};

//==============================================================================
/** tanh: Pade (7,6) approximant in the normal operating range (|x| < 2.5,
    error < 2e-7), the library function beyond it. */
inline double fastTanh (double x) noexcept
{
    if (std::fabs (x) > 2.5) return std::tanh (x);
    const double x2 = x * x;
    return x * (135135.0 + x2 * (17325.0 + x2 * (378.0 + x2)))
             / (135135.0 + x2 * (62370.0 + x2 * (3150.0 + 28.0 * x2)));
}

//==============================================================================
/** Single-ended (class-A style) gain stage: a biased tanh, normalised to unity
    small-signal gain and zero output at zero input. The bias makes the two
    half-waves clip at different levels, which produces even harmonics. The
    output carries a level-dependent DC shift, so follow it with a DcBlock,
    exactly like the coupling capacitor after a real stage. */
struct ClassAStage
{
    double k = 1.0, b = 0.0, t0 = 0.0, norm = 1.0;

    void set (double drive, double bias) noexcept
    {
        k    = drive;
        b    = bias;
        t0   = std::tanh (k * b);
        norm = 1.0 / (k * (1.0 - t0 * t0));
    }

    inline double process (double x) const noexcept { return (fastTanh (k * (x + b)) - t0) * norm; }
};

//==============================================================================
/** Transformer core model. The core flux is the (leaky) integral of the
    voltage, so low frequencies produce much more flux than high ones. The
    flux is soft-saturated and then differentiated back to a voltage.
    With an unsaturated core the structure is an exact identity. */
struct Transformer
{
    double lam = 0.999, oneMinusLam = 0.001, invOneMinusLam = 1000.0;
    double pSat = 1.0, invPSat = 1.0;
    double phi = 0.0, psiPrev = 0.0;

    void set (double cornerHz, double fs, double satLevel) noexcept
    {
        lam            = std::exp (-2.0 * kPi * cornerHz / fs);
        oneMinusLam    = 1.0 - lam;
        invOneMinusLam = 1.0 / oneMinusLam;
        // algebraic sigmoid q/sqrt(1+q^2), scaled to have the same cubic term as tanh at satLevel
        pSat           = satLevel * 1.2247448713915890;
        invPSat        = 1.0 / pSat;
    }

    void reset() noexcept { phi = psiPrev = 0.0; }

    inline double process (double x) noexcept
    {
        phi = lam * phi + oneMinusLam * x;
        const double q   = phi * invPSat;
        const double psi = pSat * q / std::sqrt (1.0 + q * q);   // smooth everywhere (it gets differentiated)
        const double y   = (psi - lam * psiPrev) * invOneMinusLam;
        psiPrev = psi;
        return y;
    }
};

//==============================================================================
/** Small fast PRNG for the optional analog noise floor. */
struct Rng
{
    uint32_t s = 0x9E3779B9u;

    inline float uniform() noexcept // [-0.5, 0.5)
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) (s >> 8) * (1.0f / 16777216.0f) - 0.5f;
    }

    /** Approximately gaussian, unit RMS (sum of four uniforms). */
    inline float gauss() noexcept
    {
        return (uniform() + uniform() + uniform() + uniform()) * 1.7320508f;
    }
};

//==============================================================================
/** Soft knee helper: returns the knee-shaped overshoot (dB) and its slope. */
inline void softKnee (double over, double width, double& value, double& slope) noexcept
{
    const double h = 0.5 * width;
    if (over <= -h)       { value = 0.0;  slope = 0.0; }
    else if (over >= h)   { value = over; slope = 1.0; }
    else
    {
        const double z = over + h;
        value = z * z / (2.0 * width);
        slope = z / width;
    }
}

} // namespace twinfet

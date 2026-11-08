// TwinFET — panel artwork and layout (header only, no JUCE dependency).
//
// Everything is drawn in base units: the faceplate is 1000 x 360 and the ADVANCED drawer adds
// 180 below it. The drawing goes through a tiny "canvas" adapter G so exactly the same code renders
// inside the plugin (JUCE adapter in PluginEditor.cpp) and in the offline preview (cairo adapter in
// tests/ui_preview.cpp).
//
// Adapter G must provide:
//   save() restore() clipRect(x,y,w,h)
//   setColour(argb)  setLinear(x1,y1,x2,y2,stops,n)  setRadial(cx,cy,r,stops,n)
//   fillRect  fillRoundRect(x,y,w,h,r)  strokeRoundRect(x,y,w,h,r,lw)
//   fillEllipse(x,y,w,h)  strokeEllipse(x,y,w,h,lw)  line(x1,y1,x2,y2,lw)
//   newPath() moveTo lineTo arc(cx,cy,r,a0,a1,newSubPath) closePath() fillPath() strokePath(lw)
//   text(utf8, x,y,w,h, size, align(-1/0/1), tracking)
//   grain(x,y,w,h, kind, seed, strength)   -> per-pixel texture made with makeGrain()
// Angles: 0 = 12 o'clock, clockwise (JUCE convention). Light comes from the upper left.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace twinui
{

constexpr float kW = 1000.0f;
constexpr float kMainH = 360.0f;               // the faceplate
constexpr float kDrawerH = 180.0f;             // the ADVANCED drawer that slides out below it
constexpr float kFullH = kMainH + kDrawerH;
constexpr float kPi = 3.14159265358979f;
constexpr float kStartAngle = kPi * 1.25f;
constexpr float kEndAngle   = kPi * 2.75f;

struct GradStop { float pos; uint32_t argb; };

struct Rect
{
    float x, y, w, h;
    float cx() const { return x + 0.5f * w; }
    float cy() const { return y + 0.5f * h; }
    float right() const { return x + w; }
    float bottom() const { return y + h; }
};

inline float angleFor (float proportion) { return kStartAngle + proportion * (kEndAngle - kStartAngle); }

/** Interactive components are larger than the artwork so shadows are not clipped. */
constexpr float knobPad   = 14.0f;
constexpr float buttonPad = 6.0f;
inline float knobRadius (float rectWidth) { return 0.47f * rectWidth; }
inline float px (float cx, float r, float a) { return cx + r * std::sin (a); }
inline float py (float cy, float r, float a) { return cy - r * std::cos (a); }

//==============================================================================
/** Every control position, in base units. */
namespace layout
{
    inline Rect leftEar()  { return { 0.0f, 0.0f, 34.0f, kMainH }; }
    inline Rect rightEar() { return { kW - 34.0f, 0.0f, 34.0f, kMainH }; }

    // ---- faceplate header: the mode lamp with the mode's name, the two main read-outs
    inline Rect jewel()      { return { 412.0f, 20.0f, 24.0f, 24.0f }; }
    inline Rect modeName()   { return { 444.0f, 18.0f, 130.0f, 28.0f }; }
    inline Rect grReadout()  { return { 606.0f, 9.0f, 146.0f, 36.0f }; }
    inline Rect outReadout() { return { 774.0f, 9.0f, 146.0f, 36.0f }; }
    constexpr float readoutCaptionH = 13.0f;   // caption printed under the big read-outs

    // ---- meter block
    inline Rect vu()       { return { 54.0f, 74.0f, 328.0f, 212.0f }; }
    inline Rect meterButton (int i) { return { 112.0f + 68.0f * (float) i, 298.0f, 62.0f, 26.0f }; }

    // ---- input / output
    inline Rect input()    { return { 422.0f, 126.0f, 128.0f, 128.0f }; }
    inline Rect output()   { return { 596.0f, 126.0f, 128.0f, 128.0f }; }
    inline Rect inputReadout()  { return { 450.0f, 266.0f, 72.0f, 21.0f }; }
    inline Rect outputReadout() { return { 624.0f, 266.0f, 72.0f, 21.0f }; }
    inline Rect ratioButton (int i) { return { 466.0f + 56.0f * (float) i, 298.0f, 50.0f, 26.0f }; }

    // ---- attack / release (stacked)
    inline Rect attack()   { return { 822.0f, 104.0f, 56.0f, 56.0f }; }
    inline Rect release()  { return { 822.0f, 236.0f, 56.0f, 56.0f }; }
    inline Rect attackReadout()  { return { 816.0f, 175.0f, 68.0f, 19.0f }; }
    inline Rect releaseReadout() { return { 816.0f, 307.0f, 68.0f, 19.0f }; }

    // ---- the pull that opens the ADVANCED drawer
    inline Rect advancedHandle() { return { 434.0f, 333.0f, 132.0f, 21.0f }; }

    // ---- ADVANCED drawer (below the faceplate)
    constexpr float dy = kMainH;
    inline Rect drawer()           { return { 30.0f, dy + 2.0f, kW - 60.0f, kDrawerH - 8.0f }; }
    inline Rect modeButton (int i) { return { 64.0f + 84.0f * (float) i, dy + 30.0f, 78.0f, 38.0f }; }
    inline Rect revButton (int i)  { return { 346.0f + 70.0f * (float) i, dy + 34.0f, 64.0f, 30.0f }; }
    inline Rect osButton (int i)   { return { 512.0f + 50.0f * (float) i, dy + 35.0f, 44.0f, 28.0f }; }
    inline Rect valuesButton()     { return { 840.0f, dy + 33.0f, 96.0f, 32.0f }; }
    inline Rect mix()              { return { 80.0f, dy + 102.0f, 52.0f, 52.0f }; }
    inline Rect hpf()              { return { 246.0f, dy + 102.0f, 52.0f, 52.0f }; }
    inline Rect mixReadout()       { return { 146.0f, dy + 118.0f, 60.0f, 19.0f }; }
    inline Rect hpfReadout()       { return { 312.0f, dy + 118.0f, 60.0f, 19.0f }; }
    inline Rect toggle (int i)     { return { 404.0f + 98.0f * (float) i, dy + 106.0f, 88.0f, 34.0f }; }   // LINK, COMP OFF, NOISE
    inline Rect plate()            { return { 786.0f, dy + 106.0f, 150.0f, 34.0f }; }
}

//==============================================================================
inline uint32_t withAlpha (uint32_t argb, float a)
{
    const uint32_t al = (uint32_t) std::lround (255.0f * (a < 0.0f ? 0.0f : (a > 1.0f ? 1.0f : a)));
    return (argb & 0x00ffffffu) | (al << 24);
}

inline uint32_t mix (uint32_t a, uint32_t b, float t)
{
    auto ch = [t] (uint32_t x, uint32_t y, int sh)
    {
        const float fx = (float) ((x >> sh) & 0xffu), fy = (float) ((y >> sh) & 0xffu);
        return (uint32_t) std::lround (fx + (fy - fx) * t) << sh;
    };
    return ch (a, b, 24) | ch (a, b, 16) | ch (a, b, 8) | ch (a, b, 0);
}

//==============================================================================
/** Panel material (revision) and lamps / wear (mode). */
struct Palette
{
    bool lightPanel = false;
    uint32_t panelA = 0xff2a2c2f, panelB = 0xff17181a;     // anodised panel, upper left -> lower right
    float grainStrength = 0.075f;
    uint32_t ink = 0xebe9e3d6, inkDim = 0xc4b9b2a3;       // silkscreen (slightly translucent)
    uint32_t meterLamp = 0xffffd79a;                      // VU back-light (mode)
    bool centerLamp = false;                              // ANALOG: one bulb behind the middle of the meter
    uint32_t jewel = 0xffff3b2b;                          // mode lamp (mode)
    uint32_t lampHot = 0xfffff2d6, lampEdge = 0xffeab063; // lit push buttons (incandescent)
    uint32_t led = 0xffffcf8e;                            // seven-segment read-outs (pale amber LEDs)
    uint32_t ledFilter = 0xff120d09;                      // tinted filter window behind them
    float wear = 0.0f;                                    // 0 = new, 1 = well used (ANALOG)
};

inline Palette paletteFor (int revision, int mode)
{
    Palette p;
    if (revision == 1)   // silver-blue anodised aluminium
    {
        p.lightPanel = true;
        p.panelA = 0xffbccbdc;
        p.panelB = 0xff7b8ea5;
        p.grainStrength = 0.10f;
        p.ink = 0xf0101317;
        p.inkDim = 0xd0323b46;
    }
    switch (mode)
    {
        case 0:  p.meterLamp = 0xffff9a3c; p.jewel = 0xffff8a12; break;   // FAT: deep amber
        case 1:  p.meterLamp = 0xffd6e6ff; p.jewel = 0xffcfe2ff; break;   // CLEAN: cool white
        default: p.meterLamp = 0xffffcf7a; p.jewel = 0xffe8241a;          // ANALOG: one incandescent bulb, red jewel,
                 p.centerLamp = true; p.wear = 1.0f; break;               //         and a unit that has seen some sessions
    }
    return p;
}

//==============================================================================
/** Deterministic per-pixel textures, values in [-1, 1].
    kind 0: brushed metal (horizontal), 1: fine anodised grain, 2: meter paper. */
inline void makeGrain (std::vector<float>& v, int w, int h, int kind, uint32_t seed, float pxPerUnit)
{
    v.clear();
    if (w <= 0 || h <= 0) return;
    const size_t n = (size_t) w * (size_t) h;
    v.assign (n, 0.0f);
    uint32_t s = seed * 2654435761u + 0x9e3779b9u;
    auto rnd = [&s]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xffffffu) / 8388608.0f - 1.0f; };

    std::vector<float> tmp;
    auto blurRows = [&] (int radius)
    {
        tmp.resize ((size_t) w + 1);
        for (int y = 0; y < h; ++y)
        {
            float* r = v.data() + (size_t) y * (size_t) w;
            tmp[0] = 0.0f;
            for (int x = 0; x < w; ++x) tmp[(size_t) x + 1] = tmp[(size_t) x] + r[x];
            for (int x = 0; x < w; ++x)
            {
                const int lo = std::max (0, x - radius), hi = std::min (w, x + radius + 1);
                r[x] = (tmp[(size_t) hi] - tmp[(size_t) lo]) / (float) (hi - lo);
            }
        }
    };
    auto blurCols = [&] (int radius)
    {
        tmp.resize ((size_t) h + 1);
        for (int x = 0; x < w; ++x)
        {
            tmp[0] = 0.0f;
            for (int y = 0; y < h; ++y) tmp[(size_t) y + 1] = tmp[(size_t) y] + v[(size_t) y * (size_t) w + (size_t) x];
            for (int y = 0; y < h; ++y)
            {
                const int lo = std::max (0, y - radius), hi = std::min (h, y + radius + 1);
                v[(size_t) y * (size_t) w + (size_t) x] = (tmp[(size_t) hi] - tmp[(size_t) lo]) / (float) (hi - lo);
            }
        }
    };
    auto normalise = [&] (float target)
    {
        double sum = 0.0, sq = 0.0;
        for (float a : v) { sum += a; sq += (double) a * a; }
        const double mean = sum / (double) n;
        const double sd = std::sqrt (std::max (1.0e-12, sq / (double) n - mean * mean));
        for (auto& a : v) a = (float) (((double) a - mean) / sd * (double) target);
    };

    for (auto& a : v) a = rnd();

    if (kind == 0)
    {
        // long horizontal streaks + a brightness per hairline + fine grain
        const int radius = std::max (3, (int) (18.0f * pxPerUnit));
        blurRows (radius);
        blurRows (radius);
        normalise (1.0f);
        std::vector<float> rowOff ((size_t) h);
        for (auto& r : rowOff) r = rnd();
        for (int y = 0; y < h; ++y)
        {
            const float ro = rowOff[(size_t) y];
            float* r = v.data() + (size_t) y * (size_t) w;
            for (int x = 0; x < w; ++x) r[x] = 0.55f * r[x] + 0.65f * ro + 0.25f * rnd();
        }
    }
    else if (kind == 1)
    {
        blurRows (1);
        blurCols (1);
    }
    else
    {
        // paper: fine fibres plus soft blotches
        blurRows (1);
        blurCols (1);
        normalise (1.0f);
        const int cell = std::max (8, (int) (22.0f * pxPerUnit));
        const int gw = w / cell + 2, gh = h / cell + 2;
        std::vector<float> grid ((size_t) gw * (size_t) gh);
        for (auto& gv : grid) gv = rnd();
        auto G = [&] (int a, int b) { return grid[(size_t) b * (size_t) gw + (size_t) a]; };
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const float fx = (float) x / (float) cell, fy = (float) y / (float) cell;
                const int ix = (int) fx, iy = (int) fy;
                const float tx = fx - (float) ix, ty = fy - (float) iy;
                const float blot = (G (ix, iy) * (1.0f - tx) + G (ix + 1, iy) * tx) * (1.0f - ty)
                                 + (G (ix, iy + 1) * (1.0f - tx) + G (ix + 1, iy + 1) * tx) * ty;
                v[(size_t) y * (size_t) w + (size_t) x] += 0.9f * blot;
            }
    }
    normalise (0.42f);
    for (auto& a : v) a = std::max (-1.0f, std::min (1.0f, a));
}

//==============================================================================
/** Soft shadow under a rounded rectangle (stacked translucent layers). */
template <class G>
void softShadow (G& g, Rect b, float radius, float spread, float dy, float strength)
{
    const int layers = 6;
    for (int i = layers; i >= 1; --i)
    {
        const float e = spread * (float) i / (float) layers;
        g.setColour (withAlpha (0xff000000, strength / (float) layers * 1.6f));
        g.fillRoundRect (b.x - e, b.y - e + dy, b.w + 2.0f * e, b.h + 2.0f * e, radius + e);
    }
}

/** Silkscreen: sits on the metal, so no drop shadow, just a hint of print thickness. */
template <class G>
void ink (G& g, const Palette& p, const char* t, Rect r, float size, int align, float tracking, bool dim = false)
{
    g.setColour (p.lightPanel ? 0x18ffffff : 0x22000000);
    g.text (t, r.x, r.y + 0.6f, r.w, r.h, size, align, tracking);
    g.setColour (dim ? p.inkDim : p.ink);
    g.text (t, r.x, r.y, r.w, r.h, size, align, tracking);
}

template <class G>
void screw (G& g, float cx, float cy, float r)
{
    // countersink
    const GradStop sink[] = { { 0.0f, 0x66000000 }, { 0.75f, 0x33000000 }, { 1.0f, 0x00000000 } };
    g.setRadial (cx + 0.6f, cy + 0.9f, r * 1.45f, sink, 3);
    g.fillEllipse (cx - 1.45f * r, cy - 1.45f * r, 2.9f * r, 2.9f * r);
    // head
    const GradStop head[] = { { 0.0f, 0xffe6e8eb }, { 0.5f, 0xff9a9ea4 }, { 1.0f, 0xff4f5358 } };
    g.setRadial (cx - 0.45f * r, cy - 0.5f * r, 1.7f * r, head, 3);
    g.fillEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r);
    g.setColour (0x55000000);
    g.strokeEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r, 0.6f);
    // cross recess: dark cut with a lit lower edge
    const float l = 0.6f * r;
    for (int k = 0; k < 2; ++k)
    {
        const float ang = 0.5f + (float) k * kPi * 0.5f;
        const float dx = l * std::cos (ang), dy = l * std::sin (ang);
        g.setColour (0x55ffffff);
        g.line (cx - dx + 0.3f, cy - dy + 0.5f, cx + dx + 0.3f, cy + dy + 0.5f, 0.24f * r);
        g.setColour (0xe0141518);
        g.line (cx - dx, cy - dy, cx + dx, cy + dy, 0.22f * r);
    }
}

//==============================================================================
/** Deterministic random numbers for wear marks: the same marks on every computer. */
struct WearRng
{
    uint32_t s;
    explicit WearRng (uint32_t seed) : s (seed * 2654435761u + 0x6d2b79f5u) { if (s == 0) s = 1; next(); }
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni() { return (float) (next() & 0xffffffu) / 16777216.0f; }
    float range (float a, float b) { return a + (b - a) * uni(); }
};

inline uint32_t hashText (const char* t)
{
    uint32_t h = 2166136261u;
    for (; t != nullptr && *t != 0; ++t) { h ^= (uint8_t) *t; h *= 16777619u; }
    return h;
}

/** A thin, slightly bowed scratch; its deeper middle part catches more light. Angle in radians, x right, y down. */
template <class G>
void scratch (G& g, float x, float y, float ang, float len, float bow, float width, uint32_t colour, float alpha)
{
    const float ca = std::cos (ang), sa = std::sin (ang);
    auto path = [&] (float t0, float t1)
    {
        g.newPath();
        for (int i = 0; i <= 6; ++i)
        {
            const float t = t0 + (t1 - t0) * (float) i / 6.0f;
            const float d = t * len, off = bow * len * 4.0f * t * (1.0f - t);
            const float X = x + ca * d - sa * off, Y = y + sa * d + ca * off;
            if (i == 0) g.moveTo (X, Y); else g.lineTo (X, Y);
        }
    };
    path (0.0f, 1.0f);
    g.setColour (withAlpha (colour, 0.5f * alpha));
    g.strokePath (width);
    path (0.2f, 0.75f);
    g.setColour (withAlpha (colour, alpha));
    g.strokePath (0.6f * width);
}

/** Dust, hairline scratches and a finger smudge on glass or a lens (the caller clips to it). */
template <class G>
void glassWear (G& g, Rect b, uint32_t seed, float amount)
{
    if (amount <= 0.0f) return;
    WearRng rnd (seed);
    const float area = b.w * b.h;
    const int specks = (int) (amount * (2.0f + area / 240.0f));
    for (int i = 0; i < specks; ++i)
    {
        const float s = rnd.range (0.22f, 0.7f);
        g.setColour (withAlpha (0xffe9e5dd, rnd.range (0.10f, 0.30f)));
        g.fillEllipse (b.x + rnd.uni() * b.w - s, b.y + rnd.uni() * b.h - s, 2.0f * s, 2.0f * s * rnd.range (0.6f, 1.0f));
    }
    const int lines = (int) (amount * (1.0f + area / 1500.0f));
    for (int i = 0; i < lines; ++i)
    {
        const float len = rnd.range (0.15f, 0.7f) * b.w;
        scratch (g, b.x + rnd.uni() * (b.w - 0.3f * len), b.y + rnd.range (0.1f, 0.9f) * b.h, rnd.range (-0.35f, 0.35f), len,
                 rnd.range (-0.03f, 0.03f), rnd.range (0.25f, 0.45f), 0xfff2efe8, rnd.range (0.05f, 0.10f) * std::min (1.6f, amount));
    }
    if (amount > 0.3f)   // a soft fingerprint smudge
    {
        const float a = std::min (1.5f, amount);
        const float r = rnd.range (0.25f, 0.45f) * std::min (b.w, 2.0f * b.h);
        const float cx = b.x + rnd.range (0.2f, 0.8f) * b.w, cy = b.y + rnd.range (0.3f, 0.7f) * b.h;
        const GradStop sm[] = { { 0.0f, withAlpha (0xffffffff, 0.035f * a) }, { 0.7f, withAlpha (0xffffffff, 0.012f * a) }, { 1.0f, 0x00ffffff } };
        g.setRadial (cx, cy, r, sm, 3);
        g.fillEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r);
    }
}

//==============================================================================
/** Satin chrome (frames, bezels), lit from the upper left. */
template <class G>
void chromeFill (G& g, Rect r, float radius)
{
    const GradStop s[] = { { 0.0f, 0xfff3f5f7 }, { 0.22f, 0xffc4c9cf }, { 0.5f, 0xff80868d }, { 0.78f, 0xff4b5056 }, { 1.0f, 0xff9ca2a9 } };
    g.setLinear (r.x, r.y, r.x + 0.35f * r.w, r.bottom(), s, 5);
    g.fillRoundRect (r.x, r.y, r.w, r.h, radius);
    g.setColour (0x70ffffff);
    g.line (r.x + radius, r.y + 0.45f, r.right() - radius, r.y + 0.45f, 0.7f);
}

/** Stainless button-head screw with a hex socket. */
template <class G>
void hexScrew (G& g, float cx, float cy, float r)
{
    const GradStop sh[] = { { 0.0f, 0x70000000 }, { 0.75f, 0x30000000 }, { 1.0f, 0x00000000 } };
    g.setRadial (cx + 0.5f, cy + 0.9f, 1.5f * r, sh, 3);
    g.fillEllipse (cx + 0.5f - 1.5f * r, cy + 0.9f - 1.5f * r, 3.0f * r, 3.0f * r);
    const GradStop head[] = { { 0.0f, 0xfff4f5f7 }, { 0.35f, 0xffb8bdc3 }, { 0.75f, 0xff6c7178 }, { 1.0f, 0xff3d4146 } };
    g.setRadial (cx - 0.4f * r, cy - 0.45f * r, 1.6f * r, head, 4);
    g.fillEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r);
    g.setColour (0x66000000);
    g.strokeEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r, 0.5f);
    const float s = 0.42f * r;
    auto hex = [&] (float ox, float oy)
    {
        g.newPath();
        for (int i = 0; i < 6; ++i)
        {
            const float a = (float) i * kPi / 3.0f + 0.3f;
            const float X = cx + ox + s * std::cos (a), Y = cy + oy + s * std::sin (a);
            if (i == 0) g.moveTo (X, Y); else g.lineTo (X, Y);
        }
        g.closePath();
    };
    hex (0.22f * r * 0.5f, 0.32f * r * 0.5f);   // lit lower wall of the socket
    g.setColour (0x70ffffff);
    g.fillPath();
    hex (0.0f, 0.0f);
    g.setColour (0xf0101114);
    g.fillPath();
}

/** Polished rack handle: a round bar between two posts that stand off the ear. */
template <class G>
void rackHandle (G& g, float cx, float y0, float y1, const Palette& p)
{
    const float bw = 9.0f, pr = 7.0f;
    for (int i = 4; i >= 1; --i)   // shadow on the ear: the handle stands about a centimetre off the panel
    {
        const float e = 1.2f * (float) i;
        g.setColour (withAlpha (0xff000000, p.lightPanel ? 0.06f : 0.09f));
        g.fillRoundRect (cx - 0.5f * bw + 4.0f - e, y0 + 5.0f - e, bw + 2.0f * e, (y1 - y0) + 2.0f * e, 0.5f * bw + e);
    }
    // the bar: a polished cylinder reflecting a bright window and a dark room
    const GradStop cyl[] = { { 0.00f, 0xff2b2e32 }, { 0.14f, 0xff9097a0 }, { 0.30f, 0xfff8f9fb }, { 0.42f, 0xffcfd4da },
                             { 0.62f, 0xff6c727a }, { 0.86f, 0xff25282c }, { 1.00f, 0xff5a5f66 } };
    g.setLinear (cx - 0.5f * bw, 0.0f, cx + 0.5f * bw, 0.0f, cyl, 7);
    g.fillRoundRect (cx - 0.5f * bw, y0, bw, y1 - y0, 0.5f * bw);
    const GradStop env[] = { { 0.0f, 0x00ffffff }, { 0.3f, 0x16ffffff }, { 0.55f, 0x00ffffff }, { 0.85f, 0x18000000 }, { 1.0f, 0x00000000 } };
    g.setLinear (0.0f, y0, 0.0f, y1, env, 5);
    g.fillRoundRect (cx - 0.5f * bw, y0, bw, y1 - y0, 0.5f * bw);
    for (const float postY : { y0, y1 })
    {
        const GradStop sh[] = { { 0.0f, 0x66000000 }, { 1.0f, 0x00000000 } };
        g.setRadial (cx + 2.0f, postY + 3.0f, pr + 4.0f, sh, 2);
        g.fillEllipse (cx + 2.0f - pr - 4.0f, postY + 3.0f - pr - 4.0f, 2.0f * (pr + 4.0f), 2.0f * (pr + 4.0f));
        const GradStop face[] = { { 0.0f, 0xfffafbfc }, { 0.4f, 0xffc3c8ce }, { 0.85f, 0xff6a7077 }, { 1.0f, 0xff3a3e43 } };
        g.setRadial (cx - 0.35f * pr, postY - 0.4f * pr, 1.7f * pr, face, 4);
        g.fillEllipse (cx - pr, postY - pr, 2.0f * pr, 2.0f * pr);
        g.setColour (0x55000000);
        g.strokeEllipse (cx - pr, postY - pr, 2.0f * pr, 2.0f * pr, 0.6f);
        hexScrew (g, cx, postY, 3.0f);
    }
}

/** Small domed rivet. */
template <class G>
void rivet (G& g, float cx, float cy, float r)
{
    g.setColour (0x55000000);
    g.fillEllipse (cx - r + 0.3f, cy - r + 0.5f, 2.0f * r, 2.0f * r);
    const GradStop d[] = { { 0.0f, 0xfffbfcfd }, { 0.5f, 0xffaeb4bb }, { 1.0f, 0xff5c6168 } };
    g.setRadial (cx - 0.35f * r, cy - 0.4f * r, 1.5f * r, d, 3);
    g.fillEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r);
}

/** Engraved serial plate. */
template <class G>
void serialPlate (G& g, Rect r)
{
    softShadow (g, r, 2.0f, 2.0f, 1.2f, 0.45f);
    const GradStop pl[] = { { 0.0f, 0xffe4e7ea }, { 1.0f, 0xffa9afb6 } };
    g.setLinear (r.x, r.y, r.x, r.bottom(), pl, 2);
    g.fillRoundRect (r.x, r.y, r.w, r.h, 2.0f);
    g.grain (r.x, r.y, r.w, r.h, 0, 1234u, 0.10f);
    g.setColour (0x90ffffff);
    g.line (r.x + 2.0f, r.y + 0.5f, r.right() - 2.0f, r.y + 0.5f, 0.7f);
    g.setColour (0x80000000);
    g.strokeRoundRect (r.x, r.y, r.w, r.h, 2.0f, 0.6f);
    auto engrave = [&] (const char* t, float y, float size, float tracking)
    {
        g.setColour (0x70ffffff);
        g.text (t, r.x + 10.0f, y + 0.6f, r.w - 20.0f, size + 2.0f, size, 0, tracking);
        g.setColour (0xe0262a2e);
        g.text (t, r.x + 10.0f, y, r.w - 20.0f, size + 2.0f, size, 0, tracking);
    };
    engrave ("TWINFET  MODEL TF-2", r.y + 4.0f, 11.0f, 0.14f);
    engrave ("SER. No. 02417", r.bottom() - 15.0f, 9.5f, 0.18f);
    rivet (g, r.x + 5.0f, r.cy(), 1.9f);
    rivet (g, r.right() - 5.0f, r.cy(), 1.9f);
}

//==============================================================================
/** Black phenolic skirted knob with a polished band and a spun-aluminium insert. */
template <class G>
void knob (G& g, float cx, float cy, float r, float angle, bool big, float wear = 0.0f, uint32_t seed = 1u)
{
    // ambient + contact shadow (light from the upper left)
    {
        const GradStop sh[] = { { 0.0f, 0x8c000000 }, { 0.78f, 0x60000000 }, { 0.90f, 0x22000000 }, { 1.0f, 0x00000000 } };
        const float ox = 0.06f * r, oy = 0.12f * r;
        g.setRadial (cx + ox, cy + oy, 1.22f * r, sh, 4);
        g.fillEllipse (cx + ox - 1.22f * r, cy + oy - 1.22f * r, 2.44f * r, 2.44f * r);
    }

    // skirt
    const GradStop skirt[] = { { 0.0f, 0xff36383b }, { 0.55f, 0xff141516 }, { 1.0f, 0xff060606 } };
    g.setRadial (cx - 0.35f * r, cy - 0.45f * r, 1.55f * r, skirt, 3);
    g.fillEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r);

    // knurled edge: fine ribs, lit on the upper left
    const int ribs = big ? 90 : 60;
    for (int i = 0; i < ribs; ++i)
    {
        const float a = 2.0f * kPi * (float) i / (float) ribs;
        const float lit = 0.5f + 0.5f * std::cos (a + 0.75f);
        g.setColour (withAlpha (0xffffffff, 0.015f + 0.075f * lit * lit));
        g.line (px (cx, r * 0.89f, a), py (cy, r * 0.89f, a), px (cx, r * 0.965f, a), py (cy, r * 0.965f, a), big ? 1.1f : 0.9f);
    }

    // polished metal band around the skirt
    {
        const GradStop band[] = { { 0.0f, 0xfff7f8fa }, { 0.3f, 0xffb4bac1 }, { 0.55f, 0xff50555b }, { 0.8f, 0xff8e949b }, { 1.0f, 0xffd5d9de } };
        g.setLinear (cx - r, cy - r, cx + r, cy + r, band, 5);
        const float br = r * 0.984f;
        g.strokeEllipse (cx - br, cy - br, 2.0f * br, 2.0f * br, big ? 1.9f : 1.3f);
    }

    // body
    const float t = r * 0.82f;
    const GradStop body[] = { { 0.0f, 0xff2d2f32 }, { 0.6f, 0xff151617 }, { 1.0f, 0xff0b0b0c } };
    g.setLinear (cx - 0.5f * t, cy - t, cx + 0.5f * t, cy + t, body, 3);
    g.fillEllipse (cx - t, cy - t, 2.0f * t, 2.0f * t);
    const GradStop sheen[] = { { 0.0f, 0x26ffffff }, { 1.0f, 0x00ffffff } };   // satin, not glossy
    g.setRadial (cx - 0.3f * t, cy - 0.55f * t, 0.85f * t, sheen, 2);
    g.fillEllipse (cx - t, cy - t, 2.0f * t, 2.0f * t);
    g.setColour (0x1cffffff);
    g.newPath(); g.arc (cx, cy, t, -2.0f, 0.2f, true); g.strokePath (0.9f);

    // painted pointer line (body + skirt)
    const float pw = big ? 3.0f : 2.3f;
    g.setColour (0x99000000);
    g.line (px (cx, r * 0.55f, angle) + 0.5f, py (cy, r * 0.55f, angle) + 0.8f,
            px (cx, r * 0.93f, angle) + 0.5f, py (cy, r * 0.93f, angle) + 0.8f, pw);
    g.setColour (0xffe9e4d8);
    g.line (px (cx, r * 0.55f, angle), py (cy, r * 0.55f, angle), px (cx, r * 0.93f, angle), py (cy, r * 0.93f, angle), pw);

    // spun-aluminium insert
    const float c = r * 0.44f;
    g.setColour (0xcc000000);
    g.fillEllipse (cx - c - 0.6f, cy - c + 0.3f, 2.0f * c + 1.2f, 2.0f * c + 1.4f);
    const GradStop base[] = { { 0.0f, 0xffd9dce0 }, { 1.0f, 0xff8c9096 } };
    g.setLinear (cx - c, cy - c, cx + c, cy + c, base, 2);
    g.fillEllipse (cx - c, cy - c, 2.0f * c, 2.0f * c);
    const int wedges = big ? 72 : 48;   // anisotropic "bow-tie" highlight of spun metal
    for (int i = 0; i < wedges; ++i)
    {
        const float a0 = 2.0f * kPi * (float) i / (float) wedges;
        const float a1 = 2.0f * kPi * (float) (i + 1) / (float) wedges + 0.01f;
        const float am = 0.5f * (a0 + a1);
        const float hl = std::pow (std::fabs (std::cos (am - 0.8f)), 6.0f);
        const float dk = std::pow (std::fabs (std::sin (am - 0.8f)), 8.0f);
        g.setColour (hl > dk ? withAlpha (0xffffffff, 0.42f * hl) : withAlpha (0xff000000, 0.22f * dk));
        g.newPath();
        g.moveTo (cx, cy);
        g.lineTo (px (cx, c, a0), py (cy, c, a0));
        g.lineTo (px (cx, c, a1), py (cy, c, a1));
        g.closePath();
        g.fillPath();
    }
    for (int i = 1; i <= (big ? 10 : 7); ++i)   // turning rings
    {
        const float rr = c * (float) i / (big ? 10.5f : 7.5f);
        g.setColour (withAlpha (i % 2 ? 0xffffffff : 0xff000000, i % 2 ? 0.07f : 0.05f));
        g.strokeEllipse (cx - rr, cy - rr, 2.0f * rr, 2.0f * rr, 0.6f);
    }
    g.setColour (0x99ffffff);   // chamfered edge
    g.newPath(); g.arc (cx, cy, c - 0.4f, -2.4f, 0.1f, true); g.strokePath (0.9f);
    g.setColour (0x99000000);
    g.newPath(); g.arc (cx, cy, c - 0.4f, 0.9f, 3.6f, true); g.strokePath (0.9f);

    if (wear > 0.0f)
    {
        WearRng rnd (seed);
        for (int i = 0; i < 4; ++i)   // paint chipped off the pointer
        {
            const float rr = r * rnd.range (0.58f, 0.9f), cr = rnd.range (0.45f, 1.1f) * (big ? 1.0f : 0.7f);
            g.setColour (withAlpha (0xff151617, rnd.range (0.55f, 0.9f) * wear));
            g.fillEllipse (px (cx, rr, angle) - cr + rnd.range (-0.5f, 0.5f), py (cy, rr, angle) - cr, 2.0f * cr, 2.0f * cr);
        }
        for (int i = 0; i < 10; ++i)   // the knurled rim polished by fingers
        {
            const float a0 = rnd.range (0.0f, 2.0f * kPi);
            g.setColour (withAlpha (0xffffffff, 0.05f * wear));
            g.newPath(); g.arc (cx, cy, r * 0.93f, a0, a0 + rnd.range (0.2f, 0.6f), true); g.strokePath (big ? 2.2f : 1.6f);
        }
        for (int i = 0; i < 9; ++i)    // swirl marks on the insert
        {
            const float a0 = rnd.range (0.0f, 2.0f * kPi), rr = c * rnd.range (0.3f, 0.92f);
            g.setColour (withAlpha (i % 3 == 0 ? 0xff000000 : 0xffffffff, 0.12f * wear));
            g.newPath(); g.arc (cx, cy, rr, a0, a0 + rnd.range (0.3f, 1.2f), true); g.strokePath (0.35f);
        }
    }
}

//==============================================================================
/** Square illuminated push button: frosted lens with a printed legend, incandescent lamp, chrome bezel. */
template <class G>
void litButton (G& g, Rect b, bool on, bool hover, bool down, const Palette& p, const char* label, const char* sub)
{
    if (down) b.y += 0.8f;
    const float rr = 2.2f, bw = 1.3f;   // a thin polished bezel

    softShadow (g, b, rr, 2.6f, 1.4f, p.lightPanel ? 0.32f : 0.5f);
    chromeFill (g, b, rr);
    g.setColour (0x66000000);
    g.strokeRoundRect (b.x, b.y, b.w, b.h, rr, 0.45f);
    const Rect l { b.x + bw, b.y + bw, b.w - 2.0f * bw, b.h - 2.0f * bw };
    g.setColour (0xf0060606);   // the hairline gap between bezel and lens
    g.fillRoundRect (l.x - 0.45f, l.y - 0.45f, l.w + 0.9f, l.h + 0.9f, rr - 0.6f);

    uint32_t lensMid = 0xff8f8a80;
    if (on)
    {
        const GradStop lens[] = { { 0.0f, p.lampHot }, { 0.65f, mix (p.lampHot, p.lampEdge, 0.65f) }, { 1.0f, mix (p.lampEdge, 0xff6b4a22, 0.35f) } };
        g.setRadial (l.cx(), l.cy() + 0.15f * l.h, 0.62f * std::max (l.w, l.h), lens, 3);
        g.fillRoundRect (l.x, l.y, l.w, l.h, rr - 1.0f);
        lensMid = mix (p.lampHot, p.lampEdge, 0.35f);
    }
    else
    {
        const GradStop lens[] = { { 0.0f, hover ? 0xffaaa498 : 0xff9b968b }, { 1.0f, hover ? 0xff79746b : 0xff6c675f } };
        g.setLinear (l.x, l.y, l.x, l.bottom(), lens, 2);
        g.fillRoundRect (l.x, l.y, l.w, l.h, rr - 1.0f);
        const GradStop inner[] = { { 0.0f, 0x00000000 }, { 1.0f, 0x30000000 } };   // dark lamp cavity behind the diffuser
        g.setRadial (l.cx(), l.cy(), 0.7f * std::max (l.w, l.h), inner, 2);
        g.fillRoundRect (l.x, l.y, l.w, l.h, rr - 1.0f);
        if (p.wear > 0.0f)
        {
            g.setColour (withAlpha (0xffc8a060, 0.12f * p.wear));   // yellowed diffuser
            g.fillRoundRect (l.x, l.y, l.w, l.h, rr - 1.0f);
        }
    }
    g.grain (l.x, l.y, l.w, l.h, 1, 77u, on ? 0.06f : 0.085f);   // frosted texture
    g.setColour (0x70000000);
    g.strokeRoundRect (l.x, l.y, l.w, l.h, rr - 1.0f, 0.8f);
    const GradStop gloss[] = { { 0.0f, 0x2affffff }, { 1.0f, 0x00ffffff } };
    g.setLinear (l.x, l.y, l.x, l.y + 0.45f * l.h, gloss, 2);
    g.fillRoundRect (l.x, l.y, l.w, 0.45f * l.h, rr - 1.0f);

    // printed legend
    const uint32_t legend = on ? 0xe6211a10 : 0xe0201d18;
    const bool hasSub = sub != nullptr && sub[0] != 0;
    const float main = hasSub ? std::fmin (15.0f, l.h * 0.44f) : std::fmin (15.0f, l.h * 0.62f);
    g.setColour (legend);
    if (hasSub)
    {
        g.text (label, l.x, l.y + l.h * 0.10f, l.w, l.h * 0.50f, main, 0, 0.07f);
        g.setColour (withAlpha (legend, 0.70f));
        g.text (sub, l.x, l.y + l.h * 0.55f, l.w, l.h * 0.34f, main * 0.66f, 0, 0.12f);
    }
    else
    {
        g.text (label, l.x, l.y, l.w, l.h, main, 0, 0.07f);
    }

    if (p.wear > 0.0f)   // rubbed legend, scuffed lens
    {
        WearRng rnd (hashText (label) ^ 0x5bd1e995u);
        for (int i = 0; i < 6; ++i)
        {
            const float rb = rnd.range (0.4f, 1.0f);
            g.setColour (withAlpha (lensMid, rnd.range (0.45f, 0.8f) * p.wear));
            g.fillEllipse (l.x + rnd.range (0.2f, 0.8f) * l.w - rb, l.y + rnd.range (0.25f, 0.75f) * l.h - rb, 2.0f * rb, 1.4f * rb);
        }
        g.save();
        g.clipRect (l.x, l.y, l.w, l.h);
        glassWear (g, l, hashText (label), 1.3f * p.wear);
        g.restore();
    }
}

//==============================================================================
/** Unlit push-button cap (white or black) in a chrome bezel. Pressed or latched, it sinks deeper and darkens. */
template <class G>
void capButton (G& g, Rect b, bool engaged, bool hover, bool down, const Palette& p, const char* label, const char* sub, bool white)
{
    const float rr = 2.2f, bw = 1.3f;   // a thin polished bezel
    const bool in = engaged || down;
    softShadow (g, b, rr, 2.6f, 1.4f, p.lightPanel ? 0.32f : 0.5f);
    chromeFill (g, b, rr);
    g.setColour (0x66000000);
    g.strokeRoundRect (b.x, b.y, b.w, b.h, rr, 0.45f);
    const Rect hole { b.x + bw, b.y + bw, b.w - 2.0f * bw, b.h - 2.0f * bw };
    g.setColour (0xff040404);
    g.fillRoundRect (hole.x, hole.y, hole.w, hole.h, rr - 1.0f);

    // the cap: proud when released; pressed in, it sits deeper and lower, in the bezel's shadow
    const float inset = in ? 1.4f : 0.5f;
    const Rect c { hole.x + inset, hole.y + inset + (in ? 0.8f : 0.0f), hole.w - 2.0f * inset, hole.h - 2.0f * inset - (in ? 0.4f : 0.0f) };
    uint32_t top = white ? 0xfff6f2e8 : 0xff3a3c40, bot = white ? 0xffd2cab8 : 0xff141517;
    if (white && p.wear > 0.0f) { top = mix (top, 0xffe8d7b0, 0.35f * p.wear); bot = mix (bot, 0xffc0aa80, 0.30f * p.wear); }   // aged plastic
    if (in)         { top = mix (top, 0xff000000, white ? 0.30f : 0.40f); bot = mix (bot, 0xff000000, white ? 0.30f : 0.40f); }
    else if (hover) { top = mix (top, 0xffffffff, 0.08f); bot = mix (bot, 0xffffffff, 0.06f); }
    const GradStop cap[] = { { 0.0f, top }, { 1.0f, bot } };
    g.setLinear (c.x, c.y, c.x + 0.25f * c.w, c.bottom(), cap, 2);
    g.fillRoundRect (c.x, c.y, c.w, c.h, 1.8f);
    g.grain (c.x, c.y, c.w, c.h, 1, 991u + hashText (label) % 97u, white ? 0.035f : 0.05f);

    // edges: lit top, shaded bottom; a pressed cap hardly catches any light
    g.setColour (withAlpha (0xffffffff, in ? 0.08f : (white ? 0.75f : 0.22f)));
    g.line (c.x + 1.8f, c.y + 0.6f, c.right() - 1.8f, c.y + 0.6f, 0.9f);
    g.setColour (withAlpha (0xff000000, white ? 0.28f : 0.55f));
    g.line (c.x + 1.8f, c.bottom() - 0.6f, c.right() - 1.8f, c.bottom() - 0.6f, 1.0f);
    if (in)   // the bezel throws a shadow onto the sunken cap
    {
        const GradStop sh[] = { { 0.0f, 0x99000000 }, { 1.0f, 0x00000000 } };
        g.setLinear (c.x, c.y, c.x, c.y + 0.4f * c.h, sh, 2);
        g.fillRoundRect (c.x, c.y, c.w, 0.4f * c.h, 1.8f);
        g.setLinear (c.x, c.y, c.x + 0.18f * c.w, c.y, sh, 2);
        g.fillRoundRect (c.x, c.y, 0.18f * c.w, c.h, 1.8f);
    }
    else      // released caps are slightly domed
    {
        const GradStop dome[] = { { 0.0f, withAlpha (0xffffffff, white ? 0.30f : 0.10f) }, { 1.0f, 0x00ffffff } };
        g.setRadial (c.x + 0.35f * c.w, c.y + 0.25f * c.h, 0.6f * c.w, dome, 2);
        g.fillRoundRect (c.x, c.y, c.w, c.h, 1.8f);
    }
    if (! white && p.wear > 0.0f)   // fingers have polished the middle of the black caps
    {
        const GradStop pol[] = { { 0.0f, withAlpha (0xffffffff, 0.10f * p.wear) }, { 1.0f, 0x00ffffff } };
        g.setRadial (c.cx(), c.cy(), 0.45f * c.w, pol, 2);
        g.fillRoundRect (c.x, c.y, c.w, c.h, 1.8f);
    }

    // printed legend
    const uint32_t legend = white ? (in ? 0xd8141414 : 0xe0181818) : (in ? 0xb0e3ddd0 : 0xe6ece7db);
    const bool hasSub = sub != nullptr && sub[0] != 0;
    const float main = hasSub ? std::fmin (14.0f, c.h * 0.44f) : std::fmin (14.0f, c.h * 0.56f);
    g.setColour (legend);
    if (hasSub)
    {
        g.text (label, c.x, c.y + c.h * 0.08f, c.w, c.h * 0.52f, main, 0, 0.07f);
        g.setColour (withAlpha (legend, 0.7f));
        g.text (sub, c.x, c.y + c.h * 0.56f, c.w, c.h * 0.36f, main * 0.7f, 0, 0.10f);
    }
    else
    {
        g.text (label, c.x, c.y, c.w, c.h, main, 0, 0.07f);
    }

    if (p.wear > 0.0f)   // legend rubbed, a few scuffs
    {
        WearRng rnd (hashText (label) ^ 0x9e3779b9u);
        const uint32_t base = mix (top, bot, 0.5f);
        for (int i = 0; i < 6; ++i)
        {
            const float rb = rnd.range (0.4f, 1.0f);
            g.setColour (withAlpha (base, rnd.range (0.5f, 0.85f) * p.wear));
            g.fillEllipse (c.x + rnd.range (0.2f, 0.8f) * c.w - rb, c.y + rnd.range (0.3f, 0.7f) * c.h - rb, 2.0f * rb, 1.4f * rb);
        }
        g.save();
        g.clipRect (c.x, c.y, c.w, c.h);
        glassWear (g, c, hashText (label), 0.8f * p.wear);
        g.restore();
    }
}

//==============================================================================
/** Brushed-metal pull tab that opens and closes the ADVANCED drawer. */
template <class G>
void drawerPull (G& g, Rect b, bool open, bool hover, bool down, const Palette& p)
{
    if (down) b.y += 0.6f;
    softShadow (g, b, 3.0f, 2.5f, 1.5f, p.lightPanel ? 0.3f : 0.5f);
    const GradStop plate[] = { { 0.0f, hover ? 0xfff6f7f9 : 0xffe9ecef }, { 0.5f, hover ? 0xffbcc2c9 : 0xffadb3ba }, { 1.0f, 0xff6c7279 } };
    g.setLinear (b.x, b.y, b.x, b.bottom(), plate, 3);
    g.fillRoundRect (b.x, b.y, b.w, b.h, 3.0f);
    g.grain (b.x, b.y, b.w, b.h, 0, 5150u, 0.10f);
    g.setColour (0x80ffffff);
    g.line (b.x + 3.0f, b.y + 0.5f, b.right() - 3.0f, b.y + 0.5f, 0.8f);
    g.setColour (0x70000000);
    g.strokeRoundRect (b.x, b.y, b.w, b.h, 3.0f, 0.7f);

    // engraved legend (dark cut with a lit lower lip) and an arrow
    const Rect t { b.x + 12.0f, b.y, b.w - 34.0f, b.h };
    g.setColour (0x60ffffff);
    g.text ("ADVANCED", t.x, t.y + 0.6f, t.w, t.h, 12.5f, 0, 0.20f);
    g.setColour (0xe0202326);
    g.text ("ADVANCED", t.x, t.y, t.w, t.h, 12.5f, 0, 0.20f);
    const float ax = b.right() - 15.0f, ay = b.cy();
    g.newPath();
    if (open) { g.moveTo (ax - 4.5f, ay + 2.5f); g.lineTo (ax + 4.5f, ay + 2.5f); g.lineTo (ax, ay - 3.0f); }
    else      { g.moveTo (ax - 4.5f, ay - 2.5f); g.lineTo (ax + 4.5f, ay - 2.5f); g.lineTo (ax, ay + 3.0f); }
    g.closePath();
    g.setColour (0xe0202326);
    g.fillPath();
    rivet (g, b.x + 6.0f, b.cy(), 1.8f);
    rivet (g, b.right() - 6.0f, b.cy(), 1.8f);
}

//==============================================================================
/** Seven-segment bits: a=1 top, b=2 upper right, c=4 lower right, d=8 bottom, e=16 lower left, f=32 upper left, g=64 middle. */
inline int segmentMask (char c)
{
    switch (c)
    {
        case '0': case 'O': return 0x3f;
        case '1': return 0x06;
        case '2': return 0x5b;
        case '3': return 0x4f;
        case '4': return 0x66;
        case '5': case 'S': return 0x6d;
        case '6': return 0x7d;
        case '7': return 0x07;
        case '8': return 0x7f;
        case '9': return 0x6f;
        case '-': return 0x40;
        case 'o': return 0x5c;
        case 'F': return 0x71;
        case 'E': return 0x79;
        case 'r': return 0x50;
        case 'n': return 0x54;
        case 'A': return 0x77;
        case 'b': return 0x7c;
        case 'C': return 0x39;
        case 'c': return 0x58;
        case 'd': return 0x5e;
        case 'H': return 0x76;
        case 'L': return 0x38;
        case 'P': return 0x73;
        case 't': return 0x78;
        case 'U': return 0x3e;
        case 'u': return 0x1c;
        case 'y': return 0x6e;
        default:  return 0;
    }
}

namespace detail
{
    /** Polygon in cell space (u right, v down), italicised like a real LED display. */
    template <class G>
    void segPoly (G& g, float x0, float y0, float h, float slant, const float* uv, int n)
    {
        g.newPath();
        for (int i = 0; i < n; ++i)
        {
            const float X = x0 + uv[2 * i] + slant * (h - uv[2 * i + 1]);
            const float Y = y0 + uv[2 * i + 1];
            if (i == 0) g.moveTo (X, Y); else g.lineTo (X, Y);
        }
        g.closePath();
    }

    inline float dieSpread (uint32_t k)   // fixed per-segment brightness spread of real LED dies, -1..1
    {
        k ^= k >> 16; k *= 0x7feb352du; k ^= k >> 15; k *= 0x846ca68bu; k ^= k >> 16;
        return (float) (k & 0xffffu) / 32767.5f - 1.0f;
    }
}

/** One seven-segment digit with decimal point: lit segments glow a little, unlit ones stay faintly visible.
    Real LED dies are never identical; on a well-used unit (wear) some segments have aged noticeably. */
template <class G>
void segmentCell (G& g, float x0, float y0, float w, float h, int mask, bool point, const Palette& p, float intensity, uint32_t seed)
{
    const float t = 0.13f * h, ht = 0.5f * t, gp = 0.03f * h, slant = 0.10f;
    const float L = ht, R = w - ht, T = ht, M = 0.5f * h, B = h - ht;

    float seg[7][12];
    auto hseg = [&] (int i, float c)
    {
        const float q[] = { L + gp, c, L + gp + ht, c - ht, R - gp - ht, c - ht, R - gp, c, R - gp - ht, c + ht, L + gp + ht, c + ht };
        for (int k = 0; k < 12; ++k) seg[i][k] = q[k];
    };
    auto vseg = [&] (int i, float m, float top, float bot)
    {
        const float q[] = { m, top + gp, m + ht, top + gp + ht, m + ht, bot - gp - ht, m, bot - gp, m - ht, bot - gp - ht, m - ht, top + gp + ht };
        for (int k = 0; k < 12; ++k) seg[i][k] = q[k];
    };
    hseg (0, T); vseg (1, R, T, M); vseg (2, R, M, B); hseg (3, B); vseg (4, L, M, B); vseg (5, L, T, M); hseg (6, M);

    const uint32_t ghost = withAlpha (mix (p.led, 0xff8a8a8a, 0.6f), 0.075f);
    const uint32_t core = mix (p.led, 0xffffffff, 0.18f);
    const float spread = 0.05f + 0.10f * p.wear;
    for (int i = 0; i < 7; ++i)
    {
        if ((mask & (1 << i)) == 0)
        {
            detail::segPoly (g, x0, y0, h, slant, seg[i], 6);
            g.setColour (ghost);
            g.fillPath();
            continue;
        }
        float a = intensity * (0.90f + spread * detail::dieSpread (seed * 8u + (uint32_t) i));
        if (p.wear > 0.0f && detail::dieSpread (seed * 13u + (uint32_t) i * 7u + 3u) < -0.82f)
            a *= 1.0f - 0.4f * p.wear;   // a tired segment
        // soft light spill through the filter: several faint, widening layers
        const float widths[] = { 3.2f, 2.3f, 1.5f, 0.8f };
        const float alphas[] = { 0.030f, 0.040f, 0.055f, 0.075f };
        for (int k = 0; k < 4; ++k)
        {
            detail::segPoly (g, x0, y0, h, slant, seg[i], 6);
            g.setColour (withAlpha (p.led, alphas[k] * a));
            g.strokePath (widths[k] * t);
        }
        detail::segPoly (g, x0, y0, h, slant, seg[i], 6);
        g.setColour (withAlpha (core, a));
        g.fillPath();
        // the middle of a lit segment is a little hotter than its edges
        const bool horiz = (i == 0 || i == 3 || i == 6);
        const float u0 = horiz ? seg[i][2] : seg[i][0], v0 = horiz ? seg[i][1] : seg[i][3];
        const float u1 = horiz ? seg[i][4] : seg[i][6], v1 = horiz ? seg[i][7] : seg[i][5];
        g.setColour (withAlpha (mix (core, 0xffffffff, 0.45f), 0.45f * a));
        g.line (x0 + u0 + slant * (h - v0), y0 + v0, x0 + u1 + slant * (h - v1), y0 + v1, 0.32f * t);
    }

    const float dx = x0 + w + 0.85f * t + slant * ht, dy = y0 + B, dr = 0.62f * t;
    if (point)
    {
        g.setColour (withAlpha (p.led, 0.10f * intensity));
        g.fillEllipse (dx - 2.0f * dr, dy - 2.0f * dr, 4.0f * dr, 4.0f * dr);
        g.setColour (withAlpha (core, 0.9f * intensity));
    }
    else
    {
        g.setColour (ghost);
    }
    g.fillEllipse (dx - dr, dy - dr, 2.0f * dr, 2.0f * dr);
}

/** Seven-segment LED read-out: separate digit modules behind a tinted filter glass in a polished bezel,
    the unit printed on the glass. The glass carries a little dust and a few hairlines (more on a worn unit).
    'value' may contain digits, '-', '.', and the letters segmentMask() knows; it is right-aligned in 'cells'. */
template <class G>
void ledReadout (G& g, Rect b, const char* value, const char* unit, const Palette& p, int cells,
                 bool unitLit = true, float intensity = 1.0f)
{
    const float r = 2.0f, bz = 1.25f;

    // thin polished bezel, slightly raised off the panel
    const Rect fr { b.x - bz, b.y - bz, b.w + 2.0f * bz, b.h + 2.0f * bz };
    softShadow (g, fr, r + bz, 1.6f, 1.0f, p.lightPanel ? 0.30f : 0.45f);
    chromeFill (g, fr, r + bz);
    g.setColour (0x66000000);
    g.strokeRoundRect (fr.x, fr.y, fr.w, fr.h, r + bz, 0.45f);
    g.setColour (0xf0000000);
    g.fillRoundRect (b.x - 0.5f, b.y - 0.5f, b.w + 1.0f, b.h + 1.0f, r + 0.4f);
    const GradStop bg[] = { { 0.0f, mix (p.ledFilter, 0xff000000, 0.45f) }, { 1.0f, p.ledFilter } };
    g.setLinear (b.x, b.y, b.x, b.bottom(), bg, 2);
    g.fillRoundRect (b.x, b.y, b.w, b.h, r);

    // digits
    const float h = 0.58f * b.h, w = 0.50f * h, pitch = w + 0.34f * h;
    const bool hasUnit = unit != nullptr && unit[0] != 0;
    const float unitSize = std::max (9.5f, 0.34f * b.h);
    int unitChars = 0;
    if (hasUnit)
        for (const char* u = unit; *u != 0; ++u)
            if ((*u & 0xc0) != 0x80) ++unitChars;
    const float unitW = hasUnit ? 0.52f * unitSize * (float) unitChars : 0.0f;
    const float gapU = hasUnit ? 0.30f * h : 0.0f;
    const float digitsW = pitch * (float) cells - (pitch - w);
    const float x0 = b.cx() - 0.5f * (digitsW + gapU + unitW);
    const float y0 = b.cy() - 0.5f * h;

    // each digit is its own little LED package, a touch darker than the filter
    for (int i = 0; i < cells; ++i)
    {
        const float mx = x0 + pitch * (float) i - 0.12f * h, my = y0 - 0.14f * h, mw = pitch - 0.06f * h;
        g.setColour (0xff070504);
        g.fillRoundRect (mx, my, mw, 1.28f * h, 0.6f);
        g.setColour (0x0dffffff);
        g.line (mx + 0.5f, my + 0.4f, mx + mw - 0.5f, my + 0.4f, 0.4f);
    }

    char ch[16];
    bool dp[16];
    int n = 0;
    for (const char* v = value; v != nullptr && *v != 0 && n < 16; ++v)
    {
        if (*v == '.')
        {
            if (n == 0) { ch[0] = ' '; dp[0] = false; n = 1; }
            dp[n - 1] = true;
            continue;
        }
        ch[n] = *v;
        dp[n] = false;
        ++n;
    }
    for (int i = 0; i < cells; ++i)
    {
        const int k = i - (cells - n);
        const int mask = k >= 0 ? segmentMask (ch[k]) : 0;
        const bool point = k >= 0 && dp[k];
        segmentCell (g, x0 + pitch * (float) i, y0, w, h, mask, point, p, intensity, (uint32_t) (i * 31 + cells * 7) + (uint32_t) (b.x * 3.0f));
    }

    // unit legend printed on the glass (partly rubbed off on a worn unit)
    if (hasUnit)
    {
        const Rect ur { x0 + digitsW + gapU, y0 + h - 0.95f * unitSize, unitW + 0.5f * unitSize, unitSize };
        g.setColour (unitLit ? 0x96d6cbb6 : 0x3cd6cbb6);
        g.text (unit, ur.x, ur.y, ur.w, ur.h, unitSize, -1, 0.04f);
        if (p.wear > 0.0f)
        {
            WearRng rnd (hashText (unit) + (uint32_t) b.x);
            for (int i = 0; i < 3; ++i)
            {
                const float rb = rnd.range (0.4f, 0.9f);
                g.setColour (withAlpha (p.ledFilter, 0.75f * p.wear));
                g.fillEllipse (ur.x + rnd.range (0.0f, 0.6f) * ur.w - rb, ur.y + rnd.range (0.3f, 0.8f) * ur.h - rb, 2.0f * rb, 1.5f * rb);
            }
        }
    }

    // the glass: reflection on the upper part, bezel shadow, dust and hairlines
    const GradStop glass[] = { { 0.0f, 0x1cffffff }, { 0.45f, 0x06ffffff }, { 0.46f, 0x00ffffff }, { 1.0f, 0x14000000 } };
    g.setLinear (b.x, b.y, b.x, b.bottom(), glass, 4);
    g.fillRoundRect (b.x, b.y, b.w, b.h, r);
    const GradStop inner[] = { { 0.0f, 0x88000000 }, { 1.0f, 0x00000000 } };
    g.setLinear (b.x, b.y, b.x, b.y + 0.22f * b.h, inner, 2);
    g.fillRoundRect (b.x, b.y, b.w, 0.22f * b.h, r);
    g.save();
    g.clipRect (b.x, b.y, b.w, b.h);
    glassWear (g, b, (uint32_t) (b.x * 7.0f + b.y * 13.0f), 0.7f + 1.3f * p.wear);
    g.restore();
}

/** Small caption printed under a read-out. */
template <class G>
void readoutCaption (G& g, const Palette& p, Rect r, const char* text)
{
    ink (g, p, text, r, 10.5f, 0, 0.16f, true);
}

//==============================================================================
/** Faceted jewel indicator lamp in a chrome holder (shows the mode). */
template <class G>
void jewel (G& g, float cx, float cy, float r, uint32_t colour)
{
    // light spilling onto the panel
    const GradStop glow[] = { { 0.0f, withAlpha (colour, 0.20f) }, { 0.5f, withAlpha (colour, 0.06f) }, { 1.0f, withAlpha (colour, 0.0f) } };
    g.setRadial (cx, cy, 2.0f * r, glow, 3);
    g.fillEllipse (cx - 2.0f * r, cy - 2.0f * r, 4.0f * r, 4.0f * r);

    // chrome holder: contact shadow, ring, dark gap
    g.setColour (0x66000000);
    g.fillEllipse (cx - 1.32f * r + 0.4f, cy - 1.32f * r + 0.9f, 2.64f * r, 2.64f * r);
    const GradStop holder[] = { { 0.0f, 0xffeff1f3 }, { 0.45f, 0xff9da2a8 }, { 0.55f, 0xff6c7076 }, { 1.0f, 0xff2e3134 } };
    g.setLinear (cx - r, cy - 1.3f * r, cx + r, cy + 1.3f * r, holder, 4);
    g.fillEllipse (cx - 1.3f * r, cy - 1.3f * r, 2.6f * r, 2.6f * r);
    g.setColour (0xe0050505);
    g.fillEllipse (cx - 1.04f * r, cy - 1.04f * r, 2.08f * r, 2.08f * r);

    // coloured glass, lit from behind by a small lamp: hot centre, deep edge
    const uint32_t deep = mix (colour, 0xff000000, 0.62f);
    const uint32_t hot  = mix (colour, 0xfffff4dc, 0.55f);
    const GradStop lens[] = { { 0.0f, hot }, { 0.35f, colour }, { 0.8f, mix (colour, deep, 0.6f) }, { 1.0f, deep } };
    g.setRadial (cx + 0.05f * r, cy + 0.08f * r, 1.0f * r, lens, 4);
    g.fillEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r);

    // bevel facets around a flat table
    const int facets = 12;
    const float tableR = 0.52f * r;
    for (int i = 0; i < facets; ++i)
    {
        const float a0 = 2.0f * kPi * (float) i / (float) facets, a1 = a0 + 2.0f * kPi / (float) facets;
        const float am = 0.5f * (a0 + a1);
        const float facing = std::cos (am + 0.8f);                 // > 0: faces the light
        const float alt = (i % 2 == 0) ? 1.0f : 0.55f;             // cut facets alternate
        g.setColour (facing > 0.0f ? withAlpha (0xffffffff, 0.16f * facing * alt)
                                   : withAlpha (0xff000000, 0.22f * -facing * alt));
        g.newPath();
        g.moveTo (px (cx, tableR, a0), py (cy, tableR, a0));
        g.lineTo (px (cx, r, a0), py (cy, r, a0));
        g.lineTo (px (cx, r, a1), py (cy, r, a1));
        g.lineTo (px (cx, tableR, a1), py (cy, tableR, a1));
        g.closePath();
        g.fillPath();
    }
    g.setColour (withAlpha (deep, 0.55f));
    g.strokeEllipse (cx - tableR, cy - tableR, 2.0f * tableR, 2.0f * tableR, 0.5f);

    // small sharp reflection of the room light + faint secondary
    g.setColour (0xd8ffffff);
    g.fillEllipse (cx - 0.55f * r, cy - 0.62f * r, 0.26f * r, 0.17f * r);
    g.setColour (0x40ffffff);
    g.fillEllipse (cx + 0.30f * r, cy + 0.42f * r, 0.18f * r, 0.10f * r);
}

//==============================================================================
/** VU meter geometry shared by the static and the moving parts. */
struct VuGeometry
{
    Rect z, f;
    float pivX, pivY, R, half;
    explicit VuGeometry (Rect b)
    {
        z = { b.x + 7.0f, b.y + 7.0f, b.w - 14.0f, b.h - 14.0f };
        f = { z.x + 9.0f, z.y + 9.0f, z.w - 18.0f, z.h - 18.0f };
        pivX = f.cx();
        pivY = f.y + f.h * 1.08f;
        R = f.h * 0.93f;
        half = 46.0f * kPi / 180.0f;
    }
    float angleFor (float deflection) const { return -half + 2.0f * half * deflection; }
};

/** Legend printed under "VU" for each meter source. */
inline const char* vuLegend (int meterIndex)
{
    switch (meterIndex)
    {
        case 0:  return "GAIN REDUCTION  dB";
        case 1:  return "OUTPUT   0 VU = -18 dBFS";
        case 2:  return "OUTPUT   0 VU = -14 dBFS";
        default: return "INPUT   0 VU = -18 dBFS";
    }
}

/** VU meter, static part: mounting plate, bezel, back-lit paper face and printing. Fills the whole rect.
    FAT/CLEAN: two lamps behind the lower corners. ANALOG: one bulb behind the middle of the scale. */
template <class G>
void vuStatic (G& g, Rect b, const Palette& p, int meterIndex)
{
    const VuGeometry geo (b);
    const Rect z = geo.z, f = geo.f;

    // cut-out in the panel: walls facing the light (bottom, right) catch it
    const GradStop plate[] = { { 0.0f, 0xff0a0b0c }, { 1.0f, 0xff16171a } };
    g.setLinear (b.x, b.y, b.x, b.bottom(), plate, 2);
    g.fillRect (b.x, b.y, b.w, b.h);
    g.setColour (p.lightPanel ? 0x58ffffff : 0x26ffffff);
    g.line (b.x, b.bottom() - 0.5f, b.right(), b.bottom() - 0.5f, 1.0f);
    g.setColour (p.lightPanel ? 0x30ffffff : 0x14ffffff);
    g.line (b.right() - 0.5f, b.y, b.right() - 0.5f, b.bottom(), 1.0f);
    for (int i = 0; i < 4; ++i)   // the meter's mounting screws
        screw (g, i % 2 == 0 ? b.x + 3.6f : b.right() - 3.6f, i < 2 ? b.y + 3.6f : b.bottom() - 3.6f, 2.3f);

    // matte black bezel and its shadow on the mounting plate
    softShadow (g, z, 6.0f, 3.0f, 1.5f, 0.5f);
    const GradStop bez[] = { { 0.0f, 0xff2c2e31 }, { 0.5f, 0xff18191b }, { 1.0f, 0xff0c0c0d } };
    g.setLinear (z.x, z.y, z.x, z.bottom(), bez, 3);
    g.fillRoundRect (z.x, z.y, z.w, z.h, 6.0f);
    g.grain (z.x, z.y, z.w, z.h, 1, 31u, 0.05f);
    g.setColour (0x2bffffff);
    g.strokeRoundRect (z.x + 0.5f, z.y + 0.5f, z.w - 1.0f, z.h - 1.0f, 6.0f, 1.0f);
    {   // thin polished trim around the window
        const GradStop trim[] = { { 0.0f, 0xffeef1f4 }, { 0.4f, 0xff9aa0a7 }, { 0.75f, 0xff464b51 }, { 1.0f, 0xffb6bcc3 } };
        g.setLinear (f.x, f.y, f.right(), f.bottom(), trim, 4);
        g.strokeRoundRect (f.x - 2.4f, f.y - 2.4f, f.w + 4.8f, f.h + 4.8f, 4.0f, 1.3f);
    }

    // paper face
    g.setColour (0xff000000);
    g.fillRoundRect (f.x - 1.0f, f.y - 1.0f, f.w + 2.0f, f.h + 2.0f, 3.0f);
    const GradStop paper[] = { { 0.0f, 0xfff4ebd3 }, { 1.0f, 0xffe6d9b8 } };
    g.setLinear (f.x, f.y, f.x, f.bottom(), paper, 2);
    g.fillRoundRect (f.x, f.y, f.w, f.h, 2.5f);
    g.grain (f.x, f.y, f.w, f.h, 2, 913u, 0.07f);

    g.save();
    g.clipRect (f.x, f.y, f.w, f.h);
    if (p.wear > 0.0f)   // paper yellowed with age, a few faint stains
    {
        g.setColour (withAlpha (0xffc89a50, 0.10f * p.wear));
        g.fillRect (f.x, f.y, f.w, f.h);
        WearRng rnd (777u);
        for (int i = 0; i < 3; ++i)
        {
            const float rr = rnd.range (12.0f, 30.0f), cx = f.x + rnd.range (0.1f, 0.9f) * f.w, cy = f.y + rnd.range (0.1f, 0.6f) * f.h;
            const GradStop st[] = { { 0.0f, withAlpha (0xff8a6a3a, 0.06f * p.wear) }, { 0.8f, withAlpha (0xff8a6a3a, 0.03f * p.wear) }, { 1.0f, 0x008a6a3a } };
            g.setRadial (cx, cy, rr, st, 3);
            g.fillEllipse (cx - rr, cy - rr, 2.0f * rr, 2.0f * rr);
        }
    }

    if (p.centerLamp)
    {
        // ANALOG: incandescent bulbs you can actually see. Unlit, the paper sits in the dark; two bulbs in the
        // lower lip of the window and one behind the middle of the scale each throw their own pool of light.
        g.setColour (0x553a1a02);
        g.fillRect (f.x, f.y, f.w, f.h);
        const uint32_t light = mix (p.meterLamp, 0xffffb050, 0.18f);   // incandescent light: warm amber
        const float bulbX[] = { f.x + 0.20f * f.w, f.x + 0.80f * f.w };
        for (const float bx : bulbX)
        {
            const float by = f.bottom() + 1.0f;
            const GradStop pool[] = { { 0.0f, withAlpha (light, 0.95f) }, { 0.18f, withAlpha (light, 0.72f) }, { 0.45f, withAlpha (light, 0.32f) },
                                      { 0.8f, withAlpha (light, 0.07f) }, { 1.0f, withAlpha (light, 0.0f) } };
            g.setRadial (bx, by, 0.62f * f.w, pool, 5);
            g.fillRect (f.x, f.y, f.w, f.h);
            const GradStop hot[] = { { 0.0f, 0xfffffbea }, { 0.35f, withAlpha (0xffffe6b4, 0.75f) }, { 1.0f, 0x00ffd890 } };
            g.setRadial (bx, by - 4.0f, 42.0f, hot, 3);
            g.fillEllipse (bx - 42.0f, by - 46.0f, 84.0f, 84.0f);
        }
        {   // the middle bulb, behind the scale: a broader, softer glow
            const float lx = f.cx(), ly = f.y + 0.62f * f.h;
            const GradStop mid[] = { { 0.0f, withAlpha (light, 0.62f) }, { 0.5f, withAlpha (light, 0.22f) }, { 1.0f, withAlpha (light, 0.0f) } };
            g.setRadial (lx, ly, 0.38f * f.w, mid, 3);
            g.fillRect (f.x, f.y, f.w, f.h);
            const GradStop core[] = { { 0.0f, withAlpha (0xfffff3dc, 0.35f) }, { 1.0f, 0x00fff3dc } };
            g.setRadial (lx, ly + 4.0f, 0.12f * f.w, core, 2);
            g.fillRect (f.x, f.y, f.w, f.h);
        }
        const GradStop vig[] = { { 0.0f, 0x00000000 }, { 0.65f, 0x00000000 }, { 1.0f, 0x50000000 } };
        g.setRadial (f.cx(), f.bottom(), 0.90f * f.w, vig, 3);
        g.fillRect (f.x, f.y, f.w, f.h);

        // the two lower bulbs themselves: small glass envelopes with a glowing filament, half hidden by the lip
        for (const float bx : bulbX)
        {
            const float by = f.bottom() - 1.0f;
            g.setColour (0x50000000);   // socket shadow
            g.fillEllipse (bx - 10.0f, by - 6.0f, 20.0f, 14.0f);
            const GradStop glass[] = { { 0.0f, 0xfffffbf0 }, { 0.45f, 0xffffe6b0 }, { 0.8f, withAlpha (0xffffc070, 0.85f) }, { 1.0f, withAlpha (0xffd08a40, 0.0f) } };
            g.setRadial (bx, by - 1.0f, 9.5f, glass, 4);
            g.fillEllipse (bx - 9.5f, by - 8.5f, 19.0f, 15.0f);
            g.setColour (0xffffffff);   // filament
            g.line (bx - 3.0f, by - 1.5f, bx + 3.0f, by - 1.5f, 1.0f);
            g.setColour (0x90ffffff);   // highlight on the glass
            g.fillEllipse (bx - 6.0f, by - 6.5f, 3.4f, 2.0f);
        }
    }
    else
    {
        for (int side = 0; side < 2; ++side)   // two lamps behind the lower corners
        {
            const float lx = side == 0 ? f.x + 0.18f * f.w : f.right() - 0.18f * f.w;
            const GradStop lamp[] = { { 0.0f, withAlpha (p.meterLamp, 0.62f) }, { 0.55f, withAlpha (p.meterLamp, 0.22f) }, { 1.0f, withAlpha (p.meterLamp, 0.0f) } };
            g.setRadial (lx, f.bottom() + 6.0f, 0.75f * f.w, lamp, 3);
            g.fillRect (f.x, f.y, f.w, f.h);
        }
        const GradStop vig[] = { { 0.0f, 0x00000000 }, { 0.7f, 0x14000000 }, { 1.0f, 0x45000000 } };
        g.setRadial (f.cx(), f.bottom() - 0.25f * f.h, 0.85f * f.w, vig, 3);
        g.fillRect (f.x, f.y, f.w, f.h);
    }

    // printing
    auto defl = [] (float vu) { return std::pow (10.0f, (vu - 3.0f) / 20.0f); };
    const uint32_t black = 0xf01b1915, red = 0xf0ad2e25;
    const float pivX = geo.pivX, pivY = geo.pivY, R = geo.R;

    g.setColour (black);
    g.newPath(); g.arc (pivX, pivY, R, geo.angleFor (defl (-20.0f)), geo.angleFor (defl (0.0f)), true); g.strokePath (1.3f);
    g.setColour (red);
    g.newPath(); g.arc (pivX, pivY, R + 2.0f, geo.angleFor (defl (0.0f)), geo.angleFor (1.0f), true); g.strokePath (4.0f);
    g.setColour (black);
    g.newPath(); g.arc (pivX, pivY, R - 22.0f, geo.angleFor (0.0f), geo.angleFor (1.0f), true); g.strokePath (0.8f);

    const int marks[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    for (int m : marks)
    {
        const float a = geo.angleFor (defl ((float) m));
        const bool major = (m == -20 || m == -10 || m == -7 || m == -5 || m == -3 || m == 0 || m == 3);
        g.setColour (m > 0 ? red : black);
        g.line (px (pivX, R, a), py (pivY, R, a), px (pivX, R + (major ? 9.0f : 6.0f), a), py (pivY, R + (major ? 9.0f : 6.0f), a), major ? 1.4f : 1.0f);
        char txt[8];
        int k = 0;
        const int v = m < 0 ? -m : m;
        if (m > 0) txt[k++] = '+';
        if (m < 0) txt[k++] = '-';
        if (v >= 10) txt[k++] = (char) ('0' + v / 10);
        txt[k++] = (char) ('0' + v % 10);
        txt[k] = 0;
        const float tr = R + 19.0f;
        g.text (txt, px (pivX, tr, a) - 15.0f, py (pivY, tr, a) - 8.0f, 30.0f, 16.0f, 14.0f, 0, 0.0f);
    }
    for (int i = 0; i <= 10; ++i)   // percent ticks on the inner arc
    {
        const float a = geo.angleFor ((float) i / 10.0f);
        g.setColour (black);
        g.line (px (pivX, R - 22.0f, a), py (pivY, R - 22.0f, a), px (pivX, R - 27.0f, a), py (pivY, R - 27.0f, a), 0.7f);
    }

    g.setColour (black);
    g.text ("VU", f.cx() - 40.0f, f.y + 0.52f * f.h, 80.0f, 26.0f, 27.0f, 0, 0.10f);
    g.setColour (0xc02e2a24);
    g.text (vuLegend (meterIndex), f.cx() - 120.0f, f.y + 0.69f * f.h, 240.0f, 13.0f, 11.5f, 0, 0.16f);
    g.restore();

    // the bezel casts a shadow onto the face (top + left)
    const GradStop topSh[] = { { 0.0f, 0x66000000 }, { 1.0f, 0x00000000 } };
    g.setLinear (f.x, f.y, f.x, f.y + 12.0f, topSh, 2);
    g.fillRect (f.x, f.y, f.w, 12.0f);
    g.setLinear (f.x, f.y, f.x + 8.0f, f.y, topSh, 2);
    g.fillRect (f.x, f.y, 8.0f, f.h);

}

/** VU meter, moving part: needle with its shadow and the pivot housing (the glass is vuGlass()). */
template <class G>
void vuDynamic (G& g, Rect b, float deflection)
{
    const VuGeometry geo (b);
    const Rect f = geo.f;
    g.save();
    g.clipRect (f.x, f.y, f.w, f.h);

    const float d = deflection < -0.03f ? -0.03f : (deflection > 1.15f ? 1.15f : deflection);
    const float a = geo.angleFor (d);
    const float r0 = f.h * 0.10f, r1 = geo.R + 7.0f;
    const float pivX = geo.pivX, pivY = geo.pivY;
    for (int i = 3; i >= 1; --i)   // soft shadow on the paper, away from the light
    {
        g.setColour (withAlpha (0xff000000, 0.06f));
        g.line (px (pivX, r0, a) + 2.5f, py (pivY, r0, a) + 3.0f, px (pivX, r1, a) + 2.5f, py (pivY, r1, a) + 3.0f, 1.0f + 1.4f * (float) i);
    }
    g.setColour (0xff111111);
    g.line (px (pivX, r0, a), py (pivY, r0, a), px (pivX, r1 - 30.0f, a), py (pivY, r1 - 30.0f, a), 1.9f);
    g.line (px (pivX, r1 - 30.0f, a), py (pivY, r1 - 30.0f, a), px (pivX, r1, a), py (pivY, r1, a), 1.2f);

    // moulded pivot housing (covers the needle root)
    const float hx = f.cx(), hy = f.bottom() + 4.0f, hr = 30.0f;
    const GradStop hsh[] = { { 0.0f, 0x55000000 }, { 0.8f, 0x22000000 }, { 1.0f, 0x00000000 } };
    g.setRadial (hx + 1.5f, hy + 1.0f, hr + 6.0f, hsh, 3);
    g.fillEllipse (hx + 1.5f - hr - 6.0f, hy + 1.0f - hr - 6.0f, 2.0f * (hr + 6.0f), 2.0f * (hr + 6.0f));
    const GradStop hous[] = { { 0.0f, 0xff34363a }, { 0.5f, 0xff1b1c1e }, { 1.0f, 0xff0b0b0c } };
    g.setLinear (hx - hr, hy - hr, hx + 0.4f * hr, hy, hous, 3);
    g.newPath(); g.arc (hx, hy, hr, -kPi * 0.5f, kPi * 0.5f, true); g.closePath(); g.fillPath();
    g.setColour (0x30ffffff);
    g.newPath(); g.arc (hx, hy, hr - 0.6f, -1.45f, 0.35f, true); g.strokePath (1.0f);
    g.restore();

}

/** VU meter cover glass, drawn above the needle: reflection, dust and hairlines. On a worn (ANALOG) unit
    the glass is properly scratched: cleaning swirls, scattered hairlines and two deeper marks. */
template <class G>
void vuGlass (G& g, Rect b, const Palette& p)
{
    const VuGeometry geo (b);
    const Rect f = geo.f;
    const GradStop refl[] = { { 0.0f, 0x2cffffff }, { 0.45f, 0x08ffffff }, { 1.0f, 0x00ffffff } };
    g.setLinear (f.x, f.y, f.x + 0.35f * f.w, f.y + 0.6f * f.h, refl, 3);
    g.fillRoundRect (f.x, f.y, f.w, f.h * 0.6f, 2.5f);
    g.setColour (0x0cffffff);
    g.newPath();
    g.moveTo (f.x + 0.62f * f.w, f.y);
    g.lineTo (f.x + 0.70f * f.w, f.y);
    g.lineTo (f.x + 0.48f * f.w, f.bottom());
    g.lineTo (f.x + 0.40f * f.w, f.bottom());
    g.closePath();
    g.fillPath();

    g.save();
    g.clipRect (f.x, f.y, f.w, f.h);
    glassWear (g, f, 4321u, 0.6f + 1.0f * p.wear);
    if (p.wear > 0.0f)
    {
        WearRng rnd (2468u);
        const float a = p.wear;
        for (int i = 0; i < 12; ++i)   // swirls from wiping the glass
        {
            const float cx = f.x + rnd.range (0.0f, 1.0f) * f.w, cy = f.y + rnd.range (0.2f, 1.4f) * f.h;
            const float r = rnd.range (40.0f, 150.0f), a0 = rnd.range (-1.2f, 1.2f);
            g.setColour (withAlpha (0xffffffff, a * rnd.range (0.08f, 0.16f)));
            g.newPath(); g.arc (cx, cy, r, a0, a0 + rnd.range (0.15f, 0.5f), true); g.strokePath (rnd.range (0.3f, 0.5f));
        }
        for (int i = 0; i < 28; ++i)   // scattered hairlines
            scratch (g, f.x + rnd.uni() * f.w, f.y + rnd.uni() * f.h, rnd.range (-kPi, kPi), rnd.range (6.0f, 70.0f),
                     rnd.range (-0.04f, 0.04f), rnd.range (0.3f, 0.5f), 0xffffffff, a * rnd.range (0.11f, 0.24f));
        for (int i = 0; i < 2; ++i)    // two deeper scratches that catch the light, each with its dark lower edge
        {
            const float x = f.x + rnd.range (0.1f, 0.6f) * f.w, y = f.y + rnd.range (0.15f, 0.75f) * f.h;
            const float ang = rnd.range (-0.5f, 0.5f), len = rnd.range (60.0f, 130.0f), bow = rnd.range (-0.03f, 0.03f);
            scratch (g, x + 0.3f, y + 0.6f, ang, len, bow, 0.7f, 0xff000000, 0.14f * a);
            scratch (g, x, y, ang, len, bow, 0.65f, 0xffffffff, 0.40f * a);
        }
    }
    g.restore();
}


//==============================================================================
/** Printed scale around a knob. */
template <class G>
void knobScale (G& g, const Palette& p, Rect k, const float* props, const char* const* labels, int n, int minorPerGap,
                float labelGap, float size)
{
    const float cx = k.cx(), cy = k.cy(), r = 0.5f * k.w;
    if (minorPerGap > 0)
        for (int i = 0; i + 1 < n; ++i)
            for (int j = 1; j <= minorPerGap; ++j)
            {
                const float t = props[i] + (props[i + 1] - props[i]) * (float) j / (float) (minorPerGap + 1);
                const float a = angleFor (t);
                g.setColour (withAlpha (p.inkDim, 0.75f));
                g.line (px (cx, r + 4.0f, a), py (cy, r + 4.0f, a), px (cx, r + 8.0f, a), py (cy, r + 8.0f, a), 1.0f);
            }
    for (int i = 0; i < n; ++i)
    {
        const float a = angleFor (props[i]);
        g.setColour (p.ink);
        g.line (px (cx, r + 3.0f, a), py (cy, r + 3.0f, a), px (cx, r + 10.0f, a), py (cy, r + 10.0f, a), 1.5f);
        const float tr = r + labelGap;
        ink (g, p, labels[i], { px (cx, tr, a) - 22.0f, py (cy, tr, a) - 8.0f, 44.0f, 16.0f }, size, 0, 0.02f, true);
    }
}

inline float hpfProportion (float hz) { return std::pow ((hz - 20.0f) / 480.0f, 0.45f); }

//==============================================================================
/** Colour of the bare faceplate at (x, y): base gradient, sheen and vignette (as panel() paints them). */
inline uint32_t panelColourAt (const Palette& p, float x, float y)
{
    auto lin = [x, y] (float x0, float y0, float x1, float y1)
    {
        const float dx = x1 - x0, dy = y1 - y0;
        return std::max (0.0f, std::min (1.0f, ((x - x0) * dx + (y - y0) * dy) / (dx * dx + dy * dy)));
    };
    uint32_t c = mix (p.panelA, p.panelB, lin (0.0f, 0.0f, 0.55f * kW, 1.25f * kMainH));
    const float ts = lin (0.15f * kW, 0.0f, 0.55f * kW, kMainH);
    const float sheenPeak = (p.lightPanel ? 38.0f : 12.0f) / 255.0f;
    c = mix (c, 0xffffffff, sheenPeak * (ts < 0.45f ? ts / 0.45f : (1.0f - ts) / 0.55f));
    const float d = std::hypot (x - 0.45f * kW, y - 0.42f * kMainH) / (0.62f * kW);
    const float va = d < 0.75f ? d / 0.75f * (16.0f / 255.0f) : (16.0f + (56.0f - 16.0f) * std::min (1.0f, (d - 0.75f) / 0.25f)) / 255.0f;
    return mix (c, 0xff000000, va);
}

/** A faceplate that has lived in a studio rack: hairline scratches, finish rubbed around the knobs,
    worn edges and rack-screw marks, grime in the corners, rubbed silkscreen. Only drawn in ANALOG. */
template <class G>
void panelWear (G& g, const Palette& p)
{
    const float a = p.wear;
    WearRng rnd (p.lightPanel ? 7771u : 4441u);
    const uint32_t bare  = p.lightPanel ? 0xfff6f8fa : 0xffc9cdd3;   // bare aluminium under the anodising
    const uint32_t grime = 0xff15120e;

    for (int i = 0; i < 150; ++i)   // fine scratches all over (cleaning, cables, other gear)
    {
        const float x = rnd.range (36.0f, kW - 36.0f), y = rnd.range (4.0f, kMainH - 4.0f);
        const bool horizontal = rnd.uni() < 0.6f;
        const float ang = horizontal ? rnd.range (-0.25f, 0.25f) : rnd.range (-kPi, kPi);
        const float len = rnd.uni() < 0.85f ? rnd.range (3.0f, 26.0f) : rnd.range (30.0f, 90.0f);
        const bool dark = p.lightPanel && rnd.uni() < 0.35f;
        scratch (g, x, y, ang, len, rnd.range (-0.04f, 0.04f), rnd.range (0.3f, 0.6f), dark ? grime : bare, a * rnd.range (0.05f, 0.16f));
    }
    for (int j = 0; j < 4; ++j)   // a few long, deeper ones
        scratch (g, rnd.range (120.0f, 760.0f), rnd.range (90.0f, 320.0f), rnd.range (-0.6f, 0.6f), rnd.range (70.0f, 150.0f),
                 rnd.range (-0.03f, 0.03f), 0.7f, bare, 0.2f * a);

    // finish rubbed around the knobs by fingers: faint polished halo + concentric marks
    const Rect knobs[] = { layout::input(), layout::output(), layout::attack(), layout::release() };
    for (const Rect& k : knobs)
    {
        const float kr = knobRadius (k.w);
        const GradStop halo[] = { { 0.0f, 0x00ffffff }, { 0.78f, 0x00ffffff }, { 0.9f, withAlpha (bare, 0.05f * a) }, { 1.0f, 0x00ffffff } };
        g.setRadial (k.cx(), k.cy(), kr * 1.32f, halo, 4);
        g.fillEllipse (k.cx() - kr * 1.32f, k.cy() - kr * 1.32f, kr * 2.64f, kr * 2.64f);
        for (int j = 0; j < 22; ++j)
        {
            const float rr = kr * rnd.range (1.02f, 1.22f);
            const float a0 = rnd.range (0.0f, 2.0f * kPi), len = rnd.range (0.15f, 0.8f);
            g.setColour (withAlpha (bare, a * rnd.range (0.02f, 0.06f)));
            g.newPath(); g.arc (k.cx(), k.cy(), rr, a0, a0 + len, true); g.strokePath (rnd.range (0.3f, 0.6f));
        }
    }

    // worn edges of the ears and marks around the rack screws
    for (int side = 0; side < 2; ++side)
    {
        const Rect e = side == 0 ? layout::leftEar() : layout::rightEar();
        const float edgeX = side == 0 ? 1.5f : kW - 1.5f;
        for (int j = 0; j < 40; ++j)
        {
            const float y = rnd.range (2.0f, kMainH - 2.0f), rr = rnd.range (0.4f, 1.6f);
            g.setColour (withAlpha (bare, a * rnd.range (0.10f, 0.30f)));
            g.fillEllipse (edgeX - rr + rnd.range (-1.0f, 1.0f), y - 2.0f * rr, 2.0f * rr, 4.0f * rr);
        }
        for (const float sy : { 50.0f, kMainH - 50.0f })
            for (int j = 0; j < 9; ++j)
            {
                const float rr = rnd.range (9.0f, 14.0f), a0 = rnd.range (0.0f, 2.0f * kPi);
                g.setColour (withAlpha (bare, a * rnd.range (0.05f, 0.12f)));
                g.newPath(); g.arc (e.cx(), sy + 4.0f, rr, a0, a0 + rnd.range (0.4f, 1.6f), true); g.strokePath (0.5f);
            }
    }
    for (int j = 0; j < 60; ++j)   // top and bottom edges
    {
        const bool topEdge = rnd.uni() < 0.5f;
        const float x = rnd.range (36.0f, kW - 36.0f), rr = rnd.range (0.4f, 1.4f);
        g.setColour (withAlpha (bare, a * rnd.range (0.08f, 0.22f)));
        g.fillEllipse (x - 2.5f * rr, (topEdge ? 1.0f : kMainH - 3.0f) - rr, 5.0f * rr, 2.0f * rr);
    }

    // grime in the corners and along the header rule
    const float grimeSpots[][3] = { { 40.0f, 6.0f, 22.0f }, { kW - 40.0f, 6.0f, 22.0f }, { 40.0f, kMainH - 6.0f, 22.0f }, { kW - 40.0f, kMainH - 6.0f, 22.0f },
                                    { 60.0f, 64.0f, 12.0f }, { kW - 60.0f, 64.0f, 12.0f } };
    for (auto& gs : grimeSpots)
    {
        const GradStop gr[] = { { 0.0f, withAlpha (grime, 0.22f * a) }, { 1.0f, 0x00000000 } };
        g.setRadial (gs[0], gs[1], gs[2], gr, 2);
        g.fillEllipse (gs[0] - gs[2], gs[1] - gs[2], 2.0f * gs[2], 2.0f * gs[2]);
    }

    // silkscreen rubbed off where hands go most: little patches of bare finish over the print
    auto rubInk = [&] (Rect r, int count)
    {
        for (int j = 0; j < count; ++j)
        {
            const float rr = rnd.range (0.5f, 1.5f);
            const float x = r.x + rnd.uni() * r.w, y = r.y + rnd.uni() * r.h;
            g.setColour (withAlpha (panelColourAt (p, x, y), a * rnd.range (0.55f, 0.9f)));
            g.fillEllipse (x - rr, y - 0.6f * rr, 2.0f * rr, 1.2f * rr);
        }
    };
    const Rect in = layout::input(), out = layout::output();
    rubInk ({ in.x + 30.0f, 74.0f, 68.0f, 16.0f }, 26);           // INPUT
    rubInk ({ out.x + 24.0f, 74.0f, 80.0f, 16.0f }, 26);          // OUTPUT
    rubInk ({ 806.0f, 66.0f, 88.0f, 14.0f }, 18);                 // ATTACK
    rubInk ({ 802.0f, 198.0f, 96.0f, 14.0f }, 18);                // RELEASE
    rubInk ({ in.x - 6.0f, in.bottom() - 18.0f, 30.0f, 14.0f }, 10);
    rubInk ({ out.right() - 18.0f, out.bottom() - 18.0f, 34.0f, 14.0f }, 10);
    rubInk ({ 60.0f, 14.0f, 250.0f, 34.0f }, 30);                 // logo

    for (const Rect& k : { in, out })   // fingerprint haze near the big knobs
    {
        const float r = 34.0f, cx = k.cx() + rnd.range (-60.0f, 60.0f), cy = k.cy() + rnd.range (40.0f, 70.0f);
        const GradStop sm[] = { { 0.0f, withAlpha (0xffffffff, 0.035f * a) }, { 1.0f, 0x00ffffff } };
        g.setRadial (cx, cy, r, sm, 2);
        g.fillEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r);
    }
}

//==============================================================================
/** The ADVANCED drawer: a black brushed plate on chrome slide rails below the faceplate. */
template <class G>
void drawerArt (G& g, const Palette& p)
{
    const float y0 = kMainH;
    g.setColour (0xff0a0a0b);   // the space under the faceplate
    g.fillRect (0.0f, y0, kW, kDrawerH);
    const Rect d = layout::drawer();

    for (const float rx : { d.x - 10.0f, d.right() + 2.0f })   // slide rails
    {
        const GradStop rail[] = { { 0.0f, 0xff5b6067 }, { 0.35f, 0xffd9dde2 }, { 0.6f, 0xff9097a0 }, { 1.0f, 0xff3b3f44 } };
        g.setLinear (rx, 0.0f, rx + 8.0f, 0.0f, rail, 4);
        g.fillRect (rx, y0, 8.0f, d.h + 4.0f);
        for (int i = 0; i < 3; ++i)
            hexScrew (g, rx + 4.0f, y0 + 28.0f + 52.0f * (float) i, 2.4f);
    }

    const GradStop sg[] = { { 0.0f, 0xff1d1e20 }, { 1.0f, 0xff121314 } };
    g.setLinear (d.x, d.y, d.x, d.bottom(), sg, 2);
    g.fillRoundRect (d.x, d.y, d.w, d.h, 5.0f);
    g.grain (d.x, d.y, d.w, d.h, 0, 99u, 0.065f);
    g.setColour (0x22ffffff);
    g.line (d.x + 5.0f, d.bottom() - 0.5f, d.right() - 5.0f, d.bottom() - 0.5f, 1.0f);
    for (int i = 0; i < 4; ++i)
        hexScrew (g, i % 2 == 0 ? d.x + 10.0f : d.right() - 10.0f, i < 2 ? d.y + 12.0f : d.bottom() - 10.0f, 3.4f);

    const GradStop topSh[] = { { 0.0f, 0xcc000000 }, { 1.0f, 0x00000000 } };   // the faceplate's shadow
    g.setLinear (0.0f, y0, 0.0f, y0 + 16.0f, topSh, 2);
    g.fillRect (0.0f, y0, kW, 16.0f);

    // the drawer is black on both revisions, so its legends use light ink
    Palette sp = p;
    sp.lightPanel = false;
    sp.ink = 0xebe9e3d6;
    sp.inkDim = 0xc4b9b2a3;

    ink (g, sp, "MODE",         { 64.0f, y0 + 12.0f, 246.0f, 14.0f }, 14.0f, 0, 0.18f);
    ink (g, sp, "REVISION",     { 346.0f, y0 + 12.0f, 134.0f, 14.0f }, 14.0f, 0, 0.18f);
    ink (g, sp, "OVERSAMPLING", { 512.0f, y0 + 12.0f, 294.0f, 14.0f }, 14.0f, 0, 0.18f);
    ink (g, sp, "DISPLAYS",     { 840.0f, y0 + 12.0f, 96.0f, 14.0f }, 14.0f, 0, 0.18f);
    for (const float sx : { 328.0f, 496.0f, 823.0f })   // engraved group separators
    {
        g.setColour (0x70000000);
        g.fillRect (sx, y0 + 14.0f, 1.0f, 56.0f);
        g.setColour (0x16ffffff);
        g.fillRect (sx + 1.0f, y0 + 14.0f, 1.0f, 56.0f);
    }
    g.setColour (0x60000000);
    g.fillRect (60.0f, y0 + 80.0f, kW - 120.0f, 1.0f);
    g.setColour (0x12ffffff);
    g.fillRect (60.0f, y0 + 81.0f, kW - 120.0f, 1.0f);

    const Rect mx = layout::mix(), hp = layout::hpf();
    ink (g, sp, "MIX",    { mx.x - 20.0f, y0 + 87.0f, mx.w + 40.0f, 12.0f }, 13.0f, 0, 0.16f);
    ink (g, sp, "SC HPF", { hp.x - 20.0f, y0 + 87.0f, hp.w + 40.0f, 12.0f }, 13.0f, 0, 0.16f);
    {
        const float pr[] = { 0.0f, 1.0f };
        const char* lb[] = { "DRY", "WET" };
        knobScale (g, sp, mx, pr, lb, 2, 0, 16.0f, 11.0f);
    }
    {
        const float pr[] = { 0.0f, 1.0f };
        const char* lb[] = { "OFF", "500" };
        knobScale (g, sp, hp, pr, lb, 2, 0, 16.0f, 11.0f);
    }
    ink (g, sp, "SWITCHES", { 404.0f, y0 + 89.0f, 284.0f, 12.0f }, 13.0f, 0, 0.16f);
    const char* tCaps[] = { "STEREO DETECTOR", "COLOUR ONLY", "HISS + HUM (ANALOG)" };
    for (int i = 0; i < 3; ++i)
    {
        const Rect t = layout::toggle (i);
        ink (g, sp, tCaps[i], { t.x - 10.0f, t.bottom() + 4.0f, t.w + 20.0f, 12.0f }, 10.5f, 0, 0.08f, true);
    }
    serialPlate (g, layout::plate());

    if (p.wear > 0.0f)   // the drawer gets handled too
    {
        WearRng rnd (9090u);
        for (int i = 0; i < 40; ++i)
            scratch (g, rnd.range (d.x + 8.0f, d.right() - 30.0f), rnd.range (d.y + 6.0f, d.bottom() - 6.0f), rnd.range (-0.3f, 0.3f),
                     rnd.range (4.0f, 30.0f), rnd.range (-0.04f, 0.04f), rnd.range (0.3f, 0.5f), 0xffc9cdd3, p.wear * rnd.range (0.04f, 0.12f));
    }
}

//==============================================================================
/** The static artwork: anodised faceplate (ears, handles, legends, scales) and the drawer below it. */
template <class G>
void panel (G& g, const Palette& p)
{
    const float H = kMainH;

    // ---- anodised body: light from the upper left
    const GradStop body[] = { { 0.0f, p.panelA }, { 1.0f, p.panelB } };
    g.setLinear (0.0f, 0.0f, 0.55f * kW, H * 1.25f, body, 2);
    g.fillRect (0.0f, 0.0f, kW, H);
    g.grain (0.0f, 0.0f, kW, H, 0, 4242u, p.grainStrength);
    const GradStop sheen[] = { { 0.0f, 0x00ffffff }, { 0.45f, p.lightPanel ? 0x26ffffffu : 0x0cffffffu }, { 1.0f, 0x00ffffff } };
    g.setLinear (0.15f * kW, 0.0f, 0.55f * kW, H, sheen, 3);
    g.fillRect (0.0f, 0.0f, kW, H);
    const GradStop vig[] = { { 0.0f, 0x00000000 }, { 0.75f, 0x10000000 }, { 1.0f, 0x38000000 } };
    g.setRadial (0.45f * kW, 0.42f * H, 0.62f * kW, vig, 3);
    g.fillRect (0.0f, 0.0f, kW, H);

    // ---- rack ears: same metal, a fine seam, slots, screws and a handle
    for (int side = 0; side < 2; ++side)
    {
        const Rect e = side == 0 ? layout::leftEar() : layout::rightEar();
        const float seamX = side == 0 ? e.right() : e.x;
        g.setColour (0x70000000);
        g.fillRect (seamX - 0.5f, 0.0f, 1.0f, H);
        g.setColour (p.lightPanel ? 0x60ffffff : 0x1affffff);
        g.fillRect (seamX + (side == 0 ? 0.5f : -1.5f), 0.0f, 1.0f, H);
        for (const float sy : { 50.0f, H - 50.0f })
        {
            g.setColour (0xff0b0b0c);                       // slot through the ear
            g.fillRoundRect (e.cx() - 7.0f, sy - 15.0f, 14.0f, 30.0f, 7.0f);
            const GradStop slot[] = { { 0.0f, 0xaa000000 }, { 1.0f, 0x00000000 } };
            g.setLinear (0.0f, sy - 15.0f, 0.0f, sy - 7.0f, slot, 2);
            g.fillRoundRect (e.cx() - 7.0f, sy - 15.0f, 14.0f, 30.0f, 7.0f);
            g.setColour (p.lightPanel ? 0x80ffffff : 0x30ffffff);
            g.newPath(); g.arc (e.cx(), sy + 8.0f, 7.0f, 1.6f, 4.7f, true); g.strokePath (0.8f);
            screw (g, e.cx(), sy + 4.0f, 5.5f);
        }
        rackHandle (g, e.cx(), 96.0f, H - 96.0f, p);
    }

    // ---- header
    ink (g, p, "TWINFET", { 56.0f, 12.0f, 260.0f, 40.0f }, 44.0f, -1, 0.10f);
    ink (g, p, "DUAL-REVISION FET LIMITING AMPLIFIER", { 58.0f, 48.0f, 360.0f, 14.0f }, 13.0f, -1, 0.20f, true);
    g.setColour (0x40000000);
    g.fillRect (54.0f, 64.0f, kW - 108.0f, 1.0f);
    g.setColour (p.lightPanel ? 0x60ffffff : 0x12ffffff);
    g.fillRect (54.0f, 65.0f, kW - 108.0f, 1.0f);

    // ---- meter section
    ink (g, p, "METER", { 54.0f, 298.0f, 54.0f, 26.0f }, 15.0f, -1, 0.14f);

    // ---- input / output
    const Rect in = layout::input(), out = layout::output();
    ink (g, p, "INPUT",  { in.x, 72.0f, in.w, 20.0f }, 20.0f, 0, 0.16f);
    ink (g, p, "OUTPUT", { out.x, 72.0f, out.w, 20.0f }, 20.0f, 0, 0.16f);
    {
        const float pr[] = { 0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f };
        const char* lb[] = { "-24", "-12", "0", "+12", "+24", "+36" };
        knobScale (g, p, in, pr, lb, 6, 3, 19.0f, 14.0f);
    }
    {
        const float pr[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
        const char* lb[] = { "-24", "-12", "0", "+12", "+24" };
        knobScale (g, p, out, pr, lb, 5, 3, 19.0f, 14.0f);
    }
    ink (g, p, "RATIO", { 404.0f, 298.0f, 56.0f, 26.0f }, 15.0f, 1, 0.14f);

    // ---- attack / release
    const Rect at = layout::attack(), re = layout::release();
    ink (g, p, "ATTACK",  { 790.0f, 66.0f, 120.0f, 14.0f }, 16.0f, 0, 0.16f);
    ink (g, p, "RELEASE", { 790.0f, 198.0f, 120.0f, 14.0f }, 16.0f, 0, 0.16f);
    {
        const float pr[] = { 0.0f, 1.0f / 6.0f, 2.0f / 6.0f, 0.5f, 4.0f / 6.0f, 5.0f / 6.0f, 1.0f };
        const char* lb[] = { "1", "2", "3", "4", "5", "6", "7" };
        knobScale (g, p, at, pr, lb, 7, 0, 15.0f, 13.0f);
        knobScale (g, p, re, pr, lb, 7, 0, 15.0f, 13.0f);
    }
    for (const Rect& k : { at, re })
    {
        const float rl = 0.707f * (0.5f * k.w + 15.0f);
        const float yy = k.cy() + rl - 6.0f;
        ink (g, p, "SLOW", { k.cx() - rl - 50.0f, yy, 40.0f, 12.0f }, 11.0f, 1, 0.10f, true);
        ink (g, p, "FAST", { k.cx() + rl + 10.0f, yy, 40.0f, 12.0f }, 11.0f, -1, 0.10f, true);
    }

    // ---- the faceplate's lower edge: a lit chamfer and its thickness
    g.setColour (p.lightPanel ? 0x70ffffff : 0x30ffffff);
    g.fillRect (0.0f, H - 2.0f, kW, 1.0f);
    g.setColour (0x90000000);
    g.fillRect (0.0f, H - 1.0f, kW, 1.0f);

    if (p.wear > 0.0f)
        panelWear (g, p);

    drawerArt (g, p);   // only visible while the drawer is open
}

} // namespace twinui

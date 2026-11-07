// TwinFET — panel artwork and layout (header only, no JUCE dependency).
//
// Everything is drawn in base units on a 1000 x 470 canvas. The drawing goes through a tiny
// "canvas" adapter G so exactly the same code renders inside the plugin (JUCE adapter in
// PluginEditor.cpp) and in the offline preview (cairo adapter in tests/ui_preview.cpp).
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
constexpr float kH = 470.0f;
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
    inline Rect leftEar()  { return { 0.0f, 0.0f, 34.0f, kH }; }
    inline Rect rightEar() { return { kW - 34.0f, 0.0f, 34.0f, kH }; }

    // header: MODE and REVISION selectors, mode lamp
    inline Rect jewel()            { return { 368.0f, 21.0f, 24.0f, 24.0f }; }
    inline Rect modeButton (int i) { return { 452.0f + 84.0f * (float) i, 14.0f, 78.0f, 38.0f }; }
    inline Rect revButton (int i)  { return { 818.0f + 68.0f * (float) i, 18.0f, 64.0f, 30.0f }; }

    // meter block
    inline Rect vu()       { return { 54.0f, 74.0f, 328.0f, 212.0f }; }
    inline Rect meterButton (int i) { return { 112.0f + 68.0f * (float) i, 298.0f, 62.0f, 26.0f }; }

    // input / output
    inline Rect input()    { return { 422.0f, 126.0f, 128.0f, 128.0f }; }
    inline Rect output()   { return { 596.0f, 126.0f, 128.0f, 128.0f }; }
    inline Rect inputReadout()  { return { 450.0f, 266.0f, 72.0f, 21.0f }; }
    inline Rect outputReadout() { return { 624.0f, 266.0f, 72.0f, 21.0f }; }
    inline Rect ratioButton (int i) { return { 466.0f + 56.0f * (float) i, 298.0f, 50.0f, 26.0f }; }

    // attack / release (stacked)
    inline Rect attack()   { return { 822.0f, 104.0f, 56.0f, 56.0f }; }
    inline Rect release()  { return { 822.0f, 236.0f, 56.0f, 56.0f }; }
    inline Rect attackReadout()  { return { 816.0f, 175.0f, 68.0f, 19.0f }; }
    inline Rect releaseReadout() { return { 816.0f, 307.0f, 68.0f, 19.0f }; }

    // utility strip
    inline Rect strip()    { return { 50.0f, 344.0f, 900.0f, 114.0f }; }
    inline Rect mix()      { return { 94.0f, 369.0f, 52.0f, 52.0f }; }
    inline Rect hpf()      { return { 204.0f, 369.0f, 52.0f, 52.0f }; }
    inline Rect mixReadout() { return { 88.0f, 435.0f, 64.0f, 19.0f }; }
    inline Rect hpfReadout() { return { 198.0f, 435.0f, 64.0f, 19.0f }; }
    inline Rect toggle (int i) { return { 314.0f + 100.0f * (float) i, 380.0f, 88.0f, 34.0f }; }  // LINK, COMP OFF, NOISE

    inline Rect grReadout()  { return { 636.0f, 372.0f, 140.0f, 40.0f }; }
    inline Rect outReadout() { return { 790.0f, 372.0f, 140.0f, 40.0f }; }
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
/** Panel material (revision) and lamp colours (mode). */
struct Palette
{
    bool lightPanel = false;
    uint32_t panelA = 0xff2a2c2f, panelB = 0xff17181a;     // anodised panel, upper left -> lower right
    float grainStrength = 0.075f;
    uint32_t ink = 0xebe9e3d6, inkDim = 0xc4b9b2a3;       // silkscreen (slightly translucent)
    uint32_t meterLamp = 0xffffd79a;                      // VU back-light (mode)
    uint32_t jewel = 0xffff3b2b;                          // mode lamp (mode)
    uint32_t lampHot = 0xfffff2d6, lampEdge = 0xffeab063; // lit push buttons (incandescent)
    uint32_t led = 0xffffcf8e;                            // seven-segment read-outs (pale amber LEDs)
    uint32_t ledFilter = 0xff120d09;                      // tinted filter window behind them
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
        default: p.meterLamp = 0xffffd27a; p.jewel = 0xffe8241a; break;   // ANALOG: classic incandescent, red jewel
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
/** Black phenolic skirted knob with a spun-aluminium insert. */
template <class G>
void knob (G& g, float cx, float cy, float r, float angle, bool big)
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
        g.line (px (cx, r * 0.905f, a), py (cy, r * 0.905f, a), px (cx, r * 0.99f, a), py (cy, r * 0.99f, a), big ? 1.1f : 0.9f);
    }
    // skirt chamfer: highlight upper left, shade lower right
    g.setColour (0x2effffff);
    g.newPath(); g.arc (cx, cy, r * 0.995f, -2.2f, 0.4f, true); g.strokePath (1.0f);
    g.setColour (0x66000000);
    g.newPath(); g.arc (cx, cy, r * 0.995f, 1.2f, 3.6f, true); g.strokePath (1.2f);

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
            px (cx, r * 0.97f, angle) + 0.5f, py (cy, r * 0.97f, angle) + 0.8f, pw);
    g.setColour (0xffe9e4d8);
    g.line (px (cx, r * 0.55f, angle), py (cy, r * 0.55f, angle), px (cx, r * 0.97f, angle), py (cy, r * 0.97f, angle), pw);

    // spun-aluminium insert
    const float c = r * 0.44f;
    g.setColour (0xcc000000);
    g.fillEllipse (cx - c - 0.6f, cy - c + 0.3f, 2.0f * c + 1.2f, 2.0f * c + 1.4f);
    const GradStop base[] = { { 0.0f, 0xffd9dce0 }, { 1.0f, 0xff8c9096 } };
    g.setLinear (cx - c, cy - c, cx + c, cy + c, base, 2);
    g.fillEllipse (cx - c, cy - c, 2.0f * c, 2.0f * c);
    // anisotropic "bow-tie" highlight of spun metal
    const int wedges = big ? 72 : 48;
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
    // turning rings
    for (int i = 1; i <= (big ? 10 : 7); ++i)
    {
        const float rr = c * (float) i / (big ? 10.5f : 7.5f);
        g.setColour (withAlpha (i % 2 ? 0xffffffff : 0xff000000, i % 2 ? 0.07f : 0.05f));
        g.strokeEllipse (cx - rr, cy - rr, 2.0f * rr, 2.0f * rr, 0.6f);
    }
    // chamfered edge
    g.setColour (0x99ffffff);
    g.newPath(); g.arc (cx, cy, c - 0.4f, -2.4f, 0.1f, true); g.strokePath (0.9f);
    g.setColour (0x99000000);
    g.newPath(); g.arc (cx, cy, c - 0.4f, 0.9f, 3.6f, true); g.strokePath (0.9f);
}

//==============================================================================
/** Square illuminated push button: frosted lens with a printed legend, incandescent lamp. */
template <class G>
void litButton (G& g, Rect b, bool on, bool hover, bool down, const Palette& p, const char* label, const char* sub)
{
    if (down) b.y += 0.8f;
    const float rr = 2.5f;

    softShadow (g, b, rr, 3.0f, 1.6f, p.lightPanel ? 0.35f : 0.55f);

    // bezel
    const GradStop bez[] = { { 0.0f, 0xff434548 }, { 0.5f, 0xff222325 }, { 1.0f, 0xff111213 } };
    g.setLinear (b.x, b.y, b.x, b.bottom(), bez, 3);
    g.fillRoundRect (b.x, b.y, b.w, b.h, rr);
    g.setColour (0x30ffffff);
    g.line (b.x + rr, b.y + 0.5f, b.right() - rr, b.y + 0.5f, 1.0f);

    // lens
    const Rect l { b.x + 2.5f, b.y + 2.5f, b.w - 5.0f, b.h - 5.0f };
    if (on)
    {
        const GradStop lens[] = { { 0.0f, p.lampHot }, { 0.65f, mix (p.lampHot, p.lampEdge, 0.65f) }, { 1.0f, mix (p.lampEdge, 0xff6b4a22, 0.35f) } };
        g.setRadial (l.cx(), l.cy() + 0.15f * l.h, 0.62f * std::max (l.w, l.h), lens, 3);
        g.fillRoundRect (l.x, l.y, l.w, l.h, rr - 1.0f);
    }
    else
    {
        const GradStop lens[] = { { 0.0f, hover ? 0xffaaa498 : 0xff9b968b }, { 1.0f, hover ? 0xff79746b : 0xff6c675f } };
        g.setLinear (l.x, l.y, l.x, l.bottom(), lens, 2);
        g.fillRoundRect (l.x, l.y, l.w, l.h, rr - 1.0f);
        const GradStop inner[] = { { 0.0f, 0x00000000 }, { 1.0f, 0x30000000 } };   // dark lamp cavity behind the diffuser
        g.setRadial (l.cx(), l.cy(), 0.7f * std::max (l.w, l.h), inner, 2);
        g.fillRoundRect (l.x, l.y, l.w, l.h, rr - 1.0f);
    }
    // frosted texture + inner edge
    g.grain (l.x, l.y, l.w, l.h, 1, 77u, on ? 0.06f : 0.085f);
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

/** One seven-segment digit with decimal point: lit segments glow a little, unlit ones stay faintly visible. */
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
    for (int i = 0; i < 7; ++i)
    {
        if ((mask & (1 << i)) == 0)
        {
            detail::segPoly (g, x0, y0, h, slant, seg[i], 6);
            g.setColour (ghost);
            g.fillPath();
            continue;
        }
        const float a = intensity * (0.90f + 0.05f * detail::dieSpread (seed * 8u + (uint32_t) i));
        // soft light spill through the filter: several faint, widening layers
        const float widths[] = { 3.2f, 2.3f, 1.5f, 0.8f };
        const float alphas[] = { 0.030f, 0.040f, 0.055f, 0.075f };
        for (int k = 0; k < 4; ++k)
        {
            detail::segPoly (g, x0, y0, h, slant, seg[i], 6);
            g.setColour (withAlpha (p.led, alphas[k] * intensity));
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

/** Seven-segment LED read-out behind a tinted filter window, with the unit printed on the filter.
    'value' may contain digits, '-', '.', and the letters segmentMask() knows; it is right-aligned in 'cells'. */
template <class G>
void ledReadout (G& g, Rect b, const char* value, const char* unit, const Palette& p, int cells,
                 bool unitLit = true, float intensity = 1.0f)
{
    const float r = 2.0f;

    // window cut into the panel: the lower edge catches the light, the cut itself is dark
    g.setColour (p.lightPanel ? 0x50ffffff : 0x18ffffff);
    g.fillRoundRect (b.x - 0.8f, b.y - 0.4f, b.w + 1.6f, b.h + 1.6f, r + 0.8f);
    g.setColour (0xf0000000);
    g.fillRoundRect (b.x - 0.8f, b.y - 0.8f, b.w + 1.6f, b.h + 1.4f, r + 0.8f);
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

    // unit legend printed on the filter
    if (hasUnit)
    {
        g.setColour (unitLit ? 0x96d6cbb6 : 0x3cd6cbb6);
        g.text (unit, x0 + digitsW + gapU, y0 + h - 0.95f * unitSize, unitW + 0.5f * unitSize, unitSize, unitSize, -1, 0.04f);
    }

    // filter glass: slight reflection on the upper part, darker lower part, bezel shadow
    const GradStop glass[] = { { 0.0f, 0x1cffffff }, { 0.45f, 0x06ffffff }, { 0.46f, 0x00ffffff }, { 1.0f, 0x14000000 } };
    g.setLinear (b.x, b.y, b.x, b.bottom(), glass, 4);
    g.fillRoundRect (b.x, b.y, b.w, b.h, r);
    const GradStop inner[] = { { 0.0f, 0x88000000 }, { 1.0f, 0x00000000 } };
    g.setLinear (b.x, b.y, b.x, b.y + 0.22f * b.h, inner, 2);
    g.fillRoundRect (b.x, b.y, b.w, 0.22f * b.h, r);
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

/** VU meter, static part: mounting cut-out, bezel, back-lit paper face and printing. Fills the whole rect. */
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

    // matte black bezel and its shadow on the mounting plate
    softShadow (g, z, 6.0f, 3.0f, 1.5f, 0.5f);
    const GradStop bez[] = { { 0.0f, 0xff2c2e31 }, { 0.5f, 0xff18191b }, { 1.0f, 0xff0c0c0d } };
    g.setLinear (z.x, z.y, z.x, z.bottom(), bez, 3);
    g.fillRoundRect (z.x, z.y, z.w, z.h, 6.0f);
    g.grain (z.x, z.y, z.w, z.h, 1, 31u, 0.05f);
    g.setColour (0x2bffffff);
    g.strokeRoundRect (z.x + 0.5f, z.y + 0.5f, z.w - 1.0f, z.h - 1.0f, 6.0f, 1.0f);

    // paper face
    g.setColour (0xff000000);
    g.fillRoundRect (f.x - 1.0f, f.y - 1.0f, f.w + 2.0f, f.h + 2.0f, 3.0f);
    const GradStop paper[] = { { 0.0f, 0xfff4ebd3 }, { 1.0f, 0xffe6d9b8 } };
    g.setLinear (f.x, f.y, f.x, f.bottom(), paper, 2);
    g.fillRoundRect (f.x, f.y, f.w, f.h, 2.5f);
    g.grain (f.x, f.y, f.w, f.h, 2, 913u, 0.07f);

    // back-light: two lamps behind the lower corners, coloured by the mode
    for (int side = 0; side < 2; ++side)
    {
        const float lx = side == 0 ? f.x + 0.18f * f.w : f.right() - 0.18f * f.w;
        const GradStop lamp[] = { { 0.0f, withAlpha (p.meterLamp, 0.62f) }, { 0.55f, withAlpha (p.meterLamp, 0.22f) }, { 1.0f, withAlpha (p.meterLamp, 0.0f) } };
        g.setRadial (lx, f.bottom() + 6.0f, 0.75f * f.w, lamp, 3);
        g.fillRect (f.x, f.y, f.w, f.h);
    }
    const GradStop vig[] = { { 0.0f, 0x00000000 }, { 0.7f, 0x14000000 }, { 1.0f, 0x45000000 } };
    g.setRadial (f.cx(), f.bottom() - 0.25f * f.h, 0.85f * f.w, vig, 3);
    g.fillRect (f.x, f.y, f.w, f.h);

    g.save();
    g.clipRect (f.x, f.y, f.w, f.h);

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

/** VU meter, moving part: needle with its shadow, pivot housing, glass. */
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

    // glass: soft reflection band and a faint streak
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
/** The static faceplate: anodised panel, ears, legends, scales, strip. */
template <class G>
void panel (G& g, const Palette& p)
{
    // ---- anodised body: light from the upper left
    const GradStop body[] = { { 0.0f, p.panelA }, { 1.0f, p.panelB } };
    g.setLinear (0.0f, 0.0f, 0.55f * kW, kH * 1.25f, body, 2);
    g.fillRect (0.0f, 0.0f, kW, kH);
    g.grain (0.0f, 0.0f, kW, kH, 0, 4242u, p.grainStrength);
    const GradStop sheen[] = { { 0.0f, 0x00ffffff }, { 0.45f, p.lightPanel ? 0x26ffffffu : 0x0cffffffu }, { 1.0f, 0x00ffffff } };
    g.setLinear (0.15f * kW, 0.0f, 0.55f * kW, kH, sheen, 3);
    g.fillRect (0.0f, 0.0f, kW, kH);
    const GradStop vig[] = { { 0.0f, 0x00000000 }, { 0.75f, 0x10000000 }, { 1.0f, 0x38000000 } };
    g.setRadial (0.45f * kW, 0.42f * kH, 0.62f * kW, vig, 3);
    g.fillRect (0.0f, 0.0f, kW, kH);

    // ---- rack ears: same metal, separated by a fine seam
    for (int side = 0; side < 2; ++side)
    {
        const Rect e = side == 0 ? layout::leftEar() : layout::rightEar();
        const float seamX = side == 0 ? e.right() : e.x;
        g.setColour (0x70000000);
        g.fillRect (seamX - 0.5f, 0.0f, 1.0f, kH);
        g.setColour (p.lightPanel ? 0x60ffffff : 0x1affffff);
        g.fillRect (seamX + (side == 0 ? 0.5f : -1.5f), 0.0f, 1.0f, kH);
        for (float sy : { 58.0f, kH - 58.0f })
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
    }

    // ---- header
    ink (g, p, "TWINFET", { 56.0f, 12.0f, 260.0f, 40.0f }, 44.0f, -1, 0.10f);
    ink (g, p, "DUAL-REVISION FET LIMITING AMPLIFIER", { 58.0f, 48.0f, 360.0f, 14.0f }, 13.0f, -1, 0.20f, true);
    ink (g, p, "MODE", { 392.0f, 14.0f, 52.0f, 38.0f }, 17.0f, 1, 0.18f);
    ink (g, p, "REVISION", { 712.0f, 18.0f, 98.0f, 30.0f }, 15.0f, 1, 0.18f);
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

    // ---- utility strip: a recessed black sub-panel
    const Rect s = layout::strip();
    g.setColour (p.lightPanel ? 0x70ffffff : 0x22ffffff);
    g.fillRoundRect (s.x, s.y + 1.2f, s.w, s.h, 6.0f);
    const GradStop sg[] = { { 0.0f, 0xff0e0f10 }, { 1.0f, 0xff1a1b1d } };
    g.setLinear (s.x, s.y, s.x, s.bottom(), sg, 2);
    g.fillRoundRect (s.x, s.y, s.w, s.h, 6.0f);
    g.grain (s.x, s.y, s.w, s.h, 0, 99u, 0.06f);
    const GradStop inner[] = { { 0.0f, 0x99000000 }, { 1.0f, 0x00000000 } };
    g.setLinear (s.x, s.y, s.x, s.y + 10.0f, inner, 2);
    g.fillRoundRect (s.x, s.y, s.w, 10.0f, 6.0f);

    // the strip is always black, so its legends use light ink
    Palette sp = p;
    sp.lightPanel = false;
    sp.ink = 0xebe9e3d6;
    sp.inkDim = 0xc4b9b2a3;

    const Rect mx = layout::mix(), hp = layout::hpf();
    ink (g, sp, "MIX",    { mx.x - 20.0f, 352.0f, mx.w + 40.0f, 14.0f }, 15.0f, 0, 0.16f);
    ink (g, sp, "SC HPF", { hp.x - 20.0f, 352.0f, hp.w + 40.0f, 14.0f }, 15.0f, 0, 0.16f);
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
    const char* tCaps[] = { "STEREO DETECTOR", "COLOUR ONLY", "HISS + HUM (ANALOG)" };
    for (int i = 0; i < 3; ++i)
    {
        const Rect t = layout::toggle (i);
        ink (g, sp, tCaps[i], { t.x - 10.0f, t.bottom() + 5.0f, t.w + 20.0f, 13.0f }, 11.5f, 0, 0.08f, true);
    }
    ink (g, sp, "SWITCHES", { 314.0f, 352.0f, 288.0f, 14.0f }, 15.0f, 0, 0.16f);

    const Rect gr = layout::grReadout(), ol = layout::outReadout();
    ink (g, sp, "GAIN REDUCTION", { gr.x, 352.0f, gr.w, 14.0f }, 13.0f, 0, 0.14f);
    ink (g, sp, "OUTPUT LEVEL",   { ol.x, 352.0f, ol.w, 14.0f }, 13.0f, 0, 0.14f);
    ink (g, sp, "MODEL TF-2", { ol.x, 422.0f, ol.w, 14.0f }, 11.0f, 1, 0.14f, true);
}

} // namespace twinui

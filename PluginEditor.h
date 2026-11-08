#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <tuple>

#include "PluginProcessor.h"
#include "TwinPanel.h"

//==============================================================================
/** Per-editor cache of the procedural textures (brushed metal, frosted lenses, meter paper). */
struct GrainCache
{
    juce::Image get (int kind, uint32_t seed, int pixelW, int pixelH, float strength, float pixelsPerUnit);

private:
    struct Key
    {
        int kind; uint32_t seed; int w, h, strength;
        bool operator< (const Key& o) const { return std::tie (kind, seed, w, h, strength) < std::tie (o.kind, o.seed, o.w, o.h, o.strength); }
    };
    std::map<Key, juce::Image> images;
};

//==============================================================================
/** Draws the knobs and push buttons with the shared panel artwork (TwinPanel.h). */
class TwinLook final : public juce::LookAndFeel_V4
{
public:
    TwinLook();

    float scale = 1.0f;                       // UI scale: pixels per base unit
    twinui::Palette palette = twinui::paletteFor (0, 2);
    juce::Typeface::Ptr face;                 // embedded panel font
    GrainCache grain;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override {}
};

//==============================================================================
/** Rotary knob whose mouse area is the knob itself (the component is padded for the shadow). */
class KnobSlider final : public juce::Slider
{
public:
    bool hitTest (int x, int y) override;
};

/** Lit push button; only the lens/bezel is clickable (the padding holds the shadow). */
class LitButton final : public juce::TextButton
{
public:
    bool hitTest (int x, int y) override;
    float pad = twinui::buttonPad;   // in pixels, updated on resize
};

/** Static faceplate, cached as an image (re-rendered only on resize or revision change). */
class PanelArt final : public juce::Component
{
public:
    explicit PanelArt (TwinLook& l) : look (l) { setInterceptsMouseClicks (false, false); setBufferedToImage (true); }
    void paint (juce::Graphics&) override;
private:
    TwinLook& look;
};

/** Faceted mode lamp with the mode's name printed next to it; the lamp colour follows the mode. */
class JewelLamp final : public juce::Component
{
public:
    explicit JewelLamp (TwinLook& l) : look (l) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    static constexpr float pad = 7.0f;   // base units around layout::jewel() for the glow
    juce::String modeName;
private:
    TwinLook& look;
};

/** Metal pull tab that opens and closes the ADVANCED drawer. */
class DrawerPull final : public juce::Button
{
public:
    explicit DrawerPull (TwinLook& l) : juce::Button ("advanced"), look (l) {}
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    bool open = false;
    static constexpr float pad = 5.0f;
private:
    TwinLook& look;
};

/** Seven-segment LED read-out; the unit is printed on its filter window. */
class Readout final : public juce::Component
{
public:
    Readout (TwinLook& l, int numCells, const char* unitUtf8, float brightness, const char* captionText = "")
        : look (l), cells (numCells), unit (juce::String::fromUTF8 (unitUtf8)), caption (captionText), intensity (brightness)
    {
        setInterceptsMouseClicks (false, false);
    }
    bool hasCaption() const { return caption.isNotEmpty(); }
    void setValue (const juce::String& v, bool unitOn = true)
    {
        if (v != value || unitOn != unitLit) { value = v; unitLit = unitOn; repaint(); }
    }
    void paint (juce::Graphics&) override;
    static constexpr float pad = 4.0f;   // base units around the window (room for its bezel and shadow)
private:
    TwinLook& look;
    int cells;
    juce::String unit, caption, value;
    bool unitLit = true;
    float intensity;
};

/** VU meter with second-order needle ballistics (~300 ms, slight overshoot). Opaque.
    The face is rendered once into an image; only the needle is drawn every frame. */
class VuMeter final : public juce::Component
{
public:
    explicit VuMeter (TwinLook& l) : look (l) { setOpaque (true); setInterceptsMouseClicks (false, false); }
    void setMeterIndex (int m)          { if (meterIndex != m) { meterIndex = m; repaint(); } }
    void tick (float target, float dt);  // target deflection 0..1 (1 = +3 VU)
    void paint (juce::Graphics&) override;
private:
    TwinLook& look;
    float pos = 0.708f, vel = 0.0f;
    int meterIndex = 0;

    juce::Image face, glassLayer;   // cached: everything under the needle / the cover glass above it
    int faceMeter = -1;
    uint32_t faceLamp = 0;
    bool faceLight = false, faceCentre = false;
    float faceWear = -1.0f;
};

//==============================================================================
class TwinFetEditor final : public juce::AudioProcessorEditor,
                            private juce::Timer
{
public:
    explicit TwinFetEditor (TwinFetProcessor&);
    ~TwinFetEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshSelectors();
    void setDrawerOpen (bool open);
    void setShowValues (bool show);
    void updateSizeLimits();
    void updateVisibility();
    void setupKnob (KnobSlider&, bool big, const juce::String& tooltip, Readout& readout,
                    std::function<void (Readout&, double)> show);
    juce::Rectangle<int> scaled (const twinui::Rect& r, float pad = 0.0f) const;

    TwinFetProcessor& proc;
    TwinLook look;
    float s = 1.0f;

    PanelArt panelArt { look };
    JewelLamp jewelLamp { look };
    DrawerPull drawerPull { look };
    KnobSlider inputKnob, outputKnob, attackKnob, releaseKnob, mixKnob, hpfKnob;
    Readout inputRead { look, 4, "dB", 0.85f }, outputRead { look, 4, "dB", 0.85f },
            attackRead { look, 3, "\xc2\xb5s", 0.85f }, releaseRead { look, 4, "ms", 0.85f },
            mixRead { look, 3, "%", 0.85f }, hpfRead { look, 3, "Hz", 0.85f },
            grRead { look, 4, "dB", 1.0f, "GAIN REDUCTION" }, outRead { look, 4, "VU", 1.0f, "OUTPUT LEVEL" };
    std::array<LitButton, 5> ratioButtons;
    std::array<LitButton, 3> modeButtons;
    std::array<LitButton, 2> revButtons;
    std::array<LitButton, 4> meterButtons;
    std::array<LitButton, 6> osButtons;
    LitButton linkButton, compOffButton, noiseButton, valuesButton;
    VuMeter vu { look };
    juce::TooltipWindow tooltipWindow { this, 650 };

    int ratioIndex = 0, modeIndex = 2, revIndex = 0, meterIndex = 0, osIndex = 0;
    int paintedRevision = -1, shownOsFactor = 0;
    float paintedWear = -1.0f;
    bool drawerOpen = false, showValues = true;
    int frame = 0;

    // attachments last: destroyed first
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> inputAtt, outputAtt, attackAtt, releaseAtt, mixAtt, hpfAtt;
    std::unique_ptr<ButtonAttachment> linkAtt, compOffAtt, noiseAtt;
    std::unique_ptr<juce::ParameterAttachment> ratioAtt, modeAtt, revAtt, meterAtt, osAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TwinFetEditor)
};

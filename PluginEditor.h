#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>

#include "PluginProcessor.h"

//==============================================================================
/** Custom look: dark hardware panel, metal-skirt knobs, push buttons with lamps. */
class TwinLook final : public juce::LookAndFeel_V4
{
public:
    TwinLook();

    float scale = 1.0f;
    juce::Colour accent { 0xfff2b33d };

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    static juce::Font font (float height, bool bold = false);
};

//==============================================================================
/** Analog VU meter with second-order needle ballistics (~300 ms, slight overshoot). */
class VuMeter final : public juce::Component
{
public:
    void setGainReductionMode (bool gr)          { if (grMode != gr) { grMode = gr; repaint(); } }
    void setReadout (const juce::String& text)   { readout = text; }
    void setAccent (juce::Colour c)              { accent = c; repaint(); }

    /** target: needle deflection 0..1 (1 = +3 VU). Call at a fixed rate. */
    void tick (float target, float dt);

    void paint (juce::Graphics&) override;

private:
    static float deflectionForVu (float vu) { return std::pow (10.0f, (vu - 3.0f) / 20.0f); }

    float pos = 0.708f, vel = 0.0f;
    bool grMode = true;
    juce::String readout;
    juce::Colour accent { 0xfff2b33d };
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
    void setupKnob (juce::Slider&, const char* paramId, bool big, const juce::String& tooltip);
    juce::Rectangle<float> sc (float x, float y, float w, float h) const;
    void drawKnobScale (juce::Graphics&, juce::Slider&, const juce::StringArray& labels,
                        const juce::Array<float>& values);

    TwinFetProcessor& proc;
    TwinLook look;
    float s = 1.0f;   // UI scale

    juce::Slider inputKnob, outputKnob, attackKnob, releaseKnob, mixKnob, hpfKnob;
    std::array<juce::TextButton, 5> ratioButtons;
    std::array<juce::TextButton, 3> modeButtons;
    std::array<juce::TextButton, 2> revButtons;
    std::array<juce::TextButton, 4> meterButtons;
    juce::ToggleButton linkToggle { "LINK" }, compOffToggle { "COMP OFF" }, noiseToggle { "NOISE" };
    VuMeter vu;
    juce::TooltipWindow tooltipWindow { this, 650 };

    int ratioIndex = 0, modeIndex = 2, revIndex = 0, meterIndex = 0;

    // attachments last: destroyed first
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> inputAtt, outputAtt, attackAtt, releaseAtt, mixAtt, hpfAtt;
    std::unique_ptr<ButtonAttachment> linkAtt, compOffAtt, noiseAtt;
    std::unique_ptr<juce::ParameterAttachment> ratioAtt, modeAtt, revAtt, meterAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TwinFetEditor)
};

#include "PluginEditor.h"

#include <cmath>

namespace
{
    constexpr float baseW = 940.0f;
    constexpr float baseH = 420.0f;

    const juce::Colour silver   { 0xffdcdcdc };
    const juce::Colour dimText  { 0xff8e939b };
    const juce::Colour amber    { 0xfff2b33d };
    const juce::Colour blueLamp { 0xff58a0ff };

    juce::String ua (const char* utf8) { return juce::String::fromUTF8 (utf8); }
}

//==============================================================================
TwinLook::TwinLook()
{
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xf01d1f23));
    setColour (juce::TooltipWindow::textColourId, silver);
    setColour (juce::TooltipWindow::outlineColourId, juce::Colour (0xff3a3d44));
}

juce::Font TwinLook::font (float height, bool bold)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

void TwinLook::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                 float startAngle, float endAngle, juce::Slider& slider)
{
    const auto area   = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
    const float size  = juce::jmin (area.getWidth(), area.getHeight());
    const auto r      = area.withSizeKeepingCentre (size, size);
    const auto c      = r.getCentre();
    const float rad   = size * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const bool big    = (bool) slider.getProperties().getWithDefault ("big", false);

    // value arc
    {
        juce::Path track, value;
        const float ar = rad * 0.97f, th = juce::jmax (1.5f, rad * 0.06f);
        track.addCentredArc (c.x, c.y, ar, ar, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colour (0xff2a2c31));
        g.strokePath (track, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        value.addCentredArc (c.x, c.y, ar, ar, 0.0f, startAngle, angle, true);
        g.setColour (accent.withAlpha (slider.isEnabled() ? 0.95f : 0.35f));
        g.strokePath (value, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // drop shadow
    const float kr = rad * (big ? 0.80f : 0.78f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (juce::Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (c.translated (0.0f, rad * 0.06f)));

    // metal skirt
    {
        juce::ColourGradient skirt (juce::Colour (0xffe9e9e9), c.x - kr, c.y - kr,
                                    juce::Colour (0xff6d6f73), c.x + kr, c.y + kr, false);
        skirt.addColour (0.5, juce::Colour (0xffb3b5b9));
        g.setGradientFill (skirt);
        g.fillEllipse (juce::Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (c));

        // knurled edge
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        const int teeth = big ? 48 : 32;
        for (int i = 0; i < teeth; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / (float) teeth;
            const juce::Point<float> p1 (c.x + kr * 0.93f * std::sin (a), c.y - kr * 0.93f * std::cos (a));
            const juce::Point<float> p2 (c.x + kr * std::sin (a), c.y - kr * std::cos (a));
            g.drawLine ({ p1, p2 }, juce::jmax (0.8f, kr * 0.025f));
        }
    }

    // black cap
    const float cr = kr * 0.78f;
    {
        juce::ColourGradient cap (juce::Colour (0xff3b3d42), c.x, c.y - cr,
                                  juce::Colour (0xff0d0e10), c.x, c.y + cr, false);
        g.setGradientFill (cap);
        g.fillEllipse (juce::Rectangle<float> (cr * 2.0f, cr * 2.0f).withCentre (c));
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawEllipse (juce::Rectangle<float> (cr * 2.0f, cr * 2.0f).withCentre (c).reduced (1.0f), 1.0f);
    }

    // pointer
    {
        const float r1 = cr * 0.25f, r2 = kr * 0.97f;
        const juce::Point<float> a1 (c.x + r1 * std::sin (angle), c.y - r1 * std::cos (angle));
        const juce::Point<float> a2 (c.x + r2 * std::sin (angle), c.y - r2 * std::cos (angle));
        g.setColour (juce::Colours::white);
        g.drawLine ({ a1, a2 }, juce::jmax (2.0f, kr * 0.075f));
    }
}

void TwinLook::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                     bool highlighted, bool down)
{
    const auto bounds = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on     = b.getToggleState();
    const float corner = bounds.getHeight() * 0.18f;
    const auto style  = b.getProperties().getWithDefault ("style", "plain").toString();

    if (style == "ratio")
    {
        // cream push button with a lamp when engaged
        juce::ColourGradient grad (on ? juce::Colour (0xfffff6dc) : juce::Colour (0xffcfcac0),
                                   bounds.getX(), bounds.getY(),
                                   on ? juce::Colour (0xffe9d9ae) : juce::Colour (0xff9c978d),
                                   bounds.getX(), bounds.getBottom(), false);
        if (down) grad.multiplyOpacity (0.85f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f * scale), corner);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, corner);
        if (on)
        {
            g.setColour (accent.withAlpha (0.35f));
            g.drawRoundedRectangle (bounds.expanded (1.5f), corner + 1.5f, 2.0f * scale);
        }
        if (highlighted)
        {
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.fillRoundedRectangle (bounds, corner);
        }
        return;
    }

    // segment / small buttons
    auto fill = on ? accent.withMultipliedBrightness (0.9f) : juce::Colour (0xff24262b);
    if (down) fill = fill.darker (0.2f);
    else if (highlighted) fill = fill.brighter (0.08f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 1.5f * scale), corner);
    g.setColour (fill);
    g.fillRoundedRectangle (bounds, corner);
    g.setColour (on ? accent.brighter (0.4f) : juce::Colour (0xff3c3f46));
    g.drawRoundedRectangle (bounds, corner, 1.0f);
}

void TwinLook::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool on    = b.getToggleState();
    const auto style = b.getProperties().getWithDefault ("style", "plain").toString();
    const float h     = (float) b.getHeight();

    if (style == "ratio")
        g.setColour (juce::Colour (0xff1b1b1b));
    else
        g.setColour (on ? juce::Colour (0xff111214) : silver);

    g.setFont (font (h * (style == "segment" ? 0.36f : 0.44f), true));
    g.drawText (b.getButtonText(), b.getLocalBounds().toFloat(), juce::Justification::centred, false);
}

void TwinLook::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto bounds = b.getLocalBounds().toFloat();
    const float led   = bounds.getHeight() * 0.42f;
    const auto ledArea = juce::Rectangle<float> (led, led).withCentre ({ bounds.getX() + led * 0.9f, bounds.getCentreY() });
    const bool on = b.getToggleState();

    if (on)
    {
        g.setColour (accent.withAlpha (0.35f));
        g.fillEllipse (ledArea.expanded (led * 0.45f));
    }
    g.setColour (on ? accent : juce::Colour (0xff3a2a1a).interpolatedWith (juce::Colour (0xff2a2c31), 0.6f));
    g.fillEllipse (ledArea);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (ledArea, 1.0f);

    g.setColour (highlighted ? juce::Colours::white : silver);
    g.setFont (font (bounds.getHeight() * 0.42f, true));
    g.drawText (b.getButtonText(), bounds.withTrimmedLeft (led * 2.0f), juce::Justification::centredLeft, false);
}

//==============================================================================
void VuMeter::tick (float target, float dt)
{
    // VU movement: ~300 ms to 99 %, ~1 % overshoot
    constexpr float omega = 19.0f, zeta = 0.80f;
    target = juce::jlimit (0.0f, 1.12f, target);
    const float accel = omega * omega * (target - pos) - 2.0f * zeta * omega * vel;
    vel += accel * dt;
    pos += vel * dt;
    if (pos < -0.03f) { pos = -0.03f; vel = 0.0f; }   // end stops
    if (pos > 1.15f)  { pos = 1.15f;  vel = 0.0f; }
    repaint();
}

void VuMeter::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float w = b.getWidth(), h = b.getHeight();
    const float u = w / 324.0f;   // design unit

    // bezel
    g.setColour (juce::Colour (0xff0b0b0c));
    g.fillRoundedRectangle (b, 8.0f * u);
    const auto face = b.reduced (7.0f * u);
    {
        juce::ColourGradient grad (juce::Colour (0xfff6ecd2), face.getCentreX(), face.getY(),
                                   juce::Colour (0xffe2d3ad), face.getCentreX(), face.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (face, 5.0f * u);
    }

    // geometry: pivot below the visible face
    const juce::Point<float> pivot (face.getCentreX(), face.getY() + h * 1.02f);
    const float rScale = h * 0.86f;
    const float halfSweep = juce::degreesToRadians (47.0f);
    auto angleFor = [halfSweep] (float d) { return -halfSweep + 2.0f * halfSweep * d; };
    auto pointAt  = [&pivot] (float ang, float radius)
    {
        return juce::Point<float> (pivot.x + radius * std::sin (ang), pivot.y - radius * std::cos (ang));
    };

    g.saveState();
    g.reduceClipRegion (face.toNearestInt());

    // arcs
    {
        juce::Path black, red;
        black.addCentredArc (pivot.x, pivot.y, rScale, rScale, 0.0f, angleFor (deflectionForVu (-20.0f)), angleFor (deflectionForVu (0.0f)), true);
        red.addCentredArc (pivot.x, pivot.y, rScale, rScale, 0.0f, angleFor (deflectionForVu (0.0f)), angleFor (1.0f), true);
        g.setColour (juce::Colour (0xff1c1a17));
        g.strokePath (black, juce::PathStrokeType (1.6f * u));
        g.setColour (juce::Colour (0xffc0362c));
        g.strokePath (red, juce::PathStrokeType (5.0f * u));
    }

    // ticks + numbers
    const int marks[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    g.setFont (TwinLook::font (11.5f * u, true));
    for (int m : marks)
    {
        const float a = angleFor (deflectionForVu ((float) m));
        const bool major = (m == -20 || m == -10 || m == -7 || m == -5 || m == -3 || m == 0 || m == 3);
        g.setColour (m > 0 ? juce::Colour (0xffc0362c) : juce::Colour (0xff1c1a17));
        g.drawLine ({ pointAt (a, rScale), pointAt (a, rScale + (major ? 9.0f : 6.0f) * u) }, (major ? 1.6f : 1.1f) * u);
        const juce::String txt = m > 0 ? "+" + juce::String (m) : juce::String (m);
        const auto tp = pointAt (a, rScale + 19.0f * u);
        g.drawText (txt, juce::Rectangle<float> (30.0f * u, 14.0f * u).withCentre (tp),
                    juce::Justification::centred, false);
    }

    // captions
    g.setColour (juce::Colour (0xff3b352b));
    g.setFont (TwinLook::font (20.0f * u, true));
    g.drawText ("VU", juce::Rectangle<float> (60.0f * u, 24.0f * u).withCentre ({ face.getCentreX(), face.getY() + face.getHeight() * 0.56f }),
                juce::Justification::centred, false);
    g.setFont (TwinLook::font (9.5f * u, true));
    g.drawText (grMode ? "GAIN REDUCTION" : "OUTPUT LEVEL",
                juce::Rectangle<float> (180.0f * u, 14.0f * u).withCentre ({ face.getCentreX(), face.getY() + face.getHeight() * 0.69f }),
                juce::Justification::centred, false);

    // readout
    g.setColour (juce::Colour (0xff5a5245));
    g.setFont (TwinLook::font (10.5f * u, false));
    g.drawText (readout, juce::Rectangle<float> (180.0f * u, 14.0f * u).withCentre ({ face.getCentreX(), face.getBottom() - 12.0f * u }),
                juce::Justification::centred, false);

    // needle
    {
        const float a = angleFor (pos);
        const auto tip = pointAt (a, rScale + 6.0f * u);
        const auto base = pointAt (a, h * 0.30f);
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.drawLine ({ base.translated (2.5f * u, 2.5f * u), tip.translated (2.5f * u, 2.5f * u) }, 1.8f * u);
        g.setColour (juce::Colour (0xff141414));
        g.drawLine ({ base, tip }, 1.6f * u);
    }

    // pivot cover
    {
        const auto cover = juce::Rectangle<float> (face.getX(), face.getBottom() - 26.0f * u, face.getWidth(), 26.0f * u);
        juce::ColourGradient cg (juce::Colour (0x00000000), cover.getX(), cover.getY(),
                                 juce::Colour (0x33000000), cover.getX(), cover.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRect (cover);
    }
    g.restoreState();

    // glass highlight
    {
        juce::ColourGradient glass (juce::Colours::white.withAlpha (0.20f), face.getX(), face.getY(),
                                    juce::Colours::white.withAlpha (0.0f), face.getX(), face.getCentreY(), false);
        g.setGradientFill (glass);
        g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.5f), 5.0f * u);
    }

    // backlight tint for the revision
    g.setColour (accent.withAlpha (0.05f));
    g.fillRoundedRectangle (face, 5.0f * u);
}

//==============================================================================
TwinFetEditor::TwinFetEditor (TwinFetProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);

    setupKnob (inputKnob,   pid::input,   true,  ua ("Вхід: наскільки сильно сигнал вдавлюється у фіксований поріг. Більше — більше компресії і кольору."));
    setupKnob (outputKnob,  pid::output,  true,  ua ("Вихід: компенсація гучності. У режимі ANALOG ще й драйвить вихідний підсилювач (більше — гарячіше)."));
    setupKnob (attackKnob,  pid::attack,  false, ua ("Атака 1…7. 7 — найшвидша (20 мкс), 1 — найповільніша (800 мкс)."));
    setupKnob (releaseKnob, pid::release, false, ua ("Відновлення 1…7. 7 — найшвидше (50 мс), 1 — 1,1 с. Після довгої компресії стає повільнішим, як у залізі."));
    setupKnob (mixKnob,     pid::mix,     false, ua ("Mix: паралельна компресія. Сухий і оброблений сигнали вирівняні по фазі."));
    setupKnob (hpfKnob,     pid::scHpf,   false, ua ("ФВЧ у сайдчейні: бочка й бас менше «качають» компресор. Off — вимкнено."));

    inputAtt   = std::make_unique<SliderAttachment> (proc.apvts, pid::input,   inputKnob);
    outputAtt  = std::make_unique<SliderAttachment> (proc.apvts, pid::output,  outputKnob);
    attackAtt  = std::make_unique<SliderAttachment> (proc.apvts, pid::attack,  attackKnob);
    releaseAtt = std::make_unique<SliderAttachment> (proc.apvts, pid::release, releaseKnob);
    mixAtt     = std::make_unique<SliderAttachment> (proc.apvts, pid::mix,     mixKnob);
    hpfAtt     = std::make_unique<SliderAttachment> (proc.apvts, pid::scHpf,   hpfKnob);

    // double click returns to the parameter default
    auto setDefault = [this] (juce::Slider& sl, const char* id)
    {
        if (auto* prm = proc.apvts.getParameter (id))
            sl.setDoubleClickReturnValue (true, (double) prm->convertFrom0to1 (prm->getDefaultValue()));
    };
    setDefault (inputKnob, pid::input);   setDefault (outputKnob, pid::output);
    setDefault (attackKnob, pid::attack); setDefault (releaseKnob, pid::release);
    setDefault (mixKnob, pid::mix);       setDefault (hpfKnob, pid::scHpf);

    // ratio buttons (vertical column, like the hardware)
    const char* ratioNames[] = { "4", "8", "12", "20", "ALL" };
    for (int i = 0; i < 5; ++i)
    {
        auto& b = ratioButtons[(size_t) i];
        b.setButtonText (ratioNames[i]);
        b.getProperties().set ("style", "ratio");
        b.setTooltip (i < 4 ? ua ("Ratio. Вищий ratio — вищий поріг, жорсткіше коліно.")
                            : ua ("ALL («всі кнопки»): ~16:1+, зміщення робочих точок, лаг атаки на транзієнтах, перекомпресія і багато бруду."));
        b.onClick = [this, i] { if (ratioAtt != nullptr) ratioAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }

    const char* modeNames[] = { "1  FAT", "2  CLEAN", "3  ANALOG" };
    const char* modeTips[]  = {
        "FAT: лінійна компресія + «роздування» звуку (щільність, гармоніки, вага) у дусі CLA-76. Чим сильніше тиснеш, тим товще.",
        "CLEAN: лінійна, точна компресія. BLACK — без жодного насичення. BLUE — м'яке, дороге насичення (трансформатори + клас А).",
        "ANALOG: нелінійна модель схеми — сайдчейн зі зворотним зв'язком, пульсації детектора на басу, FET, підсилювачі, трансформатори."
    };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = modeButtons[(size_t) i];
        b.setButtonText (modeNames[i]);
        b.getProperties().set ("style", "segment");
        b.setTooltip (ua (modeTips[i]));
        b.onClick = [this, i] { if (modeAtt != nullptr) modeAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }

    const char* revNames[] = { "BLACK", "BLUE" };
    const char* revTips[]  = { "BLACK — blackface (LN): чистіший, тихіший, класичний.",
                               "BLUE — bluestripe: FET-підсилювачі, більше гармонік і насичення, соковитіший." };
    for (int i = 0; i < 2; ++i)
    {
        auto& b = revButtons[(size_t) i];
        b.setButtonText (revNames[i]);
        b.getProperties().set ("style", "segment");
        b.setTooltip (ua (revTips[i]));
        b.onClick = [this, i] { if (revAtt != nullptr) revAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }

    const char* meterNames[] = { "GR", "+4", "+8", "IN" };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = meterButtons[(size_t) i];
        b.setButtonText (meterNames[i]);
        b.getProperties().set ("style", "small");
        b.onClick = [this, i] { if (meterAtt != nullptr) meterAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    meterButtons[0].setTooltip (ua ("Стрілка показує подавлення (GR)."));
    meterButtons[1].setTooltip (ua ("Вихідний рівень: 0 VU = −18 dBFS."));
    meterButtons[2].setTooltip (ua ("Вихідний рівень: 0 VU = −14 dBFS."));
    meterButtons[3].setTooltip (ua ("Вхідний рівень: 0 VU = −18 dBFS."));

    linkToggle.setTooltip (ua ("Стерео-лінк: обидва канали компресуються однаково."));
    compOffToggle.setTooltip (ua ("Компресію вимкнено, лишається тільки колір схеми (як «Attack Off» на залізі)."));
    noiseToggle.setTooltip (ua ("Шум і фон заліза (лише в режимі ANALOG)."));
    for (auto* t : { &linkToggle, &compOffToggle, &noiseToggle })
        addAndMakeVisible (*t);
    linkAtt    = std::make_unique<ButtonAttachment> (proc.apvts, pid::link,    linkToggle);
    compOffAtt = std::make_unique<ButtonAttachment> (proc.apvts, pid::compOff, compOffToggle);
    noiseAtt   = std::make_unique<ButtonAttachment> (proc.apvts, pid::noise,   noiseToggle);

    addAndMakeVisible (vu);

    ratioAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (pid::ratio),
                   [this] (float v) { ratioIndex = juce::roundToInt (v); refreshSelectors(); });
    modeAtt  = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (pid::mode),
                   [this] (float v) { modeIndex = juce::roundToInt (v); refreshSelectors(); });
    revAtt   = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (pid::revision),
                   [this] (float v) { revIndex = juce::roundToInt (v); refreshSelectors(); });
    meterAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (pid::meter),
                   [this] (float v) { meterIndex = juce::roundToInt (v); refreshSelectors(); });
    ratioAtt->sendInitialUpdate();
    modeAtt->sendInitialUpdate();
    revAtt->sendInitialUpdate();
    meterAtt->sendInitialUpdate();

    setResizable (true, true);
    setResizeLimits ((int) (baseW * 0.7f), (int) (baseH * 0.7f), (int) (baseW * 2.0f), (int) (baseH * 2.0f));
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio (baseW / baseH);
    setSize ((int) baseW, (int) baseH);

    startTimerHz (60);
}

TwinFetEditor::~TwinFetEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TwinFetEditor::setupKnob (juce::Slider& k, const char* paramId, bool big, const juce::String& tooltip)
{
    juce::ignoreUnused (paramId);
    k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    k.setMouseDragSensitivity (big ? 320 : 240);
    k.getProperties().set ("big", big);
    k.setTooltip (tooltip);
    k.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (k);
}

void TwinFetEditor::refreshSelectors()
{
    for (int i = 0; i < 5; ++i) ratioButtons[(size_t) i].setToggleState (i == ratioIndex, juce::dontSendNotification);
    for (int i = 0; i < 3; ++i) modeButtons[(size_t) i].setToggleState (i == modeIndex, juce::dontSendNotification);
    for (int i = 0; i < 2; ++i) revButtons[(size_t) i].setToggleState (i == revIndex, juce::dontSendNotification);
    for (int i = 0; i < 4; ++i) meterButtons[(size_t) i].setToggleState (i == meterIndex, juce::dontSendNotification);

    look.accent = revIndex == 1 ? blueLamp : amber;
    vu.setAccent (look.accent);
    vu.setGainReductionMode (meterIndex == 0);
    noiseToggle.setEnabled (modeIndex == 2);
    noiseToggle.setAlpha (modeIndex == 2 ? 1.0f : 0.4f);
    repaint();
}

juce::Rectangle<float> TwinFetEditor::sc (float x, float y, float w, float h) const
{
    return { x * s, y * s, w * s, h * s };
}

//==============================================================================
void TwinFetEditor::resized()
{
    s = (float) getWidth() / baseW;
    look.scale = s;

    inputKnob.setBounds   (sc (40.0f, 104.0f, 140.0f, 140.0f).toNearestInt());
    outputKnob.setBounds  (sc (210.0f, 104.0f, 140.0f, 140.0f).toNearestInt());
    attackKnob.setBounds  (sc (394.0f, 100.0f, 72.0f, 72.0f).toNearestInt());
    releaseKnob.setBounds (sc (394.0f, 214.0f, 72.0f, 72.0f).toNearestInt());

    for (int i = 0; i < 5; ++i)
        ratioButtons[(size_t) i].setBounds (sc (500.0f, 78.0f + 42.0f * (float) i, 62.0f, 32.0f).toNearestInt());

    vu.setBounds (sc (592.0f, 66.0f, 324.0f, 196.0f).toNearestInt());
    for (int i = 0; i < 4; ++i)
        meterButtons[(size_t) i].setBounds (sc (612.0f + 74.0f * (float) i, 272.0f, 64.0f, 26.0f).toNearestInt());

    revButtons[0].setBounds (sc (738.0f, 14.0f, 86.0f, 30.0f).toNearestInt());
    revButtons[1].setBounds (sc (830.0f, 14.0f, 86.0f, 30.0f).toNearestInt());

    for (int i = 0; i < 3; ++i)
        modeButtons[(size_t) i].setBounds (sc (24.0f + 142.0f * (float) i, 344.0f, 134.0f, 40.0f).toNearestInt());

    mixKnob.setBounds (sc (478.0f, 330.0f, 62.0f, 62.0f).toNearestInt());
    hpfKnob.setBounds (sc (566.0f, 330.0f, 62.0f, 62.0f).toNearestInt());

    linkToggle.setBounds    (sc (656.0f, 348.0f, 78.0f, 30.0f).toNearestInt());
    compOffToggle.setBounds (sc (738.0f, 348.0f, 98.0f, 30.0f).toNearestInt());
    noiseToggle.setBounds   (sc (840.0f, 348.0f, 84.0f, 30.0f).toNearestInt());
}

void TwinFetEditor::drawKnobScale (juce::Graphics& g, juce::Slider& k, const juce::StringArray& labels,
                                   const juce::Array<float>& values)
{
    const auto b = k.getBounds().toFloat();
    const auto c = b.getCentre();
    const float rad = b.getWidth() * 0.5f;
    const auto params = k.getRotaryParameters();
    g.setFont (TwinLook::font (juce::jmax (8.0f, 10.0f * s), true));
    for (int i = 0; i < values.size(); ++i)
    {
        const float prop = (float) k.valueToProportionOfLength ((double) values[i]);
        const float a = params.startAngleRadians + prop * (params.endAngleRadians - params.startAngleRadians);
        const juce::Point<float> t (c.x + (rad + 12.0f * s) * std::sin (a), c.y - (rad + 12.0f * s) * std::cos (a));
        g.setColour (dimText);
        g.drawText (labels[i], juce::Rectangle<float> (34.0f * s, 12.0f * s).withCentre (t), juce::Justification::centred, false);
    }
}

void TwinFetEditor::paint (juce::Graphics& g)
{
    const auto all = getLocalBounds().toFloat();
    const bool blue = revIndex == 1;

    // panel
    {
        juce::ColourGradient bg (juce::Colour (0xff1d1f22), 0.0f, 0.0f,
                                 juce::Colour (0xff0f1012), 0.0f, all.getBottom(), false);
        g.setGradientFill (bg);
        g.fillAll();
        // brushed texture
        g.setColour (juce::Colours::white.withAlpha (0.012f));
        for (float y = 0.0f; y < all.getHeight(); y += 3.0f * s)
            g.drawHorizontalLine ((int) y, 0.0f, all.getWidth());
    }

    // header
    g.setColour (juce::Colour (0xff0b0c0d));
    g.fillRect (sc (0.0f, 0.0f, baseW, 56.0f));
    g.setColour (silver);
    g.setFont (TwinLook::font (26.0f * s, true));
    g.drawText ("TWINFET", sc (24.0f, 8.0f, 200.0f, 30.0f), juce::Justification::centredLeft, false);
    g.setColour (dimText);
    g.setFont (TwinLook::font (10.5f * s, true));
    g.drawText (ua ("FET LIMITING AMPLIFIER  \xc2\xb7  BLACK / BLUE  \xc2\xb7  3 MODES"), sc (24.0f, 36.0f, 420.0f, 14.0f),
                juce::Justification::centredLeft, false);
    g.drawText ("REVISION", sc (660.0f, 14.0f, 70.0f, 30.0f), juce::Justification::centredRight, false);

    // blue stripe behind the meter for the BLUE revision
    if (blue)
    {
        juce::ColourGradient stripe (juce::Colour (0xff1f4f9e), 0.0f, 62.0f * s, juce::Colour (0xff143a78), 0.0f, 268.0f * s, false);
        g.setGradientFill (stripe);
        g.fillRoundedRectangle (sc (580.0f, 60.0f, 340.0f, 246.0f), 8.0f * s);
    }
    else
    {
        g.setColour (juce::Colour (0xff141518));
        g.fillRoundedRectangle (sc (580.0f, 60.0f, 340.0f, 246.0f), 8.0f * s);
    }

    // section labels
    auto label = [&] (const juce::String& t, juce::Rectangle<float> r, float size = 12.0f)
    {
        g.setColour (silver);
        g.setFont (TwinLook::font (size * s, true));
        g.drawText (t, r, juce::Justification::centred, false);
    };
    label ("INPUT",   sc (40.0f, 70.0f, 140.0f, 16.0f), 13.0f);
    label ("OUTPUT",  sc (210.0f, 70.0f, 140.0f, 16.0f), 13.0f);
    label ("ATTACK",  sc (390.0f, 64.0f, 80.0f, 14.0f), 11.0f);
    label ("RELEASE", sc (390.0f, 178.0f, 80.0f, 14.0f), 11.0f);
    label ("RATIO",   sc (496.0f, 60.0f, 70.0f, 14.0f), 11.0f);
    label ("MIX",     sc (478.0f, 318.0f, 62.0f, 12.0f), 10.0f);
    label ("SC HPF",  sc (566.0f, 318.0f, 62.0f, 12.0f), 10.0f);

    // knob scales
    drawKnobScale (g, inputKnob,  { "-24", "-12", "0", "+12", "+24", "+36" }, { -24.0f, -12.0f, 0.0f, 12.0f, 24.0f, 36.0f });
    drawKnobScale (g, outputKnob, { "-24", "-12", "0", "+12", "+24" },        { -24.0f, -12.0f, 0.0f, 12.0f, 24.0f });
    drawKnobScale (g, attackKnob,  { "1", "2", "3", "4", "5", "6", "7" }, { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f });
    drawKnobScale (g, releaseKnob, { "1", "2", "3", "4", "5", "6", "7" }, { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f });

    // value readouts under the big knobs
    g.setColour (silver.withAlpha (0.85f));
    g.setFont (TwinLook::font (12.0f * s, false));
    g.drawText (inputKnob.getTextFromValue (inputKnob.getValue()),   sc (40.0f, 262.0f, 140.0f, 16.0f), juce::Justification::centred, false);
    g.drawText (outputKnob.getTextFromValue (outputKnob.getValue()), sc (210.0f, 262.0f, 140.0f, 16.0f), juce::Justification::centred, false);
    g.setFont (TwinLook::font (9.5f * s, false));
    g.setColour (dimText);
    g.drawText (ua ("FAST 7 \xe2\x86\x92 SLOW 1"), sc (384.0f, 294.0f, 92.0f, 12.0f), juce::Justification::centred, false);
    g.setColour (silver.withAlpha (0.8f));
    g.drawText (mixKnob.getTextFromValue (mixKnob.getValue()), sc (470.0f, 395.0f, 78.0f, 12.0f), juce::Justification::centred, false);
    g.drawText (hpfKnob.getTextFromValue (hpfKnob.getValue()), sc (558.0f, 395.0f, 78.0f, 12.0f), juce::Justification::centred, false);

    // bottom strip
    g.setColour (juce::Colour (0xff0d0e10));
    g.fillRect (sc (0.0f, 312.0f, baseW, baseH - 312.0f));
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawHorizontalLine ((int) (312.0f * s), 0.0f, all.getWidth());
    g.setColour (dimText);
    g.setFont (TwinLook::font (10.0f * s, true));
    g.drawText ("MODE", sc (24.0f, 322.0f, 80.0f, 14.0f), juce::Justification::centredLeft, false);
    const char* captions[] = { "linear \xc2\xb7 inflate", "linear \xc2\xb7 precise", "nonlinear \xc2\xb7 circuit" };
    g.setFont (TwinLook::font (9.5f * s, false));
    for (int i = 0; i < 3; ++i)
        g.drawText (juce::String::fromUTF8 (captions[i]), sc (24.0f + 142.0f * (float) i, 386.0f, 134.0f, 14.0f),
                    juce::Justification::centred, false);

    // screws
    auto screw = [&] (float x, float y)
    {
        const auto r = sc (x - 6.0f, y - 6.0f, 12.0f, 12.0f);
        juce::ColourGradient sg (juce::Colour (0xffbdbdbd), r.getX(), r.getY(), juce::Colour (0xff4a4a4a), r.getRight(), r.getBottom(), false);
        g.setGradientFill (sg);
        g.fillEllipse (r);
        g.setColour (juce::Colour (0xff222222));
        g.drawLine (r.getX() + r.getWidth() * 0.2f, r.getCentreY(), r.getRight() - r.getWidth() * 0.2f, r.getCentreY(), 1.4f * s);
    };
    screw (12.0f, 70.0f);  screw (baseW - 10.0f, 70.0f);
    screw (12.0f, 300.0f); screw (baseW - 10.0f, 300.0f);
}

//==============================================================================
void TwinFetEditor::timerCallback()
{
    const float gr  = proc.meterGrDb.load (std::memory_order_relaxed);
    const float in  = proc.meterInMeanAbs.load (std::memory_order_relaxed);
    const float out = proc.meterOutMeanAbs.load (std::memory_order_relaxed);
    constexpr float halfPi = juce::MathConstants<float>::halfPi;

    float target = 0.0f;
    juce::String readout;
    if (meterIndex == 0)
    {
        // the GR needle rests on 0 VU and swings left
        target  = std::pow (10.0f, (-gr - 3.0f) / 20.0f);
        readout = "GR " + juce::String (gr, 1) + " dB";
    }
    else
    {
        const float meanAbs = meterIndex == 3 ? in : out;
        const float offset  = meterIndex == 2 ? 14.0f : 18.0f;
        // average-responding, sine calibrated: rectified mean * pi/2 = peak
        const float vuDb = 20.0f * std::log10 (juce::jmax (1.0e-7f, meanAbs * halfPi)) + offset;
        target  = std::pow (10.0f, (vuDb - 3.0f) / 20.0f);
        readout = (meterIndex == 3 ? "IN " : "OUT ") + juce::String (juce::jmax (-40.0f, vuDb), 1) + " VU";
    }
    vu.setReadout (readout);
    vu.tick (target, 1.0f / 60.0f);

    // keep the value readouts under the knobs fresh while automating
    repaint (sc (40.0f, 262.0f, 310.0f, 16.0f).toNearestInt());
    repaint (sc (470.0f, 395.0f, 166.0f, 12.0f).toNearestInt());
}

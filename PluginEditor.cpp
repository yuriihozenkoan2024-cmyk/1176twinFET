#include "PluginEditor.h"
#include "FontData.h"

#include <cmath>
#include <vector>

namespace
{
    juce::String ua (const char* utf8) { return juce::String::fromUTF8 (utf8); }

    /** twinui canvas adapter on top of juce::Graphics. Draws in base units (1000 wide). */
    class JuceCanvas
    {
    public:
        JuceCanvas (juce::Graphics& gr, TwinLook& look) : g (gr), face (look.face), cache (look.grain)
        {
            g.saveState();
            g.addTransform (juce::AffineTransform::scale (look.scale));
        }

        ~JuceCanvas() { g.restoreState(); }

        void save()    { g.saveState(); }
        void restore() { g.restoreState(); }

        void clipRect (float x, float y, float w, float h)
        {
            juce::Path p;
            p.addRectangle (x, y, w, h);
            g.reduceClipRegion (p);
        }

        void setColour (uint32_t c) { g.setColour (juce::Colour (c)); }

        void setLinear (float x1, float y1, float x2, float y2, const twinui::GradStop* st, int n)
        {
            g.setGradientFill (gradient (x1, y1, x2, y2, st, n, false));
        }

        void setRadial (float cx, float cy, float r, const twinui::GradStop* st, int n)
        {
            g.setGradientFill (gradient (cx, cy, cx + r, cy, st, n, true));
        }

        void fillRect (float x, float y, float w, float h)                         { g.fillRect (juce::Rectangle<float> (x, y, w, h)); }
        void fillRoundRect (float x, float y, float w, float h, float r)           { g.fillRoundedRectangle (x, y, w, h, r); }
        void strokeRoundRect (float x, float y, float w, float h, float r, float lw) { g.drawRoundedRectangle (x, y, w, h, r, lw); }
        void fillEllipse (float x, float y, float w, float h)                      { g.fillEllipse (x, y, w, h); }
        void strokeEllipse (float x, float y, float w, float h, float lw)          { g.drawEllipse (x, y, w, h, lw); }
        void line (float x1, float y1, float x2, float y2, float lw)               { g.drawLine (x1, y1, x2, y2, lw); }

        void newPath()                      { path.clear(); }
        void moveTo (float x, float y)      { path.startNewSubPath (x, y); }
        void lineTo (float x, float y)      { path.lineTo (x, y); }
        void arc (float cx, float cy, float r, float a0, float a1, bool newSub) { path.addCentredArc (cx, cy, r, r, 0.0f, a0, a1, newSub); }
        void closePath()                    { path.closeSubPath(); }
        void fillPath()                     { g.fillPath (path); }
        void strokePath (float lw)          { g.strokePath (path, juce::PathStrokeType (lw)); }

        /** Procedural texture at the device's real pixel density (so it stays crisp on HiDPI screens). */
        void grain (float x, float y, float w, float h, int kind, uint32_t seed, float strength)
        {
            float k = g.getInternalContext().getPhysicalPixelScaleFactor();   // pixels per base unit
            if (! (k > 0.05f)) k = 1.0f;
            constexpr float maxPixels = 4.0e6f;
            if (w * h * k * k > maxPixels)
                k = std::sqrt (maxPixels / (w * h));
            const int pw = juce::jlimit (1, 4096, juce::roundToInt (w * k));
            const int ph = juce::jlimit (1, 4096, juce::roundToInt (h * k));
            const juce::Image img = cache.get (kind, seed, pw, ph, strength, k);
            g.setColour (juce::Colours::black);   // images are drawn with the current opacity
            g.drawImage (img, juce::Rectangle<float> (x, y, w, h));
        }

        void text (const char* t, float x, float y, float w, float h, float size, int align, float tracking)
        {
            const juce::FontOptions opts = face != nullptr ? juce::FontOptions (face).withHeight (size)
                                                           : juce::FontOptions (size, juce::Font::bold);
            g.setFont (juce::Font (opts.withKerningFactor (tracking)));
            const juce::Justification just = align < 0 ? juce::Justification::centredLeft
                                           : (align > 0 ? juce::Justification::centredRight : juce::Justification::centred);
            g.drawText (juce::String::fromUTF8 (t), juce::Rectangle<float> (x, y, w, h), just, false);
        }

    private:
        static juce::ColourGradient gradient (float x1, float y1, float x2, float y2,
                                              const twinui::GradStop* st, int n, bool radial)
        {
            juce::ColourGradient grad (juce::Colour (st[0].argb), x1, y1, juce::Colour (st[n - 1].argb), x2, y2, radial);
            for (int i = 1; i < n - 1; ++i)
                grad.addColour ((double) st[i].pos, juce::Colour (st[i].argb));
            return grad;
        }

        juce::Graphics& g;
        juce::Typeface::Ptr face;
        GrainCache& cache;
        juce::Path path;
    };
}

//==============================================================================
juce::Image GrainCache::get (int kind, uint32_t seed, int w, int h, float strength, float pixelsPerUnit)
{
    const Key key { kind, seed, w, h, juce::roundToInt (strength * 1000.0f) };
    if (const auto it = images.find (key); it != images.end())
        return it->second;

    std::vector<float> v;
    twinui::makeGrain (v, w, h, kind, seed, pixelsPerUnit);

    juce::Image img (juce::Image::ARGB, w, h, false);
    {
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const float t = v[(size_t) y * (size_t) w + (size_t) x] * strength;
                const auto a = (juce::uint8) juce::jlimit (0, 255, juce::roundToInt (255.0f * std::abs (t)));
                auto* px = reinterpret_cast<juce::PixelARGB*> (bd.getPixelPointer (x, y));
                if (t > 0.0f) px->setARGB (a, a, a, a);   // premultiplied white
                else          px->setARGB (a, 0, 0, 0);   // black
            }
    }

    if ((size_t) w * (size_t) h <= 600000)   // the big panel texture lives in PanelArt's cached image instead
    {
        if (images.size() > 96)
            images.clear();
        images[key] = img;
    }
    return img;
}

//==============================================================================
TwinLook::TwinLook()
{
    face = juce::Typeface::createSystemTypefaceFor (twinfont::bigShouldersBold, twinfont::bigShouldersBoldSize);

    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xf01d1f23));
    setColour (juce::TooltipWindow::textColourId, juce::Colour (0xffece5d3));
    setColour (juce::TooltipWindow::outlineColourId, juce::Colour (0xff3a3d44));
}

void TwinLook::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                 float startAngle, float endAngle, juce::Slider& slider)
{
    JuceCanvas cv (g, *this);
    const float cx = ((float) x + 0.5f * (float) width) / scale;
    const float cy = ((float) y + 0.5f * (float) height) / scale;
    const float knobW = (float) juce::jmin (width, height) / scale - 2.0f * twinui::knobPad;
    const bool big = (bool) slider.getProperties().getWithDefault ("big", false);
    const auto seed = (uint32_t) (int) slider.getProperties().getWithDefault ("seed", 1);
    twinui::knob (cv, cx, cy, twinui::knobRadius (knobW), startAngle + sliderPos * (endAngle - startAngle), big, palette.wear, seed);
}

void TwinLook::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                     bool highlighted, bool down)
{
    JuceCanvas cv (g, *this);
    const float bw = (float) b.getWidth() / scale, bh = (float) b.getHeight() / scale;
    const float pad = twinui::buttonPad;
    const twinui::Rect r { pad, pad, bw - 2.0f * pad, bh - 2.0f * pad };
    const juce::String label = b.getButtonText();
    const juce::String sub = b.getProperties().getWithDefault ("sub", juce::String()).toString();
    const int style = (int) b.getProperties().getWithDefault ("style", 0);   // 0 lit lamp, 1 white cap, 2 black cap
    if (style == 0)
        twinui::litButton (cv, r, b.getToggleState(), highlighted && b.isEnabled(), down && b.isEnabled(),
                           palette, label.toRawUTF8(), sub.toRawUTF8());
    else
        twinui::capButton (cv, r, b.getToggleState(), highlighted && b.isEnabled(), down && b.isEnabled(),
                           palette, label.toRawUTF8(), sub.toRawUTF8(), style == 1);
}

//==============================================================================
bool KnobSlider::hitTest (int x, int y)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    const float r = 0.5f * (float) juce::jmin (getWidth(), getHeight());
    return c.getDistanceFrom (juce::Point<float> ((float) x, (float) y)) <= 0.82f * r;
}

bool LitButton::hitTest (int x, int y)
{
    return getLocalBounds().reduced (juce::roundToInt (pad)).contains (x, y);
}

void PanelArt::paint (juce::Graphics& g)
{
    JuceCanvas cv (g, look);
    twinui::panel (cv, look.palette);
}

void JewelLamp::paint (juce::Graphics& g)
{
    JuceCanvas cv (g, look);
    const auto j = twinui::layout::jewel(), n = twinui::layout::modeName();
    const float ox = j.x - pad, oy = j.y - pad;   // this component's origin in base units
    twinui::jewel (cv, j.cx() - ox, j.cy() - oy, 0.36f * j.w, look.palette.jewel);
    twinui::ink (cv, look.palette, modeName.toRawUTF8(), { n.x - ox, n.y - oy, n.w, n.h }, 18.0f, -1, 0.18f);
}

void DrawerPull::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    JuceCanvas cv (g, look);
    const float w = (float) getWidth() / look.scale, h = (float) getHeight() / look.scale;
    twinui::drawerPull (cv, { pad, pad, w - 2.0f * pad, h - 2.0f * pad }, open, highlighted, down, look.palette);
}

void Readout::paint (juce::Graphics& g)
{
    JuceCanvas cv (g, look);
    const float w = (float) getWidth() / look.scale, h = (float) getHeight() / look.scale;
    const float capH = hasCaption() ? twinui::layout::readoutCaptionH : 0.0f;
    const twinui::Rect r { pad, pad, w - 2.0f * pad, h - 2.0f * pad - capH };
    twinui::ledReadout (cv, r, value.toRawUTF8(), unit.toRawUTF8(), look.palette, cells, unitLit, intensity);
    if (hasCaption())
        twinui::readoutCaption (cv, look.palette, { r.x, r.bottom() + 3.0f, r.w, 12.0f }, caption.toRawUTF8());
}

void VuMeter::tick (float target, float dt)
{
    // VU movement: ~300 ms to 99 %, ~1 % overshoot
    constexpr float omega = 19.0f, zeta = 0.80f;
    target = juce::jlimit (0.0f, 1.12f, target);
    const float accel = omega * omega * (target - pos) - 2.0f * zeta * omega * vel;
    vel += accel * dt;
    pos += vel * dt;
    if (pos < -0.03f) { pos = -0.03f; vel = 0.0f; }
    if (pos > 1.15f)  { pos = 1.15f;  vel = 0.0f; }
    repaint();
}

void VuMeter::paint (juce::Graphics& g)
{
    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    const twinui::Rect r { 0.0f, 0.0f, (float) getWidth() / look.scale, (float) getHeight() / look.scale };

    // face: rendered at the screen's physical resolution, redone only when something on it changes
    const float k = juce::jmax (0.25f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int pw = juce::jmax (1, juce::roundToInt ((float) getWidth() * k));
    const int ph = juce::jmax (1, juce::roundToInt ((float) getHeight() * k));
    if (face.isNull() || face.getWidth() != pw || face.getHeight() != ph || faceMeter != meterIndex
        || faceLamp != look.palette.meterLamp || faceLight != look.palette.lightPanel
        || faceCentre != look.palette.centerLamp || ! juce::approximatelyEqual (faceWear, look.palette.wear))
    {
        face = juce::Image (juce::Image::ARGB, pw, ph, true);
        juce::Graphics fg (face);
        fg.addTransform (juce::AffineTransform::scale ((float) pw / (float) getWidth(), (float) ph / (float) getHeight()));
        {
            JuceCanvas fc (fg, look);
            twinui::vuStatic (fc, r, look.palette, meterIndex);
        }
        glassLayer = juce::Image (juce::Image::ARGB, pw, ph, true);
        juce::Graphics gg (glassLayer);
        gg.addTransform (juce::AffineTransform::scale ((float) pw / (float) getWidth(), (float) ph / (float) getHeight()));
        JuceCanvas gc (gg, look);
        twinui::vuGlass (gc, r, look.palette);
        faceMeter = meterIndex;
        faceLamp = look.palette.meterLamp;
        faceLight = look.palette.lightPanel;
        faceCentre = look.palette.centerLamp;
        faceWear = look.palette.wear;
    }
    g.setColour (juce::Colours::black);
    g.drawImage (face, getLocalBounds().toFloat());
    {
        JuceCanvas cv (g, look);
        twinui::vuDynamic (cv, r, pos);
    }
    g.setColour (juce::Colours::black);
    g.drawImage (glassLayer, getLocalBounds().toFloat());
}

//==============================================================================
TwinFetEditor::TwinFetEditor (TwinFetProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);
    addAndMakeVisible (panelArt);
    addAndMakeVisible (jewelLamp);
    drawerOpen = (bool) proc.apvts.state.getProperty (uiprop::advancedOpen, false);
    showValues = (bool) proc.apvts.state.getProperty (uiprop::showValues, true);

    for (auto* r : { &inputRead, &outputRead, &attackRead, &releaseRead, &mixRead, &hpfRead, &grRead, &outRead })
        addAndMakeVisible (*r);
    grRead.setValue ("0.0");
    outRead.setValue ("---");

    // what each knob shows on its seven-segment read-out (same mappings as the parameter texts)
    auto showDb      = [] (Readout& r, double v) { r.setValue (juce::String (v, 1)); };
    auto showAttack  = [] (Readout& r, double v) { r.setValue (juce::String (juce::roundToInt (800.0 * std::pow (20.0 / 800.0, (v - 1.0) / 6.0)))); };
    auto showRelease = [] (Readout& r, double v) { r.setValue (juce::String (juce::roundToInt (1100.0 * std::pow (50.0 / 1100.0, (v - 1.0) / 6.0)))); };
    auto showPercent = [] (Readout& r, double v) { r.setValue (juce::String (juce::roundToInt (v))); };
    auto showHpf     = [] (Readout& r, double v)
    {
        if (v <= 20.5) r.setValue ("oFF", false);
        else           r.setValue (juce::String (juce::roundToInt (v)));
    };

    setupKnob (inputKnob,   true,  ua ("Вхід: наскільки сильно сигнал вдавлюється у фіксований поріг. Більше — більше компресії і кольору."), inputRead, showDb);
    setupKnob (outputKnob,  true,  ua ("Вихід: компенсація гучності. У режимі ANALOG ще й драйвить вихідний підсилювач (більше — гарячіше)."), outputRead, showDb);
    setupKnob (attackKnob,  false, ua ("Атака 1…7. 7 — найшвидша (20 мкс), 1 — найповільніша (800 мкс). Індикатор показує час у мікросекундах."), attackRead, showAttack);
    setupKnob (releaseKnob, false, ua ("Відновлення 1…7. 7 — найшвидше (50 мс), 1 — 1,1 с. Після довгої компресії стає повільнішим, як у залізі. Індикатор — у мілісекундах."), releaseRead, showRelease);
    setupKnob (mixKnob,     false, ua ("Mix: паралельна компресія. Сухий і оброблений сигнали вирівняні по фазі."), mixRead, showPercent);
    setupKnob (hpfKnob,     false, ua ("ФВЧ у сайдчейні: бочка й бас менше «качають» компресор. oFF — вимкнено."), hpfRead, showHpf);

    inputAtt   = std::make_unique<SliderAttachment> (proc.apvts, pid::input,   inputKnob);
    outputAtt  = std::make_unique<SliderAttachment> (proc.apvts, pid::output,  outputKnob);
    attackAtt  = std::make_unique<SliderAttachment> (proc.apvts, pid::attack,  attackKnob);
    releaseAtt = std::make_unique<SliderAttachment> (proc.apvts, pid::release, releaseKnob);
    mixAtt     = std::make_unique<SliderAttachment> (proc.apvts, pid::mix,     mixKnob);
    hpfAtt     = std::make_unique<SliderAttachment> (proc.apvts, pid::scHpf,   hpfKnob);

    auto setDefault = [this] (juce::Slider& sl, const char* id)
    {
        if (auto* prm = proc.apvts.getParameter (id))
            sl.setDoubleClickReturnValue (true, (double) prm->convertFrom0to1 (prm->getDefaultValue()));
    };
    setDefault (inputKnob, pid::input);   setDefault (outputKnob, pid::output);
    setDefault (attackKnob, pid::attack); setDefault (releaseKnob, pid::release);
    setDefault (mixKnob, pid::mix);       setDefault (hpfKnob, pid::scHpf);

    // initial read-outs
    for (auto* k : { &inputKnob, &outputKnob, &attackKnob, &releaseKnob, &mixKnob, &hpfKnob })
        if (k->onValueChange)
            k->onValueChange();

    // ratio
    const char* ratioNames[] = { "4:1", "8:1", "12:1", "20:1", "ALL" };
    for (int i = 0; i < 5; ++i)
    {
        auto& b = ratioButtons[(size_t) i];
        b.setButtonText (ratioNames[i]);
        b.getProperties().set ("style", 1);   // white caps
        b.setTooltip (i < 4 ? ua ("Ratio. Вищий ratio — вищий поріг і жорсткіше коліно.")
                            : ua ("ALL («всі кнопки»): ~16:1 і більше, зміщення робочих точок, лаг атаки на транзієнтах, перекомпресія і багато бруду."));
        b.onClick = [this, i] { if (ratioAtt != nullptr) ratioAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }

    // mode
    const char* modeNames[] = { "FAT", "CLEAN", "ANALOG" };
    const char* modeSubs[]  = { "INFLATE", "PRECISE", "CIRCUIT" };
    const char* modeTips[]  = {
        "FAT: лінійна компресія + «роздування» звуку (щільність, гармоніки, вага) у дусі CLA-76. Чим сильніше тиснеш, тим товще.",
        "CLEAN: лінійна, точна компресія. BLACK — без жодного насичення. BLUE — м'яке, дороге насичення (трансформатори + клас А).",
        "ANALOG: нелінійна модель схеми — сайдчейн зі зворотним зв'язком, пульсації детектора на басу, FET, підсилювачі, трансформатори."
    };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = modeButtons[(size_t) i];
        b.setButtonText (modeNames[i]);
        b.getProperties().set ("sub", modeSubs[i]);
        b.setTooltip (ua (modeTips[i]));
        b.onClick = [this, i] { if (modeAtt != nullptr) modeAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }

    // revision
    const char* revNames[] = { "BLACK", "BLUE" };
    const char* revTips[]  = { "BLACK — blackface (LN): чистіший, тихіший, класичний.",
                               "BLUE — bluestripe: FET-підсилювачі, більше гармонік і насичення, соковитіший." };
    for (int i = 0; i < 2; ++i)
    {
        auto& b = revButtons[(size_t) i];
        b.setButtonText (revNames[i]);
        b.setTooltip (ua (revTips[i]));
        b.onClick = [this, i] { if (revAtt != nullptr) revAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }

    // meter
    const char* meterNames[] = { "GR", "+4", "+8", "INPUT" };
    const char* meterTips[]  = { "Стрілка показує подавлення (GR).",
                                 "Вихідний рівень: 0 VU = −18 dBFS.",
                                 "Вихідний рівень: 0 VU = −14 dBFS.",
                                 "Вхідний рівень: 0 VU = −18 dBFS." };
    for (int i = 0; i < 4; ++i)
    {
        auto& b = meterButtons[(size_t) i];
        b.setButtonText (meterNames[i]);
        b.getProperties().set ("style", 2);   // black caps
        b.setTooltip (ua (meterTips[i]));
        b.onClick = [this, i] { if (meterAtt != nullptr) meterAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }

    // switches
    linkButton.setButtonText ("LINK");
    compOffButton.setButtonText ("COMP OFF");
    noiseButton.setButtonText ("NOISE");
    linkButton.setTooltip (ua ("Стерео-лінк: обидва канали компресуються однаково."));
    compOffButton.setTooltip (ua ("Компресію вимкнено, лишається тільки колір схеми (як «Attack Off» на залізі)."));
    noiseButton.setTooltip (ua ("Шум і фон заліза (лише в режимі ANALOG)."));
    for (auto* b : { &linkButton, &compOffButton, &noiseButton })
    {
        b->setClickingTogglesState (true);
        addAndMakeVisible (*b);
    }
    linkAtt    = std::make_unique<ButtonAttachment> (proc.apvts, pid::link,    linkButton);
    compOffAtt = std::make_unique<ButtonAttachment> (proc.apvts, pid::compOff, compOffButton);
    noiseAtt   = std::make_unique<ButtonAttachment> (proc.apvts, pid::noise,   noiseButton);

    // oversampling
    const char* osNames[] = { "AUTO", "1\xc3\x97", "2\xc3\x97", "4\xc3\x97", "8\xc3\x97", "16\xc3\x97" };
    const char* osTips[]  = {
        "Оверсемплінг AUTO: 4× до 50 кГц, 2× до 100 кГц, вище — без нього. Під кнопкою видно, що зараз працює.",
        "Без оверсемплінгу: найменша затримка й навантаження, але насичення і «роздування» дають аліасинг (≈ −30 дБ на гарячих верхах).",
        "2×: аліасинг ≈ −93 дБ, затримка 3 семпли.",
        "4×: аліасинг нижче −100 дБ (рекомендовано), затримка 4 семпли.",
        "8×: майже не чистіше за 4×, удвічі більше навантаження на процесор.",
        "16×: для перфекціоністів — ще вчетверо більше навантаження, на слух як 4×."
    };
    for (int i = 0; i < 6; ++i)
    {
        auto& b = osButtons[(size_t) i];
        b.setButtonText (juce::String::fromUTF8 (osNames[i]));
        b.getProperties().set ("style", 2);
        b.setTooltip (ua (osTips[i]));
        b.onClick = [this, i] { if (osAtt != nullptr) osAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }

    // displays on/off
    valuesButton.setButtonText ("VALUES");
    valuesButton.getProperties().set ("style", 1);
    valuesButton.setTooltip (ua ("Показати або сховати всі цифрові індикатори — лишаються тільки ручки і стрілка."));
    valuesButton.onClick = [this] { setShowValues (! showValues); };
    addAndMakeVisible (valuesButton);

    // the drawer pull
    drawerPull.setTooltip (ua ("ADVANCED: режим, ревізія, оверсемплінг, mix, SC HPF, перемикачі і показ цифр."));
    drawerPull.onClick = [this] { setDrawerOpen (! drawerOpen); };
    addAndMakeVisible (drawerPull);

    addAndMakeVisible (vu);

    ratioAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (pid::ratio),
                   [this] (float v) { ratioIndex = juce::roundToInt (v); refreshSelectors(); });
    modeAtt  = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (pid::mode),
                   [this] (float v) { modeIndex = juce::roundToInt (v); refreshSelectors(); });
    revAtt   = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (pid::revision),
                   [this] (float v) { revIndex = juce::roundToInt (v); refreshSelectors(); });
    meterAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (pid::meter),
                   [this] (float v) { meterIndex = juce::roundToInt (v); refreshSelectors(); });
    osAtt    = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (pid::oversampling),
                   [this] (float v) { osIndex = juce::roundToInt (v); refreshSelectors(); });
    ratioAtt->sendInitialUpdate();
    modeAtt->sendInitialUpdate();
    revAtt->sendInitialUpdate();
    meterAtt->sendInitialUpdate();
    osAtt->sendInitialUpdate();

    setResizable (true, true);
    updateSizeLimits();
    setSize ((int) twinui::kW, (int) (drawerOpen ? twinui::kFullH : twinui::kMainH));
    updateVisibility();

    startTimerHz (60);
}

TwinFetEditor::~TwinFetEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TwinFetEditor::setupKnob (KnobSlider& k, bool big, const juce::String& tooltip, Readout& readout,
                              std::function<void (Readout&, double)> show)
{
    k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.setRotaryParameters (twinui::kStartAngle, twinui::kEndAngle, true);
    k.setMouseDragSensitivity (big ? 320 : 240);
    k.getProperties().set ("big", big);
    k.setTooltip (tooltip);
    k.setPopupDisplayEnabled (true, false, this);
    k.getProperties().set ("seed", (int) (twinui::hashText (tooltip.toRawUTF8()) & 0x7fffffff));
    k.onValueChange = [&k, &readout, show = std::move (show)] { show (readout, k.getValue()); };
    addAndMakeVisible (k);
}

void TwinFetEditor::refreshSelectors()
{
    for (int i = 0; i < 5; ++i) ratioButtons[(size_t) i].setToggleState (i == ratioIndex, juce::dontSendNotification);
    for (int i = 0; i < 3; ++i) modeButtons[(size_t) i].setToggleState (i == modeIndex, juce::dontSendNotification);
    for (int i = 0; i < 2; ++i) revButtons[(size_t) i].setToggleState (i == revIndex, juce::dontSendNotification);
    for (int i = 0; i < 4; ++i) meterButtons[(size_t) i].setToggleState (i == meterIndex, juce::dontSendNotification);
    for (int i = 0; i < 6; ++i) osButtons[(size_t) i].setToggleState (i == osIndex, juce::dontSendNotification);
    valuesButton.setToggleState (showValues, juce::dontSendNotification);
    valuesButton.getProperties().set ("sub", showValues ? "ON" : "OFF");

    // revision = panel material; mode = lamp colours, meter lamp and (ANALOG) a well-used unit
    look.palette = twinui::paletteFor (revIndex, modeIndex);
    if (revIndex != paintedRevision || ! juce::approximatelyEqual (look.palette.wear, paintedWear))
    {
        paintedRevision = revIndex;
        paintedWear = look.palette.wear;
        panelArt.repaint();   // the only expensive repaint
    }
    vu.setMeterIndex (meterIndex);
    vu.repaint();
    const char* modeNames[] = { "FAT", "CLEAN", "ANALOG" };
    jewelLamp.modeName = modeNames[juce::jlimit (0, 2, modeIndex)];
    jewelLamp.repaint();
    noiseButton.setEnabled (modeIndex == 2);
    noiseButton.setAlpha (modeIndex == 2 ? 1.0f : 0.45f);
    repaint();
}

void TwinFetEditor::updateSizeLimits()
{
    const float h = drawerOpen ? twinui::kFullH : twinui::kMainH;
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) twinui::kW / (double) h);
    setResizeLimits ((int) (twinui::kW * 0.7f), (int) (h * 0.7f), (int) (twinui::kW * 2.0f), (int) (h * 2.0f));
}

void TwinFetEditor::setDrawerOpen (bool open)
{
    drawerOpen = open;
    proc.apvts.state.setProperty (uiprop::advancedOpen, open, nullptr);
    const float scaleNow = getWidth() > 0 ? (float) getWidth() / twinui::kW : 1.0f;
    updateSizeLimits();
    setSize (juce::roundToInt (twinui::kW * scaleNow),
             juce::roundToInt ((drawerOpen ? twinui::kFullH : twinui::kMainH) * scaleNow));
    updateVisibility();
}

void TwinFetEditor::setShowValues (bool show)
{
    showValues = show;
    proc.apvts.state.setProperty (uiprop::showValues, show, nullptr);
    updateVisibility();
    refreshSelectors();
}

void TwinFetEditor::updateVisibility()
{
    drawerPull.open = drawerOpen;
    drawerPull.repaint();
    for (auto* r : { &inputRead, &outputRead, &attackRead, &releaseRead, &grRead, &outRead })
        r->setVisible (showValues);
    mixRead.setVisible (showValues && drawerOpen);
    hpfRead.setVisible (showValues && drawerOpen);
    for (auto& b : modeButtons) b.setVisible (drawerOpen);
    for (auto& b : revButtons)  b.setVisible (drawerOpen);
    for (auto& b : osButtons)   b.setVisible (drawerOpen);
    for (juce::Component* c : { (juce::Component*) &valuesButton, (juce::Component*) &mixKnob, (juce::Component*) &hpfKnob,
                                (juce::Component*) &linkButton, (juce::Component*) &compOffButton, (juce::Component*) &noiseButton })
        c->setVisible (drawerOpen);
}

juce::Rectangle<int> TwinFetEditor::scaled (const twinui::Rect& r, float pad) const
{
    return juce::Rectangle<float> ((r.x - pad) * s, (r.y - pad) * s, (r.w + 2.0f * pad) * s, (r.h + 2.0f * pad) * s).toNearestInt();
}

//==============================================================================
void TwinFetEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1c1f));
}

void TwinFetEditor::resized()
{
    namespace L = twinui::layout;
    s = (float) getWidth() / twinui::kW;
    look.scale = s;

    panelArt.setBounds (0, 0, getWidth(), juce::roundToInt (twinui::kFullH * s));
    {
        const auto j = L::jewel(), n = L::modeName();
        const float pd = JewelLamp::pad;
        jewelLamp.setBounds (scaled ({ j.x - pd, j.y - pd, (n.right() - j.x) + 2.0f * pd, j.h + 2.0f * pd }));
    }
    drawerPull.setBounds (scaled (L::advancedHandle(), DrawerPull::pad));
    vu.setBounds (scaled (L::vu()));

    inputKnob.setBounds   (scaled (L::input(),   twinui::knobPad));
    outputKnob.setBounds  (scaled (L::output(),  twinui::knobPad));
    attackKnob.setBounds  (scaled (L::attack(),  twinui::knobPad));
    releaseKnob.setBounds (scaled (L::release(), twinui::knobPad));
    mixKnob.setBounds     (scaled (L::mix(),     twinui::knobPad));
    hpfKnob.setBounds     (scaled (L::hpf(),     twinui::knobPad));

    inputRead.setBounds   (scaled (L::inputReadout(),   Readout::pad));
    outputRead.setBounds  (scaled (L::outputReadout(),  Readout::pad));
    attackRead.setBounds  (scaled (L::attackReadout(),  Readout::pad));
    releaseRead.setBounds (scaled (L::releaseReadout(), Readout::pad));
    mixRead.setBounds     (scaled (L::mixReadout(),     Readout::pad));
    hpfRead.setBounds     (scaled (L::hpfReadout(),     Readout::pad));
    auto withCaption = [] (twinui::Rect r) { r.h += twinui::layout::readoutCaptionH; return r; };
    grRead.setBounds      (scaled (withCaption (L::grReadout()),  Readout::pad));
    outRead.setBounds     (scaled (withCaption (L::outReadout()), Readout::pad));

    auto placeButton = [this] (LitButton& b, const twinui::Rect& r)
    {
        b.pad = twinui::buttonPad * s;
        b.setBounds (scaled (r, twinui::buttonPad));
    };
    for (int i = 0; i < 5; ++i) placeButton (ratioButtons[(size_t) i], L::ratioButton (i));
    for (int i = 0; i < 3; ++i) placeButton (modeButtons[(size_t) i],  L::modeButton (i));
    for (int i = 0; i < 2; ++i) placeButton (revButtons[(size_t) i],   L::revButton (i));
    for (int i = 0; i < 4; ++i) placeButton (meterButtons[(size_t) i], L::meterButton (i));
    for (int i = 0; i < 6; ++i) placeButton (osButtons[(size_t) i],    L::osButton (i));
    placeButton (valuesButton,  L::valuesButton());
    placeButton (linkButton,    L::toggle (0));
    placeButton (compOffButton, L::toggle (1));
    placeButton (noiseButton,   L::toggle (2));
}

//==============================================================================
void TwinFetEditor::timerCallback()
{
    const float gr  = proc.meterGrDb.load (std::memory_order_relaxed);
    const float in  = proc.meterInMeanAbs.load (std::memory_order_relaxed);
    const float out = proc.meterOutMeanAbs.load (std::memory_order_relaxed);
    constexpr float halfPi = juce::MathConstants<float>::halfPi;

    // average-responding, sine calibrated: rectified mean * pi/2 = peak; 0 VU = -18 dBFS
    const float outVu = 20.0f * std::log10 (juce::jmax (1.0e-7f, out * halfPi)) + 18.0f;
    const float inVu  = 20.0f * std::log10 (juce::jmax (1.0e-7f, in * halfPi)) + 18.0f;

    float target = 0.0f;
    if (meterIndex == 0)
    {
        target = std::pow (10.0f, (-gr - 3.0f) / 20.0f);    // GR needle rests on 0 VU and swings left
    }
    else
    {
        const float vuDb = meterIndex == 3 ? inVu : (meterIndex == 2 ? outVu - 4.0f : outVu);
        target = std::pow (10.0f, (vuDb - 3.0f) / 20.0f);
    }
    vu.tick (target, 1.0f / 60.0f);

    const int osNow = proc.activeOversampling.load (std::memory_order_relaxed);
    if (osNow != shownOsFactor)
    {
        shownOsFactor = osNow;
        osButtons[0].getProperties().set ("sub", juce::String (osNow) + juce::String::fromUTF8 ("\xc3\x97"));
        osButtons[0].repaint();
    }

    if (++frame % 3 == 0)
    {
        grRead.setValue (juce::String (gr, 1));
        outRead.setValue (outVu > -40.0f ? juce::String (outVu, 1) : juce::String ("---"));   // dashes = no signal
    }
}

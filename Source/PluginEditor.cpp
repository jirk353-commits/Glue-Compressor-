#include "PluginEditor.h"

using namespace juce;

namespace dgui
{
Font uiFont (float size, bool bold, float kerning)
{
    Font f { FontOptions (size, bold ? Font::bold : Font::plain) };
    return kerning > 0.0f ? f.withExtraKerningFactor (kerning) : f;
}

void embossed (Graphics& g, const String& t, Rectangle<float> r, Font f, Colour c, Justification j)
{
    g.setFont (f);
    g.setColour (Colours::black.withAlpha (0.75f));
    g.drawText (t, r.translated (0, -1.0f), j, false);
    g.setColour (Colours::white.withAlpha (0.22f));
    g.drawText (t, r.translated (0, 1.2f), j, false);
    g.setColour (c);
    g.drawText (t, r, j, false);
}

//==============================================================================
LookAndFeel::LookAndFeel()
{
    setColour (PopupMenu::backgroundColourId, Colour (0xff1b2128));
    setColour (PopupMenu::textColourId, col::text);
    setColour (PopupMenu::highlightedBackgroundColourId, col::amber);
    setColour (PopupMenu::highlightedTextColourId, Colours::black);
    setColour (ComboBox::textColourId, col::amber);
}

void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float sa, float ea, Slider& s)
{
    const auto cap = s.findColour (Slider::rotarySliderFillColourId);
    const auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float size = jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float tickR = size * 0.5f - 1.0f, skirtR = size * 0.40f, bodyR = size * 0.31f;
    const float angle = sa + pos * (ea - sa);

    // engraved scale
    for (int i = 0; i <= 10; ++i)
    {
        const float a = sa + (float) i / 10.0f * (ea - sa);
        const bool major = i % 5 == 0;
        auto p1 = c.getPointOnCircumference (tickR - (major ? 7.0f : 5.0f), a), p2 = c.getPointOnCircumference (tickR, a);
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawLine ({ p1.translated (0, -1), p2.translated (0, -1) }, major ? 2.0f : 1.4f);
        g.setColour (col::text.withAlpha (major ? 0.95f : 0.7f));
        g.drawLine ({ p1, p2 }, major ? 2.0f : 1.4f);
    }
    // value arc
    {
        Path arc;
        arc.addCentredArc (c.x, c.y, skirtR + 4.0f, skirtR + 4.0f, 0.0f, sa, angle, true);
        g.setColour (cap.withAlpha (0.85f));
        g.strokePath (arc, PathStrokeType (2.5f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // metallic skirt
    Path skirt; skirt.addEllipse (c.x - skirtR, c.y - skirtR, skirtR * 2, skirtR * 2);
    DropShadow (Colours::black.withAlpha (0.85f), (int) (size * 0.14f), { 0, (int) (size * 0.06f) }).drawForPath (g, skirt);
    ColourGradient metal (Colour (0xffd9dde2), c.x - skirtR, c.y - skirtR, Colour (0xff5a6068), c.x + skirtR, c.y + skirtR, false);
    metal.addColour (0.5, Colour (0xff9aa1aa));
    g.setGradientFill (metal);
    g.fillPath (skirt);
    for (int i = 0; i < 60; ++i)   // knurl
    {
        const float a = (float) i / 60.0f * MathConstants<float>::twoPi;
        g.setColour (i % 2 ? Colours::black.withAlpha (0.25f) : Colours::white.withAlpha (0.18f));
        g.drawLine ({ c.getPointOnCircumference (bodyR + 1, a), c.getPointOnCircumference (skirtR - 1, a) }, 1.0f);
    }
    // black body
    g.setGradientFill (ColourGradient (Colour (0xff3a3f46), c.x, c.y - bodyR, Colour (0xff0b0c0e), c.x, c.y + bodyR, false));
    g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2);
    g.setColour (Colours::black);
    g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2, 1.2f);
    // coloured cap
    const float capR = bodyR * 0.45f;
    g.setGradientFill (ColourGradient (cap.brighter (0.5f), c.x, c.y - capR, cap.darker (0.4f), c.x, c.y + capR, false));
    g.fillEllipse (c.x - capR, c.y - capR, capR * 2, capR * 2);
    g.setColour (Colours::white.withAlpha (0.35f));
    g.fillEllipse (c.x - capR * 0.5f, c.y - capR * 0.75f, capR, capR * 0.6f);
    // pointer
    g.setColour (Colours::white);
    g.drawLine ({ c.getPointOnCircumference (capR + 2, angle), c.getPointOnCircumference (skirtR - 2, angle) }, jmax (2.5f, size * 0.03f));
}

void LookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    DropShadow (Colours::black.withAlpha (0.7f), 4, { 0, down ? 0 : 2 }).drawForRectangle (g, r.toNearestInt());
    g.setGradientFill (ColourGradient (Colour (down ? 0xff23272d : 0xff4a5059), 0, r.getY(), Colour (0xff1c1f24), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (Colours::white.withAlpha (hi ? 0.35f : 0.18f));
    g.drawLine (r.getX() + 3, r.getY() + 1, r.getRight() - 3, r.getY() + 1, 1.0f);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    // LED
    const auto led = b.findColour (TextButton::buttonOnColourId);
    if (b.getWidth() < 50)       // narrow button: the LED becomes a thin strip on top
    {
        auto strip = Rectangle<float> (r.getX() + 6, r.getY() + 3, r.getWidth() - 12, 3);
        if (b.getToggleState()) { g.setColour (led.withAlpha (0.35f)); g.fillRoundedRectangle (strip.expanded (2, 2), 2); g.setColour (led); }
        else g.setColour (Colour (0xff101214));
        g.fillRoundedRectangle (strip, 1.5f);
        return;
    }
    const float d = jmin (8.0f, r.getHeight() * 0.3f);
    auto l = Rectangle<float> (r.getX() + 7, r.getCentreY() - d * 0.5f, d, d);
    if (b.getToggleState())
    {
        g.setColour (led.withAlpha (0.35f));
        g.fillEllipse (l.expanded (4));
        g.setColour (led);
    }
    else g.setColour (Colour (0xff101214));
    g.fillEllipse (l);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawEllipse (l, 0.8f);
}

void LookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (2).withTrimmedLeft (b.getWidth() < 50 ? 0.0f : 14.0f).withTrimmedTop (b.getWidth() < 50 ? 4.0f : 0.0f);
    g.setFont (uiFont (jmin (14.0f, b.getHeight() * 0.5f), true, 0.04f));
    g.setColour (b.getToggleState() ? col::text : col::textDim.withAlpha (0.85f));
    g.drawFittedText (b.getButtonText(), r.toNearestInt(), Justification::centred, 1, 0.75f);
}

void LookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox&)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (col::lcd);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (r, 5.0f, 1.5f);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawLine (r.getX() + 4, r.getBottom() + 0.5f, r.getRight() - 4, r.getBottom() + 0.5f, 1.0f);
    Path a;
    const float ax = (float) w - 18.0f, ay = (float) h * 0.5f;
    a.addTriangle (ax - 5, ay - 2.5f, ax + 5, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (col::amber);
    g.fillPath (a);
}
Font LookAndFeel::getComboBoxFont (ComboBox&) { return uiFont (15.5f); }
Font LookAndFeel::getPopupMenuFont() { return uiFont (15.0f, false); }
void LookAndFeel::positionComboBoxText (ComboBox& b, Label& l) { l.setBounds (12, 1, b.getWidth() - 36, b.getHeight() - 2); l.setFont (getComboBoxFont (b)); }

//==============================================================================
Knob::Knob (APVTS& s, const String& id, const String& t, Colour cap, bool b) : title (t), big (b)
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (big ? 260 : 210);
    slider.setColour (Slider::rotarySliderFillColourId, cap);
    addAndMakeVisible (slider);
    att = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
    if (auto* p = s.getParameter (id)) slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    slider.onValueChange = [this] { repaint(); };
}
void Knob::resized()
{
    auto r = getLocalBounds();
    r.removeFromTop (big ? 24 : 22);
    r.removeFromBottom (big ? 30 : 28);
    const int s = jmin (r.getWidth(), r.getHeight());
    slider.setBounds (r.withSizeKeepingCentre (s, s));
}
void Knob::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    embossed (g, title, r.removeFromTop (big ? 24 : 22), uiFont (big ? 15.0f : 13.5f, true, 0.1f), col::text, Justification::centred);
    auto lcd = r.removeFromBottom (big ? 30 : 28).withSizeKeepingCentre (jmin ((float) getWidth() - 8, big ? 96.0f : 84.0f), big ? 24.0f : 22.0f);
    g.setColour (col::lcd);
    g.fillRoundedRectangle (lcd, 4.0f);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (lcd, 4.0f, 1.2f);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawLine (lcd.getX() + 3, lcd.getBottom() + 0.8f, lcd.getRight() - 3, lcd.getBottom() + 0.8f, 1.0f);
    g.setColour (slider.findColour (Slider::rotarySliderFillColourId).brighter (0.3f));
    g.setFont (uiFont (big ? 15.0f : 14.0f));
    g.drawText (slider.getTextFromValue (slider.getValue()), lcd, Justification::centred, false);
}

//==============================================================================
ChoiceButtons::ChoiceButtons (RangedAudioParameter& param, const StringArray& labels, Colour led)
{
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add (new TextButton (labels[i]));
        b->setColour (TextButton::buttonOnColourId, led);
        b->onClick = [this, i] { if (att) att->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    att = std::make_unique<ParameterAttachment> (param, [this] (float v) { update (roundToInt (v)); });
    att->sendInitialUpdate();
}
void ChoiceButtons::update (int index)
{
    selected = index;
    for (int i = 0; i < buttons.size(); ++i) buttons[i]->setToggleState (i == index, dontSendNotification);
    if (onChange) onChange (index);
}
void ChoiceButtons::resized()
{
    const int n = buttons.size();
    const float w = (float) getWidth() / (float) n;
    for (int i = 0; i < n; ++i)
        buttons[i]->setBounds (roundToInt ((float) i * w), 0, roundToInt ((float) (i + 1) * w) - roundToInt ((float) i * w), getHeight());
}

Toggle::Toggle (APVTS& s, const String& id, const String& text, Colour led) : TextButton (text)
{
    setClickingTogglesState (true);
    setColour (TextButton::buttonOnColourId, led);
    att = std::make_unique<APVTS::ButtonAttachment> (s, id, *this);
}

//==============================================================================
float VUMeter::pos (float vu)
{
    static const float t[][2] = { { -20, 0 }, { -10, 0.2f }, { -7, 0.33f }, { -5, 0.45f }, { -3, 0.58f }, { -2, 0.65f },
                                  { -1, 0.72f }, { 0, 0.79f }, { 1, 0.86f }, { 2, 0.93f }, { 3, 1.0f } };
    if (vu <= t[0][0]) return 0;
    for (size_t i = 1; i < std::size (t); ++i)
        if (vu <= t[i][0]) return t[i - 1][1] + (vu - t[i - 1][0]) / (t[i][0] - t[i - 1][0]) * (t[i][1] - t[i - 1][1]);
    return 1.03f;
}
void VUMeter::setValue (float vu, float grDb, int m)
{
    mode = m; gr = grDb;
    const float target = pos (vu);
    const float old = needle;
    needle += (target - needle) * 0.18f;
    if (std::abs (old - needle) > 0.0005f) repaint();
}
void VUMeter::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff2a2f36), 0, b.getY(), Colour (0xff0d0f12), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 8.0f);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.5f);
    auto face = b.reduced (8.0f);
    Path fp; fp.addRoundedRectangle (face, 5.0f);
    g.setGradientFill (ColourGradient (Colour (0xfffff3d6), face.getCentreX(), face.getBottom(), Colour (0xffe0c68f), face.getX(), face.getY(), true));
    g.fillPath (fp);
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (fp);
    const Point<float> pivot { face.getCentreX(), face.getBottom() + face.getHeight() * 0.3f };
    const float R = face.getHeight() * 1.0f, span = 0.72f;
    auto ang = [span] (float p) { return -span + p * 2.0f * span; };
    const Colour ink (0xff2a1d12);
    Path arc; arc.addCentredArc (pivot.x, pivot.y, R, R, 0, ang (0), ang (0.79f), true);
    g.setColour (ink); g.strokePath (arc, PathStrokeType (1.5f));
    Path red; red.addCentredArc (pivot.x, pivot.y, R + 2.5f, R + 2.5f, 0, ang (0.79f), ang (1.0f), true);
    g.setColour (Colour (0xffc8432f)); g.strokePath (red, PathStrokeType (5.0f));
    g.setFont (uiFont (12.0f));
    for (float m : { -20.f, -10.f, -7.f, -5.f, -3.f, -1.f, 0.f, 1.f, 2.f, 3.f })
    {
        const float a = ang (pos (m));
        g.setColour (m > 0 ? Colour (0xffc8432f) : ink);
        g.drawLine ({ pivot.getPointOnCircumference (R, a), pivot.getPointOnCircumference (R + 9, a) }, 1.5f);
        const String lbl = mode == 1 ? String ((int) std::abs (m) <= 20 ? (int) (m <= 0 ? -m : -m) : 0) : (m > 0 ? "+" + String ((int) m) : String ((int) -m));
        g.drawText (mode == 1 ? (m <= 0 ? String ((int) -m) : "") : lbl, Rectangle<float> (34, 14).withCentre (pivot.getPointOnCircumference (R + 19, a)), Justification::centred, false);
    }
    g.setColour (ink);
    g.setFont (uiFont (21.0f, true, 0.06f));
    g.drawText (mode == 1 ? "GR" : "VU", Rectangle<float> (face.getX(), face.getY() + face.getHeight() * 0.48f, face.getWidth(), 24), Justification::centred);
    g.setFont (uiFont (11.0f, true, 0.2f));
    g.drawText (mode == 1 ? "GAIN REDUCTION dB" : (mode == 0 ? "INPUT LEVEL" : "OUTPUT LEVEL"),
                Rectangle<float> (face.getX(), face.getY() + face.getHeight() * 0.48f + 24, face.getWidth(), 14), Justification::centred);
    g.setFont (uiFont (12.0f));
    g.drawText ("GR " + String (gr, 1) + " dB", face.reduced (10, 6), Justification::bottomRight);
    g.drawText ("ROHIT JI", face.reduced (10, 6), Justification::bottomLeft);
    const float a = ang (jlimit (-0.02f, 1.03f, needle));
    g.setColour (Colours::black.withAlpha (0.2f));
    g.drawLine ({ pivot.translated (3, 2), pivot.getPointOnCircumference (R + 6, a).translated (3, 2) }, 2.0f);
    g.setColour (Colour (0xff1a120c));
    g.drawLine ({ pivot, pivot.getPointOnCircumference (R + 6, a) }, 1.8f);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.3f), 0, face.getY(), Colours::transparentWhite, 0, face.getCentreY(), false));
    g.fillRect (face.withHeight (face.getHeight() * 0.35f));
}

//==============================================================================
namespace
{
    const Rectangle<int> meterW  { 44, 84, 336, 312 };
    const Rectangle<int> compW   { 396, 84, 760, 312 };
    const Rectangle<int> eqW     { 44, 412, 516, 292 };
    const Rectangle<int> satW    { 576, 412, 424, 292 };
    const Rectangle<int> outW    { 1016, 412, 140, 292 };
    const char* satInfo[] =
    {
        "TRIODE - tube glow, rich 2nd harmonic",
        "WARMTH - soft, round and thick",
        "TAPE - smooth peaks, low bump, silky top",
        "ANALOG - console punch, API-style bite",
        "RETRO - band-limited grit and character"
    };
}

MainPanel::MainPanel (DrumGlueProcessor& p)
    : proc (p),
      analog (p.apvts, "analog", "ANALOG COLOUR", col::amber),
      compOn (p.apvts, "comp_on", "ON", col::red), eqOn (p.apvts, "eq_on", "ON", col::blue), satOn (p.apvts, "sat_on", "ON", col::amber),
      thrust (p.apvts, "thrust", "THRUST", col::green),
      ratio (*p.apvts.getParameter ("ratio"), dg::GlueCompressor::ratioNames(), col::red),
      attack (*p.apvts.getParameter ("attack"), dg::GlueCompressor::attackNames(), col::red),
      release (*p.apvts.getParameter ("release"), dg::GlueCompressor::releaseNames(), col::red),
      knee (*p.apvts.getParameter ("knee"), dg::GlueCompressor::kneeNames(), col::red),
      type (*p.apvts.getParameter ("type"), { "NEW", "OLD" }, col::green),
      wFreq (*p.apvts.getParameter ("warmth_freq"), dg::ToneEq::warmthFreqs(), col::blue),
      bFreq (*p.apvts.getParameter ("bite_freq"), dg::ToneEq::biteFreqs(), col::blue),
      aFreq (*p.apvts.getParameter ("air_freq"), dg::ToneEq::airFreqs(), col::blue),
      satType (*p.apvts.getParameter ("sat_type"), DrumGlueProcessor::satNames(), col::amber),
      thresh (p.apvts, "thresh", "THRESHOLD", col::red, true), makeup (p.apvts, "makeup", "MAKEUP", col::red, true),
      compMix (p.apvts, "comp_mix", "MIX", col::red, true),
      lowCut (p.apvts, "lowcut", "LOW CUT", col::blue), warmth (p.apvts, "warmth", "WARMTH", col::blue),
      bite (p.apvts, "bite", "BITE", col::blue), air (p.apvts, "air", "AIR", col::blue),
      drive (p.apvts, "drive", "DRIVE", col::amber, true), satMix (p.apvts, "sat_mix", "MIX", col::amber, true),
      inGain (p.apvts, "in_gain", "INPUT", Colour (0xffe8e8e8)), outGain (p.apvts, "out_gain", "OUTPUT", Colour (0xffe8e8e8))
{
    setLookAndFeel (&lnf);
    setOpaque (true);
    for (Component* c : std::initializer_list<Component*> { &vu, &mIn, &mGr, &mOut, &analog, &compOn, &eqOn, &satOn, &thrust, &ratio, &attack, &release,
                                                            &knee, &type, &wFreq, &bFreq, &aFreq, &satType, &thresh, &makeup, &compMix, &lowCut,
                                                            &warmth, &bite, &air, &drive, &satMix, &inGain, &outGain, &presetBox })
        addAndMakeVisible (c);

    TextButton* mb[] = { &mIn, &mGr, &mOut };
    for (int i = 0; i < 3; ++i)
    {
        mb[i]->setColour (TextButton::buttonOnColourId, col::amber);
        mb[i]->setClickingTogglesState (true);
        mb[i]->setRadioGroupId (77);
        mb[i]->onClick = [this, i] { meterMode = i; };
    }
    mGr.setToggleState (true, dontSendNotification);
    satType.onChange = [this] (int) { repaint (satW); };

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this] { const int id = presetBox.getSelectedId(); if (id > 0 && id - 1 != proc.getCurrentProgram()) proc.setCurrentProgram (id - 1); };
    startTimerHz (40);
}

MainPanel::~MainPanel() { stopTimer(); setLookAndFeel (nullptr); }

void MainPanel::resized()
{
    renderTexture();
    presetBox.setBounds (W - 44 - 290, 22, 290, 34);

    // meter
    vu.setBounds (meterW.getX() + 16, meterW.getY() + 40, meterW.getWidth() - 32, 186);
    const int bw = 70, bx = meterW.getX() + 16;
    mIn.setBounds (bx, meterW.getY() + 236, bw, 30);
    mGr.setBounds (bx + bw + 4, meterW.getY() + 236, bw, 30);
    mOut.setBounds (bx + 2 * (bw + 4), meterW.getY() + 236, bw, 30);
    analog.setBounds (meterW.getX() + 16, meterW.getY() + 272, meterW.getWidth() - 32, 30);

    // compressor
    compOn.setBounds (compW.getRight() - 16 - 80, compW.getY() + 8, 80, 28);
    thresh.setBounds  (compW.getX() + 14,  compW.getY() + 46, 150, 250);
    makeup.setBounds  (compW.getX() + 164, compW.getY() + 46, 150, 250);
    compMix.setBounds (compW.getX() + 314, compW.getY() + 46, 150, 250);
    {
        const int x = compW.getX() + 484, w = compW.getRight() - 16 - x;
        int y = compW.getY() + 60;
        for (Component* c : std::initializer_list<Component*> { &ratio, &attack, &release, &knee })
        {
            c->setBounds (x, y, w, 30);
            y += 52;
        }
        type.setBounds (x, y, w / 2 - 6, 30);
        thrust.setBounds (x + w / 2 + 6, y, w / 2 - 6, 30);
    }

    // eq
    eqOn.setBounds (eqW.getRight() - 16 - 80, eqW.getY() + 8, 80, 28);
    {
        Knob* k[] = { &lowCut, &warmth, &bite, &air };
        const int kw = (eqW.getWidth() - 32) / 4;
        for (int i = 0; i < 4; ++i) k[i]->setBounds (eqW.getX() + 16 + i * kw, eqW.getY() + 42, kw, 172);
        wFreq.setBounds (eqW.getX() + 22, eqW.getY() + 236, 2 * kw - 12, 30);
        bFreq.setBounds (eqW.getX() + 16 + 2 * kw + 6, eqW.getY() + 236, kw - 12, 30);
        aFreq.setBounds (eqW.getX() + 16 + 3 * kw + 6, eqW.getY() + 236, kw - 12, 30);
    }

    // saturation
    satOn.setBounds (satW.getRight() - 16 - 80, satW.getY() + 8, 80, 28);
    satType.setBounds (satW.getX() + 14, satW.getY() + 46, satW.getWidth() - 28, 32);
    drive.setBounds  (satW.getX() + 40,  satW.getY() + 108, 160, 178);
    satMix.setBounds (satW.getX() + 224, satW.getY() + 108, 160, 178);

    // output
    inGain.setBounds  (outW.getX() + 8, outW.getY() + 40, outW.getWidth() - 16, 120);
    outGain.setBounds (outW.getX() + 8, outW.getY() + 164, outW.getWidth() - 16, 120);
}

void MainPanel::renderTexture()
{
    texture = Image (Image::RGB, W, H, true);
    Graphics g (texture);
    g.setGradientFill (ColourGradient (col::plateTop, 0, 0, col::plateBot, 0, (float) H, false));
    g.fillAll();
    // heavy brushed grain - clearly visible texture
    Random rnd (2024);
    for (int y = 0; y < H; ++y)
    {
        for (int k = 0; k < 6; ++k)
        {
            const float x0 = rnd.nextFloat() * W, len = 40.0f + rnd.nextFloat() * 260.0f;
            g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (0.03f + rnd.nextFloat() * 0.06f));
            g.fillRect (x0, (float) y, len, 1.0f);
        }
    }
    for (int i = 0; i < 20000; ++i)
    {
        g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (rnd.nextFloat() * 0.07f));
        g.fillRect (rnd.nextFloat() * W, rnd.nextFloat() * H, 1.0f, 1.0f);
    }
    // sheen
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.10f), W * 0.3f, 0, Colours::transparentWhite, W * 0.6f, H * 0.6f, false));
    g.fillAll();

    // rack ears
    for (int side = 0; side < 2; ++side)
    {
        auto ear = Rectangle<float> (side == 0 ? 0.0f : (float) W - 28.0f, 0.0f, 28.0f, (float) H);
        g.setGradientFill (ColourGradient (Colour (0xff20262d), ear.getX(), 0, Colour (0xff39424d), ear.getRight(), 0, false));
        g.fillRect (ear);
        g.setColour (Colours::black.withAlpha (0.6f));
        g.drawVerticalLine (side == 0 ? 28 : W - 29, 0.0f, (float) H);
        for (float yy : { 40.0f, (float) H - 40.0f })
        {
            auto s = Rectangle<float> (10, 16).withCentre ({ ear.getCentreX(), yy });
            g.setColour (Colour (0xff0c0e10));
            g.fillRoundedRectangle (s, 5.0f);
            g.setColour (Colours::white.withAlpha (0.15f));
            g.drawRoundedRectangle (s.translated (0, 1), 5.0f, 1.0f);
        }
    }
}

void MainPanel::drawWell (Graphics& g, Rectangle<int> r, const String& title, Colour accent)
{
    auto rf = r.toFloat();
    // recessed well: dark fill, inner shadow, highlight at the bottom edge
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (rf.translated (0, 1.5f), 9.0f, 1.5f);
    g.setGradientFill (ColourGradient (Colour (0xff12161b), 0, rf.getY(), Colour (0xff1d232a), 0, rf.getBottom(), false));
    g.fillRoundedRectangle (rf, 9.0f);
    g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.55f), 0, rf.getY(), Colours::transparentBlack, 0, rf.getY() + 18, false));
    g.fillRoundedRectangle (rf, 9.0f);
    g.setColour (col::wellEdge);
    g.drawRoundedRectangle (rf.reduced (0.5f), 9.0f, 1.5f);
    // title plate
    g.setColour (accent);
    g.fillRoundedRectangle (rf.getX() + 16, rf.getY() + 13, 6, 22, 3);
    embossed (g, title, Rectangle<float> (rf.getX() + 30, rf.getY() + 8, 400, 32), uiFont (17.0f, true, 0.16f), col::text, Justification::centredLeft);
}

void MainPanel::paint (Graphics& g)
{
    g.drawImage (texture, getLocalBounds().toFloat());

    // header (engraved into the faceplate)
    embossed (g, "DRUM GLUE", Rectangle<float> (46, 12, 420, 40), uiFont (36.0f, true, 0.18f), Colour (0xfffff4dc), Justification::centredLeft);
    embossed (g, "ROHIT JI   |   ANALOG BUS COMPRESSOR STRIP", Rectangle<float> (48, 50, 520, 20), uiFont (13.5f, true, 0.2f), col::textDim, Justification::centredLeft);
    embossed (g, "PRESET", Rectangle<float> ((float) W - 44 - 290 - 90, 22, 80, 34), uiFont (14.0f, true, 0.2f), col::textDim, Justification::centredRight);

    drawWell (g, meterW, "METER", col::amber);
    drawWell (g, compW, "GLUE COMPRESSOR", col::red);
    drawWell (g, eqW, "TONE EQ", col::blue);
    drawWell (g, satW, "SATURATION", col::amber);
    drawWell (g, outW, "LEVEL", Colour (0xffe8e8e8));

    // compressor row labels
    {
        const int x = compW.getX() + 484, w = compW.getRight() - 16 - x;
        const char* lbl[] = { "RATIO", "ATTACK  ms", "RELEASE  s", "KNEE", "CHARACTER" };
        for (int i = 0; i < 5; ++i)
            embossed (g, lbl[i], Rectangle<float> ((float) x, (float) (compW.getY() + 40 + i * 52), (float) w, 18), uiFont (12.5f, true, 0.16f), col::textDim, Justification::centredLeft);
    }
    embossed (g, "WARMTH FREQ  Hz", Rectangle<float> ((float) eqW.getX() + 22, (float) eqW.getY() + 218, 240, 16), uiFont (11.5f, true, 0.14f), col::textDim, Justification::centredLeft);
    embossed (g, "BITE FREQ", Rectangle<float> ((float) eqW.getX() + 16 + 2 * (eqW.getWidth() - 32) / 4 + 6, (float) eqW.getY() + 218, 120, 16), uiFont (11.5f, true, 0.14f), col::textDim, Justification::centredLeft);
    embossed (g, "AIR FREQ", Rectangle<float> ((float) eqW.getX() + 16 + 3 * (eqW.getWidth() - 32) / 4 + 6, (float) eqW.getY() + 218, 120, 16), uiFont (11.5f, true, 0.14f), col::textDim, Justification::centredLeft);
    {
        auto r = Rectangle<float> ((float) satW.getX() + 14, (float) satW.getY() + 84, (float) satW.getWidth() - 28, 22);
        g.setColour (col::amber.brighter (0.2f));
        g.setFont (uiFont (13.5f, false));
        g.drawText (satInfo[jlimit (0, 4, satType.getSelected())], r, Justification::centred, false);
    }
    embossed (g, "v1.0  |  ROHIT JI", Rectangle<float> ((float) W - 46 - 300, (float) H - 14, 300, 14), uiFont (11.0f, true, 0.2f), col::textDim, Justification::centredRight);
}

void MainPanel::timerCallback()
{
    auto sm = [] (float& v, float t) { v = t > v ? t : v + (t - v) * 0.2f; };
    sm (inLvl, proc.inLevel.load()); sm (outLvl, proc.outLevel.load());
    grLvl += (proc.grLevel.load() - grLvl) * 0.35f;
    float vuVal = meterMode == 1 ? -grLvl : (meterMode == 0 ? inLvl : outLvl) + 18.0f;
    vu.setValue (vuVal, grLvl, meterMode);
    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1) presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
}
} // namespace dgui

//==============================================================================
DrumGlueEditor::DrumGlueEditor (DrumGlueProcessor& p) : AudioProcessorEditor (&p), panel (p)
{
    addAndMakeVisible (panel);
    setResizable (true, true);
    setResizeLimits (dgui::MainPanel::W * 3 / 5, dgui::MainPanel::H * 3 / 5, dgui::MainPanel::W * 2, dgui::MainPanel::H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) dgui::MainPanel::W / (double) dgui::MainPanel::H);
    setSize (dgui::MainPanel::W, dgui::MainPanel::H);
}
void DrumGlueEditor::resized()
{
    panel.setBounds (0, 0, dgui::MainPanel::W, dgui::MainPanel::H);
    panel.setTransform (AffineTransform::scale ((float) getWidth() / (float) dgui::MainPanel::W));
}

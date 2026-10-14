#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace dgui
{
using APVTS = juce::AudioProcessorValueTreeState;

namespace col
{
    const juce::Colour plateTop  { 0xff4a5868 };
    const juce::Colour plateBot  { 0xff2c3540 };
    const juce::Colour well      { 0xff161b21 };
    const juce::Colour wellEdge  { 0xff0b0e11 };
    const juce::Colour text      { 0xfff3f0e6 };
    const juce::Colour textDim   { 0xffc6ccd4 };
    const juce::Colour blue      { 0xff47b4ff };
    const juce::Colour amber     { 0xffffb52e };
    const juce::Colour red       { 0xffff5a47 };
    const juce::Colour green     { 0xff6ff0a0 };
    const juce::Colour lcd       { 0xff0a0d10 };
}

juce::Font uiFont (float size, bool bold = true, float kerning = 0.0f);
/** Engraved / embossed text: dark shadow above, light highlight below, then the colour. */
void embossed (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Font, juce::Colour, juce::Justification);

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
};

class Knob : public juce::Component
{
public:
    Knob (APVTS&, const juce::String& id, const juce::String& title, juce::Colour cap, bool big = false);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;
private:
    juce::String title; bool big;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

class ChoiceButtons : public juce::Component
{
public:
    ChoiceButtons (juce::RangedAudioParameter&, const juce::StringArray& labels, juce::Colour led);
    void resized() override;
    std::function<void (int)> onChange;
    int getSelected() const { return selected; }
private:
    void update (int);
    juce::OwnedArray<juce::TextButton> buttons;
    std::unique_ptr<juce::ParameterAttachment> att;
    int selected = -1;
};

class Toggle : public juce::TextButton
{
public:
    Toggle (APVTS&, const juce::String& id, const juce::String& text, juce::Colour led);
private:
    std::unique_ptr<APVTS::ButtonAttachment> att;
};

class VUMeter : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void setValue (float vu, float grDb, int mode);
private:
    static float pos (float vu);
    float needle = 0.0f, gr = 0.0f; int mode = 1;
};

class MainPanel : public juce::Component, private juce::Timer
{
public:
    explicit MainPanel (DrumGlueProcessor&);
    ~MainPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int W = 1200, H = 720;
private:
    void timerCallback() override;
    void renderTexture();
    void drawWell (juce::Graphics&, juce::Rectangle<int>, const juce::String& title, juce::Colour accent);

    DrumGlueProcessor& proc;
    LookAndFeel lnf;
    juce::Image texture;
    VUMeter vu;
    juce::TextButton mIn { "IN" }, mGr { "GR" }, mOut { "OUT" };
    Toggle analog, compOn, eqOn, satOn, thrust;
    ChoiceButtons ratio, attack, release, knee, type, wFreq, bFreq, aFreq, satType;
    Knob thresh, makeup, compMix, lowCut, warmth, bite, air, drive, satMix, inGain, outGain;
    juce::ComboBox presetBox;
    int meterMode = 1;
    float inLvl = -100, outLvl = -100, grLvl = 0;
};
} // namespace dgui

class DrumGlueEditor : public juce::AudioProcessorEditor
{
public:
    explicit DrumGlueEditor (DrumGlueProcessor&);
    void resized() override;
private:
    dgui::MainPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumGlueEditor)
};

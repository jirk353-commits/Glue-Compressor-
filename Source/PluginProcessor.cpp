#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

namespace
{
    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };
    const std::vector<Preset>& presets()
    {
        // sat_type: 0 Triode 1 Warmth 2 Tape 3 Analog 4 Retro
        static const std::vector<Preset> list =
        {
            { "Drum Bus Glue",      {} },
            { "Punchy Kick & Snare",{ { "thresh", -16 }, { "ratio", 2 }, { "attack", 3 }, { "release", 1 }, { "makeup", 4 }, { "bite", 2.5f }, { "bite_freq", 1 }, { "sat_type", 3 }, { "drive", 25 } } },
            { "Smash Room",         { { "thresh", -26 }, { "ratio", 3 }, { "attack", 0 }, { "release", 0 }, { "makeup", 9 }, { "comp_mix", 45 }, { "sat_type", 2 }, { "drive", 35 }, { "air", 2 } } },
            { "Tape Warmth",        { { "thresh", -12 }, { "ratio", 1 }, { "warmth", 2.5f }, { "warmth_freq", 1 }, { "sat_type", 2 }, { "drive", 45 }, { "air", -1 } } },
            { "Analog Console",     { { "thresh", -10 }, { "ratio", 0 }, { "knee", 2 }, { "sat_type", 3 }, { "drive", 30 }, { "warmth", 1 }, { "air", 1.5f } } },
            { "Triode Glow",        { { "thresh", -14 }, { "ratio", 1 }, { "type", 1 }, { "sat_type", 0 }, { "drive", 40 }, { "warmth", 1.5f } } },
            { "Retro Grit",         { { "thresh", -20 }, { "ratio", 2 }, { "attack", 1 }, { "makeup", 5 }, { "sat_type", 4 }, { "drive", 55 }, { "lowcut", 60 }, { "air", -3 } } },
            { "Gentle Mix Bus",     { { "thresh", -8 }, { "ratio", 0 }, { "attack", 4 }, { "knee", 2 }, { "makeup", 1.5f }, { "sat_type", 3 }, { "drive", 12 }, { "air", 1 } } },
            { "Parallel Crush",     { { "thresh", -32 }, { "ratio", 3 }, { "attack", 0 }, { "release", 1 }, { "makeup", 12 }, { "comp_mix", 35 }, { "sat_type", 1 }, { "drive", 50 } } },
            { "Air & Sparkle",      { { "thresh", -12 }, { "ratio", 1 }, { "air", 4 }, { "air_freq", 1 }, { "bite", 1.5f }, { "bite_freq", 2 }, { "sat_type", 0 }, { "drive", 20 } } },
            { "Lo-Fi Drums",        { { "thresh", -22 }, { "ratio", 2 }, { "makeup", 6 }, { "lowcut", 120 }, { "sat_type", 4 }, { "drive", 70 }, { "air", -5 }, { "sat_mix", 80 } } },
        };
        return list;
    }
}

const StringArray& DrumGlueProcessor::satNames()
{
    static const StringArray n { "TRIODE", "WARMTH", "TAPE", "ANALOG", "RETRO" };
    return n;
}

AudioProcessorValueTreeState::ParameterLayout DrumGlueProcessor::createLayout()
{
    using F = AudioParameterFloat;
    using A = AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<RangedAudioParameter>> v;
    auto db = A().withLabel ("dB").withStringFromValueFunction ([] (float x, int) { return (x > 0.05f ? "+" : "") + String (x, 1) + " dB"; });
    auto pct = A().withLabel ("%").withStringFromValueFunction ([] (float x, int) { return String (roundToInt (x)) + "%"; });
    auto hz = A().withLabel ("Hz").withStringFromValueFunction ([] (float x, int) { return x <= 20.5f ? String ("OFF") : String (roundToInt (x)) + " Hz"; });
    auto b = [&] (const char* id, const char* n, bool d) { v.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, n, d)); };
    auto c = [&] (const char* id, const char* n, const StringArray& s, int d) { v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, n, s, d)); };
    auto f = [&] (const char* id, const char* n, NormalisableRange<float> r, float d, A a) { v.push_back (std::make_unique<F> (ParameterID { id, 1 }, n, r, d, a)); };
    NormalisableRange<float> lcRange (20, 300); lcRange.setSkewForCentre (70);

    f ("in_gain", "Input", { -18, 18, 0.1f }, 0, db);
    f ("out_gain", "Output", { -18, 18, 0.1f }, 0, db);
    b ("analog", "Analog Colour", true);

    b ("comp_on", "Compressor On", true);
    f ("thresh", "Threshold", { -40, 0, 0.1f }, -12, db);
    c ("ratio", "Ratio", dg::GlueCompressor::ratioNames(), 1);
    c ("attack", "Attack", dg::GlueCompressor::attackNames(), 3);
    c ("release", "Release", dg::GlueCompressor::releaseNames(), 4);
    c ("knee", "Knee", dg::GlueCompressor::kneeNames(), 1);
    c ("type", "Type", { "NEW", "OLD" }, 0);
    b ("thrust", "Thrust", true);
    f ("makeup", "Makeup", { -6, 18, 0.1f }, 2, db);
    f ("comp_mix", "Comp Mix", { 0, 100 }, 100, pct);

    b ("eq_on", "EQ On", true);
    f ("lowcut", "Low Cut", lcRange, 20, hz);
    f ("warmth", "Warmth", { -6, 9, 0.1f }, 0, db);
    c ("warmth_freq", "Warmth Freq", dg::ToneEq::warmthFreqs(), 1);
    f ("bite", "Bite", { -6, 9, 0.1f }, 0, db);
    c ("bite_freq", "Bite Freq", dg::ToneEq::biteFreqs(), 1);
    f ("air", "Air", { -6, 9, 0.1f }, 0, db);
    c ("air_freq", "Air Freq", dg::ToneEq::airFreqs(), 1);

    b ("sat_on", "Saturation On", true);
    c ("sat_type", "Saturation Type", { "Triode", "Warmth", "Tape", "Analog", "Retro" }, 3);
    f ("drive", "Drive", { 0, 100 }, 20, pct);
    f ("sat_mix", "Saturation Mix", { 0, 100 }, 100, pct);

    return { v.begin(), v.end() };
}

DrumGlueProcessor::DrumGlueProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<RangedAudioParameter*> (prm))
            cache[r->getParameterID().toStdString()] = apvts.getRawParameterValue (r->getParameterID());
}

bool DrumGlueProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    return (o == AudioChannelSet::mono() || o == AudioChannelSet::stereo()) && l.getMainInputChannelSet() == o;
}

void DrumGlueProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    maxBlock = jmax (1, samplesPerBlock);
    os = std::make_unique<dsp::Oversampling<float>> (2, 1, dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    os->initProcessing ((size_t) maxBlock);
    osFs = fs * 2.0;
    setLatencySamples (roundToInt (os->getLatencyInSamples()));

    comp.prepare (fs);
    eq.prepare (fs);
    for (auto* b : { &tapeBump, &tapeTop, &retroHp, &retroLp, &colLow, &colHigh }) b->reset();
    tapeBump.set (dg::AC::makePeakFilter (osFs, 70.0f, 1.0f, dg::dbToGain (1.8f)));
    tapeTop.set (dg::AC::makeHighShelf (osFs, 12000.0f, 0.7f, dg::dbToGain (-2.5f)));
    retroHp.set (dg::AC::makeHighPass (osFs, 90.0f, 0.7071f));
    retroLp.set (dg::AC::makeLowPass (osFs, 7500.0f, 0.7071f));
    colLow.set (dg::AC::makeLowShelf (fs, 50.0f, 0.7f, dg::dbToGain (0.8f)));
    colHigh.set (dg::AC::makeHighShelf (fs, jmin (16000.0f, (float) fs * 0.45f), 0.7f, dg::dbToGain (-0.6f)));
    dcR = (float) (1.0 - MathConstants<double>::twoPi * 3.0 / osFs);
    dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0;
    lastSat = -1;

    for (auto* s : { &inG, &outG }) s->reset (fs, 0.05);
    for (auto* s : { &driveSm, &satMixSm }) s->reset (osFs, 0.05);
    inG.setCurrentAndTargetValue (dg::dbToGain (p ("in_gain")));
    outG.setCurrentAndTargetValue (dg::dbToGain (p ("out_gain")));
    driveSm.setCurrentAndTargetValue (dg::dbToGain (p ("drive") * 0.24f));
    satMixSm.setCurrentAndTargetValue (p ("sat_mix") * 0.01f);
}

float DrumGlueProcessor::saturate (int type, int ch, float x, float g) noexcept
{
    using namespace dg::shapers;
    constexpr float h = 0.5f;
    const float in = x * g * h;
    const float comp = std::pow (g, 0.12f) / (h * g);      // unity at low level, gentle loudness lift when driven
    switch (type)
    {
        case 0: return triode (in) * comp;
        case 1: return warmth (in) * comp;
        case 2: return tapeTop.process (ch, tapeBump.process (ch, tape (in) * comp));
        case 4:
        {
            float y = retro (retroLp.process (ch, retroHp.process (ch, x)) * g * h) * comp;
            const float steps = 2048.0f;                       // gentle bit grit
            return std::round (y * steps) / steps;
        }
        default: return analog (in) * comp;
    }
}

void DrumGlueProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals nd;
    const int total = buffer.getNumSamples();
    for (int start = 0; start < total; start += maxBlock)
    {
        AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start, jmin (maxBlock, total - start));
        processChunk (chunk);
    }
}

void DrumGlueProcessor::processChunk (AudioBuffer<float>& buffer)
{
    const int n = buffer.getNumSamples();
    const int numCh = jmin (2, getTotalNumOutputChannels());
    if (n == 0 || numCh == 0) return;

    const bool compOn = p ("comp_on") > 0.5f, eqOn = p ("eq_on") > 0.5f, satOn = p ("sat_on") > 0.5f, analogOn = p ("analog") > 0.5f;
    comp.setParams ({ p ("thresh"), (int) p ("ratio"), (int) p ("attack"), (int) p ("release"), (int) p ("knee"),
                      (int) p ("type") == 1, p ("thrust") > 0.5f, p ("makeup"), p ("comp_mix") * 0.01f });
    eq.setParams ({ p ("lowcut"), p ("warmth"), (int) p ("warmth_freq"), p ("bite"), (int) p ("bite_freq"), p ("air"), (int) p ("air_freq") });
    const int satType = (int) p ("sat_type");
    if (satType != lastSat) { lastSat = satType; for (auto* b : { &tapeBump, &tapeTop, &retroHp, &retroLp }) b->reset(); }
    inG.setTargetValue (dg::dbToGain (p ("in_gain")));
    outG.setTargetValue (dg::dbToGain (p ("out_gain")));
    driveSm.setTargetValue (dg::dbToGain (p ("drive") * 0.24f));
    satMixSm.setTargetValue (satOn ? p ("sat_mix") * 0.01f : 0.0f);

    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0);
    float inPk = 0, grMax = 0;

    // ---- input, compressor, EQ (base rate) --------------------------------
    for (int i = 0; i < n; ++i)
    {
        const float g = inG.getNextValue();
        float l = L[i] * g, r = (numCh > 1 ? R[i] : L[i]) * g;
        inPk = jmax (inPk, jmax (std::abs (l), std::abs (r)));
        if (compOn) { comp.process (l, r); grMax = jmax (grMax, comp.getGr()); }
        if (eqOn)   { l = eq.process (0, l); r = eq.process (1, r); }
        L[i] = l; if (numCh > 1) R[i] = r;
    }

    // ---- saturation (2x oversampled; always runs so latency stays constant) -
    {
        dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);
        auto up = os->processSamplesUp (block);
        const int un = (int) up.getNumSamples();
        for (int i = 0; i < un; ++i)
        {
            const float drive = driveSm.getNextValue(), mix = satMixSm.getNextValue();
            for (int c = 0; c < numCh; ++c)
            {
                float* d = up.getChannelPointer ((size_t) c);
                const float x = d[i];
                float y = mix > 0.0001f ? x + mix * (saturate (satType, c, x, drive) - x) : x;
                const float o = y - dcX[c] + dcR * dcY[c];
                dcX[c] = y; dcY[c] = o;
                d[i] = o;
            }
        }
        os->processSamplesDown (block);
    }

    // ---- analog colour + output ------------------------------------------
    double outSq = 0;
    for (int i = 0; i < n; ++i)
    {
        const float og = outG.getNextValue();
        for (int c = 0; c < numCh; ++c)
        {
            float* d = c == 0 ? L : R;
            float y = d[i];
            if (analogOn)
            {
                y = colHigh.process (c, colLow.process (c, y));
                y += 0.35f * (dg::shapers::analog (y * 0.6f) / 0.6f - y);
            }
            y *= og;
            d[i] = y;
            outSq += (double) y * y;
        }
    }
    inLevel.store (Decibels::gainToDecibels (inPk, -100.0f));
    outLevel.store (Decibels::gainToDecibels ((float) std::sqrt (outSq / (double) (n * numCh)) * 1.414f, -100.0f));
    grLevel.store (compOn ? grMax : 0.0f);
}

//==============================================================================
int DrumGlueProcessor::getNumPrograms() { return (int) presets().size(); }
const String DrumGlueProcessor::getProgramName (int i) { return isPositiveAndBelow (i, getNumPrograms()) ? String (presets()[(size_t) i].name) : String(); }

void DrumGlueProcessor::setCurrentProgram (int index)
{
    if (! isPositiveAndBelow (index, getNumPrograms())) return;
    currentProgram = index;
    auto set = [this] (const String& id, float v, bool normalised)
    {
        if (auto* prm = apvts.getParameter (id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (normalised ? v : prm->convertTo0to1 (v));
            prm->endChangeGesture();
        }
    };
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<RangedAudioParameter*> (prm))
            if (r->getParameterID() != "in_gain" && r->getParameterID() != "out_gain")
                set (r->getParameterID(), r->getDefaultValue(), true);
    for (auto& [id, val] : presets()[(size_t) index].values)
        set (id, val, false);
}

void DrumGlueProcessor::getStateInformation (MemoryBlock& dest)
{
    auto s = apvts.copyState();
    s.setProperty ("program", currentProgram, nullptr);
    if (auto xml = s.createXml()) copyXmlToBinary (*xml, dest);
}

void DrumGlueProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto t = ValueTree::fromXml (*xml);
            currentProgram = (int) t.getProperty ("program", 0);
            apvts.replaceState (t);
        }
}

AudioProcessorEditor* DrumGlueProcessor::createEditor() { return new DrumGlueEditor (*this); }

#if ! DG_TEST_BUILD
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DrumGlueProcessor(); }
#endif

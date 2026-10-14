// Offline test runner for Drum Glue: DSP checks + UI screenshot.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdio>

using namespace juce;

static int failures = 0;
static void check (bool ok, const String& what) { std::printf ("[%s] %s\n", ok ? " OK " : "FAIL", what.toRawUTF8()); if (! ok) ++failures; }
static void setP (DrumGlueProcessor& p, const String& id, float v) { auto* prm = p.apvts.getParameter (id); prm->setValueNotifyingHost (prm->convertTo0to1 (v)); }
static void neutral (DrumGlueProcessor& p)
{
    setP (p, "comp_on", 0); setP (p, "eq_on", 0); setP (p, "sat_on", 0); setP (p, "analog", 0);
}
static std::vector<float> run (DrumGlueProcessor& p, std::function<float (int)> in, int total, int block = 512)
{
    std::vector<float> out; out.reserve ((size_t) total);
    AudioBuffer<float> buf (2, block); MidiBuffer m;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = jmin (block, total - pos);
        AudioBuffer<float> b (buf.getArrayOfWritePointers(), 2, 0, n);
        for (int i = 0; i < n; ++i) { const float x = in (pos + i); b.setSample (0, i, x); b.setSample (1, i, x); }
        p.processBlock (b, m);
        for (int i = 0; i < n; ++i) out.push_back (b.getSample (0, i));
    }
    return out;
}
static float rms (const std::vector<float>& x, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += (double) x[i] * x[i]; return (float) std::sqrt (s / (double) (b - a)); }
static float sineDb (DrumGlueProcessor& p, float f, float amp)
{
    auto y = run (p, [&] (int i) { return amp * (float) std::sin (MathConstants<double>::twoPi * f * i / 48000.0); }, 48000);
    return Decibels::gainToDecibels (rms (y, 24000, y.size()) / (amp / std::sqrt (2.0f)));
}
static float harmonicDb (const std::vector<float>& x, double f)
{
    double re = 0, im = 0;
    const size_t n = x.size();
    for (size_t i = 0; i < n; ++i)
    {
        const double w = 0.5 - 0.5 * std::cos (MathConstants<double>::twoPi * (double) i / (double) n);
        re += x[i] * w * std::cos (MathConstants<double>::twoPi * f * (double) i / 48000.0);
        im += x[i] * w * std::sin (MathConstants<double>::twoPi * f * (double) i / 48000.0);
    }
    return (float) Decibels::gainToDecibels (std::sqrt (re * re + im * im) / (double) n * 4.0, -200.0);
}

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const double fs = 48000;
    Random rnd (9);

    // ---- neutral path is clean ------------------------------------------
    {
        DrumGlueProcessor p; p.prepareToPlay (fs, 512); neutral (p);
        bool flat = true;
        for (float f : { 50.f, 1000.f, 10000.f }) { const float g = sineDb (p, f, 0.3f); std::printf ("  neutral @%5.0f Hz: %+.3f dB\n", f, g); flat &= std::abs (g) < 0.1f; }
        std::printf ("  latency %d samples\n", p.getLatencySamples());
        check (flat, "all sections off = flat response");
    }

    // ---- compressor: ratio & character ----------------------------------
    for (int r = 0; r < 4; ++r)
    {
        DrumGlueProcessor p; p.prepareToPlay (fs, 512); neutral (p);
        setP (p, "comp_on", 1); setP (p, "thrust", 0); setP (p, "knee", 0); setP (p, "thresh", -30); setP (p, "ratio", (float) r); setP (p, "makeup", 0);
        const float a = sineDb (p, 1000, 0.5f);         // input about -9 dBFS rms
        static const float ratios[] = { 1.5f, 2, 4, 10 };
        const float inDb = Decibels::gainToDecibels (0.5f / std::sqrt (2.0f)) + 3.0f;
        const float expect = -(inDb + 30.0f) * (1.0f - 1.0f / ratios[r]);
        std::printf ("  ratio %-4s: gain %+.1f dB (expected about %+.1f), GR %.1f\n", dg::GlueCompressor::ratioNames()[r].toRawUTF8(), a, expect, p.grLevel.load());
        check (std::abs (a - expect) < 2.0f, "ratio " + dg::GlueCompressor::ratioNames()[r] + " behaves");
    }
    {
        DrumGlueProcessor p; p.prepareToPlay (fs, 512); neutral (p);
        setP (p, "comp_on", 1); setP (p, "thresh", -25); setP (p, "type", 1);
        auto y = run (p, [&] (int i) { return (i % 12000 < 600 ? 0.9f : 0.05f) * (float) std::sin (i * 0.02); }, 96000);
        bool fin = true; for (auto v : y) fin &= std::isfinite (v) && std::abs (v) < 2.0f;
        check (fin && p.grLevel.load() >= 0.0f, "OLD (feedback) character stable on drum-like hits");
    }
    {
        auto grWith = [&] (bool thrust)
        {
            DrumGlueProcessor p; p.prepareToPlay (fs, 512); neutral (p);
            setP (p, "comp_on", 1); setP (p, "thresh", -30); setP (p, "thrust", thrust ? 1.0f : 0.0f);
            run (p, [] (int i) { return 0.6f * (float) std::sin (MathConstants<double>::twoPi * 50.0 * i / 48000.0); }, 48000);
            return p.grLevel.load();
        };
        const float off = grWith (false), on = grWith (true);
        std::printf ("  50 Hz kick-like tone: GR %.1f dB without THRUST, %.1f dB with THRUST\n", off, on);
        check (on < off - 3.0f, "THRUST stops low end from pumping the compressor");
    }

    // ---- EQ ---------------------------------------------------------------
    {
        DrumGlueProcessor p; p.prepareToPlay (fs, 512); neutral (p);
        setP (p, "eq_on", 1); setP (p, "warmth", 6); setP (p, "air", 6); setP (p, "bite", 4);
        const float w = sineDb (p, 60, 0.2f), mid = sineDb (p, 3000, 0.2f), hi = sineDb (p, 16000, 0.2f);
        setP (p, "lowcut", 120);
        const float lo = sineDb (p, 30, 0.2f);
        std::printf ("  EQ: low cut @30 Hz %+.1f, warmth @60 Hz %+.1f, 3 kHz %+.1f, 16 kHz %+.1f dB\n", lo, w, mid, hi);
        check (lo < -6.0f && w > 2.0f && mid > 2.5f && hi > 4.0f, "tone EQ: low cut, warmth, bite, air");
    }

    // ---- saturation emulations ------------------------------------------
    for (int t = 0; t < 5; ++t)
    {
        DrumGlueProcessor p; p.prepareToPlay (fs, 512); neutral (p);
        setP (p, "sat_on", 1); setP (p, "sat_type", (float) t); setP (p, "drive", 60);
        auto y = run (p, [] (int i) { return 0.4f * (float) std::sin (MathConstants<double>::twoPi * 500.0 * i / 48000.0); }, 48000);
        std::vector<float> tail (y.end() - 16384, y.end());
        const float h1 = harmonicDb (tail, 500), h2 = harmonicDb (tail, 1000), h3 = harmonicDb (tail, 1500);
        bool fin = true; float pk = 0; for (auto v : y) { fin &= std::isfinite (v); pk = jmax (pk, std::abs (v)); }
        std::printf ("  %-7s level %+5.1f dB  H2 %6.1f dBc  H3 %6.1f dBc  peak %.2f\n", DrumGlueProcessor::satNames()[t].toRawUTF8(),
                     h1 - Decibels::gainToDecibels (0.4f), h2 - h1, h3 - h1, pk);
        check (fin && pk < 1.5f && jmax (h2, h3) - h1 > -45.0f, DrumGlueProcessor::satNames()[t] + " adds harmonics, stays stable");
    }

    // ---- presets + state --------------------------------------------------
    {
        DrumGlueProcessor p; p.prepareToPlay (fs, 333);
        bool ok = true;
        for (int i = 0; i < p.getNumPrograms(); ++i)
        {
            p.setCurrentProgram (i);
            auto y = run (p, [&] (int k) { return ((k % 9600) < 400 ? 0.8f : 0.1f) * (rnd.nextFloat() * 2 - 1); }, 48000, 333);
            float pk = 0; bool fin = true; for (auto v : y) { fin &= std::isfinite (v); pk = jmax (pk, std::abs (v)); }
            std::printf ("  preset %-20s GR %5.1f dB  peak %.2f\n", p.getProgramName (i).toRawUTF8(), p.grLevel.load(), pk);
            ok &= fin && pk < 3.0f;
        }
        check (ok, "all presets stable");
        DrumGlueProcessor a, b; a.setCurrentProgram (3);
        MemoryBlock mb; a.getStateInformation (mb); b.setStateInformation (mb.getData(), (int) mb.getSize());
        check (b.getCurrentProgram() == 3 && b.value ("sat_type") == 2.0f, "state save/restore");
    }

    if (argc > 1)
    {
        DrumGlueProcessor p; p.prepareToPlay (fs, 512);
        p.setCurrentProgram (1);
        run (p, [&] (int k) { return ((k % 12000) < 900 ? 0.9f : 0.15f) * (float) std::sin (k * 0.03); }, 48000);
        std::unique_ptr<AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (dgui::MainPanel::W, dgui::MainPanel::H);
        for (int k = 0; k < 10; ++k) { run (p, [&] (int i) { return ((i % 12000) < 900 ? 0.9f : 0.15f) * (float) std::sin (i * 0.03); }, 2400); MessageManager::getInstance()->runDispatchLoopUntil (40); }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, argc > 2 ? (float) std::atof (argv[2]) : 1.0f);
        File out (argv[1]); out.deleteFile(); FileOutputStream os (out);
        check (PNGImageFormat().writeImageToStream (img, os), "wrote screenshot");
    }
    std::printf ("\n%s (%d failures)\n", failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED", failures);
    return failures == 0 ? 0 : 1;
}

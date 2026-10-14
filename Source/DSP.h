#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

namespace dg
{
using AC = juce::dsp::IIR::ArrayCoefficients<float>;

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept  { return 20.0f * std::log10 (std::max (g, 1.0e-7f)); }
inline float timeCoef (float ms, double fs) noexcept { return (float) std::exp (-1.0 / (std::max (0.005, (double) ms) * 0.001 * fs)); }

/** Stereo biquad, transposed direct form II. */
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] {}, z2[2] {};
    void set (const std::array<float, 6>& c) noexcept
    {
        const float inv = 1.0f / c[3];
        b0 = c[0] * inv; b1 = c[1] * inv; b2 = c[2] * inv; a1 = c[4] * inv; a2 = c[5] * inv;
    }
    void identity() noexcept { b0 = 1; b1 = b2 = a1 = a2 = 0; }
    void reset() noexcept { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    float process (int ch, float x) noexcept
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
};

//==============================================================================
/** Bus compressor in the spirit of a classic VCA console / API-style unit.
    NEW = feed-forward (tight, precise). OLD = feed-back (smoother, more "breathing").
    THRUST = side-chain tilt so kick drums do not pump the whole mix. */
class GlueCompressor
{
public:
    struct Params
    {
        float thr = -12; int ratio = 1, attack = 2, release = 4, knee = 1; bool feedback = false, thrust = true;
        float makeup = 0, mix = 1;
    };
    static const juce::StringArray& ratioNames()   { static const juce::StringArray n { "1.5", "2", "4", "10" }; return n; }
    static const juce::StringArray& attackNames()  { static const juce::StringArray n { "0.1", "0.3", "1", "3", "10", "30" }; return n; }
    static const juce::StringArray& releaseNames() { static const juce::StringArray n { "0.1", "0.3", "0.6", "1.2", "AUTO" }; return n; }
    static const juce::StringArray& kneeNames()    { static const juce::StringArray n { "HARD", "MED", "SOFT" }; return n; }

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        scHp.reset(); scTilt.reset();
        scHp.set (AC::makeHighPass (fs, 90.0f, 0.7071f));
        scTilt.set (AC::makeHighShelf (fs, 2500.0f, 0.5f, dbToGain (6.0f)));
        env = gr = fast = slow = 0; prevGain = 1;
    }

    void setParams (const Params& np)
    {
        p = np;
        static const float ratios[] = { 1.5f, 2, 4, 10 }, atk[] = { 0.1f, 0.3f, 1, 3, 10, 30 }, rel[] = { 100, 300, 600, 1200, 0 }, knees[] = { 0, 6, 12 };
        ratio = ratios[juce::jlimit (0, 3, p.ratio)];
        kneeDb = knees[juce::jlimit (0, 2, p.knee)];
        aAtt = timeCoef (atk[juce::jlimit (0, 5, p.attack)], fs);
        autoRel = p.release >= 4;
        aRel = timeCoef (autoRel ? 120.0f : rel[juce::jlimit (0, 3, p.release)], fs);
        aFast = timeCoef (80.0f, fs);
        aSlow = timeCoef (1400.0f, fs);
        aRms = 1.0f - timeCoef (2.0f, fs);
        makeupG = dbToGain (p.makeup);
    }

    void process (float& l, float& r) noexcept
    {
        float sl = l, sr = r;
        if (p.feedback) { sl *= prevGain; sr *= prevGain; }
        if (p.thrust)
        {
            sl = scTilt.process (0, scHp.process (0, sl));
            sr = scTilt.process (1, scHp.process (1, sr));
        }
        const float pk = std::max (sl * sl, sr * sr);              // stereo linked
        env += aRms * (pk - env);
        const float lvl = 10.0f * std::log10 (env + 1.0e-12f) + 3.0f;
        const float target = -kneeGain (lvl, p.thr, ratio, kneeDb);

        if (autoRel)
        {
            fast = target > fast ? aAtt * fast + (1 - aAtt) * target : aFast * fast + (1 - aFast) * target;
            slow = target > slow ? aAtt * slow + (1 - aAtt) * target : aSlow * slow + (1 - aSlow) * target;
            gr = std::max (fast, slow * 0.75f);
        }
        else
            gr = target > gr ? aAtt * gr + (1 - aAtt) * target : aRel * gr + (1 - aRel) * target;

        const float g = dbToGain (-gr);
        prevGain = g;
        const float wl = l * g * makeupG, wr = r * g * makeupG;
        l = l + p.mix * (wl - l);
        r = r + p.mix * (wr - r);
    }

    float getGr() const noexcept { return gr; }

    static float kneeGain (float x, float thr, float ratio, float knee) noexcept
    {
        const float over = x - thr, slope = 1.0f / ratio - 1.0f;
        if (knee <= 0.01f) return over > 0 ? slope * over : 0.0f;
        if (2.0f * over <= -knee) return 0.0f;
        if (2.0f * over < knee) return slope * (over + 0.5f * knee) * (over + 0.5f * knee) / (2.0f * knee);
        return slope * over;
    }

private:
    double fs = 48000;
    Params p;
    Biquad scHp, scTilt;
    float ratio = 2, kneeDb = 6, aAtt = 0, aRel = 0, aFast = 0, aSlow = 0, aRms = 1, makeupG = 1;
    float env = 0, gr = 0, fast = 0, slow = 0, prevGain = 1;
    bool autoRel = true;
};

//==============================================================================
/** Tone EQ: low cut, warmth (low shelf), bite (mid bell), air (high shelf). */
class ToneEq
{
public:
    struct Params { float lowCut = 20; float warmth = 0; int warmthFreq = 1; float bite = 0; int biteFreq = 1; float air = 0; int airFreq = 1; };
    static const juce::StringArray& warmthFreqs() { static const juce::StringArray n { "60", "100", "150", "220" }; return n; }
    static const juce::StringArray& biteFreqs()   { static const juce::StringArray n { "1.5k", "3k", "5k" }; return n; }
    static const juce::StringArray& airFreqs()    { static const juce::StringArray n { "8k", "12k", "16k" }; return n; }

    void prepare (double sampleRate) { fs = sampleRate; for (auto* b : { &lc, &warm, &bite, &air }) b->reset(); last.lowCut = -1; }
    void setParams (const Params& np)
    {
        if (np.lowCut == last.lowCut && np.warmth == last.warmth && np.warmthFreq == last.warmthFreq && np.bite == last.bite
            && np.biteFreq == last.biteFreq && np.air == last.air && np.airFreq == last.airFreq) return;
        last = np;
        static const float wf[] = { 60, 100, 150, 220 }, bf[] = { 1500, 3000, 5000 }, af[] = { 8000, 12000, 16000 };
        const float nyq = (float) fs * 0.45f;
        lcOn = np.lowCut > 20.5f;
        if (lcOn) lc.set (AC::makeHighPass (fs, np.lowCut, 0.7071f));
        warm.set (AC::makeLowShelf (fs, wf[juce::jlimit (0, 3, np.warmthFreq)], 0.7f, dbToGain (np.warmth)));
        bite.set (AC::makePeakFilter (fs, bf[juce::jlimit (0, 2, np.biteFreq)], 0.9f, dbToGain (np.bite)));
        air.set (AC::makeHighShelf (fs, std::min (af[juce::jlimit (0, 2, np.airFreq)], nyq), 0.6f, dbToGain (np.air)));
    }
    float process (int ch, float x) noexcept
    {
        if (lcOn) x = lc.process (ch, x);
        return air.process (ch, bite.process (ch, warm.process (ch, x)));
    }
private:
    double fs = 48000;
    Params last;
    Biquad lc, warm, bite, air;
    bool lcOn = false;
};

//==============================================================================
namespace shapers
{
    inline float triode (float x) noexcept      // asymmetric: strong 2nd harmonic
    {
        constexpr float k = 0.55f;
        return x >= 0.0f ? 1.0f - std::exp (-x) : -(1.0f - std::exp (k * x)) / k;
    }
    inline float warmth (float x) noexcept      // biased tanh
    {
        constexpr float b = 0.3f;
        static const float tb = std::tanh (b), n = 1.0f / (1.0f - tb * tb);
        return (std::tanh (x + b) - tb) * n;
    }
    inline float tape (float x) noexcept        // soft, round compression of peaks
    {
        const float a = std::abs (x);
        return x / std::cbrt (1.0f + a * a * a);
    }
    inline float analog (float x) noexcept      // console / transformer: mostly odd with a touch of even
    {
        const float y = std::tanh (1.15f * x + 0.06f * x * x) / 1.15f;
        return y;
    }
    inline float retro (float x) noexcept       // harder, gritty
    {
        return x / std::sqrt (1.0f + x * x * 1.8f) * 1.15f;
    }
}

} // namespace dg

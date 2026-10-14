#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <unordered_map>
#include "DSP.h"

class DrumGlueProcessor : public juce::AudioProcessor
{
public:
    DrumGlueProcessor();
    ~DrumGlueProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Drum Glue"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& satNames();
    float value (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    std::atomic<float> inLevel { -100 }, outLevel { -100 }, grLevel { 0 };

private:
    void processChunk (juce::AudioBuffer<float>&);
    float p (const char* id) const { return cache.at (id)->load(); }
    float saturate (int type, int ch, float x, float g) noexcept;

    dg::GlueCompressor comp;
    dg::ToneEq eq;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    dg::Biquad tapeBump, tapeTop, retroHp, retroLp, colLow, colHigh;
    juce::SmoothedValue<float> inG, outG, driveSm, satMixSm;
    float dcX[2] {}, dcY[2] {}, dcR = 0.999f;
    double fs = 48000, osFs = 96000;
    int maxBlock = 512, lastSat = -1, currentProgram = 0;
    std::unordered_map<std::string, std::atomic<float>*> cache;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumGlueProcessor)
};

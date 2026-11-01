#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class BassFXProcessor : public juce::AudioProcessor
{
public:
    BassFXProcessor();
    ~BassFXProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BassFX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::StringArray getDistPresetNames();
    static juce::StringArray getReverbPresetNames();

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float>* distOnP     = nullptr;
    std::atomic<float>* distPresetP = nullptr;
    std::atomic<float>* distDriveP  = nullptr;
    std::atomic<float>* distMixP    = nullptr;
    std::atomic<float>* distKeepP   = nullptr;
    std::atomic<float>* revOnP      = nullptr;
    std::atomic<float>* revPresetP  = nullptr;
    std::atomic<float>* revMixP     = nullptr;
    std::atomic<float>* outP        = nullptr;

    juce::dsp::LinkwitzRileyFilter<float> crossover;
    juce::dsp::IIR::Filter<float> tone[2], dcBlock[2], wetHpf[2];
    juce::dsp::Reverb reverb;
    juce::AudioBuffer<float> wetBuffer;

    juce::SmoothedValue<float> distDrive, distMix, revMix, outGain;

    double currentSampleRate = 44100.0;
    int lastDistPreset = -1;
    int lastRevPreset  = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassFXProcessor)
};

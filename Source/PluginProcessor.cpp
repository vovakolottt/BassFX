#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // ---------- ПРЕСЕТЫ ДИСТОРШНА ----------
    // type: 0 = Warm Tube, 1 = Fuzz, 2 = Grit, 3 = Fold
    struct DistPreset { int type; float driveMul; float toneHz; float makeup; };
    const DistPreset distPresets[] =
    {
        { 0, 1.0f, 5500.0f, 0.90f },  // Warm Tube
        { 1, 1.5f, 4500.0f, 0.60f },  // Fuzz
        { 2, 1.0f, 6500.0f, 0.90f },  // Grit
        { 3, 0.8f, 7000.0f, 0.70f },  // Fold
    };

    // ---------- ПРЕСЕТЫ РЕВЕРБА ----------
    // hpHz = фильтр верхних частот перед ревербом (чтобы не мутить низ)
    // side = усиление боковой (Side) составляющей для ширины
    struct ReverbPreset { float room, damp, width, hpHz, wetGain, side; };
    const ReverbPreset reverbPresets[] =
    {
        { 0.85f, 0.40f, 1.0f, 200.0f, 0.90f, 1.4f },  // Wide Hall
        { 0.60f, 0.20f, 1.0f, 250.0f, 0.80f, 1.2f },  // Plate
        { 0.30f, 0.50f, 0.8f, 180.0f, 0.80f, 1.0f },  // Small Room
        { 0.97f, 0.60f, 1.0f, 300.0f, 0.70f, 1.6f },  // Ambient Wash
    };

    float shape (int type, float x)
    {
        switch (type)
        {
            case 0: { const float b = 0.15f;                         // асимметричный tanh (ламповый)
                      return std::tanh (x + b) - std::tanh (b); }
            case 1:  return juce::jlimit (-1.0f, 1.0f, x * 2.0f);     // жёсткий клиппинг
            case 2:  return 0.6366f * std::atan (x * 1.5f) * 1.3f;    // мягкий гранж
            case 3:  return std::sin (x * 1.5f);                      // wavefolder
            default: return x;
        }
    }
}

juce::StringArray BassFXProcessor::getDistPresetNames()
{
    return { "Warm Tube", "Fuzz", "Grit", "Fold" };
}

juce::StringArray BassFXProcessor::getReverbPresetNames()
{
    return { "Wide Hall", "Plate", "Small Room", "Ambient Wash" };
}

juce::AudioProcessorValueTreeState::ParameterLayout BassFXProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "distOn", 1 },     "Distortion On", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "distPreset", 1 }, "Distortion Preset", getDistPresetNames(), 0));
    l.add (std::make_unique<AudioParameterFloat>  (ParameterID { "distDrive", 1 },  "Drive", NormalisableRange<float> (0.0f, 1.0f), 0.4f));
    l.add (std::make_unique<AudioParameterFloat>  (ParameterID { "distMix", 1 },    "Dist Mix", NormalisableRange<float> (0.0f, 1.0f), 1.0f));
    l.add (std::make_unique<AudioParameterFloat>  (ParameterID { "distKeep", 1 },   "Clean Low Freq",
                                                   NormalisableRange<float> (40.0f, 300.0f, 0.0f, 0.5f), 100.0f));

    l.add (std::make_unique<AudioParameterBool>   (ParameterID { "revOn", 1 },      "Reverb On", false));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "revPreset", 1 },  "Reverb Preset", getReverbPresetNames(), 0));
    l.add (std::make_unique<AudioParameterFloat>  (ParameterID { "revMix", 1 },     "Reverb Mix", NormalisableRange<float> (0.0f, 1.0f), 0.3f));

    l.add (std::make_unique<AudioParameterFloat>  (ParameterID { "out", 1 },        "Output dB", NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    return l;
}

BassFXProcessor::BassFXProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    distOnP     = apvts.getRawParameterValue ("distOn");
    distPresetP = apvts.getRawParameterValue ("distPreset");
    distDriveP  = apvts.getRawParameterValue ("distDrive");
    distMixP    = apvts.getRawParameterValue ("distMix");
    distKeepP   = apvts.getRawParameterValue ("distKeep");
    revOnP      = apvts.getRawParameterValue ("revOn");
    revPresetP  = apvts.getRawParameterValue ("revPreset");
    revMixP     = apvts.getRawParameterValue ("revMix");
    outP        = apvts.getRawParameterValue ("out");
}

bool BassFXProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void BassFXProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    crossover.prepare (spec);
    crossover.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    crossover.reset();

    reverb.prepare (spec);
    reverb.reset();

    wetBuffer.setSize (2, juce::jmax (samplesPerBlock, 512));

    for (int ch = 0; ch < 2; ++ch)
    {
        tone[ch].reset();
        wetHpf[ch].reset();
        dcBlock[ch].reset();
        dcBlock[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 25.0f);
    }

    lastDistPreset = -1;
    lastRevPreset  = -1;

    distDrive.reset (sampleRate, 0.05);
    distMix.reset   (sampleRate, 0.05);
    revMix.reset    (sampleRate, 0.05);
    outGain.reset   (sampleRate, 0.05);

    distDrive.setCurrentAndTargetValue (distDriveP->load());
    distMix.setCurrentAndTargetValue   (distOnP->load() > 0.5f ? distMixP->load() : 0.0f);
    revMix.setCurrentAndTargetValue    (0.0f);
    outGain.setCurrentAndTargetValue   (juce::Decibels::decibelsToGain (outP->load()));
}

void BassFXProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n     = buffer.getNumSamples();
    if (numCh == 0 || n == 0) return;

    // =================== DISTORTION ===================
    const int dIdx = juce::jlimit (0, juce::numElementsInArray (distPresets) - 1, (int) distPresetP->load());
    const auto& dp = distPresets[dIdx];

    if (dIdx != lastDistPreset)
    {
        for (int ch = 0; ch < 2; ++ch)
            tone[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSampleRate, dp.toneHz);
        lastDistPreset = dIdx;
    }

    crossover.setCutoffFrequency (distKeepP->load());
    distDrive.setTargetValue (distDriveP->load());
    distMix.setTargetValue   (distOnP->load() > 0.5f ? distMixP->load() : 0.0f);

    for (int i = 0; i < n; ++i)
    {
        const float drive = distDrive.getNextValue();
        const float mix   = distMix.getNextValue();
        const float g     = 1.0f + drive * 30.0f * dp.driveMul;

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* d = buffer.getWritePointer (ch);
            float low = 0.0f, high = 0.0f;
            crossover.processSample (ch, d[i], low, high);   // низ остаётся чистым

            float shaped = shape (dp.type, high * g) * dp.makeup;
            shaped = tone[ch].processSample (shaped);
            shaped = dcBlock[ch].processSample (shaped);

            d[i] = low + high + (shaped - high) * mix;
        }
    }

    // =================== REVERB ===================
    const int rIdx = juce::jlimit (0, juce::numElementsInArray (reverbPresets) - 1, (int) revPresetP->load());
    const auto& rp = reverbPresets[rIdx];

    if (rIdx != lastRevPreset)
    {
        juce::Reverb::Parameters p;
        p.roomSize   = rp.room;
        p.damping    = rp.damp;
        p.width      = rp.width;
        p.wetLevel   = 0.4f;
        p.dryLevel   = 0.0f;
        p.freezeMode = 0.0f;
        reverb.setParameters (p);

        for (int ch = 0; ch < 2; ++ch)
            wetHpf[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, rp.hpHz);
        lastRevPreset = rIdx;
    }

    if (wetBuffer.getNumSamples() < n)
        wetBuffer.setSize (2, n, false, false, true);

    for (int ch = 0; ch < numCh; ++ch)
    {
        wetBuffer.copyFrom (ch, 0, buffer, ch, 0, n);
        float* w = wetBuffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
            w[i] = wetHpf[ch].processSample (w[i]);   // реверб только на верха
    }

    {
        juce::dsp::AudioBlock<float> block (wetBuffer);
        auto sub = block.getSubsetChannelBlock (0, (size_t) numCh).getSubBlock (0, (size_t) n);
        juce::dsp::ProcessContextReplacing<float> ctx (sub);
        reverb.process (ctx);
    }

    revMix.setTargetValue (revOnP->load() > 0.5f ? revMixP->load() * rp.wetGain : 0.0f);

    const float* wL = wetBuffer.getReadPointer (0);
    const float* wR = numCh > 1 ? wetBuffer.getReadPointer (1) : nullptr;
    float* oL = buffer.getWritePointer (0);
    float* oR = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        const float m = revMix.getNextValue();
        if (oR != nullptr)
        {
            const float mid  = 0.5f * (wL[i] + wR[i]);
            const float side = 0.5f * (wL[i] - wR[i]) * rp.side;   // расширяем стерео
            oL[i] += (mid + side) * m;
            oR[i] += (mid - side) * m;
        }
        else
            oL[i] += wL[i] * m;
    }

    // =================== OUTPUT ===================
    outGain.setTargetValue (juce::Decibels::decibelsToGain (outP->load()));
    for (int i = 0; i < n; ++i)
    {
        const float g = outGain.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }
}

juce::AudioProcessorEditor* BassFXProcessor::createEditor()
{
    return new BassFXEditor (*this);
}

void BassFXProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void BassFXProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BassFXProcessor();
}

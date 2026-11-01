#pragma once
#include "PluginProcessor.h"

class BassFXEditor : public juce::AudioProcessorEditor
{
public:
    explicit BassFXEditor (BassFXProcessor&);
    ~BassFXEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob { juce::Slider slider; juce::Label label; };
    void setupKnob (Knob& k, const juce::String& text);
    static void place (Knob& k, juce::Rectangle<int> r);

    BassFXProcessor& proc;

    juce::ToggleButton distOn { "DISTORTION" }, revOn { "REVERB" };
    juce::ComboBox distPreset, revPreset;
    Knob drive, distMix, keep, revMix, out;

    juce::Rectangle<int> leftPanel, rightPanel;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    // атрибуты объявлены после компонентов, поэтому уничтожаются раньше них
    std::unique_ptr<BA> distOnA, revOnA;
    std::unique_ptr<CA> distPresetA, revPresetA;
    std::unique_ptr<SA> driveA, distMixA, keepA, revMixA, outA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BassFXEditor)
};

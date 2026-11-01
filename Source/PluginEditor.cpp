#include "PluginEditor.h"

namespace
{
    const juce::Colour bg      (0xff15171c);
    const juce::Colour panel   (0xff1f232b);
    const juce::Colour distCol (0xffff7a3d);
    const juce::Colour revCol  (0xff3dc7ff);
}

void BassFXEditor::setupKnob (Knob& k, const juce::String& text)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.label);
}

void BassFXEditor::place (Knob& k, juce::Rectangle<int> r)
{
    k.label.setBounds (r.removeFromTop (18));
    k.slider.setBounds (r);
}

BassFXEditor::BassFXEditor (BassFXProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (600, 400);

    addAndMakeVisible (distOn);
    addAndMakeVisible (revOn);
    distOn.setColour (juce::ToggleButton::tickColourId, distCol);
    revOn.setColour  (juce::ToggleButton::tickColourId, revCol);

    addAndMakeVisible (distPreset);
    addAndMakeVisible (revPreset);
    distPreset.addItemList (BassFXProcessor::getDistPresetNames(), 1);
    revPreset.addItemList  (BassFXProcessor::getReverbPresetNames(), 1);

    setupKnob (drive,   "Drive");
    setupKnob (distMix, "Mix");
    setupKnob (keep,    "Clean Low (Hz)");
    setupKnob (revMix,  "Mix");
    setupKnob (out,     "Output (dB)");

    drive.slider.setColour   (juce::Slider::rotarySliderFillColourId, distCol);
    distMix.slider.setColour (juce::Slider::rotarySliderFillColourId, distCol);
    keep.slider.setColour    (juce::Slider::rotarySliderFillColourId, distCol);
    revMix.slider.setColour  (juce::Slider::rotarySliderFillColourId, revCol);

    auto& v = proc.apvts;
    distOnA     = std::make_unique<BA> (v, "distOn", distOn);
    revOnA      = std::make_unique<BA> (v, "revOn", revOn);
    distPresetA = std::make_unique<CA> (v, "distPreset", distPreset);
    revPresetA  = std::make_unique<CA> (v, "revPreset", revPreset);
    driveA      = std::make_unique<SA> (v, "distDrive", drive.slider);
    distMixA    = std::make_unique<SA> (v, "distMix", distMix.slider);
    keepA       = std::make_unique<SA> (v, "distKeep", keep.slider);
    revMixA     = std::make_unique<SA> (v, "revMix", revMix.slider);
    outA        = std::make_unique<SA> (v, "out", out.slider);
}

void BassFXEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    g.setColour (panel);
    g.fillRoundedRectangle (leftPanel.toFloat(),  12.0f);
    g.fillRoundedRectangle (rightPanel.toFloat(), 12.0f);

    g.setColour (distCol.withAlpha (0.6f));
    g.drawRoundedRectangle (leftPanel.toFloat(), 12.0f, 1.5f);
    g.setColour (revCol.withAlpha (0.6f));
    g.drawRoundedRectangle (rightPanel.toFloat(), 12.0f, 1.5f);

    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("BassFX", 20, getHeight() - 70, 200, 40, juce::Justification::centredLeft);
}

void BassFXEditor::resized()
{
    auto area   = getLocalBounds().reduced (16);
    auto footer = area.removeFromBottom (90);
    place (out, footer.removeFromRight (130));

    leftPanel  = area.removeFromLeft (area.getWidth() / 2).reduced (6);
    rightPanel = area.reduced (6);

    // --- левая панель: дисторшн ---
    {
        auto r = leftPanel.reduced (14);
        distOn.setBounds     (r.removeFromTop (32));
        r.removeFromTop (6);
        distPreset.setBounds (r.removeFromTop (30));
        r.removeFromTop (14);
        const int w = r.getWidth() / 3;
        place (drive,   r.removeFromLeft (w));
        place (distMix, r.removeFromLeft (w));
        place (keep,    r);
    }

    // --- правая панель: реверб ---
    {
        auto r = rightPanel.reduced (14);
        revOn.setBounds     (r.removeFromTop (32));
        r.removeFromTop (6);
        revPreset.setBounds (r.removeFromTop (30));
        r.removeFromTop (14);
        place (revMix, r.withSizeKeepingCentre (110, r.getHeight()));
    }
}

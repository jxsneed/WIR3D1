#pragma once
#include <JuceHeader.h>

class WIR3DProcessor;

class WiredLook : public juce::LookAndFeel_V4
{
public:
    WiredLook();
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float pos, float start, float end, juce::Slider& s) override;
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& bgColour,
                              bool isMouseOver, bool isButtonDown) override;
};

class WIR3DEditor : public juce::AudioProcessorEditor
{
public:
    explicit WIR3DEditor(WIR3DProcessor&);
    ~WIR3DEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    WIR3DProcessor& processor;

    // Main row knobs
    juce::Slider bend, tune, speed, harmo, smile;
    // Secondary row knobs
    juce::Slider rptMix, masterFilter;
    // Dropdowns & Buttons
    juce::ComboBox repeatDiv, key, scale, rangeBox, presetBox;
    juce::TextButton saveBtn{"SAVE"};

    std::unique_ptr<SliderAttachment> bendAtt, tuneAtt, speedAtt, harmoAtt, smileAtt;
    std::unique_ptr<SliderAttachment> rptMixAtt, filterAtt;
    std::unique_ptr<ComboAttachment> repeatAtt, keyAtt, scaleAtt, rangeAtt;

    WiredLook look;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WIR3DEditor)
};

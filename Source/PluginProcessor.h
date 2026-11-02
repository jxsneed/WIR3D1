#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>
#include "PitchDetector.h"
#include "ScaleQuantizer.h"

class WIR3DProcessor : public juce::AudioProcessor
{
public:
    WIR3DProcessor();
    ~WIR3DProcessor() override = default;

    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    void loadPreset(int index);
    void saveUserPreset(int slotIndex, const juce::String& name);
    juce::StringArray getPresetNames() const;

    juce::AudioProcessorValueTreeState params;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    void updatePitchRange(int rangeIndex);

    double sampleRate = 44100.0;

    // Advanced Pitch Detector & Quantizer (inspired by qpitch-2.0)
    PitchDetector pitchDetector;
    int currentRangeIndex = 0;
    int lockedTargetMidi = -1;
    float smoothedTargetMidi = -1.0f;
    float smoothedInputMidi = -1.0f;
    int pitchHoldSamples = 0;
    float currentPitchRatio = 1.0f;
    float smoothRatio = 1.0f;

    // Mono input buffer for pitch analysis
    std::vector<float> monoAnalysisBuffer;

    // Pitch shifter primary delay lines
    std::vector<float> delay[2];
    int writePos = 0;
    float phase = 0.0f;
    float grainDepth = 1500.0f;

    // Harmonizer secondary pitch shift & ping-pong delay lines
    float harmoPhase = 0.0f;
    std::vector<float> pingPongL;
    std::vector<float> pingPongR;
    int pingPongPosL = 0;
    int pingPongPosR = 0;
    float pingPongFilterL = 0.0f;
    float pingPongFilterR = 0.0f;

    // Beat Repeat buffer
    std::vector<float> repeatBuf[2];
    int repeatWritePos = 0;
    int repeatReadPos = 0;
    int currentSliceLength = 2048;
    float repeatFilterState[2] = {0.0f, 0.0f};

    // Smile & Filter states
    std::array<float, 2> hpState = {0.0f, 0.0f};
    std::array<float, 2> echoState = {0.0f, 0.0f};
    std::vector<std::vector<float>> smileEcho;
    int smileEchoPos = 0;

    // Master Filter (Bipolar LP / HP)
    std::array<float, 2> masterFiltState1 = {0.0f, 0.0f};
    std::array<float, 2> masterFiltState2 = {0.0f, 0.0f};

    // Presets
    struct PresetData {
        juce::String name;
        float bend, tune, speed, harmo, smile;
        int repeat;
        float rptMix, filter;
        int root, scale, range;
    };
    std::vector<PresetData> presets;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WIR3DProcessor)
};

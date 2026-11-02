#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

static constexpr float kPi = 3.14159265358979323846f;

WIR3DProcessor::WIR3DProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "PARAMS", createParams())
#endif
{
    // Presets with updated scales and vocal ranges
    presets = {
        {"01 // Wired Lain",      0.0f,  80.0f,  85.0f, 35.0f, 25.0f, 2,  0.0f,   0.0f, 0, 2, 0},
        {"02 // Matrix Snap",     0.0f, 100.0f, 100.0f, 45.0f, 40.0f, 2, 40.0f,   0.0f, 0, 0, 0},
        {"03 // Protocol 7",      7.0f,  65.0f,  70.0f, 75.0f, 45.0f, 1, 15.0f,  15.0f, 0, 4, 2},
        {"04 // Shibuya Net",     0.0f,  85.0f,  90.0f, 30.0f, 30.0f, 3, 20.0f,  30.0f, 0, 1, 2},
        {"05 // Glitch Terminal", 0.0f,  90.0f,  95.0f, 40.0f, 60.0f, 4, 70.0f, -20.0f, 0, 4, 0},
        {"06 // Analog Ghost",   -5.0f,  45.0f,  40.0f, 60.0f, 15.0f, 1,  0.0f, -40.0f, 0, 2, 1},
        {"07 // User 1",          0.0f,  75.0f,  85.0f,  0.0f,  0.0f, 2,  0.0f,   0.0f, 0, 0, 0},
        {"08 // User 2",          0.0f,  75.0f,  85.0f,  0.0f,  0.0f, 2,  0.0f,   0.0f, 0, 0, 0}
    };
}

juce::AudioProcessorValueTreeState::ParameterLayout WIR3DProcessor::createParams()
{
    using P = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<P>("bend", "BEND", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<P>("tune", "TUNE", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 75.0f));
    layout.add(std::make_unique<P>("speed", "SPEED", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 85.0f));
    layout.add(std::make_unique<P>("harmo", "HARMO DELAY", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 35.0f));
    layout.add(std::make_unique<P>("smile", "SMILE", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f));
    layout.add(std::make_unique<C>("repeat", "BEAT REPEAT", juce::StringArray{"1/4", "1/8", "1/16", "1/32", "Glitch"}, 2));
    layout.add(std::make_unique<P>("rptMix", "RPT MIX", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<P>("filter", "MASTER FILTER", juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<C>("root", "KEY", juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 0));

    juce::StringArray scaleChoices;
    for (int s = 0; s < ScaleQuantizer::NumScales; ++s)
        scaleChoices.add(ScaleQuantizer::getScaleName(s));
    layout.add(std::make_unique<C>("scale", "SCALE", scaleChoices, 2));

    layout.add(std::make_unique<C>("range", "VOCAL RANGE",
        juce::StringArray{"Wide (All)", "Low (Bass/Bari)", "Mid (Tenor/Alto)", "High (Soprano)"}, 0));

    return layout;
}

void WIR3DProcessor::updatePitchRange(int rangeIndex)
{
    currentRangeIndex = rangeIndex;
    struct RangeHz { float minHz; float maxHz; };
    static constexpr RangeHz ranges[] = {
        { 55.0f, 1100.0f }, // Wide
        { 55.0f,  300.0f }, // Low
        { 90.0f,  550.0f }, // Mid
        { 160.0f, 1100.0f } // High
    };
    const int idx = juce::jlimit(0, 3, rangeIndex);
    pitchDetector.setFrequencyRange(ranges[idx].minHz, ranges[idx].maxHz);
}

void WIR3DProcessor::prepareToPlay(double sr, int maxBlock)
{
    sampleRate = sr;
    pitchDetector.prepare(sr, maxBlock);

    auto* rangeParam = params.getRawParameterValue("range");
    updatePitchRange(rangeParam ? (int)rangeParam->load() : 0);

    const int maxDelay = (int)(sr * 0.6) + 8192;
    for (auto& d : delay) d.assign((size_t)maxDelay, 0.0f);
    writePos = 0;

    phase = 0.0f;
    harmoPhase = 0.0f;
    smoothRatio = 1.0f;
    currentPitchRatio = 1.0f;
    grainDepth = (float)(sr * 0.035);

    lockedTargetMidi = -1;
    smoothedTargetMidi = -1.0f;
    smoothedInputMidi = -1.0f;
    pitchHoldSamples = 0;

    monoAnalysisBuffer.assign(static_cast<size_t>(maxBlock + 64), 0.0f);

    // Ping-Pong Delay buffers (~1 sec)
    const size_t ppSize = static_cast<size_t>(sr * 1.0) + 16;
    pingPongL.assign(ppSize, 0.0f);
    pingPongR.assign(ppSize, 0.0f);
    pingPongPosL = 0;
    pingPongPosR = 0;
    pingPongFilterL = 0.0f;
    pingPongFilterR = 0.0f;

    // Beat Repeat buffer (~1.5 sec)
    const size_t repSize = static_cast<size_t>(sr * 1.5) + 32;
    for (auto& rb : repeatBuf) rb.assign(repSize, 0.0f);
    repeatWritePos = 0;
    repeatReadPos = 0;
    currentSliceLength = (int)(sr * 0.125);
    repeatFilterState[0] = 0.0f;
    repeatFilterState[1] = 0.0f;

    // Smile & Filter states
    hpState = {0.0f, 0.0f};
    echoState = {0.0f, 0.0f};
    const size_t smileEchoSize = static_cast<size_t>(sr * 0.6) + 32;
    smileEcho.assign(2, std::vector<float>(smileEchoSize, 0.0f));
    smileEchoPos = 0;

    masterFiltState1 = {0.0f, 0.0f};
    masterFiltState2 = {0.0f, 0.0f};
}

void WIR3DProcessor::releaseResources()
{
    pitchDetector.reset();
}

bool WIR3DProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

void WIR3DProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = buffer.getNumChannels();
    const int n = buffer.getNumSamples();
    if (channels == 0 || n == 0) return;

    auto get = [this](const char* id) { return params.getRawParameterValue(id)->load(); };

    const float bend = get("bend");
    const float tune = get("tune") * 0.01f;
    const float speed = get("speed") * 0.01f;
    const float harmo = get("harmo") * 0.01f;
    const float smile = get("smile") * 0.01f;
    const int repeatDiv = (int)get("repeat");
    const float rptMix = get("rptMix") * 0.01f;
    const float masterFiltVal = get("filter");
    const int root = (int)get("root");
    const int scale = (int)get("scale");
    const int rangeVal = (int)get("range");

    if (rangeVal != currentRangeIndex)
        updatePitchRange(rangeVal);

    // Prepare mono signal for pitch detector
    if ((int)monoAnalysisBuffer.size() < n)
        monoAnalysisBuffer.resize((size_t)(n + 32), 0.0f);

    for (int i = 0; i < n; ++i)
    {
        float mono = buffer.getSample(0, i);
        if (channels > 1) mono = 0.5f * (mono + buffer.getSample(1, i));
        monoAnalysisBuffer[(size_t)i] = mono;
    }

    // High accuracy YIN pitch tracking
    const float detectedHz = pitchDetector.detectPitch(monoAnalysisBuffer.data(), n);
    const bool pitchValid = detectedHz > 0.0f && pitchDetector.isPitchValid();
    const int maxHoldSamples = static_cast<int>(0.160 * sampleRate);

    if (pitchValid)
    {
        float inputMidi = ScaleQuantizer::hzToMidi(detectedHz);

        // De-octaving on input tracking
        if (smoothedInputMidi >= 0.0f)
        {
            while (inputMidi - smoothedInputMidi > 6.0f) inputMidi -= 12.0f;
            while (smoothedInputMidi - inputMidi > 6.0f) inputMidi += 12.0f;
        }
        if (smoothedInputMidi < 0.0f)
            smoothedInputMidi = inputMidi;
        smoothedInputMidi = smoothedInputMidi * 0.70f + inputMidi * 0.30f;

        // Target Scale Note with Hysteresis (prevents vibrato warble)
        const int nearestMidi = ScaleQuantizer::findNearestScaleMidi(inputMidi, root, scale);

        if (lockedTargetMidi < 0)
        {
            lockedTargetMidi = nearestMidi;
            smoothedTargetMidi = static_cast<float>(lockedTargetMidi);
        }
        else if (nearestMidi != lockedTargetMidi)
        {
            const float currentDistance = std::abs(inputMidi - static_cast<float>(lockedTargetMidi));
            const float newDistance = std::abs(inputMidi - static_cast<float>(nearestMidi));
            const float hysteresis = (speed >= 0.90f) ? 0.12f : 0.36f;
            const float forcedDistance = (speed >= 0.90f) ? 0.60f : 1.05f;

            if (newDistance + hysteresis < currentDistance || currentDistance > forcedDistance)
            {
                lockedTargetMidi = nearestMidi;
            }
        }

        // Target glide vs hard snap
        if (speed >= 0.92f)
        {
            smoothedTargetMidi = static_cast<float>(lockedTargetMidi);
        }
        else
        {
            const float transitionMs = std::max(2.0f, (1.0f - speed) * 320.0f);
            const float transCoeff = std::exp(-static_cast<float>(n)
                                              / (transitionMs * 0.001f * static_cast<float>(sampleRate)));
            smoothedTargetMidi = smoothedTargetMidi * transCoeff + static_cast<float>(lockedTargetMidi) * (1.0f - transCoeff);
        }

        pitchHoldSamples = maxHoldSamples;

        // Pitch-synchronous adaptive grain window
        const float periodSamples = static_cast<float>(sampleRate / detectedHz);
        grainDepth = std::clamp(periodSamples * 2.5f, 256.0f, static_cast<float>(sampleRate * 0.040));
    }
    else
    {
        if (pitchHoldSamples > 0)
            pitchHoldSamples -= n;
    }

    // Calculate target ratio
    float rawCorrection = 0.0f;
    if (smoothedTargetMidi >= 0.0f && (pitchValid || pitchHoldSamples > 0))
    {
        rawCorrection = (smoothedTargetMidi - smoothedInputMidi) * tune;
    }
    const float totalShift = bend + rawCorrection;
    const float targetRatio = std::clamp(std::pow(2.0f, totalShift / 12.0f), 0.5f, 2.0f);

    // Instant hard snap if SPEED >= 0.92, else smooth sample transition
    float smoothFactor = 1.0f;
    if (speed < 0.92f)
    {
        const float normSpeed = speed / 0.92f;
        const float minTau = 0.002f;
        const float maxTau = 0.320f;
        const float tau = maxTau * std::pow(minTau / maxTau, normSpeed);
        smoothFactor = 1.0f - std::exp(-1.0f / (tau * static_cast<float>(sampleRate)));
    }

    const int delaySize = (int)delay[0].size();
    const float depth = grainDepth;
    const float baseDelay = depth * 1.5f;

    // Harmonizer +7 semitones (musical fifth)
    const float harmoRatio = std::pow(2.0f, 7.0f / 12.0f);
    const float harmoPhaseRate = (1.0f - harmoRatio) / depth;

    // Ping-pong delay parameters
    const int ppDelayL = juce::jlimit(1, (int)pingPongL.size() - 1, (int)(sampleRate * 0.18));
    const int ppDelayR = juce::jlimit(1, (int)pingPongR.size() - 1, (int)(sampleRate * 0.36));

    // Beat Repeat slice size
    static const float sliceDurations[] = {0.40f, 0.20f, 0.10f, 0.05f, 0.025f};
    const int targetSliceSamples = juce::jlimit(128, (int)repeatBuf[0].size() - 16,
        (int)(sampleRate * sliceDurations[juce::jlimit(0, 4, repeatDiv)]));
    currentSliceLength = targetSliceSamples;

    // Master filter setup
    const bool masterFilterActive = std::abs(masterFiltVal) > 1.0f;
    float mfAlpha = 0.0f;
    bool isLP = true;
    if (masterFilterActive)
    {
        if (masterFiltVal < 0.0f)
        {
            isLP = true;
            const float norm = -masterFiltVal * 0.01f;
            const float cutoff = 20000.0f * std::pow(200.0f / 20000.0f, norm);
            mfAlpha = std::exp(-2.0f * kPi * cutoff / (float)sampleRate);
        }
        else
        {
            isLP = false;
            const float norm = masterFiltVal * 0.01f;
            const float cutoff = 20.0f * std::pow(3800.0f / 20.0f, norm);
            mfAlpha = std::exp(-2.0f * kPi * cutoff / (float)sampleRate);
        }
    }

    const int smileEchoDelay = juce::jlimit(1, (int)smileEcho[0].size() - 1, (int)(sampleRate * (0.16 + 0.12 * smile)));

    for (int i = 0; i < n; ++i)
    {
        if (speed >= 0.92f)
            smoothRatio = targetRatio;
        else
            smoothRatio += (targetRatio - smoothRatio) * smoothFactor;

        // Pitch shifter phase
        const float phaseRate = (1.0f - smoothRatio) / depth;
        phase += phaseRate;
        if (phase >= 1.0f) phase -= std::floor(phase);
        else if (phase < 0.0f) phase += 1.0f - std::floor(phase);

        const float p1 = phase;
        const float p2 = std::fmod(phase + 0.5f, 1.0f);
        const float d1 = baseDelay + p1 * depth;
        const float d2 = baseDelay + p2 * depth;
        const float w1 = 0.5f - 0.5f * std::cos(2.0f * kPi * p1);
        const float w2 = 1.0f - w1;

        // Harmonizer phase
        harmoPhase += harmoPhaseRate;
        if (harmoPhase >= 1.0f) harmoPhase -= std::floor(harmoPhase);
        else if (harmoPhase < 0.0f) harmoPhase += 1.0f - std::floor(harmoPhase);
        const float hp1 = harmoPhase;
        const float hp2 = std::fmod(harmoPhase + 0.5f, 1.0f);
        const float hd1 = baseDelay + hp1 * depth;
        const float hd2 = baseDelay + hp2 * depth;
        const float hw1 = 0.5f - 0.5f * std::cos(2.0f * kPi * hp1);
        const float hw2 = 1.0f - hw1;

        // Beat Repeat loop
        repeatReadPos = (repeatReadPos + 1) % currentSliceLength;
        const float repeatFade = (repeatReadPos < 64) ? (repeatReadPos / 64.0f) :
                                 (repeatReadPos > currentSliceLength - 64) ? ((currentSliceLength - repeatReadPos) / 64.0f) : 1.0f;

        for (int ch = 0; ch < channels; ++ch)
        {
            const float in = buffer.getSample(ch, i);
            delay[(size_t)ch][(size_t)writePos] = in;
            repeatBuf[(size_t)ch][(size_t)repeatWritePos] = in;

            auto readMain = [&](float d) -> float
            {
                float pos = (float)writePos - d;
                while (pos < 0.0f) pos += (float)delaySize;
                while (pos >= (float)delaySize) pos -= (float)delaySize;
                const int a = (int)pos;
                const int b = (a + 1) % delaySize;
                const float frac = pos - (float)a;
                return delay[(size_t)ch][(size_t)a] * (1.0f - frac) + delay[(size_t)ch][(size_t)b] * frac;
            };

            // Main vocal pitch shift
            float shifted = readMain(d1) * w1 + readMain(d2) * w2;
            if (std::abs(smoothRatio - 1.0f) < 0.001f && tune < 0.01f && std::abs(bend) < 0.01f)
                shifted = in;

            // Harmonizer voice (+7 semitones)
            const float harmoVoice = (readMain(hd1) * hw1 + readMain(hd2) * hw2);

            // Ping-Pong delay
            float ppOut = 0.0f;
            if (ch == 0) // Left
            {
                const int readIdxL = (pingPongPosL + (int)pingPongL.size() - ppDelayL) % (int)pingPongL.size();
                const float delayedL = pingPongL[(size_t)readIdxL];
                pingPongFilterL = 0.7f * pingPongFilterL + 0.3f * delayedL;
                const float rFeedback = pingPongR[(size_t)((pingPongPosR + (int)pingPongR.size() - ppDelayR) % (int)pingPongR.size())];
                pingPongL[(size_t)pingPongPosL] = harmoVoice * 0.7f + rFeedback * 0.38f;
                ppOut = pingPongFilterL;
            }
            else // Right
            {
                const int readIdxR = (pingPongPosR + (int)pingPongR.size() - ppDelayR) % (int)pingPongR.size();
                const float delayedR = pingPongR[(size_t)readIdxR];
                pingPongFilterR = 0.7f * pingPongFilterR + 0.3f * delayedR;
                const float lFeedback = pingPongL[(size_t)((pingPongPosL + (int)pingPongL.size() - ppDelayL) % (int)pingPongL.size())];
                pingPongR[(size_t)pingPongPosR] = lFeedback * 0.42f;
                ppOut = pingPongFilterR;
            }

            float out = shifted + ppOut * harmo * 0.85f;

            // Beat Repeat slice
            if (rptMix > 0.005f)
            {
                const int rIdx = (repeatWritePos - currentSliceLength + repeatReadPos + (int)repeatBuf[0].size()) % (int)repeatBuf[0].size();
                float repSample = repeatBuf[(size_t)ch][(size_t)rIdx] * repeatFade;
                repeatFilterState[ch] = 0.65f * repeatFilterState[ch] + 0.35f * repSample;
                repSample = repSample - repeatFilterState[ch] * 0.5f;
                out = out * (1.0f - rptMix) + repSample * rptMix;
            }

            // SMILE Bloom: Highpass + echo + saturation
            const float hpCut = 25.0f + smile * 1600.0f;
            const float hpA = std::exp(-2.0f * kPi * hpCut / (float)sampleRate);
            hpState[(size_t)ch] = hpA * hpState[(size_t)ch] + (1.0f - hpA) * out;
            out = out - hpState[(size_t)ch] * (0.15f + smile * 0.75f);

            float delayed = smileEcho[(size_t)ch][(size_t)((smileEchoPos + (int)smileEcho[0].size() - smileEchoDelay) % (int)smileEcho[0].size())];
            smileEcho[(size_t)ch][(size_t)smileEchoPos] = out + delayed * (smile * 0.55f);
            out += delayed * smile * 0.48f;
            out = std::tanh(out * (1.0f + smile * 1.5f)) / (1.0f + smile * 0.15f);

            // Master Filter
            if (masterFilterActive)
            {
                if (isLP)
                {
                    masterFiltState1[(size_t)ch] = mfAlpha * masterFiltState1[(size_t)ch] + (1.0f - mfAlpha) * out;
                    masterFiltState2[(size_t)ch] = mfAlpha * masterFiltState2[(size_t)ch] + (1.0f - mfAlpha) * masterFiltState1[(size_t)ch];
                    out = masterFiltState2[(size_t)ch];
                }
                else
                {
                    masterFiltState1[(size_t)ch] = mfAlpha * masterFiltState1[(size_t)ch] + (1.0f - mfAlpha) * out;
                    out = out - masterFiltState1[(size_t)ch];
                }
            }

            const float wet = juce::jlimit(0.0f, 1.0f, 0.28f + tune * 0.55f + std::abs(bend) / 24.0f + harmo * 0.2f);
            buffer.setSample(ch, i, in * (1.0f - wet) + out * wet);
        }

        writePos = (writePos + 1) % delaySize;
        repeatWritePos = (repeatWritePos + 1) % (int)repeatBuf[0].size();
        pingPongPosL = (pingPongPosL + 1) % (int)pingPongL.size();
        pingPongPosR = (pingPongPosR + 1) % (int)pingPongR.size();
        smileEchoPos = (smileEchoPos + 1) % (int)smileEcho[0].size();
    }
}

void WIR3DProcessor::loadPreset(int index)
{
    if (index < 0 || index >= (int)presets.size()) return;
    currentPreset = index;
    const auto& p = presets[(size_t)index];

    auto setParam = [this](const char* id, float val) {
        if (auto* param = params.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(val));
    };

    setParam("bend", p.bend);
    setParam("tune", p.tune);
    setParam("speed", p.speed);
    setParam("harmo", p.harmo);
    setParam("smile", p.smile);
    setParam("repeat", (float)p.repeat);
    setParam("rptMix", p.rptMix);
    setParam("filter", p.filter);
    setParam("root", (float)p.root);
    setParam("scale", (float)p.scale);
    setParam("range", (float)p.range);
}

void WIR3DProcessor::saveUserPreset(int slotIndex, const juce::String& name)
{
    if (slotIndex < 0 || slotIndex >= (int)presets.size()) return;
    auto get = [this](const char* id) { return params.getRawParameterValue(id)->load(); };

    presets[(size_t)slotIndex] = {
        name,
        get("bend"),
        get("tune"),
        get("speed"),
        get("harmo"),
        get("smile"),
        (int)get("repeat"),
        get("rptMix"),
        get("filter"),
        (int)get("root"),
        (int)get("scale"),
        (int)get("range")
    };
    currentPreset = slotIndex;
}

juce::StringArray WIR3DProcessor::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& p : presets) names.add(p.name);
    return names;
}

juce::AudioProcessorEditor* WIR3DProcessor::createEditor() { return new WIR3DEditor(*this); }
bool WIR3DProcessor::hasEditor() const { return true; }
const juce::String WIR3DProcessor::getName() const { return JucePlugin_Name; }
bool WIR3DProcessor::acceptsMidi() const { return false; }
bool WIR3DProcessor::producesMidi() const { return false; }
bool WIR3DProcessor::isMidiEffect() const { return false; }
double WIR3DProcessor::getTailLengthSeconds() const { return 2.0; }
int WIR3DProcessor::getNumPrograms() { return (int)presets.size(); }
int WIR3DProcessor::getCurrentProgram() { return currentPreset; }
void WIR3DProcessor::setCurrentProgram(int index) { loadPreset(index); }
const juce::String WIR3DProcessor::getProgramName(int index) { return (index >= 0 && index < (int)presets.size()) ? presets[(size_t)index].name : juce::String(); }
void WIR3DProcessor::changeProgramName(int index, const juce::String& newName) { if (index >= 0 && index < (int)presets.size()) presets[(size_t)index].name = newName; }

void WIR3DProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto s = params.copyState();
    s.setProperty("currentPreset", currentPreset, nullptr);
    std::unique_ptr<juce::XmlElement> xml(s.createXml());
    copyXmlToBinary(*xml, dest);
}

void WIR3DProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(params.state.getType()))
    {
        params.replaceState(juce::ValueTree::fromXml(*xml));
        if (xml->hasAttribute("currentPreset"))
            currentPreset = xml->getIntAttribute("currentPreset");
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WIR3DProcessor(); }

#pragma once
#include <cmath>
#include <array>
#include <vector>
#include <algorithm>

class ScaleQuantizer
{
public:
    enum ScaleType
    {
        Chromatic = 0,
        Major,
        Minor,
        MajorPentatonic,
        MinorPentatonic,
        Blues,
        Dorian,
        Phrygian,
        Mixolydian,
        NumScales
    };

    ScaleQuantizer() = default;

    static float hzToMidi(float freqHz, float refA4 = 440.0f)
    {
        if (freqHz <= 0.0f) return -1.0f;
        return 69.0f + 12.0f * std::log2(freqHz / refA4);
    }

    static float midiToHz(float midiNote, float refA4 = 440.0f)
    {
        return refA4 * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
    }

    static int quantizeMidiNote(int midiNote, int rootKey, int scaleType);
    static int findNearestScaleMidi(float midiNote, int rootKey, int scaleType);
    static const char* getScaleName(int scaleType);
};

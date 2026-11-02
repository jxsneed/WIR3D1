#include "ScaleQuantizer.h"

static const std::array<std::array<bool, 12>, ScaleQuantizer::NumScales> scaleMasks = {{
    // Chromatic
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    // Major
    {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1},
    // Minor (Natural)
    {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0},
    // Major Pentatonic
    {1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0},
    // Minor Pentatonic
    {1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0},
    // Blues
    {1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0},
    // Dorian
    {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0},
    // Phrygian
    {1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0},
    // Mixolydian
    {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0}
}};

static const char* scaleNames[ScaleQuantizer::NumScales] = {
    "Chromatic", "Major", "Minor", "Major Pentatonic", "Minor Pentatonic",
    "Blues", "Dorian", "Phrygian", "Mixolydian"
};

const char* ScaleQuantizer::getScaleName(int scaleType)
{
    if (scaleType >= 0 && scaleType < NumScales)
        return scaleNames[scaleType];
    return "Chromatic";
}

int ScaleQuantizer::quantizeMidiNote(int midiNote, int rootKey, int scaleType)
{
    if (scaleType == Chromatic || scaleType < 0 || scaleType >= NumScales)
        return midiNote;

    const auto& mask = scaleMasks[(size_t)scaleType];
    const int noteInOctave = ((midiNote % 12) - rootKey + 12) % 12;
    int bestNote = noteInOctave;
    int bestDist = 12;

    for (int n = 0; n < 12; ++n)
    {
        if (mask[(size_t)n])
        {
            const int dist = std::abs(n - noteInOctave);
            if (dist < bestDist)
            {
                bestDist = dist;
                bestNote = n;
            }
        }
    }
    return midiNote + (bestNote - noteInOctave);
}

int ScaleQuantizer::findNearestScaleMidi(float midiNote, int rootKey, int scaleType)
{
    if (scaleType == Chromatic || scaleType < 0 || scaleType >= NumScales)
        return static_cast<int>(std::round(midiNote));

    const auto& mask = scaleMasks[(size_t)scaleType];
    const int center = static_cast<int>(std::round(midiNote));
    int bestMidi = center;
    float bestDistance = 999.0f;

    for (int candidate = center - 12; candidate <= center + 12; ++candidate)
    {
        const int noteClass = ((candidate - rootKey) % 12 + 12) % 12;
        if (mask[(size_t)noteClass])
        {
            const float distance = std::abs(static_cast<float>(candidate) - midiNote);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestMidi = candidate;
            }
        }
    }
    return bestMidi;
}

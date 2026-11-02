#include "PitchDetector.h"

PitchDetector::PitchDetector() {}

void PitchDetector::prepare(double sr, int maxBlock)
{
    sampleRate = sr;
    maxBlockSize = maxBlock;
    setFrequencyRange(55.0f, 1100.0f); // Default vocal range
}

void PitchDetector::setFrequencyRange(float minHz, float maxHz)
{
    maxHz = std::max(maxHz, minHz + 20.0f);
    minPeriod = static_cast<int>(sampleRate / maxHz);
    if (minPeriod < 4) minPeriod = 4;

    maxPeriod = static_cast<int>(sampleRate / minHz);
    if (maxPeriod > static_cast<int>(sampleRate / 2))
        maxPeriod = static_cast<int>(sampleRate / 2);

    const int neededSize = 2 * maxPeriod + 4;
    workBuffer.assign(static_cast<size_t>(neededSize), 0.0f);
    diffBuffer.assign(static_cast<size_t>(maxPeriod + 2), 0.0f);
    normDiffBuffer.assign(static_cast<size_t>(maxPeriod + 2), 1.0f);

    historyFill = 0;
    samplesSinceAnalysis = 0;
    lastPitch = 0.0f;
    confidence = 0.0f;
}

void PitchDetector::reset()
{
    std::fill(workBuffer.begin(), workBuffer.end(), 0.0f);
    std::fill(diffBuffer.begin(), diffBuffer.end(), 0.0f);
    std::fill(normDiffBuffer.begin(), normDiffBuffer.end(), 1.0f);
    historyFill = 0;
    samplesSinceAnalysis = 0;
    lastPitch = 0.0f;
    confidence = 0.0f;
}

float PitchDetector::detectPitch(const float* buffer, int numSamples)
{
    if (workBuffer.empty() || numSamples <= 0)
        return 0.0f;

    const int historySize = static_cast<int>(workBuffer.size());
    const int samplesToCopy = std::min(numSamples, historySize);

    // Shift history and copy new samples
    if (samplesToCopy < historySize)
    {
        std::move(workBuffer.begin() + samplesToCopy, workBuffer.end(), workBuffer.begin());
        std::copy(buffer + numSamples - samplesToCopy, buffer + numSamples, workBuffer.end() - samplesToCopy);
    }
    else
    {
        std::copy(buffer + numSamples - historySize, buffer + numSamples, workBuffer.begin());
    }

    historyFill = std::min(historySize, historyFill + samplesToCopy);
    if (historyFill < minPeriod * 2)
        return 0.0f;

    // Run analysis every ~128 samples (~2.9 ms at 44.1k)
    samplesSinceAnalysis += numSamples;
    if (samplesSinceAnalysis < 128)
        return lastPitch;

    samplesSinceAnalysis %= 128;
    const float* analysis = workBuffer.data() + historySize - historyFill;
    const float tau = yinPeriod(analysis, historyFill);

    if (tau < static_cast<float>(minPeriod))
    {
        confidence = 0.0f;
        lastPitch = 0.0f;
        return 0.0f;
    }

    lastPitch = static_cast<float>(sampleRate / tau);
    return lastPitch;
}

float PitchDetector::yinPeriod(const float* buffer, int numSamples)
{
    const int maxTau = std::min(maxPeriod, numSamples / 2);
    if (maxTau <= minPeriod * 2) return -1.0f;

    const int N = numSamples - maxTau;
    if (N < maxTau / 2) return -1.0f;

    // Check RMS energy to reject quiet noise or breaths
    double energy = 0.0;
    for (int j = 0; j < N; j += 4)
    {
        const float s = buffer[j];
        energy += static_cast<double>(s) * s;
    }
    if (energy / (N / 4.0) < 0.00008)
    {
        confidence = 0.0f;
        return -1.0f;
    }

    // Step 1: Difference Function
    std::fill(diffBuffer.begin(), diffBuffer.begin() + maxTau + 1, 0.0f);
    for (int tau = 1; tau <= maxTau; ++tau)
    {
        double sum = 0.0;
        for (int j = 0; j < N; j += 2)
        {
            const float d = buffer[j] - buffer[j + tau];
            sum += static_cast<double>(d) * d;
        }
        diffBuffer[static_cast<size_t>(tau)] = static_cast<float>(sum);
    }

    // Step 2: Cumulative Mean Normalized Difference Function (CMNDF)
    std::fill(normDiffBuffer.begin(), normDiffBuffer.begin() + maxTau + 1, 1.0f);
    double runningSum = 0.0;
    for (int tau = 1; tau <= maxTau; ++tau)
    {
        runningSum += static_cast<double>(diffBuffer[static_cast<size_t>(tau)]);
        if (runningSum > 0.0)
            normDiffBuffer[static_cast<size_t>(tau)] = diffBuffer[static_cast<size_t>(tau)] * static_cast<float>(tau) / static_cast<float>(runningSum);
    }

    // Step 3: Absolute Threshold Search
    int bestTau = -1;
    float bestVal = 1.0f;

    for (int tau = minPeriod; tau <= maxTau; ++tau)
    {
        if (normDiffBuffer[static_cast<size_t>(tau)] < bestVal)
        {
            bestVal = normDiffBuffer[static_cast<size_t>(tau)];
            bestTau = tau;
        }

        if (normDiffBuffer[static_cast<size_t>(tau)] < threshold)
        {
            if (tau + 1 <= maxTau && normDiffBuffer[static_cast<size_t>(tau + 1)] > normDiffBuffer[static_cast<size_t>(tau)]
                && tau - 1 >= minPeriod && normDiffBuffer[static_cast<size_t>(tau - 1)] > normDiffBuffer[static_cast<size_t>(tau)])
            {
                bestTau = tau;
                bestVal = normDiffBuffer[static_cast<size_t>(tau)];
                break; // First true fundamental valley
            }
        }
    }

    if (bestTau < minPeriod)
    {
        confidence = 0.0f;
        return -1.0f;
    }

    // Step 4: Subharmonic / Octave-Down Prevention (Divisors 2, 3, 4)
    for (int divisor = 2; divisor <= 4; ++divisor)
    {
        const int candidate = bestTau / divisor;
        if (candidate >= minPeriod && candidate <= maxTau)
        {
            const float candidateVal = normDiffBuffer[static_cast<size_t>(candidate)];
            if (candidateVal < 0.32f && candidateVal <= bestVal * 1.85f)
            {
                bestTau = candidate;
                bestVal = candidateVal;
                break;
            }
        }
    }

    confidence = 1.0f - bestVal;

    // Step 5: Parabolic Interpolation
    float offset = 0.0f;
    if (bestTau > 0 && bestTau < maxTau)
        offset = parabolicInterpolation(normDiffBuffer.data(), bestTau, maxTau + 1);

    return static_cast<float>(bestTau) + offset;
}

float PitchDetector::parabolicInterpolation(const float* diff, int tau, int len) const
{
    if (tau <= 0 || tau >= len - 1)
        return 0.0f;

    const float a = diff[tau - 1];
    const float b = diff[tau];
    const float c = diff[tau + 1];

    const float denom = 2.0f * (a + c - 2.0f * b);
    if (std::abs(denom) < 1.0e-12f)
        return 0.0f;

    return (a - c) / denom;
}

#include "Deconvolver.h"

#include <juce_dsp/juce_dsp.h>

#include <cmath>
#include <vector>

namespace
{
    constexpr float kRegularization = 1.0e-6f;

    int nextFftOrder (int minimumSize)
    {
        return juce::roundToInt (std::ceil (std::log2 (static_cast<double> (juce::jmax (2, minimumSize)))));
    }
}

juce::AudioBuffer<float> Deconvolver::mixToMono (const juce::AudioBuffer<float>& buffer)
{
    juce::AudioBuffer<float> mono (1, buffer.getNumSamples());
    mono.clear();

    if (buffer.getNumSamples() == 0)
        return mono;

    if (buffer.getNumChannels() == 1)
    {
        mono.copyFrom (0, 0, buffer, 0, 0, buffer.getNumSamples());
        return mono;
    }

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        mono.addFrom (0, 0, buffer, channel, 0, buffer.getNumSamples(), 1.0f / static_cast<float> (buffer.getNumChannels()));

    return mono;
}

juce::AudioBuffer<float> Deconvolver::deconvolve (const juce::AudioBuffer<float>& recorded,
                                                  const juce::AudioBuffer<float>& sweep)
{
    const auto recordedMono = mixToMono (recorded);
    const auto sweepMono = mixToMono (sweep);

    const auto numSamples = juce::jmax (recordedMono.getNumSamples(), sweepMono.getNumSamples());

    if (numSamples <= 0 || sweepMono.getNumSamples() <= 0)
        return {};

    const auto fftSize = juce::nextPowerOfTwo (numSamples * 2);
    const auto fftOrder = nextFftOrder (fftSize);
    const auto actualFftSize = 1 << fftOrder;

    juce::dsp::FFT fft (fftOrder);

    std::vector<float> recordedFftData (static_cast<size_t> (actualFftSize) * 2, 0.0f);
    std::vector<float> sweepFftData (static_cast<size_t> (actualFftSize) * 2, 0.0f);
    std::vector<float> irFftData (static_cast<size_t> (actualFftSize) * 2, 0.0f);

    std::memcpy (recordedFftData.data(),
                 recordedMono.getReadPointer (0),
                 static_cast<size_t> (recordedMono.getNumSamples()) * sizeof (float));

    std::memcpy (sweepFftData.data(),
                 sweepMono.getReadPointer (0),
                 static_cast<size_t> (sweepMono.getNumSamples()) * sizeof (float));

    fft.performRealOnlyForwardTransform (recordedFftData.data(), false);
    fft.performRealOnlyForwardTransform (sweepFftData.data(), false);

    irFftData[0] = recordedFftData[0] / (sweepFftData[0] + kRegularization);
    irFftData[1] = 0.0f;

    for (int bin = 1; bin < actualFftSize / 2; ++bin)
    {
        const auto recordedIndex = bin * 2;
        const auto reRecorded = recordedFftData[static_cast<size_t> (recordedIndex)];
        const auto imRecorded = recordedFftData[static_cast<size_t> (recordedIndex + 1)];
        const auto reSweep = sweepFftData[static_cast<size_t> (recordedIndex)];
        const auto imSweep = sweepFftData[static_cast<size_t> (recordedIndex + 1)];

        const auto denom = reSweep * reSweep + imSweep * imSweep + kRegularization;
        irFftData[static_cast<size_t> (recordedIndex)] = (reRecorded * reSweep + imRecorded * imSweep) / denom;
        irFftData[static_cast<size_t> (recordedIndex + 1)] = (imRecorded * reSweep - reRecorded * imSweep) / denom;
    }

    fft.performRealOnlyInverseTransform (irFftData.data());

    const auto irLength = juce::jmin (numSamples, actualFftSize);
    juce::AudioBuffer<float> impulseResponse (1, irLength);

    std::memcpy (impulseResponse.getWritePointer (0),
                 irFftData.data(),
                 static_cast<size_t> (irLength) * sizeof (float));

    return impulseResponse;
}

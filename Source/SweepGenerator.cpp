#include "SweepGenerator.h"

#include <cmath>

SweepGenerator::SweepGenerator (double startFrequencyHz,
                                double endFrequencyHz,
                                double durationSeconds,
                                double sampleRateHz)
{
    setParameters (startFrequencyHz, endFrequencyHz, durationSeconds, sampleRateHz);
}

void SweepGenerator::setParameters (double startFrequencyHz,
                                    double endFrequencyHz,
                                    double durationSeconds,
                                    double sampleRateHz)
{
    startFrequencyHz_ = startFrequencyHz;
    endFrequencyHz_ = endFrequencyHz;
    durationSeconds_ = durationSeconds;
    sampleRateHz_ = sampleRateHz;
}

juce::AudioBuffer<float> SweepGenerator::generate() const
{
    const auto numSamples = juce::jmax (1, static_cast<int> (std::round (durationSeconds_ * sampleRateHz_)));
    juce::AudioBuffer<float> buffer (1, numSamples);
    buffer.clear();

    const auto startFreq = juce::jmax (1.0e-6, startFrequencyHz_);
    const auto endFreq = juce::jmax (startFreq, endFrequencyHz_);
    const auto duration = juce::jmax (1.0e-6, durationSeconds_);
    const auto sampleRate = juce::jmax (1.0, sampleRateHz_);

    auto* channelData = buffer.getWritePointer (0);

    if (std::abs (endFreq - startFreq) < 1.0e-9)
    {
        const auto angularFrequency = juce::MathConstants<double>::twoPi * startFreq;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto time = static_cast<double> (sample) / sampleRate;
            channelData[sample] = static_cast<float> (std::sin (angularFrequency * time));
        }
    }
    else
    {
        const auto logRatio = std::log (endFreq / startFreq);
        const auto sweepConstant = juce::MathConstants<double>::twoPi * startFreq * duration / logRatio;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto time = static_cast<double> (sample) / sampleRate;
            const auto phase = sweepConstant * (std::exp (time * logRatio / duration) - 1.0);
            channelData[sample] = static_cast<float> (std::sin (phase));
        }
    }

    applyFades (buffer);
    return buffer;
}

void SweepGenerator::applyFades (juce::AudioBuffer<float>& buffer) const
{
    const auto numSamples = buffer.getNumSamples();

    if (numSamples <= 0)
        return;

    const auto fadeSamples = juce::jmin (static_cast<int> (std::round (fadeTimeSeconds_ * sampleRateHz_)),
                                       numSamples / 2);

    if (fadeSamples <= 0)
        return;

    auto* channelData = buffer.getWritePointer (0);

    for (int sample = 0; sample < fadeSamples; ++sample)
    {
        const auto fadeInGain = static_cast<float> (sample) / static_cast<float> (fadeSamples);
        const auto fadeOutGain = static_cast<float> (fadeSamples - 1 - sample) / static_cast<float> (fadeSamples);

        channelData[sample] *= fadeInGain;
        channelData[numSamples - 1 - sample] *= fadeOutGain;
    }
}

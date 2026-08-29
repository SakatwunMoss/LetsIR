#include "IRPostProcessor.h"

#include <cmath>
#include <cstring>
#include <iostream>

namespace LetsIRPostProcess
{
namespace
{
    float dbToLinear (float db) noexcept
    {
        return std::pow (10.0f, db / 20.0f);
    }

    float linearToDb (float linear) noexcept
    {
        if (linear <= 0.0f)
            return -100.0f;

        return 20.0f * std::log10 (linear);
    }

    int findFirstSampleAboveThreshold (const juce::AudioBuffer<float>& buffer, float threshold) noexcept
    {
        const auto numSamples = buffer.getNumSamples();

        if (numSamples <= 0)
            return 0;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                if (std::abs (buffer.getSample (channel, sample)) >= threshold)
                    return sample;
            }
        }

        return numSamples;
    }

    int findLastSampleAboveThreshold (const juce::AudioBuffer<float>& buffer, float threshold) noexcept
    {
        const auto numSamples = buffer.getNumSamples();

        if (numSamples <= 0)
            return 0;

        for (int sample = numSamples - 1; sample >= 0; --sample)
        {
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                if (std::abs (buffer.getSample (channel, sample)) >= threshold)
                    return sample;
            }
        }

        return 0;
    }

    void logPostProcessStage (const juce::String& stage,
                              int numSamples,
                              float peakLinear,
                              float peakDb) noexcept
    {
        const auto message = "IR post-process [" + stage + "]: samples="
                           + juce::String (numSamples)
                           + ", peak=" + juce::String (peakLinear, 6)
                           + " (" + juce::String (peakDb, 2) + " dBFS)";

        DBG (message);
        std::cout << message << std::endl;
    }
}

float computeBufferPeak (const juce::AudioBuffer<float>& buffer)
{
    return buffer.getMagnitude (0, buffer.getNumSamples());
}

void trimLeadingSilence (juce::AudioBuffer<float>& buffer)
{
    const auto peak = computeBufferPeak (buffer);

    if (peak <= 0.0f)
        return;

    const auto threshold = peak * dbToLinear (kTrimThresholdDbBelowPeak);
    const auto firstAboveThreshold = findFirstSampleAboveThreshold (buffer, threshold);

    if (firstAboveThreshold >= buffer.getNumSamples())
        return;

    const auto trimStart = juce::jmax (0, firstAboveThreshold - kTrimMarginSamples);

    if (trimStart <= 0)
        return;

    const auto newLength = buffer.getNumSamples() - trimStart;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        std::memmove (channelData, channelData + trimStart, static_cast<size_t> (newLength) * sizeof (float));
    }

    buffer.setSize (buffer.getNumChannels(), newLength, true, false, true);
}

void applyNoiseFloorHannFadeOut (juce::AudioBuffer<float>& buffer,
                                 double sampleRate,
                                 const FadeOutParams& params)
{
    const auto numSamples = buffer.getNumSamples();

    if (numSamples <= 0 || sampleRate <= 0.0)
        return;

    const auto peak = computeBufferPeak (buffer);

    if (peak <= 0.0f)
        return;

    const auto noiseFloorThreshold = peak * dbToLinear (kNoiseFloorDbBelowPeak);
    const auto lastSignificantSample = findLastSampleAboveThreshold (buffer, noiseFloorThreshold);

    const auto paramStartSample = juce::jlimit (0, numSamples - 1,
                                                static_cast<int> (std::round (params.startTimeSeconds * sampleRate)));

    const auto fadeStartSample = juce::jmax (lastSignificantSample, paramStartSample);
    const auto requestedFadeLength = juce::jmax (1, static_cast<int> (std::round (params.durationSeconds * sampleRate)));
    const auto fadeLength = juce::jmin (requestedFadeLength, numSamples - fadeStartSample);

    if (fadeLength <= 0)
        return;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        for (int n = 0; n < fadeLength; ++n)
        {
            const auto sampleIndex = fadeStartSample + n;
            const auto hannPhase = juce::MathConstants<float>::pi * static_cast<float> (n)
                                 / static_cast<float> (fadeLength - 1);
            const auto fadeGain = 0.5f * (1.0f + std::cos (hannPhase));
            channelData[sampleIndex] *= fadeGain;
        }

        for (int sample = fadeStartSample + fadeLength; sample < numSamples; ++sample)
            channelData[sample] = 0.0f;
    }
}

float normalizePeakLevel (juce::AudioBuffer<float>& buffer)
{
    const auto peakBefore = computeBufferPeak (buffer);

    if (peakBefore <= 0.0f)
        return 0.0f;

    const auto targetPeak = dbToLinear (kTargetPeakLevelDbFS);
    const auto gain = targetPeak / peakBefore;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.applyGain (channel, 0, buffer.getNumSamples(), gain);

    return peakBefore;
}

void postProcessIR (juce::AudioBuffer<float>& irBuffer,
                    double sampleRate,
                    const FadeOutParams& fadeOutParams)
{
    const auto initialSamples = irBuffer.getNumSamples();
    const auto initialPeak = computeBufferPeak (irBuffer);

    logPostProcessStage ("before",
                         initialSamples,
                         initialPeak,
                         linearToDb (initialPeak));

    trimLeadingSilence (irBuffer);

    logPostProcessStage ("after trim",
                         irBuffer.getNumSamples(),
                         computeBufferPeak (irBuffer),
                         linearToDb (computeBufferPeak (irBuffer)));

    applyNoiseFloorHannFadeOut (irBuffer, sampleRate, fadeOutParams);

    logPostProcessStage ("after fade",
                         irBuffer.getNumSamples(),
                         computeBufferPeak (irBuffer),
                         linearToDb (computeBufferPeak (irBuffer)));

    const auto peakBeforeNormalize = normalizePeakLevel (irBuffer);
    const auto peakAfterNormalize = computeBufferPeak (irBuffer);

    const auto normalizeMessage = juce::String ("IR post-process [normalize]: peak before=")
                                + juce::String (peakBeforeNormalize, 6)
                                + " (" + juce::String (linearToDb (peakBeforeNormalize), 2) + " dBFS)"
                                + ", peak after=" + juce::String (peakAfterNormalize, 6)
                                + " (" + juce::String (linearToDb (peakAfterNormalize), 2) + " dBFS)"
                                + ", target=" + juce::String (kTargetPeakLevelDbFS, 2) + " dBFS";

    DBG (normalizeMessage);
    std::cout << normalizeMessage << std::endl;

    logPostProcessStage ("done",
                         irBuffer.getNumSamples(),
                         peakAfterNormalize,
                         linearToDb (peakAfterNormalize));

    juce::ignoreUnused (initialSamples);
}

} // namespace LetsIRPostProcess

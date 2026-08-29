#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace LetsIRPostProcess
{
    /** Amplitude threshold for leading silence trim, relative to buffer peak (dB). */
    inline constexpr float kTrimThresholdDbBelowPeak = -60.0f;

    /** Samples kept before the first sample above the trim threshold. */
    inline constexpr int kTrimMarginSamples = 64;

    /** Target peak level after normalization (dBFS). */
    inline constexpr float kTargetPeakLevelDbFS = -1.0f;

    /** Noise-floor detection threshold, relative to buffer peak (dB). */
    inline constexpr float kNoiseFloorDbBelowPeak = -60.0f;

    /** Placeholder fade-out start time from the beginning of the IR (seconds). */
    inline constexpr double kDefaultFadeOutStartSeconds = 4.0;

    /** Placeholder Hann fade-out duration (seconds). */
    inline constexpr double kDefaultFadeOutDurationSeconds = 0.2;

    struct FadeOutParams
    {
        double startTimeSeconds = kDefaultFadeOutStartSeconds;
        double durationSeconds = kDefaultFadeOutDurationSeconds;
    };

    float computeBufferPeak (const juce::AudioBuffer<float>& buffer);

    void trimLeadingSilence (juce::AudioBuffer<float>& buffer);

    void applyNoiseFloorHannFadeOut (juce::AudioBuffer<float>& buffer,
                                     double sampleRate,
                                     const FadeOutParams& params = {});

    float normalizePeakLevel (juce::AudioBuffer<float>& buffer);

    void postProcessIR (juce::AudioBuffer<float>& irBuffer,
                        double sampleRate,
                        const FadeOutParams& fadeOutParams = {});
}

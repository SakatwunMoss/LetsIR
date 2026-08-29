#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class SweepGenerator
{
public:
    SweepGenerator() = default;

    SweepGenerator (double startFrequencyHz,
                    double endFrequencyHz,
                    double durationSeconds,
                    double sampleRateHz);

    void setParameters (double startFrequencyHz,
                        double endFrequencyHz,
                        double durationSeconds,
                        double sampleRateHz);

    juce::AudioBuffer<float> generate() const;

private:
    double startFrequencyHz_ = 20.0;
    double endFrequencyHz_ = 20000.0;
    double durationSeconds_ = 5.0;
    double sampleRateHz_ = 44100.0;

    static constexpr double fadeTimeSeconds_ = 0.01;

    void applyFades (juce::AudioBuffer<float>& buffer) const;
};

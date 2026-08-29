#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class Deconvolver
{
public:
    static juce::AudioBuffer<float> deconvolve (const juce::AudioBuffer<float>& recorded,
                                                const juce::AudioBuffer<float>& sweep);

private:
    static juce::AudioBuffer<float> mixToMono (const juce::AudioBuffer<float>& buffer);
};

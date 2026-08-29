#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

bool exportIRToWavFile (const juce::AudioBuffer<float>& irBuffer,
                        const juce::File& destFile,
                        double sampleRate,
                        int bitsPerSample = 24);

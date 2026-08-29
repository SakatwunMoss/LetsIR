#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>

namespace letsir
{

/** Multi-channel WAV export (default 32-bit float). */
class IRExporter
{
public:
    static bool exportWav (const juce::AudioBuffer<float>& irBuffer,
                           const juce::File& destFile,
                           double sampleRate,
                           int bitsPerSample = 32);

    static juce::String makeDefaultFileName (const juce::String& formatTag);
};

} // namespace letsir

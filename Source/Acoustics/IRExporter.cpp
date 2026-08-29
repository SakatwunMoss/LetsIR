#include "IRExporter.h"

namespace letsir
{

bool IRExporter::exportWav (const juce::AudioBuffer<float>& irBuffer,
                            const juce::File& destFile,
                            double sampleRate,
                            int bitsPerSample)
{
    if (irBuffer.getNumSamples() <= 0 || irBuffer.getNumChannels() <= 0)
        return false;

    if (sampleRate <= 0.0)
        return false;

    if (bitsPerSample != 16 && bitsPerSample != 24 && bitsPerSample != 32)
        return false;

    auto stream = destFile.createOutputStream();
    if (stream == nullptr || ! stream->openedOk())
        return false;

    std::unique_ptr<juce::OutputStream> streamOwner (stream.release());

    using Opt = juce::AudioFormatWriterOptions;
    const auto sampleFormat = (bitsPerSample == 32) ? Opt::SampleFormat::floatingPoint
                                                    : Opt::SampleFormat::integral;

    const auto options = Opt{}
        .withSampleRate (sampleRate)
        .withNumChannels (irBuffer.getNumChannels())
        .withBitsPerSample (bitsPerSample)
        .withSampleFormat (sampleFormat);

    juce::WavAudioFormat wavFormat;
    auto writer = wavFormat.createWriterFor (streamOwner, options);

    if (writer == nullptr)
        return false;

    return writer->writeFromAudioSampleBuffer (irBuffer, 0, irBuffer.getNumSamples());
}

juce::String IRExporter::makeDefaultFileName (const juce::String& formatTag)
{
    return "LetsIR_" + formatTag + "_"
         + juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S")
         + ".wav";
}

} // namespace letsir

#include "IRWavExporter.h"

bool exportIRToWavFile (const juce::AudioBuffer<float>& irBuffer,
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

    juce::WavAudioFormat wavFormat;

    std::unique_ptr<juce::AudioFormatWriter> writer (wavFormat.createWriterFor (stream.release(),
                                                                                  sampleRate,
                                                                                  static_cast<unsigned int> (irBuffer.getNumChannels()),
                                                                                  bitsPerSample,
                                                                                  {},
                                                                                  0));

    if (writer == nullptr)
        return false;

    return writer->writeFromAudioSampleBuffer (irBuffer, 0, irBuffer.getNumSamples());
}

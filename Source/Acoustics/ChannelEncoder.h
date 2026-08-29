#pragma once

#include "Material.h"
#include "Vec3.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <memory>
#include <random>
#include <vector>

namespace letsir
{

enum class OutputFormat
{
    mono = 0,
    stereo,
    quad,
    ambixFirstOrder
};

inline const char* outputFormatDisplayName (OutputFormat format)
{
    switch (format)
    {
        case OutputFormat::mono:            return "Mono";
        case OutputFormat::stereo:          return "Stereo";
        case OutputFormat::quad:            return "Quad (4ch)";
        case OutputFormat::ambixFirstOrder: return "AmbiX 1st order (ACN/SN3D)";
        default:                            return "?";
    }
}

inline int outputFormatChannelCount (OutputFormat format)
{
    switch (format)
    {
        case OutputFormat::mono:            return 1;
        case OutputFormat::stereo:          return 2;
        case OutputFormat::quad:            return 4;
        case OutputFormat::ambixFirstOrder: return 4;
        default:                            return 1;
    }
}

/**
 * Maps an arrival direction to per-channel gains.
 * Add new formats (e.g. 2nd-order Ambisonics) by implementing this interface.
 */
class IChannelEncoder
{
public:
    virtual ~IChannelEncoder() = default;

    virtual int getNumChannels() const = 0;
    virtual OutputFormat getFormat() const = 0;
    virtual juce::String getFormatName() const = 0;

    /** Write getNumChannels() gains for the given unit arrival direction (from listener). */
    virtual void getChannelGains (const Vec3& directionFromListener, float* gainsOut) const = 0;
};

class MonoEncoder final : public IChannelEncoder
{
public:
    int getNumChannels() const override { return 1; }
    OutputFormat getFormat() const override { return OutputFormat::mono; }
    juce::String getFormatName() const override { return "Mono"; }
    void getChannelGains (const Vec3&, float* gainsOut) const override { gainsOut[0] = 1.0f; }
};

class StereoEncoder final : public IChannelEncoder
{
public:
    int getNumChannels() const override { return 2; }
    OutputFormat getFormat() const override { return OutputFormat::stereo; }
    juce::String getFormatName() const override { return "Stereo"; }
    void getChannelGains (const Vec3& directionFromListener, float* gainsOut) const override;
};

class QuadEncoder final : public IChannelEncoder
{
public:
    int getNumChannels() const override { return 4; }
    OutputFormat getFormat() const override { return OutputFormat::quad; }
    juce::String getFormatName() const override { return "Quad"; }
    void getChannelGains (const Vec3& directionFromListener, float* gainsOut) const override;
};

/** AmbiX 1st order: ACN order W,Y,Z,X — SN3D normalisation. */
class AmbixEncoder final : public IChannelEncoder
{
public:
    int getNumChannels() const override { return 4; }
    OutputFormat getFormat() const override { return OutputFormat::ambixFirstOrder; }
    juce::String getFormatName() const override { return "AmbiX 1st"; }
    void getChannelGains (const Vec3& directionFromListener, float* gainsOut) const override;
};

std::unique_ptr<IChannelEncoder> createEncoder (OutputFormat format);

/** Synthesise a multi-channel IR from per-channel / per-band energy histograms. */
void synthesiseIRFromHistograms (const std::vector<std::vector<std::array<float, kNumOctaveBands>>>& channelHistograms,
                                 double sampleRate,
                                 double binDurationSeconds,
                                 juce::AudioBuffer<float>& dest,
                                 uint32_t seed = 0xA11CEu);

} // namespace letsir

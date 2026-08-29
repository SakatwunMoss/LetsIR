#include "ChannelEncoder.h"

#include <cmath>

namespace letsir
{

namespace
{
constexpr float kPi = 3.14159265358979323846f;

float equalPowerPanLeft (float azimuth) noexcept
{
    // azimuth 0 = front, +pi/2 = right. Map to stereo pan angle.
    // Use sin/cos equal-power based on horizontal angle projected to L/R.
    const float x = std::sin (azimuth); // -1..1 left..right-ish (our az: + = right)
    const float pan = 0.5f * (x + 1.0f); // 0 = left, 1 = right
    const float angle = pan * (0.5f * kPi);
    return std::cos (angle);
}

float equalPowerPanRight (float azimuth) noexcept
{
    const float x = std::sin (azimuth);
    const float pan = 0.5f * (x + 1.0f);
    const float angle = pan * (0.5f * kPi);
    return std::sin (angle);
}
} // namespace

void StereoEncoder::getChannelGains (const Vec3& directionFromListener, float* gainsOut) const
{
    float az = 0.0f, el = 0.0f;
    directionFromListener.toSpherical (az, el);
    gainsOut[0] = equalPowerPanLeft (az);
    gainsOut[1] = equalPowerPanRight (az);
}

void QuadEncoder::getChannelGains (const Vec3& directionFromListener, float* gainsOut) const
{
    // Channel order: FL, FR, BL, BR — square layout around the listener.
    float az = 0.0f, el = 0.0f;
    directionFromListener.toSpherical (az, el);
    juce::ignoreUnused (el);

    const float orderAz[4] = { -0.25f * kPi, 0.25f * kPi, -0.75f * kPi, 0.75f * kPi };

    float sum = 0.0f;
    for (int i = 0; i < 4; ++i)
    {
        float d = az - orderAz[i];
        while (d > kPi)  d -= 2.0f * kPi;
        while (d < -kPi) d += 2.0f * kPi;
        const float w = std::max (0.0f, std::cos (d));
        gainsOut[i] = w * w;
        sum += gainsOut[i];
    }

    if (sum > 1.0e-8f)
    {
        const float inv = 1.0f / std::sqrt (sum);
        for (int i = 0; i < 4; ++i)
            gainsOut[i] *= inv;
    }
    else
    {
        gainsOut[0] = gainsOut[1] = gainsOut[2] = gainsOut[3] = 0.5f;
    }
}

void AmbixEncoder::getChannelGains (const Vec3& directionFromListener, float* gainsOut) const
{
    // ACN / SN3D first order. Direction is arrival direction from listener
    // (unit vector pointing toward the source of the arrival = opposite of propagation?).
    // Convention: encode the direction of incidence (where sound comes from).
    // Our directionFromListener points from listener toward the last ray segment origin,
    // i.e. where the sound is coming from.
    const Vec3 d = directionFromListener.normalised();

    // Room frame: X=right, Y=front, Z=up
    // AmbiX ACN SN3D: W, Y, Z, X  where Y~sin(az)cos(el) in Ambisonics usually
    // Standard Ambisonics: X=cos(el)cos(az), Y=cos(el)sin(az), Z=sin(el)
    // with az=0 front, az=+90 left in traditional — but SN3D AmbiX commonly:
    //   W = 1
    //   Y = sin(az) * cos(el)   (left-right; +Y = left in AmbiX)
    //   Z = sin(el)
    //   X = cos(az) * cos(el)   (front-back; +X = front)
    //
    // Our spherical: az = atan2(x, y) so az=0 front(+Y), az=+pi/2 right(+X)
    // Mapping to Ambisonics (az_ambi = -az so +Y ambi = left):
    float az = 0.0f, el = 0.0f;
    d.toSpherical (az, el);
    const float azAmbi = -az; // convert right-handed room to AmbiX left-positive Y

    const float ce = std::cos (el);
    gainsOut[0] = 1.0f;                    // W
    gainsOut[1] = std::sin (azAmbi) * ce;   // Y
    gainsOut[2] = std::sin (el);            // Z
    gainsOut[3] = std::cos (azAmbi) * ce;   // X
}

std::unique_ptr<IChannelEncoder> createEncoder (OutputFormat format)
{
    switch (format)
    {
        case OutputFormat::mono:            return std::make_unique<MonoEncoder>();
        case OutputFormat::stereo:          return std::make_unique<StereoEncoder>();
        case OutputFormat::quad:            return std::make_unique<QuadEncoder>();
        case OutputFormat::ambixFirstOrder: return std::make_unique<AmbixEncoder>();
        default:                            return std::make_unique<MonoEncoder>();
    }
}

//==============================================================================
void synthesiseIRFromHistograms (const std::vector<std::vector<std::array<float, kNumOctaveBands>>>& channelHistograms,
                                 double sampleRate,
                                 double binDurationSeconds,
                                 juce::AudioBuffer<float>& dest,
                                 uint32_t seed)
{
    if (channelHistograms.empty() || sampleRate <= 0.0 || binDurationSeconds <= 0.0)
    {
        dest.setSize (0, 0);
        return;
    }

    const int numChannels = static_cast<int> (channelHistograms.size());
    const int numBins = static_cast<int> (channelHistograms.front().size());
    const int numSamples = std::max (1, static_cast<int> (std::ceil (numBins * binDurationSeconds * sampleRate)));

    dest.setSize (numChannels, numSamples, false, true, false);
    dest.clear();

    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> noiseDist (-1.0f, 1.0f);

    // For each band, generate a short band-limited noise burst kernel, then place
    // scaled copies at each time bin (density based on energy).
    constexpr int kKernelSamples = 64;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* write = dest.getWritePointer (ch);

        for (int band = 0; band < kNumOctaveBands; ++band)
        {
            const float centreHz = kOctaveBandCentreHz[static_cast<size_t> (band)];
            const float bandwidth = centreHz * 0.7f; // approx one-octave-ish

            // Build a bandpass-ish noise kernel via modulated noise + exponential envelope
            std::array<float, kKernelSamples> kernel {};
            float kernelEnergy = 0.0f;
            for (int i = 0; i < kKernelSamples; ++i)
            {
                const float env = std::exp (-3.0f * static_cast<float> (i) / static_cast<float> (kKernelSamples));
                const float t = static_cast<float> (i) / static_cast<float> (sampleRate);
                const float carrier = std::sin (2.0f * kPi * centreHz * t);
                // Mild bandwidth via secondary modulation
                const float mod = std::sin (2.0f * kPi * (bandwidth * 0.25f) * t);
                kernel[static_cast<size_t> (i)] = noiseDist (rng) * env * (0.7f * carrier + 0.3f * mod);
                kernelEnergy += kernel[static_cast<size_t> (i)] * kernel[static_cast<size_t> (i)];
            }

            const float invNorm = (kernelEnergy > 1.0e-12f) ? (1.0f / std::sqrt (kernelEnergy)) : 0.0f;
            for (auto& s : kernel)
                s *= invNorm;

            for (int bin = 0; bin < numBins; ++bin)
            {
                const float weight = channelHistograms[static_cast<size_t> (ch)][static_cast<size_t> (bin)][static_cast<size_t> (band)];
                if (std::abs (weight) <= 0.0f)
                    continue;

                // Signed amplitude for Ambisonics; sqrt(|E|) for positive energy-like bins
                const float amp = (weight >= 0.0f) ? std::sqrt (weight) : -std::sqrt (-weight);
                const int centreSample = static_cast<int> (std::lround (bin * binDurationSeconds * sampleRate));

                for (int i = 0; i < kKernelSamples; ++i)
                {
                    const int s = centreSample + i;
                    if (s >= 0 && s < numSamples)
                        write[s] += amp * kernel[static_cast<size_t> (i)];
                }
            }
        }
    }

    // Peak-normalise softly so exports aren't tiny / clipped
    float peak = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* r = dest.getReadPointer (ch);
        for (int i = 0; i < numSamples; ++i)
            peak = std::max (peak, std::abs (r[i]));
    }

    if (peak > 1.0e-8f)
    {
        const float scale = 0.9f / peak;
        dest.applyGain (scale);
    }
}

} // namespace letsir

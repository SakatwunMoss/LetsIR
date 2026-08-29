#include "AcousticSimulator.h"
#include "../Utf8.h"

#include <cmath>
#include <vector>

namespace letsir
{

namespace
{
constexpr float kSpeedOfSound = 343.0f;
constexpr float kPi = 3.14159265358979323846f;
constexpr double kBinDuration = 0.001;

void accumulateArrival (std::vector<std::vector<std::array<float, kNumOctaveBands>>>& histograms,
                        const IChannelEncoder& encoder,
                        const Vec3& arrivalDir,
                        float pathDistance,
                        const std::array<float, kNumOctaveBands>& bandEnergy,
                        float detectWeight,
                        float irSeconds,
                        int numBins)
{
    const float arrivalTime = pathDistance / kSpeedOfSound;
    if (arrivalTime < 0.0f || arrivalTime >= irSeconds)
        return;

    const int bin = static_cast<int> (arrivalTime / kBinDuration);
    if (bin < 0 || bin >= numBins)
        return;

    std::array<float, 16> gains {};
    encoder.getChannelGains (arrivalDir, gains.data());

    const int numChannels = encoder.getNumChannels();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float g = gains[static_cast<size_t> (ch)];
        if (std::abs (g) < 1.0e-10f)
            continue;

        for (int band = 0; band < kNumOctaveBands; ++band)
        {
            const float e = bandEnergy[static_cast<size_t> (band)] * detectWeight;
            if (e == 0.0f)
                continue;

            histograms[static_cast<size_t> (ch)][static_cast<size_t> (bin)][static_cast<size_t> (band)] += e * g;
        }
    }
}
} // namespace

AcousticSimulator::AcousticSimulator()
    : juce::Thread ("LetsIRAcousticSim"),
      dispatcher_ (*this),
      progressDispatcher_ (*this)
{
}

AcousticSimulator::~AcousticSimulator()
{
    cancel();
    stopThread (8000);
}

void AcousticSimulator::start (const SimulationSettings& settings,
                               const MaterialLibrary& library,
                               ProgressCallback onProgress,
                               FinishedCallback onFinished)
{
    if (isThreadRunning())
        return;

    settings_ = settings;
    library_ = library;
    progressCallback_ = std::move (onProgress);
    finishedCallback_ = std::move (onFinished);
    cancelFlag_.store (false);
    hasPendingResult_.store (false);
    pendingProgress_.store (0.0f);

    startThread (juce::Thread::Priority::normal);
}

void AcousticSimulator::cancel()
{
    cancelFlag_.store (true);
}

void AcousticSimulator::run()
{
    ProgressCallback progress = [this] (float p)
    {
        pendingProgress_.store (p);
        progressDispatcher_.triggerAsyncUpdate();
    };

    pendingResult_ = runSimulation (settings_, library_, cancelFlag_, progress);
    hasPendingResult_.store (true);
    dispatcher_.triggerAsyncUpdate();
}

void AcousticSimulator::CompletionDispatcher::handleAsyncUpdate()
{
    if (! owner.hasPendingResult_.exchange (false))
        return;

    if (owner.finishedCallback_)
        owner.finishedCallback_ (std::move (owner.pendingResult_));
}

void AcousticSimulator::ProgressDispatcher::handleAsyncUpdate()
{
    if (owner.progressCallback_)
        owner.progressCallback_ (owner.pendingProgress_.load());
}

SimulationResult AcousticSimulator::runSimulation (const SimulationSettings& settings,
                                                   const MaterialLibrary& library,
                                                   std::atomic<bool>& shouldCancel,
                                                   ProgressCallback& progress)
{
    SimulationResult result;
    result.sampleRate = settings.sampleRate;
    result.format = settings.outputFormat;

    RoomModel room;
    room.buildBox (settings.room, library, settings.materialIds);

    Vec3 source = settings.sourcePosition;
    Vec3 listener = settings.listenerPosition;
    if (settings.useDefaultPositions)
    {
        source = room.getDefaultSourcePosition();
        listener = room.getDefaultListenerPosition();
    }

    if (! room.containsPoint (source) || ! room.containsPoint (listener))
    {
        result.message = utf8 ("音源または受音点が部屋の外にあります");
        return result;
    }

    const float rt60 = room.estimateRT60();
    result.estimatedRT60 = rt60;

    float irSeconds = settings.irLengthSeconds;
    if (irSeconds <= 0.0f)
        irSeconds = std::min (settings.irLengthMaxSeconds, std::max (0.25f, rt60 * 1.5f));
    else
        irSeconds = std::min (irSeconds, settings.irLengthMaxSeconds);

    result.irLengthSeconds = irSeconds;

    const int numBins = std::max (1, static_cast<int> (std::ceil (irSeconds / kBinDuration)));
    auto encoder = createEncoder (settings.outputFormat);
    const int numChannels = encoder->getNumChannels();

    using BandArray = std::array<float, kNumOctaveBands>;
    std::vector<std::vector<BandArray>> histograms (
        static_cast<size_t> (numChannels),
        std::vector<BandArray> (static_cast<size_t> (numBins)));

    std::mt19937 rng (settings.randomSeed);

    const float listenerRadius = std::max (0.05f, settings.listenerRadius);
    const float listenerRadiusSq = listenerRadius * listenerRadius;
    // Detection weight approximates collecting energy through a sphere of given radius
    const float detectWeight = 1.0f / (4.0f * kPi * std::max (0.01f, listenerRadiusSq));

    // Optional explicit direct path (off by default for convolution-reverb IRs).
    if (settings.includeDirectSound)
    {
        const Vec3 toListener = listener - source;
        const float dist = toListener.length();
        if (dist > 1.0e-4f)
        {
            BandArray unity {};
            for (auto& e : unity)
                e = 1.0f / std::max (1.0f, dist); // relative pressure ~ 1/r

            const Vec3 arrivalDir = (source - listener).normalised();
            accumulateArrival (histograms, *encoder, arrivalDir, dist, unity,
                               1.0f, irSeconds, numBins);
        }
    }

    const int numRays = std::max (1, settings.numRays);
    const float energyPerRay = 1.0f / static_cast<float> (numRays);

    for (int rayIndex = 0; rayIndex < numRays; ++rayIndex)
    {
        if (shouldCancel.load())
        {
            result.message = utf8 ("キャンセルされました");
            return result;
        }

        if ((rayIndex & 63) == 0)
            progress (static_cast<float> (rayIndex) / static_cast<float> (numRays));

        Vec3 origin = source;
        Vec3 direction = randomUnitVector (rng);
        BandArray bandEnergy {};
        for (auto& e : bandEnergy)
            e = energyPerRay;

        float pathLength = 0.0f;

        for (int bounce = 0; bounce <= settings.maxBounces; ++bounce)
        {
            float wallDist = 0.0f;
            const int hitIndex = room.findClosestHit (origin, direction, wallDist, 1.0e-4f);
            if (hitIndex < 0)
                break;

            // Ray–sphere intersection with listener.
            // bounce == 0 is the unreflected direct segment — skip unless includeDirectSound.
            const bool allowDetection = settings.includeDirectSound || bounce > 0;

            if (allowDetection)
            {
                const Vec3 oc = origin - listener;
                const float bCoef = 2.0f * direction.dot (oc);
                const float cCoef = oc.lengthSquared() - listenerRadiusSq;
                const float disc = bCoef * bCoef - 4.0f * cCoef;

                if (disc >= 0.0f)
                {
                    const float sqrtDisc = std::sqrt (disc);
                    float tHit = (-bCoef - sqrtDisc) * 0.5f;
                    if (tHit < 1.0e-4f)
                        tHit = (-bCoef + sqrtDisc) * 0.5f;

                    if (tHit >= 1.0e-4f && tHit < wallDist)
                    {
                        const Vec3 arrivalDir = (direction * -1.0f).normalised();
                        accumulateArrival (histograms, *encoder, arrivalDir,
                                           pathLength + tHit, bandEnergy,
                                           detectWeight, irSeconds, numBins);
                    }
                }
            }

            const auto& poly = room.getPolygons()[static_cast<size_t> (hitIndex)];
            const Vec3 hitPoint = origin + direction * wallDist;
            pathLength += wallDist;

            if (pathLength / kSpeedOfSound >= irSeconds)
                break;

            float maxE = 0.0f;
            for (int band = 0; band < kNumOctaveBands; ++band)
            {
                const float alpha = std::clamp (poly.material.absorption[static_cast<size_t> (band)], 0.0f, 0.99f);
                bandEnergy[static_cast<size_t> (band)] *= (1.0f - alpha);
                maxE = std::max (maxE, bandEnergy[static_cast<size_t> (band)]);
            }

            if (maxE < settings.energyThreshold)
                break;

            const Vec3 specular = direction.reflected (poly.normal).normalised();
            const Vec3 diffuse = randomCosineHemisphere (poly.normal, rng);
            const float scatter = std::clamp (poly.material.scattering, 0.0f, 1.0f);

            std::uniform_real_distribution<float> u01 (0.0f, 1.0f);
            direction = (u01 (rng) < scatter) ? diffuse : specular;
            origin = hitPoint + poly.normal * 1.0e-4f;
        }
    }

    progress (1.0f);

    synthesiseIRFromHistograms (histograms, settings.sampleRate, kBinDuration,
                                result.ir, settings.randomSeed);
    result.message = utf8 ("完了");
    return result;
}

} // namespace letsir

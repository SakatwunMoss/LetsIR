#pragma once

#include "ChannelEncoder.h"
#include "MaterialLibrary.h"
#include "RoomModel.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <string>

namespace letsir
{

struct SimulationSettings
{
    RoomDimensions room;
    std::array<std::string, kNumRoomFaces> materialIds {
        "gypsum", "gypsum", "gypsum", "gypsum", "wood_floor", "gypsum"
    };

    Vec3 sourcePosition {};
    Vec3 listenerPosition {};
    bool useDefaultPositions = true;

    OutputFormat outputFormat = OutputFormat::stereo;
    double sampleRate = 48000.0;

    int numRays = 8000;
    int maxBounces = 64;
    float energyThreshold = 1.0e-6f;
    float listenerRadius = 0.25f; // metres — detection sphere

    /** Convolution-reverb IRs exclude the direct path by default. */
    bool includeDirectSound = false;

    /** If > 0, clamp IR length to this many seconds; otherwise use ~1.5 * RT60. */
    float irLengthSeconds = 0.0f; // 0 = auto
    float irLengthMaxSeconds = 4.0f;

    uint32_t randomSeed = 42;
};

struct SimulationResult
{
    juce::AudioBuffer<float> ir;
    double sampleRate = 48000.0;
    OutputFormat format = OutputFormat::stereo;
    float estimatedRT60 = 0.0f;
    float irLengthSeconds = 0.0f;
    juce::String message;
};

/**
 * Geometric-acoustics ray tracer. Runs on a background thread and reports progress.
 */
class AcousticSimulator final : private juce::Thread
{
public:
    using ProgressCallback = std::function<void (float progress01)>;
    using FinishedCallback = std::function<void (SimulationResult result)>;

    AcousticSimulator();
    ~AcousticSimulator() override;

    bool isRunning() const noexcept { return isThreadRunning(); }

    /** Start a simulation. Callbacks are invoked on the message thread via AsyncUpdater. */
    void start (const SimulationSettings& settings,
                const MaterialLibrary& library,
                ProgressCallback onProgress,
                FinishedCallback onFinished);

    void cancel();

private:
    void run() override;

    SimulationResult runSimulation (const SimulationSettings& settings,
                                    const MaterialLibrary& library,
                                    std::atomic<bool>& shouldCancel,
                                    ProgressCallback& progress);

    SimulationSettings settings_;
    MaterialLibrary library_;
    ProgressCallback progressCallback_;
    FinishedCallback finishedCallback_;
    std::atomic<bool> cancelFlag_ { false };

    // Cross-thread delivery
    class CompletionDispatcher : public juce::AsyncUpdater
    {
    public:
        AcousticSimulator& owner;
        explicit CompletionDispatcher (AcousticSimulator& o) : owner (o) {}
        void handleAsyncUpdate() override;
    };

    CompletionDispatcher dispatcher_;
    SimulationResult pendingResult_;
    std::atomic<float> pendingProgress_ { 0.0f };
    std::atomic<bool> hasPendingResult_ { false };

    class ProgressDispatcher : public juce::AsyncUpdater
    {
    public:
        AcousticSimulator& owner;
        explicit ProgressDispatcher (AcousticSimulator& o) : owner (o) {}
        void handleAsyncUpdate() override;
    };

    ProgressDispatcher progressDispatcher_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AcousticSimulator)
};

} // namespace letsir

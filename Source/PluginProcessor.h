#pragma once



#include <juce_audio_processors/juce_audio_processors.h>



#include <atomic>



namespace LetsIRCapture

{

    inline constexpr double kSweepStartFrequencyHz = 20.0;

    inline constexpr double kSweepEndFrequencyHz = 20000.0;

    inline constexpr double kSweepDurationSeconds = 5.0;



    /** Extra capture time after the sweep ends (reverb tail, I/O latency, etc.). */

    inline constexpr double kRecordingPaddingSeconds = 3.0;

}



enum class CaptureState

{

    idle,

    recording,

    done

};



class LetsIRAudioProcessor final : public juce::AudioProcessor

{

public:

    LetsIRAudioProcessor();

    ~LetsIRAudioProcessor() override;



    void prepareToPlay (double sampleRate, int samplesPerBlock) override;

    void releaseResources() override;



    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;



    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    using AudioProcessor::processBlock;



    juce::AudioProcessorEditor* createEditor() override;

    bool hasEditor() const override;



    const juce::String getName() const override;



    bool acceptsMidi() const override;

    bool producesMidi() const override;

    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;



    int getNumPrograms() override;

    int getCurrentProgram() override;

    void setCurrentProgram (int index) override;

    const juce::String getProgramName (int index) override;

    void changeProgramName (int index, const juce::String& newName) override;



    void getStateInformation (juce::MemoryBlock& destData) override;

    void setStateInformation (const void* data, int sizeInBytes) override;



    /** Starts synchronized sweep playback and input capture. */

    void startSweepPlayback();



    /** Runs deconvolution and IR post-processing after capture completes. */

    void processRecordedCapture();



    /** Returns true once when a capture has just finished. */

    bool consumeCaptureCompletePending() noexcept;



    CaptureState getCaptureState() const noexcept;

    bool isSweepPlaying() const noexcept;

    bool hasProcessedIR() const;

    juce::AudioBuffer<float> getProcessedIRCopy() const;

    double getCurrentSampleRate() const noexcept;

    void setInputGainDb (float gainDb) noexcept;

    float getInputGainDb() const noexcept;

    float getInputLevelDb() const noexcept;



private:

    juce::CriticalSection captureLock_;

    juce::AudioBuffer<float> sweepBuffer_;

    juce::AudioBuffer<float> recordedBuffer_;
    juce::AudioBuffer<float> irBuffer_;

    int sweepSampleCount_ = 0;

    int totalCaptureSampleCount_ = 0;

    std::atomic<int> capturePosition_ { 0 };

    std::atomic<CaptureState> captureState_ { CaptureState::idle };
    std::atomic<bool> captureCompletePending_ { false };

    std::atomic<float> inputGainLinear_ { 1.0f };
    std::atomic<float> inputLevelDb_ { -100.0f };

    double currentSampleRate_ = 44100.0;

    void updateInputLevelFromBuffer (const juce::AudioBuffer<float>& buffer,
                                     int numInputChannels,
                                     int numSamples) noexcept;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LetsIRAudioProcessor)

};


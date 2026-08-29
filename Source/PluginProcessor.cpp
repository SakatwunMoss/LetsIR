#include "PluginProcessor.h"

#include "Deconvolver.h"
#include "IRPostProcessor.h"
#include "PluginEditor.h"
#include "SweepGenerator.h"

#include <cmath>
#include <iostream>



LetsIRAudioProcessor::LetsIRAudioProcessor()

     : AudioProcessor (BusesProperties()

                     #if ! JucePlugin_IsMidiEffect

                      #if ! JucePlugin_IsSynth

                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)

                      #endif

                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)

                     #endif

                       )

{

}



LetsIRAudioProcessor::~LetsIRAudioProcessor() = default;



const juce::String LetsIRAudioProcessor::getName() const

{

    return JucePlugin_Name;

}



bool LetsIRAudioProcessor::acceptsMidi() const

{

   #if JucePlugin_WantsMidiInput

    return true;

   #else

    return false;

   #endif

}



bool LetsIRAudioProcessor::producesMidi() const

{

   #if JucePlugin_ProducesMidiOutput

    return true;

   #else

    return false;

   #endif

}



bool LetsIRAudioProcessor::isMidiEffect() const

{

   #if JucePlugin_IsMidiEffect

    return true;

   #else

    return false;

   #endif

}



double LetsIRAudioProcessor::getTailLengthSeconds() const

{

    return 0.0;

}



int LetsIRAudioProcessor::getNumPrograms()

{

    return 1;

}



int LetsIRAudioProcessor::getCurrentProgram()

{

    return 0;

}



void LetsIRAudioProcessor::setCurrentProgram (int index)

{

    juce::ignoreUnused (index);

}



const juce::String LetsIRAudioProcessor::getProgramName (int index)

{

    juce::ignoreUnused (index);

    return {};

}



void LetsIRAudioProcessor::changeProgramName (int index, const juce::String& newName)

{

    juce::ignoreUnused (index, newName);

}



void LetsIRAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)

{

    juce::ignoreUnused (samplesPerBlock);

    currentSampleRate_ = sampleRate;

}



void LetsIRAudioProcessor::releaseResources()

{

}



bool LetsIRAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const

{

  #if JucePlugin_IsMidiEffect

    juce::ignoreUnused (layouts);

    return true;

  #else

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()

     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())

        return false;



   #if ! JucePlugin_IsSynth

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())

        return false;

   #endif



    return true;

  #endif

}



void LetsIRAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,

                                         juce::MidiBuffer& midiMessages)

{

    juce::ignoreUnused (midiMessages);



    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();

    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    const auto numSamples = buffer.getNumSamples();



    if (captureState_.load() != CaptureState::recording)

    {

        for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)

            buffer.clear (i, 0, numSamples);



        return;

    }



    const juce::ScopedLock lock (captureLock_);



    if (sweepBuffer_.getNumSamples() == 0 || totalCaptureSampleCount_ <= 0)

    {

        captureState_.store (CaptureState::done);

        return;

    }



    auto capturePosition = capturePosition_.load();

#if JUCE_DEBUG
    const auto writePositionAtBlockStart = capturePosition;

    float maxInputAmplitudeCh0 = 0.0f;

    if (totalNumInputChannels > 0)
    {
        const auto* inputCh0 = buffer.getReadPointer (0);

        for (int i = 0; i < numSamples; ++i)
            maxInputAmplitudeCh0 = juce::jmax (maxInputAmplitudeCh0, std::abs (inputCh0[i]));
    }

    int samplesWrittenThisBlock = 0;
#endif

    for (int sample = 0; sample < numSamples; ++sample)

    {

        if (capturePosition >= totalCaptureSampleCount_)

        {

            captureState_.store (CaptureState::done);

            captureCompletePending_.store (true);

            break;

        }



        const auto recordedChannels = recordedBuffer_.getNumChannels();



        for (int channel = 0; channel < recordedChannels; ++channel)

        {

            const float inputSample = channel < totalNumInputChannels

                                          ? buffer.getSample (channel, sample)

                                          : 0.0f;

            recordedBuffer_.setSample (channel, capturePosition, inputSample);

        }

#if JUCE_DEBUG
        ++samplesWrittenThisBlock;
#endif

        if (capturePosition < sweepSampleCount_)

        {

            const auto sweepSample = sweepBuffer_.getSample (0, capturePosition);



            for (int channel = 0; channel < totalNumOutputChannels; ++channel)

                buffer.setSample (channel, sample, sweepSample);

        }

        else

        {

            for (int channel = 0; channel < totalNumOutputChannels; ++channel)

                buffer.setSample (channel, sample, 0.0f);

        }



        ++capturePosition;

    }



    capturePosition_.store (capturePosition);

#if JUCE_DEBUG
    const auto recordingWriteCalled = samplesWrittenThisBlock > 0;

    DBG ("processBlock [recording]: inputChannels=" << totalNumInputChannels
         << ", blockSamples=" << numSamples
         << ", maxInputAmpCh0=" << maxInputAmplitudeCh0
         << ", writePos=" << writePositionAtBlockStart
         << "->" << capturePosition
         << ", recordingWriteCalled=" << (recordingWriteCalled ? "yes" : "no")
         << ", samplesCopied=" << samplesWrittenThisBlock);
#endif

    if (capturePosition >= totalCaptureSampleCount_)

    {

        captureState_.store (CaptureState::done);

        captureCompletePending_.store (true);

    }

}



void LetsIRAudioProcessor::processRecordedCapture()

{

    juce::AudioBuffer<float> recordedCopy;

    juce::AudioBuffer<float> sweepCopy;



    {

        const juce::ScopedLock lock (captureLock_);

        recordedCopy.makeCopyOf (recordedBuffer_);

        sweepCopy.makeCopyOf (sweepBuffer_);

    }



    auto impulseResponse = Deconvolver::deconvolve (recordedCopy, sweepCopy);



    const auto deconvMessage = juce::String ("Deconvolution complete: IR samples=")

                             + juce::String (impulseResponse.getNumSamples())

                             + ", peak=" + juce::String (impulseResponse.getMagnitude (0, impulseResponse.getNumSamples()), 6);



    DBG (deconvMessage);

    std::cout << deconvMessage << std::endl;



    const auto sampleRate = currentSampleRate_ > 0.0 ? currentSampleRate_ : 44100.0;

    LetsIRPostProcess::postProcessIR (impulseResponse, sampleRate);



    const juce::ScopedLock lock (captureLock_);

    irBuffer_ = std::move (impulseResponse);

}



bool LetsIRAudioProcessor::consumeCaptureCompletePending() noexcept

{

    return captureCompletePending_.exchange (false);

}



void LetsIRAudioProcessor::startSweepPlayback()

{

    if (captureState_.load() == CaptureState::recording)

        return;



    const auto sampleRate = currentSampleRate_ > 0.0 ? currentSampleRate_ : 44100.0;

    const auto numInputChannels = juce::jmax (1, getTotalNumInputChannels());



    SweepGenerator generator (LetsIRCapture::kSweepStartFrequencyHz,

                              LetsIRCapture::kSweepEndFrequencyHz,

                              LetsIRCapture::kSweepDurationSeconds,

                              sampleRate);

    auto generatedSweep = generator.generate();



    const auto sweepSamples = generatedSweep.getNumSamples();

    const auto paddingSamples = juce::jmax (1, static_cast<int> (std::round (LetsIRCapture::kRecordingPaddingSeconds * sampleRate)));

    const auto totalCaptureSamples = sweepSamples + paddingSamples;



    {

        const juce::ScopedLock lock (captureLock_);

        sweepBuffer_ = std::move (generatedSweep);

        recordedBuffer_.setSize (numInputChannels, totalCaptureSamples);

        recordedBuffer_.clear();

        sweepSampleCount_ = sweepSamples;

        totalCaptureSampleCount_ = totalCaptureSamples;

        capturePosition_.store (0);

#if JUCE_DEBUG
        const float firstSampleCh0 = recordedBuffer_.getNumSamples() > 0
                                         ? recordedBuffer_.getSample (0, 0)
                                         : 0.0f;

        DBG ("startSweepPlayback: recordedBuffer channels=" << recordedBuffer_.getNumChannels()
             << ", samples=" << recordedBuffer_.getNumSamples()
             << ", totalCaptureSamples=" << totalCaptureSamples
             << ", sweepSamples=" << sweepSamples
             << ", paddingSamples=" << paddingSamples
             << ", numInputChannels=" << numInputChannels
             << ", getTotalNumInputChannels()=" << getTotalNumInputChannels()
             << ", capturePosition=" << capturePosition_.load()
             << ", captureState=recording (next)"
             << ", bufferInitialized=" << (recordedBuffer_.getNumSamples() > 0 ? "yes" : "no")
             << ", bufferCleared=" << (firstSampleCh0 == 0.0f ? "yes" : "no")
             << ", firstSampleCh0=" << firstSampleCh0);
#endif
    }

    captureState_.store (CaptureState::recording);

}



CaptureState LetsIRAudioProcessor::getCaptureState() const noexcept

{

    return captureState_.load();

}



bool LetsIRAudioProcessor::isSweepPlaying() const noexcept

{

    return captureState_.load() == CaptureState::recording;

}



bool LetsIRAudioProcessor::hasProcessedIR() const

{

    const juce::ScopedLock lock (captureLock_);

    return irBuffer_.getNumSamples() > 0;

}



juce::AudioBuffer<float> LetsIRAudioProcessor::getProcessedIRCopy() const

{

    const juce::ScopedLock lock (captureLock_);

    juce::AudioBuffer<float> copy;

    copy.makeCopyOf (irBuffer_);

    return copy;

}



double LetsIRAudioProcessor::getCurrentSampleRate() const noexcept

{

    return currentSampleRate_;

}



bool LetsIRAudioProcessor::hasEditor() const

{

    return true;

}



juce::AudioProcessorEditor* LetsIRAudioProcessor::createEditor()

{

    return new LetsIRAudioProcessorEditor (*this);

}



void LetsIRAudioProcessor::getStateInformation (juce::MemoryBlock& destData)

{

    juce::ignoreUnused (destData);

}



void LetsIRAudioProcessor::setStateInformation (const void* data, int sizeInBytes)

{

    juce::ignoreUnused (data, sizeInBytes);

}



juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()

{

    return new LetsIRAudioProcessor();

}


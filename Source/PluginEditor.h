#pragma once



#include "PluginProcessor.h"



class LetsIRAudioProcessorEditor final : public juce::AudioProcessorEditor,

                                         private juce::Timer

{

public:

    explicit LetsIRAudioProcessorEditor (LetsIRAudioProcessor&);

    ~LetsIRAudioProcessorEditor() override;



    void paint (juce::Graphics&) override;

    void resized() override;



private:

    void timerCallback() override;

    void exportIRToWav();

    static juce::String captureStateToString (CaptureState state);

    static juce::String makeDefaultExportFileName();



    LetsIRAudioProcessor& processorRef;

    juce::TextButton playSweepButton { "Play Sweep" };

    juce::TextButton exportWavButton { "Export WAV" };

    juce::Label statusLabel;

    juce::Label exportStatusLabel;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LetsIRAudioProcessorEditor)

};


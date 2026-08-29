#pragma once



#include "IRWaveformComponent.h"
#include "InputLevelMeterComponent.h"
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
    void updateIRWaveform();

    static juce::String captureStateToString (CaptureState state);

    static juce::String makeDefaultExportFileName();



    LetsIRAudioProcessor& processorRef;

    juce::TextButton playSweepButton { "Play Sweep" };

    juce::TextButton exportWavButton { "Export WAV" };

    juce::Label statusLabel;

    juce::Label exportStatusLabel;
    IRWaveformComponent irWaveformComponent;

    juce::Label inputGainLabel;
    juce::Slider inputGainSlider;
    InputLevelMeterComponent inputLevelMeter;

#if JucePlugin_Build_Standalone
    juce::ToggleButton enableInputButton { "Enable Input" };
#endif

    float meterDisplayDb_ = -60.0f;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LetsIRAudioProcessorEditor)

};


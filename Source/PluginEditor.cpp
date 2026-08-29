#include "PluginProcessor.h"

#include "PluginEditor.h"

#include "IRWavExporter.h"

#if JucePlugin_Build_Standalone
 #include <juce_audio_devices/juce_audio_devices.h>
 #include <juce_audio_utils/juce_audio_utils.h>
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#endif



LetsIRAudioProcessorEditor::LetsIRAudioProcessorEditor (LetsIRAudioProcessor& p)

    : AudioProcessorEditor (&p), processorRef (p)

{

    playSweepButton.onClick = [this]

    {

        processorRef.startSweepPlayback();

        meterDisplayDb_ = -60.0f;

        playSweepButton.setEnabled (false);

        statusLabel.setText (captureStateToString (CaptureState::recording), juce::dontSendNotification);

    };



    exportWavButton.onClick = [this] { exportIRToWav(); };



    statusLabel.setJustificationType (juce::Justification::centred);

    statusLabel.setText (captureStateToString (processorRef.getCaptureState()), juce::dontSendNotification);



    exportStatusLabel.setJustificationType (juce::Justification::centred);



    inputGainLabel.setText ("Input Gain", juce::dontSendNotification);
    inputGainLabel.setJustificationType (juce::Justification::centredLeft);

    inputGainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    inputGainSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
    inputGainSlider.setRange (-12.0, 24.0, 0.1);
    inputGainSlider.setValue (0.0, juce::dontSendNotification);
    inputGainSlider.setTextValueSuffix (" dB");
    inputGainSlider.onValueChange = [this]
    {
        processorRef.setInputGainDb (static_cast<float> (inputGainSlider.getValue()));
    };

#if JucePlugin_Build_Standalone
    enableInputButton.setClickingTogglesState (true);
    enableInputButton.setTooltip ("Unmutes Standalone input. Required for metering and recording.");
    enableInputButton.onClick = [this]
    {
        if (auto* holder = juce::StandalonePluginHolder::getInstance())
            holder->getMuteInputValue() = ! enableInputButton.getToggleState();
    };

    if (auto* holder = juce::StandalonePluginHolder::getInstance())
        enableInputButton.setToggleState (! (bool) holder->getMuteInputValue().getValue(), juce::dontSendNotification);
#endif



    addAndMakeVisible (playSweepButton);

    addAndMakeVisible (exportWavButton);

    addAndMakeVisible (statusLabel);

    addAndMakeVisible (exportStatusLabel);
    addAndMakeVisible (irWaveformComponent);
    addAndMakeVisible (inputGainLabel);
    addAndMakeVisible (inputGainSlider);
    addAndMakeVisible (inputLevelMeter);
#if JucePlugin_Build_Standalone
    addAndMakeVisible (enableInputButton);
#endif
    irWaveformComponent.setVisible (false);

    if (processorRef.hasProcessedIR())
        updateIRWaveform();

    startTimer (30);

    setSize (400, 560);

}



LetsIRAudioProcessorEditor::~LetsIRAudioProcessorEditor() = default;



void LetsIRAudioProcessorEditor::paint (juce::Graphics& g)

{

    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));



    g.setColour (juce::Colours::white);

    g.setFont (15.0f);

    g.drawFittedText ("LetsIR", getLocalBounds().reduced (0, 140), juce::Justification::centred, 1);



    // Standalone: Audio Settings -> enable input channels and select the mic/line device.

    g.setFont (12.0f);

    g.setColour (juce::Colours::lightgrey);

    g.drawFittedText ("Standalone: select input device in Audio/MIDI Settings, then turn ON \"Enable Input\"",

                      getLocalBounds().removeFromBottom (36).reduced (8, 0),

                      juce::Justification::centred,

                      2);

}



void LetsIRAudioProcessorEditor::resized()

{

    auto bounds = getLocalBounds().reduced (40);

    statusLabel.setBounds (bounds.removeFromTop (28));

    exportStatusLabel.setBounds (bounds.removeFromTop (24));

    irWaveformComponent.setBounds (bounds.removeFromTop (150));

    bounds.removeFromTop (8);

    auto inputRow = bounds.removeFromTop (120);
    inputLevelMeter.setBounds (inputRow.removeFromLeft (48));
    inputRow.removeFromLeft (8);

    auto gainArea = inputRow;
    inputGainLabel.setBounds (gainArea.removeFromTop (20));
    inputGainSlider.setBounds (gainArea.removeFromTop (28));
#if JucePlugin_Build_Standalone
    gainArea.removeFromTop (4);
    enableInputButton.setBounds (gainArea.removeFromTop (24));
#endif

    bounds.removeFromTop (8);

    auto buttonRow = bounds.withSizeKeepingCentre (160, 72);

    playSweepButton.setBounds (buttonRow.removeFromTop (32));

    buttonRow.removeFromTop (8);

    exportWavButton.setBounds (buttonRow);

}



void LetsIRAudioProcessorEditor::timerCallback()

{

    if (processorRef.consumeCaptureCompletePending())
    {
        processorRef.processRecordedCapture();
        updateIRWaveform();
    }



    const auto state = processorRef.getCaptureState();

    statusLabel.setText (captureStateToString (state), juce::dontSendNotification);

    playSweepButton.setEnabled (state != CaptureState::recording);

    exportWavButton.setEnabled (processorRef.hasProcessedIR());

#if JucePlugin_Build_Standalone
    if (auto* holder = juce::StandalonePluginHolder::getInstance())
        enableInputButton.setToggleState (! (bool) holder->getMuteInputValue().getValue(), juce::dontSendNotification);
#endif

    constexpr float kMinDb = -60.0f;
    const auto targetDb = juce::jlimit (kMinDb, 0.0f, processorRef.getInputLevelDb());

    if (targetDb >= meterDisplayDb_)
        meterDisplayDb_ = targetDb;
    else
        meterDisplayDb_ = juce::jmax (targetDb, meterDisplayDb_ - 2.5f);

    inputLevelMeter.setLevelDb (meterDisplayDb_);

}



void LetsIRAudioProcessorEditor::updateIRWaveform()

{

    if (! processorRef.hasProcessedIR())

        return;



    const auto irBuffer = processorRef.getProcessedIRCopy();

    const auto sampleRate = processorRef.getCurrentSampleRate() > 0.0

                                ? processorRef.getCurrentSampleRate()

                                : 44100.0;



    irWaveformComponent.setIRBuffer (irBuffer, sampleRate);

}



void LetsIRAudioProcessorEditor::exportIRToWav()

{

    if (! processorRef.hasProcessedIR())

        return;



    const auto defaultFileName = makeDefaultExportFileName();

    auto chooser = std::make_shared<juce::FileChooser> ("Export IR as WAV",

                                                          juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)

                                                              .getChildFile (defaultFileName),

                                                          "*.wav");



    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,

                          [safeThis = juce::Component::SafePointer<LetsIRAudioProcessorEditor> (this),

                           chooser,

                           processor = &processorRef] (const juce::FileChooser& fc)

                          {

                              if (safeThis == nullptr)

                                  return;



                              const auto destFile = fc.getResult();



                              if (destFile == juce::File{})

                                  return;



                              const auto irBuffer = processor->getProcessedIRCopy();

                              const auto sampleRate = processor->getCurrentSampleRate() > 0.0

                                                          ? processor->getCurrentSampleRate()

                                                          : 44100.0;



                              juce::File fileToWrite = destFile;



                              if (! fileToWrite.hasFileExtension (".wav"))

                                  fileToWrite = fileToWrite.withFileExtension (".wav");



                              const bool success = exportIRToWavFile (irBuffer, fileToWrite, sampleRate);



                              if (success)

                                  safeThis->exportStatusLabel.setText ("Exported: " + fileToWrite.getFileName(),

                                                                       juce::dontSendNotification);

                              else

                                  safeThis->exportStatusLabel.setText ("Export failed", juce::dontSendNotification);

                          });

}



juce::String LetsIRAudioProcessorEditor::makeDefaultExportFileName()

{

    return "IR_" + juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M") + ".wav";

}



juce::String LetsIRAudioProcessorEditor::captureStateToString (CaptureState state)

{

    switch (state)

    {

        case CaptureState::idle:      return "Idle";

        case CaptureState::recording: return "Recording...";

        case CaptureState::done:      return "Done";

        default:                      return {};

    }

}


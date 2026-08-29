#include "PluginProcessor.h"

#include "PluginEditor.h"

#include "IRWavExporter.h"



LetsIRAudioProcessorEditor::LetsIRAudioProcessorEditor (LetsIRAudioProcessor& p)

    : AudioProcessorEditor (&p), processorRef (p)

{

    playSweepButton.onClick = [this]

    {

        processorRef.startSweepPlayback();

        playSweepButton.setEnabled (false);

        statusLabel.setText (captureStateToString (CaptureState::recording), juce::dontSendNotification);

    };



    exportWavButton.onClick = [this] { exportIRToWav(); };



    statusLabel.setJustificationType (juce::Justification::centred);

    statusLabel.setText (captureStateToString (processorRef.getCaptureState()), juce::dontSendNotification);



    exportStatusLabel.setJustificationType (juce::Justification::centred);



    addAndMakeVisible (playSweepButton);

    addAndMakeVisible (exportWavButton);

    addAndMakeVisible (statusLabel);

    addAndMakeVisible (exportStatusLabel);

    startTimerHz (20);

    setSize (400, 340);

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

    g.drawFittedText ("Standalone: enable input in Audio/MIDI Settings",

                      getLocalBounds().removeFromBottom (36).reduced (8, 0),

                      juce::Justification::centred,

                      2);

}



void LetsIRAudioProcessorEditor::resized()

{

    auto bounds = getLocalBounds().reduced (40);

    statusLabel.setBounds (bounds.removeFromTop (28));

    exportStatusLabel.setBounds (bounds.removeFromTop (24));

    auto buttonRow = bounds.withSizeKeepingCentre (160, 72);

    playSweepButton.setBounds (buttonRow.removeFromTop (32));

    buttonRow.removeFromTop (8);

    exportWavButton.setBounds (buttonRow);

}



void LetsIRAudioProcessorEditor::timerCallback()

{

    if (processorRef.consumeCaptureCompletePending())

        processorRef.processRecordedCapture();



    const auto state = processorRef.getCaptureState();

    statusLabel.setText (captureStateToString (state), juce::dontSendNotification);

    playSweepButton.setEnabled (state != CaptureState::recording);

    exportWavButton.setEnabled (processorRef.hasProcessedIR());

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


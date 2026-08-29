#include "MainComponent.h"

namespace
{
void styleSlider (juce::Slider& s, double minV, double maxV, double interval, double defaultV)
{
    s.setRange (minV, maxV, interval);
    s.setValue (defaultV, juce::dontSendNotification);
    s.setSliderStyle (juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 22);
}

void styleLabel (juce::Label& l)
{
    l.setJustificationType (juce::Justification::centredLeft);
}

juce::Rectangle<int> takeRow (juce::Rectangle<int>& area, int h, int gap = 4)
{
    auto row = area.removeFromTop (h);
    area.removeFromTop (gap);
    return row;
}
} // namespace

MainComponent::MainComponent()
{
    setSize (1100, 860);
    setAudioChannels (0, 2);

    auto add = [this] (juce::Component& c) { addAndMakeVisible (c); };

    add (roomSectionLabel);
    roomSectionLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    for (auto* l : { &widthLabel, &depthLabel, &heightLabel })
    {
        styleLabel (*l);
        add (*l);
    }
    styleSlider (widthSlider, 1.0, 30.0, 0.1, 5.0);
    styleSlider (depthSlider, 1.0, 30.0, 0.1, 6.0);
    styleSlider (heightSlider, 1.5, 10.0, 0.1, 2.8);
    add (widthSlider);
    add (depthSlider);
    add (heightSlider);

    auto onParamChange = [this]
    {
        if (useDefaultPositions.getToggleState())
            applyDefaultsFromRoom();
        refreshLayoutView();
    };
    widthSlider.onValueChange = onParamChange;
    depthSlider.onValueChange = onParamChange;
    heightSlider.onValueChange = onParamChange;

    add (materialSectionLabel);
    materialSectionLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));

    for (int i = 0; i < letsir::kNumRoomFaces; ++i)
    {
        const auto face = static_cast<letsir::RoomFace> (i);
        faceLabels[static_cast<size_t> (i)].setText (utf8 (letsir::roomFaceDisplayName (face)),
                                                     juce::dontSendNotification);
        styleLabel (faceLabels[static_cast<size_t> (i)]);
        add (faceLabels[static_cast<size_t> (i)]);
        populateMaterialCombo (faceMaterialBoxes[static_cast<size_t> (i)]);
        faceMaterialBoxes[static_cast<size_t> (i)].onChange = [this] { refreshLayoutView(); };
        add (faceMaterialBoxes[static_cast<size_t> (i)]);
    }

    faceMaterialBoxes[static_cast<size_t> (letsir::RoomFace::floor)].setSelectedItemIndex (
        static_cast<int> (*materialLibrary_.indexOfId ("wood_floor")), juce::dontSendNotification);

    add (positionSectionLabel);
    positionSectionLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    add (useDefaultPositions);
    useDefaultPositions.setToggleState (true, juce::dontSendNotification);
    useDefaultPositions.onClick = [this]
    {
        const bool useDef = useDefaultPositions.getToggleState();
        for (auto* s : { &sourceX, &sourceY, &sourceZ, &listenerX, &listenerY, &listenerZ })
            s->setEnabled (! useDef);
        if (useDef)
            applyDefaultsFromRoom();
        refreshLayoutView();
    };

    styleLabel (sourceLabel);
    styleLabel (listenerLabel);
    add (sourceLabel);
    add (listenerLabel);

    for (auto* s : { &sourceX, &sourceY, &sourceZ, &listenerX, &listenerY, &listenerZ })
    {
        styleSlider (*s, 0.1, 29.0, 0.01, 1.0);
        s->setEnabled (false);
        s->onValueChange = [this] { refreshLayoutView(); };
        add (*s);
    }
    applyDefaultsFromRoom();

    add (outputSectionLabel);
    outputSectionLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    styleLabel (formatLabel);
    styleLabel (sampleRateLabel);
    styleLabel (bitDepthLabel);
    add (formatLabel);
    add (sampleRateLabel);
    add (bitDepthLabel);

    formatBox.addItem ("Mono", 1);
    formatBox.addItem ("Stereo", 2);
    formatBox.addItem ("Quad (4ch)", 3);
    formatBox.addItem ("AmbiX 1st order (ACN/SN3D)", 4);
    formatBox.setSelectedId (2, juce::dontSendNotification);
    add (formatBox);

    sampleRateBox.addItem ("44100 Hz", 1);
    sampleRateBox.addItem ("48000 Hz", 2);
    sampleRateBox.addItem ("96000 Hz", 3);
    sampleRateBox.addItem ("192000 Hz", 4);
    sampleRateBox.setSelectedId (2, juce::dontSendNotification);
    add (sampleRateBox);

    bitDepthBox.addItem ("16 bit", 1);
    bitDepthBox.addItem ("24 bit", 2);
    bitDepthBox.addItem ("32 bit float", 3);
    bitDepthBox.setSelectedId (3, juce::dontSendNotification);
    add (bitDepthBox);

    add (advancedSectionLabel);
    advancedSectionLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    styleLabel (raysLabel);
    styleLabel (bouncesLabel);
    styleLabel (irLenLabel);
    add (raysLabel);
    add (bouncesLabel);
    add (irLenLabel);
    styleSlider (raysSlider, 500.0, 50000.0, 100.0, 8000.0);
    styleSlider (bouncesSlider, 8.0, 128.0, 1.0, 64.0);
    styleSlider (irLenSlider, 0.0, 8.0, 0.1, 0.0); // 0 = auto
    irLenSlider.textFromValueFunction = [] (double v)
    {
        return v <= 0.0 ? utf8 ("自動") : juce::String (v, 1) + " s";
    };
    add (raysSlider);
    add (bouncesSlider);
    add (irLenSlider);

    simulateButton.onClick = [this] { startSimulation(); };
    cancelButton.onClick = [this] { cancelSimulation(); };
    cancelButton.setEnabled (false);
    playButton.onClick = [this] { playIR(); };
    playButton.setEnabled (false);
    exportButton.onClick = [this] { exportWav(); };
    exportButton.setEnabled (false);

    add (simulateButton);
    add (cancelButton);
    add (playButton);
    add (exportButton);

    statusLabel.setText (utf8 ("部屋パラメータを設定してシミュレートを実行してください"),
                         juce::dontSendNotification);
    statusLabel.setJustificationType (juce::Justification::centredLeft);
    add (statusLabel);

    progressLabel.setText ("", juce::dontSendNotification);
    add (progressLabel);
    add (progressBar);

    layoutView_.setMaterialLibrary (&materialLibrary_);
    add (layoutView_);
    add (waveform_);

    refreshLayoutView();
}

MainComponent::~MainComponent()
{
    simulator_.cancel();
    shutdownAudio();
}

void MainComponent::populateMaterialCombo (juce::ComboBox& box)
{
    box.clear (juce::dontSendNotification);
    int id = 1;
    for (const auto& m : materialLibrary_.getMaterials())
        box.addItem (utf8 (m.displayName), id++);
    box.setSelectedItemIndex (0, juce::dontSendNotification);
}

void MainComponent::applyDefaultsFromRoom()
{
    letsir::RoomDimensions dims {
        static_cast<float> (widthSlider.getValue()),
        static_cast<float> (depthSlider.getValue()),
        static_cast<float> (heightSlider.getValue())
    };
    letsir::RoomModel tmp;
    std::array<std::string, letsir::kNumRoomFaces> ids;
    for (int i = 0; i < letsir::kNumRoomFaces; ++i)
    {
        const int idx = faceMaterialBoxes[static_cast<size_t> (i)].getSelectedItemIndex();
        ids[static_cast<size_t> (i)] = materialLibrary_.getMaterials()[static_cast<size_t> (std::max (0, idx))].id;
    }
    tmp.buildBox (dims, materialLibrary_, ids);
    const auto src = tmp.getDefaultSourcePosition();
    const auto lis = tmp.getDefaultListenerPosition();
    sourceX.setValue (src.x, juce::dontSendNotification);
    sourceY.setValue (src.y, juce::dontSendNotification);
    sourceZ.setValue (src.z, juce::dontSendNotification);
    listenerX.setValue (lis.x, juce::dontSendNotification);
    listenerY.setValue (lis.y, juce::dontSendNotification);
    listenerZ.setValue (lis.z, juce::dontSendNotification);
}

void MainComponent::refreshLayoutView()
{
    letsir::RoomDimensions dims {
        static_cast<float> (widthSlider.getValue()),
        static_cast<float> (depthSlider.getValue()),
        static_cast<float> (heightSlider.getValue())
    };

    std::array<std::string, letsir::kNumRoomFaces> ids;
    const auto& mats = materialLibrary_.getMaterials();
    for (int i = 0; i < letsir::kNumRoomFaces; ++i)
    {
        const int idx = faceMaterialBoxes[static_cast<size_t> (i)].getSelectedItemIndex();
        ids[static_cast<size_t> (i)] = mats[static_cast<size_t> (juce::jlimit (0, (int) mats.size() - 1, idx))].id;
    }

    layoutView_.setRoomState (
        dims, ids,
        { static_cast<float> (sourceX.getValue()),
          static_cast<float> (sourceY.getValue()),
          static_cast<float> (sourceZ.getValue()) },
        { static_cast<float> (listenerX.getValue()),
          static_cast<float> (listenerY.getValue()),
          static_cast<float> (listenerZ.getValue()) });
}

letsir::SimulationSettings MainComponent::collectSettings() const
{
    letsir::SimulationSettings s;
    s.room.width = static_cast<float> (widthSlider.getValue());
    s.room.depth = static_cast<float> (depthSlider.getValue());
    s.room.height = static_cast<float> (heightSlider.getValue());

    for (int i = 0; i < letsir::kNumRoomFaces; ++i)
    {
        const int idx = faceMaterialBoxes[static_cast<size_t> (i)].getSelectedItemIndex();
        const auto& mats = materialLibrary_.getMaterials();
        s.materialIds[static_cast<size_t> (i)] = mats[static_cast<size_t> (juce::jlimit (0, (int) mats.size() - 1, idx))].id;
    }

    s.useDefaultPositions = useDefaultPositions.getToggleState();
    s.sourcePosition = {
        static_cast<float> (sourceX.getValue()),
        static_cast<float> (sourceY.getValue()),
        static_cast<float> (sourceZ.getValue())
    };
    s.listenerPosition = {
        static_cast<float> (listenerX.getValue()),
        static_cast<float> (listenerY.getValue()),
        static_cast<float> (listenerZ.getValue())
    };

    switch (formatBox.getSelectedId())
    {
        case 1:  s.outputFormat = letsir::OutputFormat::mono; break;
        case 2:  s.outputFormat = letsir::OutputFormat::stereo; break;
        case 3:  s.outputFormat = letsir::OutputFormat::quad; break;
        case 4:  s.outputFormat = letsir::OutputFormat::ambixFirstOrder; break;
        default: s.outputFormat = letsir::OutputFormat::stereo; break;
    }

    switch (sampleRateBox.getSelectedId())
    {
        case 1:  s.sampleRate = 44100.0; break;
        case 2:  s.sampleRate = 48000.0; break;
        case 3:  s.sampleRate = 96000.0; break;
        case 4:  s.sampleRate = 192000.0; break;
        default: s.sampleRate = 48000.0; break;
    }

    s.numRays = static_cast<int> (raysSlider.getValue());
    s.maxBounces = static_cast<int> (bouncesSlider.getValue());
    s.irLengthSeconds = static_cast<float> (irLenSlider.getValue());
    s.irLengthMaxSeconds = 8.0f;
    s.includeDirectSound = false;
    return s;
}

int MainComponent::selectedBitsPerSample() const
{
    switch (bitDepthBox.getSelectedId())
    {
        case 1:  return 16;
        case 2:  return 24;
        case 3:  return 32;
        default: return 32;
    }
}

void MainComponent::setControlsEnabled (bool enabled)
{
    widthSlider.setEnabled (enabled);
    depthSlider.setEnabled (enabled);
    heightSlider.setEnabled (enabled);
    for (auto& b : faceMaterialBoxes)
        b.setEnabled (enabled);
    useDefaultPositions.setEnabled (enabled);
    const bool posEnabled = enabled && ! useDefaultPositions.getToggleState();
    for (auto* s : { &sourceX, &sourceY, &sourceZ, &listenerX, &listenerY, &listenerZ })
        s->setEnabled (posEnabled);
    formatBox.setEnabled (enabled);
    sampleRateBox.setEnabled (enabled);
    bitDepthBox.setEnabled (enabled);
    raysSlider.setEnabled (enabled);
    bouncesSlider.setEnabled (enabled);
    irLenSlider.setEnabled (enabled);
    simulateButton.setEnabled (enabled);
    cancelButton.setEnabled (! enabled);
}

void MainComponent::startSimulation()
{
    if (simulator_.isRunning())
        return;

    stopPlayback();
    setControlsEnabled (false);
    playButton.setEnabled (false);
    exportButton.setEnabled (false);
    progressValue_ = 0.0;
    statusLabel.setText (utf8 ("シミュレーション中..."), juce::dontSendNotification);
    progressLabel.setText ("0%", juce::dontSendNotification);

    const auto settings = collectSettings();

    simulator_.start (
        settings,
        materialLibrary_,
        [this] (float p)
        {
            progressValue_ = static_cast<double> (p);
            progressLabel.setText (juce::String (static_cast<int> (p * 100.0f)) + "%",
                                   juce::dontSendNotification);
        },
        [this] (letsir::SimulationResult result)
        {
            lastResult_ = std::move (result);
            setControlsEnabled (true);

            if (lastResult_.ir.getNumSamples() <= 0)
            {
                statusLabel.setText (utf8 ("失敗: ") + lastResult_.message, juce::dontSendNotification);
                return;
            }

            {
                const juce::ScopedLock sl (irLock_);
                playBuffer_ = lastResult_.ir;
            }

            waveform_.setIRBuffer (lastResult_.ir, lastResult_.sampleRate);
            playButton.setEnabled (true);
            exportButton.setEnabled (true);

            statusLabel.setText (
                utf8 ("完了 — RT60≈") + juce::String (lastResult_.estimatedRT60, 2) + " s, IR "
                    + juce::String (lastResult_.irLengthSeconds, 2) + " s, "
                    + juce::String (lastResult_.ir.getNumChannels()) + "ch / "
                    + juce::String (lastResult_.ir.getNumSamples()) + " samples"
                    + utf8 ("（直接音除去）"),
                juce::dontSendNotification);
            progressValue_ = 1.0;
            progressLabel.setText ("100%", juce::dontSendNotification);
        });
}

void MainComponent::cancelSimulation()
{
    simulator_.cancel();
    statusLabel.setText (utf8 ("キャンセル要求中..."), juce::dontSendNotification);
}

void MainComponent::exportWav()
{
    if (lastResult_.ir.getNumSamples() <= 0)
        return;

    const auto formatTag = letsir::outputFormatDisplayName (lastResult_.format);
    auto defaultName = letsir::IRExporter::makeDefaultFileName (
        juce::String (formatTag).replaceCharacters (" /()", "____"));

    auto chooser = std::make_shared<juce::FileChooser> (
        utf8 ("IR を WAV として書き出し"),
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile (defaultName),
        "*.wav");

    constexpr auto flags = juce::FileBrowserComponent::saveMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::warnAboutOverwriting;

    const int bits = selectedBitsPerSample();

    chooser->launchAsync (flags, [this, chooser, bits] (const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file == juce::File())
            return;

        const bool ok = letsir::IRExporter::exportWav (lastResult_.ir, file,
                                                       lastResult_.sampleRate, bits);
        statusLabel.setText (ok ? (utf8 ("書き出し完了: ") + file.getFullPathName()
                                   + " (" + juce::String (bits) + " bit)")
                                : utf8 ("書き出しに失敗しました"),
                             juce::dontSendNotification);
    });
}

void MainComponent::playIR()
{
    const juce::ScopedLock sl (irLock_);
    if (playBuffer_.getNumSamples() <= 0)
        return;
    playPosition_.store (0);
}

void MainComponent::stopPlayback()
{
    playPosition_.store (-1);
}

void MainComponent::prepareToPlay (int, double sampleRate)
{
    deviceSampleRate_ = sampleRate;
}

void MainComponent::releaseResources() {}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    int pos = playPosition_.load();
    if (pos < 0)
        return;

    const juce::ScopedLock sl (irLock_);
    if (playBuffer_.getNumSamples() <= 0)
    {
        playPosition_.store (-1);
        return;
    }

    const int numOut = bufferToFill.buffer->getNumChannels();
    const int numIn = playBuffer_.getNumChannels();
    const int n = bufferToFill.numSamples;

    for (int i = 0; i < n; ++i)
    {
        if (pos >= playBuffer_.getNumSamples())
        {
            playPosition_.store (-1);
            return;
        }

        for (int ch = 0; ch < numOut; ++ch)
        {
            const int srcCh = juce::jmin (ch, numIn - 1);
            bufferToFill.buffer->setSample (ch, bufferToFill.startSample + i,
                                            playBuffer_.getSample (srcCh, pos));
        }
        ++pos;
    }

    playPosition_.store (pos);
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1f22));
    g.setColour (juce::Colour (0xffe8eaed));
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText (utf8 ("LetsIR — 空間IRジェネレータ"), getLocalBounds().removeFromTop (36).reduced (12, 4),
                juce::Justification::centredLeft);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (36);

    auto left = area.removeFromLeft (area.getWidth() / 2).reduced (0, 0);
    area.removeFromLeft (12);
    auto right = area;

    takeRow (left, 24);
    roomSectionLabel.setBounds (takeRow (left, 22));

    auto row = takeRow (left, 28);
    widthLabel.setBounds (row.removeFromLeft (48));
    widthSlider.setBounds (row);
    row = takeRow (left, 28);
    depthLabel.setBounds (row.removeFromLeft (48));
    depthSlider.setBounds (row);
    row = takeRow (left, 28);
    heightLabel.setBounds (row.removeFromLeft (48));
    heightSlider.setBounds (row);

    takeRow (left, 6);
    materialSectionLabel.setBounds (takeRow (left, 22));
    for (int i = 0; i < letsir::kNumRoomFaces; ++i)
    {
        row = takeRow (left, 26);
        faceLabels[static_cast<size_t> (i)].setBounds (row.removeFromLeft (56));
        faceMaterialBoxes[static_cast<size_t> (i)].setBounds (row);
    }

    takeRow (left, 6);
    positionSectionLabel.setBounds (takeRow (left, 22));
    useDefaultPositions.setBounds (takeRow (left, 24));

    row = takeRow (left, 26);
    sourceLabel.setBounds (row.removeFromLeft (48));
    auto third = row.getWidth() / 3;
    sourceX.setBounds (row.removeFromLeft (third));
    sourceY.setBounds (row.removeFromLeft (third));
    sourceZ.setBounds (row);

    row = takeRow (left, 26);
    listenerLabel.setBounds (row.removeFromLeft (48));
    third = row.getWidth() / 3;
    listenerX.setBounds (row.removeFromLeft (third));
    listenerY.setBounds (row.removeFromLeft (third));
    listenerZ.setBounds (row);

    takeRow (left, 6);
    outputSectionLabel.setBounds (takeRow (left, 22));
    row = takeRow (left, 28);
    formatLabel.setBounds (row.removeFromLeft (90));
    formatBox.setBounds (row);
    row = takeRow (left, 28);
    sampleRateLabel.setBounds (row.removeFromLeft (90));
    sampleRateBox.setBounds (row);
    row = takeRow (left, 28);
    bitDepthLabel.setBounds (row.removeFromLeft (90));
    bitDepthBox.setBounds (row);

    takeRow (left, 6);
    advancedSectionLabel.setBounds (takeRow (left, 22));
    row = takeRow (left, 26);
    raysLabel.setBounds (row.removeFromLeft (80));
    raysSlider.setBounds (row);
    row = takeRow (left, 26);
    bouncesLabel.setBounds (row.removeFromLeft (80));
    bouncesSlider.setBounds (row);
    row = takeRow (left, 26);
    irLenLabel.setBounds (row.removeFromLeft (80));
    irLenSlider.setBounds (row);

    auto actions = takeRow (right, 32);
    simulateButton.setBounds (actions.removeFromLeft (140));
    actions.removeFromLeft (8);
    cancelButton.setBounds (actions.removeFromLeft (100));
    actions.removeFromLeft (8);
    playButton.setBounds (actions.removeFromLeft (80));
    actions.removeFromLeft (8);
    exportButton.setBounds (actions.removeFromLeft (150));

    statusLabel.setBounds (takeRow (right, 28));
    row = takeRow (right, 22);
    progressBar.setBounds (row.removeFromLeft (row.getWidth() - 48));
    progressLabel.setBounds (row);

    auto layoutArea = takeRow (right, juce::jmax (180, right.getHeight() / 2));
    layoutView_.setBounds (layoutArea);
    takeRow (right, 8);
    waveform_.setBounds (right);
}

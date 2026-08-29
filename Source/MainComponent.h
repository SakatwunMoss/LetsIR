#pragma once

#include "Acoustics/AcousticSimulator.h"
#include "Acoustics/IRExporter.h"
#include "Acoustics/MaterialLibrary.h"
#include "Acoustics/RoomLayoutView.h"
#include "IRWaveformComponent.h"
#include "Utf8.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <array>
#include <memory>

class MainComponent final : public juce::AudioAppComponent
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void startSimulation();
    void cancelSimulation();
    void exportWav();
    void playIR();
    void stopPlayback();
    void applyDefaultsFromRoom();
    void refreshLayoutView();
    letsir::SimulationSettings collectSettings() const;
    int selectedBitsPerSample() const;
    void setControlsEnabled (bool enabled);
    void populateMaterialCombo (juce::ComboBox& box);

    letsir::MaterialLibrary materialLibrary_;
    letsir::AcousticSimulator simulator_;
    letsir::SimulationResult lastResult_;

    // ---- Room ----
    juce::Label roomSectionLabel { {}, utf8 ("部屋寸法 (m)") };
    juce::Label widthLabel { {}, utf8 ("幅 X") };
    juce::Label depthLabel { {}, utf8 ("奥行 Y") };
    juce::Label heightLabel { {}, utf8 ("高さ Z") };
    juce::Slider widthSlider, depthSlider, heightSlider;

    // ---- Materials (6 faces) ----
    juce::Label materialSectionLabel { {}, utf8 ("材質") };
    std::array<juce::Label, letsir::kNumRoomFaces> faceLabels;
    std::array<juce::ComboBox, letsir::kNumRoomFaces> faceMaterialBoxes;

    // ---- Positions ----
    juce::Label positionSectionLabel { {}, utf8 ("位置 (m)") };
    juce::ToggleButton useDefaultPositions { utf8 ("デフォルト位置を使用") };
    juce::Label sourceLabel { {}, utf8 ("音源") };
    juce::Label listenerLabel { {}, utf8 ("受音点") };
    juce::Slider sourceX, sourceY, sourceZ;
    juce::Slider listenerX, listenerY, listenerZ;

    // ---- Output ----
    juce::Label outputSectionLabel { {}, utf8 ("出力") };
    juce::Label formatLabel { {}, utf8 ("フォーマット") };
    juce::Label sampleRateLabel { {}, utf8 ("サンプルレート") };
    juce::Label bitDepthLabel { {}, utf8 ("ビット深度") };
    juce::ComboBox formatBox, sampleRateBox, bitDepthBox;

    // ---- Advanced ----
    juce::Label advancedSectionLabel { {}, utf8 ("詳細設定") };
    juce::Label raysLabel { {}, utf8 ("レイ本数") };
    juce::Label bouncesLabel { {}, utf8 ("最大反射") };
    juce::Label irLenLabel { {}, utf8 ("IR長さ上限 (秒)") };
    juce::Slider raysSlider, bouncesSlider, irLenSlider;

    // ---- Actions ----
    juce::TextButton simulateButton { utf8 ("シミュレート実行") };
    juce::TextButton cancelButton { utf8 ("キャンセル") };
    juce::TextButton playButton { utf8 ("再生") };
    juce::TextButton exportButton { utf8 ("WAVとして書き出し") };
    juce::Label statusLabel;
    juce::Label progressLabel;
    juce::ProgressBar progressBar { progressValue_ };
    double progressValue_ = 0.0;

    letsir::RoomLayoutView layoutView_;
    IRWaveformComponent waveform_;

    // Playback
    juce::CriticalSection irLock_;
    juce::AudioBuffer<float> playBuffer_;
    std::atomic<int> playPosition_ { -1 };
    double deviceSampleRate_ = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

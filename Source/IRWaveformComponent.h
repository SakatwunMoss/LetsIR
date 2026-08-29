#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

class IRWaveformComponent final : public juce::Component
{
public:
    IRWaveformComponent();
    ~IRWaveformComponent() override;

    void setIRBuffer (const juce::AudioBuffer<float>& buffer, double sampleRate);

    int getStartSample() const noexcept { return startSample_; }
    int getEndSample() const noexcept { return endSample_; }

    void paint (juce::Graphics& g) override;

    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    enum class DraggedMarker
    {
        none,
        start,
        end
    };

    static constexpr int kMarkerHitTolerancePx = 5;

    void updateViewRange();
    float computeRegionPeak (int regionStart, int regionLength) const noexcept;
    juce::Rectangle<int> getWaveformArea() const;
    int sampleToX (int sample) const;
    int xToSample (int x) const;
    DraggedMarker hitTestMarker (int x) const;
    float getDisplayGain() const noexcept;

    juce::AudioBuffer<float> irBufferCopy_;

    double sampleRate_ = 44100.0;
    int numSamples_ = 0;
    int startSample_ = 0;
    int endSample_ = 0;
    int viewStartSample_ = 0;
    int viewEndSample_ = 0;
    float displayPeak_ = 0.0f;
    DraggedMarker draggedMarker_ = DraggedMarker::none;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IRWaveformComponent)
};

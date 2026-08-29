#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class InputLevelMeterComponent final : public juce::Component
{
public:
    InputLevelMeterComponent();

    void setLevelDb (float levelDb) noexcept;

    void paint (juce::Graphics& g) override;

private:
    static constexpr float kMinDb = -60.0f;
    static constexpr float kMaxDb = 0.0f;

    float displayedLevelDb_ = kMinDb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InputLevelMeterComponent)
};

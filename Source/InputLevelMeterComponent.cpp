#include "InputLevelMeterComponent.h"

InputLevelMeterComponent::InputLevelMeterComponent() = default;

void InputLevelMeterComponent::setLevelDb (float levelDb) noexcept
{
    displayedLevelDb_ = juce::jlimit (kMinDb, kMaxDb, levelDb);
    repaint();
}

void InputLevelMeterComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced (2);

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (bounds.toFloat(), 4.0f);

    g.setColour (juce::Colours::darkgrey);
    g.drawRoundedRectangle (bounds.toFloat(), 4.0f, 1.0f);

    const float normalised = juce::jmap (displayedLevelDb_, kMinDb, kMaxDb, 0.0f, 1.0f);
    const int meterHeight = juce::jmax (1, static_cast<int> (std::round (normalised * static_cast<float> (bounds.getHeight()))));
    auto meterBounds = bounds.removeFromBottom (meterHeight);

    juce::ColourGradient gradient (juce::Colours::limegreen,
                                   static_cast<float> (meterBounds.getBottomLeft().getX()),
                                   static_cast<float> (meterBounds.getBottomLeft().getY()),
                                   juce::Colours::red,
                                   static_cast<float> (meterBounds.getTopLeft().getX()),
                                   static_cast<float> (meterBounds.getTopLeft().getY()),
                                   false);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (meterBounds.toFloat(), 3.0f);

    g.setColour (juce::Colours::white);
    g.setFont (11.0f);

    const auto levelText = displayedLevelDb_ <= kMinDb + 0.5f
                               ? juce::String ("-inf dBFS")
                               : juce::String (displayedLevelDb_, 1) + " dBFS";

    g.drawFittedText (levelText, getLocalBounds().reduced (4), juce::Justification::centredBottom, 1);
}

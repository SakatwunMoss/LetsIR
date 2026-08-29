#include "IRWaveformComponent.h"

namespace
{
    constexpr double kMaxAutoViewSeconds = 1.0;
    constexpr double kMinAutoViewSeconds = 0.25;
    constexpr float kActiveRegionThresholdDb = -40.0f;
}

IRWaveformComponent::IRWaveformComponent()
{
    setOpaque (true);
}

IRWaveformComponent::~IRWaveformComponent() = default;

float IRWaveformComponent::computeRegionPeak (int regionStart, int regionLength) const noexcept
{
    if (regionLength <= 0 || irBufferCopy_.getNumSamples() <= 0)
        return 0.0f;

    regionStart = juce::jlimit (0, irBufferCopy_.getNumSamples() - 1, regionStart);
    regionLength = juce::jmin (regionLength, irBufferCopy_.getNumSamples() - regionStart);

    return irBufferCopy_.getMagnitude (0, regionStart, regionLength);
}

void IRWaveformComponent::updateViewRange()
{
    viewStartSample_ = 0;
    viewEndSample_ = numSamples_ > 0 ? numSamples_ - 1 : 0;

    if (displayPeak_ <= 0.0f || numSamples_ <= 0)
        return;

    const auto threshold = displayPeak_ * juce::Decibels::decibelsToGain (kActiveRegionThresholdDb);
    startSample_ = 0;
    endSample_ = numSamples_ - 1;

    for (int sample = 0; sample < numSamples_; ++sample)
    {
        if (std::abs (irBufferCopy_.getSample (0, sample)) >= threshold)
        {
            startSample_ = sample;
            break;
        }
    }

    for (int sample = numSamples_ - 1; sample >= startSample_; --sample)
    {
        if (std::abs (irBufferCopy_.getSample (0, sample)) >= threshold)
        {
            endSample_ = sample;
            break;
        }
    }

    const auto paddingSamples = juce::jmax (64, static_cast<int> (std::round (sampleRate_ * 0.02)));
    const auto maxViewSamples = juce::jmax (1, static_cast<int> (std::round (sampleRate_ * kMaxAutoViewSeconds)));
    const auto minViewSamples = juce::jmax (1, static_cast<int> (std::round (sampleRate_ * kMinAutoViewSeconds)));

    viewStartSample_ = juce::jmax (0, startSample_ - paddingSamples);
    viewEndSample_ = juce::jmin (numSamples_ - 1,
                                 endSample_ + paddingSamples,
                                 maxViewSamples - 1);

    if (viewEndSample_ - viewStartSample_ + 1 < minViewSamples)
        viewEndSample_ = juce::jmin (numSamples_ - 1, viewStartSample_ + minViewSamples - 1);

    const auto viewLength = viewEndSample_ - viewStartSample_ + 1;
    displayPeak_ = computeRegionPeak (viewStartSample_, viewLength);
}

void IRWaveformComponent::setIRBuffer (const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    irBufferCopy_.makeCopyOf (buffer);
    sampleRate_ = sampleRate > 0.0 ? sampleRate : 44100.0;
    numSamples_ = irBufferCopy_.getNumSamples();
    displayPeak_ = computeRegionPeak (0, numSamples_);
    draggedMarker_ = DraggedMarker::none;

    updateViewRange();

    setVisible (numSamples_ > 0);
    repaint();
}

float IRWaveformComponent::getDisplayGain() const noexcept
{
    if (displayPeak_ <= 0.0f)
        return 1.0f;

    return 0.95f / displayPeak_;
}

juce::Rectangle<int> IRWaveformComponent::getWaveformArea() const
{
    return getLocalBounds().reduced (2).withTrimmedBottom (16);
}

int IRWaveformComponent::sampleToX (int sample) const
{
    const auto area = getWaveformArea();
    const auto viewLength = juce::jmax (1, viewEndSample_ - viewStartSample_);

    if (area.getWidth() <= 0)
        return area.getX();

    const auto proportion = static_cast<float> (sample - viewStartSample_) / static_cast<float> (viewLength);
    return area.getX() + juce::roundToInt (proportion * static_cast<float> (area.getWidth()));
}

int IRWaveformComponent::xToSample (int x) const
{
    const auto area = getWaveformArea();
    const auto viewLength = juce::jmax (1, viewEndSample_ - viewStartSample_);

    if (area.getWidth() <= 0)
        return viewStartSample_;

    const auto proportion = static_cast<float> (x - area.getX()) / static_cast<float> (area.getWidth());
    const auto sample = viewStartSample_ + juce::roundToInt (proportion * static_cast<float> (viewLength));
    return juce::jlimit (0, numSamples_ - 1, sample);
}

IRWaveformComponent::DraggedMarker IRWaveformComponent::hitTestMarker (int x) const
{
    if (numSamples_ <= 0)
        return DraggedMarker::none;

    const auto startDistance = std::abs (x - sampleToX (startSample_));
    const auto endDistance = std::abs (x - sampleToX (endSample_));

    if (startDistance <= kMarkerHitTolerancePx && startDistance <= endDistance)
        return DraggedMarker::start;

    if (endDistance <= kMarkerHitTolerancePx)
        return DraggedMarker::end;

    return DraggedMarker::none;
}

void IRWaveformComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().reduced (2);
    const auto area = getWaveformArea();

    g.setColour (juce::Colour (0xff1e1e1e));
    g.fillRoundedRectangle (bounds.toFloat(), 4.0f);

    g.setColour (juce::Colours::darkgrey);
    g.drawRoundedRectangle (bounds.toFloat(), 4.0f, 1.0f);

    if (numSamples_ <= 0)
    {
        g.setColour (juce::Colours::grey);
        g.drawFittedText ("No IR", bounds, juce::Justification::centred, 1);
        return;
    }

    if (displayPeak_ <= 0.0f)
    {
        g.setColour (juce::Colours::grey);
        g.drawFittedText ("Silent IR\nTurn ON \"Enable Input\" and record again",
                          bounds,
                          juce::Justification::centred,
                          2);
        return;
    }

    const auto displayGain = getDisplayGain();
    const auto numChannels = irBufferCopy_.getNumChannels();
    const auto channelHeight = static_cast<float> (area.getHeight()) / static_cast<float> (numChannels);
    const auto areaWidth = juce::jmax (1, area.getWidth());
    const auto viewLength = juce::jmax (1, viewEndSample_ - viewStartSample_ + 1);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto* data = irBufferCopy_.getReadPointer (channel);
        const auto channelCentreY = static_cast<float> (area.getY())
                                  + channelHeight * (static_cast<float> (channel) + 0.5f);
        const auto amplitudeScale = channelHeight * 0.48f;

        for (int x = 0; x < area.getWidth(); ++x)
        {
            const auto bucketStart = viewStartSample_ + (x * viewLength) / areaWidth;
            const auto bucketEnd = viewStartSample_ + ((x + 1) * viewLength) / areaWidth;
            const auto startSample = juce::jlimit (viewStartSample_, viewEndSample_, bucketStart);
            const auto endSample = juce::jlimit (viewStartSample_, viewEndSample_ + 1, bucketEnd);

            if (startSample >= endSample)
                continue;

            float minSample = data[startSample];
            float maxSample = data[startSample];

            for (int sample = startSample + 1; sample < endSample; ++sample)
            {
                minSample = juce::jmin (minSample, data[sample]);
                maxSample = juce::jmax (maxSample, data[sample]);
            }

            minSample *= displayGain;
            maxSample *= displayGain;

            const auto xPos = static_cast<float> (area.getX() + x);
            const auto topY = channelCentreY - maxSample * amplitudeScale;
            const auto bottomY = channelCentreY - minSample * amplitudeScale;
            const auto barHeight = juce::jmax (1.0f, bottomY - topY);

            g.setColour (juce::Colour (0xff3eb5ff).withAlpha (0.95f));
            g.fillRect (xPos, topY, 2.0f, barHeight);
        }

        g.setColour (juce::Colours::darkgrey.withAlpha (0.8f));
        g.drawHorizontalLine (juce::roundToInt (channelCentreY), static_cast<float> (area.getX()), static_cast<float> (area.getRight()));
    }

    const auto startX = static_cast<float> (sampleToX (startSample_));
    const auto endX = static_cast<float> (sampleToX (endSample_));
    const auto topY = static_cast<float> (area.getY());
    const auto bottomY = static_cast<float> (area.getBottom());

    g.setColour (juce::Colours::limegreen);
    g.drawLine (startX, topY, startX, bottomY, 2.0f);

    g.setColour (juce::Colours::red);
    g.drawLine (endX, topY, endX, bottomY, 2.0f);

    const auto viewStartSeconds = static_cast<double> (viewStartSample_) / sampleRate_;
    const auto viewEndSeconds = static_cast<double> (viewEndSample_ + 1) / sampleRate_;
    const auto totalSeconds = static_cast<double> (numSamples_) / sampleRate_;
    const auto infoText = "View: " + juce::String (viewStartSeconds, 2) + "s - "
                        + juce::String (viewEndSeconds, 2) + "s  (Total "
                        + juce::String (totalSeconds, 2) + "s)";

    g.setColour (juce::Colours::silver);
    g.setFont (10.5f);
    auto infoBounds = bounds;
    g.drawFittedText (infoText, infoBounds.removeFromBottom (14), juce::Justification::centred, 1);
}

void IRWaveformComponent::mouseDown (const juce::MouseEvent& event)
{
    draggedMarker_ = hitTestMarker (event.x);
}

void IRWaveformComponent::mouseDrag (const juce::MouseEvent& event)
{
    if (draggedMarker_ == DraggedMarker::none || numSamples_ <= 0)
        return;

    const auto newSample = xToSample (event.x);

    if (draggedMarker_ == DraggedMarker::start)
    {
        const auto maxStart = numSamples_ > 1 ? endSample_ - 1 : 0;
        startSample_ = juce::jlimit (0, maxStart, newSample);
    }
    else if (draggedMarker_ == DraggedMarker::end)
    {
        const auto minEnd = numSamples_ > 1 ? startSample_ + 1 : 0;
        endSample_ = juce::jlimit (minEnd, numSamples_ - 1, newSample);
    }

    updateViewRange();
    repaint();
}

void IRWaveformComponent::mouseUp (const juce::MouseEvent&)
{
    draggedMarker_ = DraggedMarker::none;
}

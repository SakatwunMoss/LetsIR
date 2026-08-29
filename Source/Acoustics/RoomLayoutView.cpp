#include "RoomLayoutView.h"
#include "../Utf8.h"

namespace letsir
{

namespace
{
juce::Colour paletteColour (size_t index) noexcept
{
    static constexpr uint32_t kPalette[] = {
        0xff6b8cae,
        0xffc4b59a,
        0xffb8956c,
        0xff8a6d5b,
        0xff7ec8e3,
        0xff9b7bb8,
        0xffc07058,
        0xff5a9e7a,
        0xffd4cfc4,
        0xffa8a8a8,
    };
    return juce::Colour (kPalette[index % (sizeof (kPalette) / sizeof (kPalette[0]))]);
}
} // namespace

RoomLayoutView::RoomLayoutView()
{
    setOpaque (true);
}

void RoomLayoutView::setMaterialLibrary (const MaterialLibrary* library) noexcept
{
    library_ = library;
}

void RoomLayoutView::setRoomState (const RoomDimensions& dims,
                                   const std::array<std::string, kNumRoomFaces>& materialIds,
                                   Vec3 sourcePosition,
                                   Vec3 listenerPosition)
{
    dims_ = dims;
    materialIds_ = materialIds;
    source_ = sourcePosition;
    listener_ = listenerPosition;
    repaint();
}

juce::Colour RoomLayoutView::colourForMaterialId (const std::string& id) const
{
    if (library_ != nullptr)
        if (auto idx = library_->indexOfId (id))
            return paletteColour (*idx);

    return juce::Colour (0xff888888);
}

juce::Rectangle<float> RoomLayoutView::computePlanRect (juce::Rectangle<float> area) const
{
    const float w = std::max (0.1f, dims_.width);
    const float d = std::max (0.1f, dims_.depth);
    const float aspect = w / d;

    auto inner = area.reduced (18.0f, 6.0f);
    inner.removeFromBottom (44.0f);

    float planW = inner.getWidth();
    float planH = planW / aspect;
    if (planH > inner.getHeight())
    {
        planH = inner.getHeight();
        planW = planH * aspect;
    }

    return { inner.getCentreX() - planW * 0.5f,
             inner.getY() + (inner.getHeight() - planH) * 0.5f,
             planW, planH };
}

void RoomLayoutView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff25262b));
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (juce::Colour (0xffe8eaed));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (utf8 ("部屋平面図（真上）"),
                bounds.withHeight (22.0f).reduced (10.0f, 2.0f),
                juce::Justification::centredLeft);

    auto drawArea = getLocalBounds().toFloat().withTrimmedTop (22.0f);
    const auto plan = computePlanRect (drawArea);

    g.setColour (colourForMaterialId (materialIds_[static_cast<size_t> (RoomFace::floor)])
                     .withAlpha (0.22f));
    g.fillRect (plan);

    constexpr float kWallThickness = 5.0f;
    const auto frontC = colourForMaterialId (materialIds_[static_cast<size_t> (RoomFace::front)]);
    const auto backC  = colourForMaterialId (materialIds_[static_cast<size_t> (RoomFace::back)]);
    const auto leftC  = colourForMaterialId (materialIds_[static_cast<size_t> (RoomFace::left)]);
    const auto rightC = colourForMaterialId (materialIds_[static_cast<size_t> (RoomFace::right)]);

    // Screen Y-up = room +Y (front at top of plan)
    g.setColour (frontC);
    g.fillRect (plan.getX(), plan.getY(), plan.getWidth(), kWallThickness);
    g.setColour (backC);
    g.fillRect (plan.getX(), plan.getBottom() - kWallThickness, plan.getWidth(), kWallThickness);
    g.setColour (leftC);
    g.fillRect (plan.getX(), plan.getY(), kWallThickness, plan.getHeight());
    g.setColour (rightC);
    g.fillRect (plan.getRight() - kWallThickness, plan.getY(), kWallThickness, plan.getHeight());

    g.setColour (juce::Colour (0xff3a3d45));
    g.drawRect (plan, 1.0f);

    g.setColour (juce::Colour (0xff9aa0a6));
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (juce::String (dims_.width, 1) + " m",
                juce::Rectangle<float> (plan.getX(), plan.getBottom() + 2.0f, plan.getWidth(), 14.0f),
                juce::Justification::centred);

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (
            -juce::MathConstants<float>::halfPi,
            plan.getX() - 12.0f,
            plan.getCentreY()));
        g.drawText (juce::String (dims_.depth, 1) + " m",
                    juce::Rectangle<float> (plan.getX() - 42.0f, plan.getCentreY() - 7.0f, 60.0f, 14.0f),
                    juce::Justification::centred);
    }

    auto toPlan = [&] (float xMeters, float yMeters) -> juce::Point<float>
    {
        const float nx = xMeters / std::max (0.1f, dims_.width);
        const float ny = yMeters / std::max (0.1f, dims_.depth);
        return { plan.getX() + nx * plan.getWidth(),
                 plan.getBottom() - ny * plan.getHeight() };
    };

    {
        const auto p = toPlan (source_.x, source_.y);
        g.setColour (juce::Colour (0xffffb74d));
        g.fillEllipse (p.x - 6.0f, p.y - 6.0f, 12.0f, 12.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (p.x - 6.0f, p.y - 6.0f, 12.0f, 12.0f, 1.0f);
        g.setColour (juce::Colour (0xffffcc80));
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (utf8 ("音源"), juce::Rectangle<float> (p.x + 8.0f, p.y - 8.0f, 36.0f, 14.0f),
                    juce::Justification::centredLeft);
    }

    {
        const auto p = toPlan (listener_.x, listener_.y);
        g.setColour (juce::Colour (0xff64b5f6));
        juce::Path mic;
        mic.addTriangle (p.x, p.y - 7.0f, p.x - 6.0f, p.y + 5.0f, p.x + 6.0f, p.y + 5.0f);
        g.fillPath (mic);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.strokePath (mic, juce::PathStrokeType (1.0f));
        g.setColour (juce::Colour (0xff90caf9));
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (utf8 ("マイク"), juce::Rectangle<float> (p.x + 8.0f, p.y - 8.0f, 40.0f, 14.0f),
                    juce::Justification::centredLeft);
    }

    auto materialName = [&] (RoomFace face) -> juce::String
    {
        const auto& id = materialIds_[static_cast<size_t> (face)];
        if (library_ != nullptr)
            if (const auto* m = library_->findById (id))
                return utf8 (m->displayName);
        return utf8 (id);
    };

    auto legend = getLocalBounds().removeFromBottom (42).reduced (10, 2);
    g.setFont (juce::FontOptions (10.5f));

    auto drawSwatch = [&] (RoomFace face, juce::Colour c, juce::Rectangle<int> cell)
    {
        g.setColour (c);
        g.fillRoundedRectangle (cell.removeFromLeft (12).withSizeKeepingCentre (10, 10).toFloat(), 2.0f);
        cell.removeFromLeft (4);
        g.setColour (juce::Colour (0xffc0c4cc));
        g.drawText (utf8 (roomFaceDisplayName (face)) + ": " + materialName (face),
                    cell, juce::Justification::centredLeft, true);
    };

    auto row1 = legend.removeFromTop (18);
    const int cellW = row1.getWidth() / 4;
    drawSwatch (RoomFace::front, frontC, row1.removeFromLeft (cellW));
    drawSwatch (RoomFace::back,  backC,  row1.removeFromLeft (cellW));
    drawSwatch (RoomFace::left,  leftC,  row1.removeFromLeft (cellW));
    drawSwatch (RoomFace::right, rightC, row1);

    g.setColour (juce::Colour (0xff9aa0a6));
    g.drawText (utf8 ("床: ") + materialName (RoomFace::floor)
                    + utf8 ("  /  天井: ") + materialName (RoomFace::ceiling)
                    + utf8 ("  /  高さ ") + juce::String (dims_.height, 1) + " m",
                legend, juce::Justification::centredLeft, true);
}

} // namespace letsir

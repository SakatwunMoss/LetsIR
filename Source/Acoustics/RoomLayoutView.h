#pragma once

#include "MaterialLibrary.h"
#include "RoomModel.h"
#include "Vec3.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <string>

namespace letsir
{

/**
 * Top-down 2D floor-plan visualisation of the current room parameters.
 * Display-only — no interactive geometry editing in phase 1.
 */
class RoomLayoutView final : public juce::Component
{
public:
    RoomLayoutView();

    void setMaterialLibrary (const MaterialLibrary* library) noexcept;

    void setRoomState (const RoomDimensions& dims,
                       const std::array<std::string, kNumRoomFaces>& materialIds,
                       Vec3 sourcePosition,
                       Vec3 listenerPosition);

    void paint (juce::Graphics& g) override;

private:
    juce::Colour colourForMaterialId (const std::string& id) const;
    juce::Rectangle<float> computePlanRect (juce::Rectangle<float> area) const;

    const MaterialLibrary* library_ = nullptr;
    RoomDimensions dims_;
    std::array<std::string, kNumRoomFaces> materialIds_ {
        "gypsum", "gypsum", "gypsum", "gypsum", "wood_floor", "gypsum"
    };
    Vec3 source_ { 2.5f, 2.1f, 1.26f };
    Vec3 listener_ { 2.5f, 4.2f, 1.26f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RoomLayoutView)
};

} // namespace letsir

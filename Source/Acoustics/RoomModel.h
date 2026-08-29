#pragma once

#include "MaterialLibrary.h"
#include "WallPolygon.h"

#include <array>
#include <string>
#include <vector>

namespace letsir
{

struct RoomDimensions
{
    float width = 5.0f;   // X (m)
    float depth = 6.0f;   // Y (m)
    float height = 2.8f;  // Z (m)
};

/**
 * Room as a list of wall polygons + materials.
 * Box rooms are built as 6 rectangles; future shapes only need a different builder.
 */
class RoomModel
{
public:
    RoomModel() = default;

    /** Build an axis-aligned box with origin at a corner (0,0,0) and extents along +X/+Y/+Z. */
    void buildBox (const RoomDimensions& dims,
                   const MaterialLibrary& library,
                   const std::array<std::string, kNumRoomFaces>& materialIds);

    const std::vector<WallPolygon>& getPolygons() const noexcept { return polygons_; }
    const RoomDimensions& getDimensions() const noexcept { return dimensions_; }

    Vec3 getDefaultSourcePosition() const noexcept;
    Vec3 getDefaultListenerPosition() const noexcept;

    /** Closest polygon intersection. Returns index into polygons_ or -1. */
    int findClosestHit (const Vec3& origin, const Vec3& direction,
                        float& outDistance,
                        float minDist = 1.0e-4f) const;

    /** Sabine RT60 estimate from average absorption at 500 Hz / 1 kHz. */
    float estimateRT60() const;

    bool containsPoint (const Vec3& p, float margin = 0.01f) const noexcept;

private:
    static WallPolygon makeQuad (const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
                                 const Material& material, RoomFace face);

    RoomDimensions dimensions_;
    std::vector<WallPolygon> polygons_;
};

} // namespace letsir

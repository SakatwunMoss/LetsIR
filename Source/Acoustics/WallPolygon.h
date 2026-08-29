#pragma once

#include "Material.h"
#include "Vec3.h"

#include <array>
#include <cmath>
#include <optional>
#include <string>

namespace letsir
{

enum class RoomFace
{
    front = 0, // +Y (far)
    back,      // -Y (near)
    left,      // -X
    right,     // +X
    floor,     // -Z
    ceiling,   // +Z
    count
};

inline constexpr int kNumRoomFaces = static_cast<int> (RoomFace::count);

inline const char* roomFaceDisplayName (RoomFace face)
{
    switch (face)
    {
        case RoomFace::front:   return "前壁";
        case RoomFace::back:    return "後壁";
        case RoomFace::left:    return "左壁";
        case RoomFace::right:   return "右壁";
        case RoomFace::floor:   return "床";
        case RoomFace::ceiling: return "天井";
        case RoomFace::count:   return "?";
    }
    return "?";
}

/** A planar rectangular wall polygon with an associated material. */
struct WallPolygon
{
    std::array<Vec3, 4> vertices {}; // CCW when viewed from inside the room
    Vec3 normal {};                  // inward-facing normal (points into the room)
    Material material {};
    RoomFace face = RoomFace::front;

    /** Ray-triangle style hit against the rectangle. Returns distance along ray if hit. */
    std::optional<float> intersect (const Vec3& origin, const Vec3& direction,
                                    float minDist = 1.0e-4f,
                                    float maxDist = 1.0e6f) const noexcept
    {
        const float denom = direction.dot (normal);
        if (std::abs (denom) < 1.0e-8f)
            return std::nullopt; // parallel

        const float t = (vertices[0] - origin).dot (normal) / denom;
        if (t < minDist || t > maxDist)
            return std::nullopt;

        const Vec3 hit = origin + direction * t;

        // Project hit into the polygon's tangent plane and test against the quad via
        // barycentric tests on two triangles (0-1-2) and (0-2-3).
        auto pointInTriangle = [] (const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c) noexcept
        {
            const Vec3 v0 = c - a;
            const Vec3 v1 = b - a;
            const Vec3 v2 = p - a;
            const float dot00 = v0.dot (v0);
            const float dot01 = v0.dot (v1);
            const float dot02 = v0.dot (v2);
            const float dot11 = v1.dot (v1);
            const float dot12 = v1.dot (v2);
            const float inv = 1.0f / (dot00 * dot11 - dot01 * dot01);
            const float u = (dot11 * dot02 - dot01 * dot12) * inv;
            const float v = (dot00 * dot12 - dot01 * dot02) * inv;
            return (u >= -1.0e-4f) && (v >= -1.0e-4f) && (u + v <= 1.0f + 1.0e-4f);
        };

        if (pointInTriangle (hit, vertices[0], vertices[1], vertices[2])
            || pointInTriangle (hit, vertices[0], vertices[2], vertices[3]))
            return t;

        return std::nullopt;
    }
};

} // namespace letsir

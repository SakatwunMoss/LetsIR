#include "RoomModel.h"

#include <algorithm>
#include <cmath>

namespace letsir
{

WallPolygon RoomModel::makeQuad (const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
                                 const Material& material, RoomFace face)
{
    WallPolygon poly;
    poly.vertices = { a, b, c, d };
    poly.material = material;
    poly.face = face;

    const Vec3 e1 = b - a;
    const Vec3 e2 = d - a;
    poly.normal = e1.cross (e2).normalised();
    return poly;
}

void RoomModel::buildBox (const RoomDimensions& dims,
                          const MaterialLibrary& library,
                          const std::array<std::string, kNumRoomFaces>& materialIds)
{
    dimensions_ = dims;
    polygons_.clear();
    polygons_.reserve (6);

    const float w = dims.width;
    const float d = dims.depth;
    const float h = dims.height;

    auto mat = [&] (RoomFace face) -> Material
    {
        const auto& id = materialIds[static_cast<size_t> (face)];
        if (const auto* found = library.findById (id))
            return *found;
        return library.getDefault();
    };

    // Vertices of the box: origin at back-left-floor corner.
    // +X = right, +Y = forward (front wall), +Z = up
    const Vec3 p000 { 0, 0, 0 };
    const Vec3 p100 { w, 0, 0 };
    const Vec3 p010 { 0, d, 0 };
    const Vec3 p110 { w, d, 0 };
    const Vec3 p001 { 0, 0, h };
    const Vec3 p101 { w, 0, h };
    const Vec3 p011 { 0, d, h };
    const Vec3 p111 { w, d, h };

    // Quads ordered CCW when viewed from inside → inward normals.
    // Front (+Y): looking toward -Y from inside near front wall
    polygons_.push_back (makeQuad (p010, p110, p111, p011, mat (RoomFace::front), RoomFace::front));
    // Back (-Y)
    polygons_.push_back (makeQuad (p100, p000, p001, p101, mat (RoomFace::back), RoomFace::back));
    // Left (-X)
    polygons_.push_back (makeQuad (p000, p010, p011, p001, mat (RoomFace::left), RoomFace::left));
    // Right (+X)
    polygons_.push_back (makeQuad (p110, p100, p101, p111, mat (RoomFace::right), RoomFace::right));
    // Floor (-Z)
    polygons_.push_back (makeQuad (p000, p100, p110, p010, mat (RoomFace::floor), RoomFace::floor));
    // Ceiling (+Z)
    polygons_.push_back (makeQuad (p011, p111, p101, p001, mat (RoomFace::ceiling), RoomFace::ceiling));

    // Ensure normals point inward (toward room centre).
    const Vec3 centre { w * 0.5f, d * 0.5f, h * 0.5f };
    for (auto& poly : polygons_)
    {
        const Vec3 faceCentre = (poly.vertices[0] + poly.vertices[1]
                                 + poly.vertices[2] + poly.vertices[3]) * 0.25f;
        if (poly.normal.dot (centre - faceCentre) < 0.0f)
            poly.normal *= -1.0f;
    }
}

Vec3 RoomModel::getDefaultSourcePosition() const noexcept
{
    return { dimensions_.width * 0.5f,
             dimensions_.depth * 0.35f,
             dimensions_.height * 0.45f };
}

Vec3 RoomModel::getDefaultListenerPosition() const noexcept
{
    return { dimensions_.width * 0.5f,
             dimensions_.depth * 0.70f,
             dimensions_.height * 0.45f };
}

int RoomModel::findClosestHit (const Vec3& origin, const Vec3& direction,
                               float& outDistance, float minDist) const
{
    int bestIndex = -1;
    float bestT = 1.0e6f;

    for (int i = 0; i < static_cast<int> (polygons_.size()); ++i)
    {
        if (auto t = polygons_[static_cast<size_t> (i)].intersect (origin, direction, minDist, bestT))
        {
            bestT = *t;
            bestIndex = i;
        }
    }

    outDistance = bestT;
    return bestIndex;
}

float RoomModel::estimateRT60() const
{
    if (polygons_.empty())
        return 1.0f;

    const float volume = dimensions_.width * dimensions_.depth * dimensions_.height;
    float totalAbsorption = 0.0f; // Sabins at mid bands

    for (const auto& poly : polygons_)
    {
        const Vec3 e1 = poly.vertices[1] - poly.vertices[0];
        const Vec3 e2 = poly.vertices[3] - poly.vertices[0];
        const float area = e1.cross (e2).length();
        // Average of 500 Hz and 1 kHz bands
        const float alpha = 0.5f * (poly.material.absorption[2] + poly.material.absorption[3]);
        totalAbsorption += area * std::max (0.01f, alpha);
    }

    if (totalAbsorption < 1.0e-3f)
        return 5.0f;

    // Sabine: RT60 = 0.161 * V / A
    const float rt60 = 0.161f * volume / totalAbsorption;
    return std::clamp (rt60, 0.15f, 8.0f);
}

bool RoomModel::containsPoint (const Vec3& p, float margin) const noexcept
{
    return p.x >= margin && p.x <= dimensions_.width - margin
        && p.y >= margin && p.y <= dimensions_.depth - margin
        && p.z >= margin && p.z <= dimensions_.height - margin;
}

} // namespace letsir

#pragma once

#include <algorithm>
#include <cmath>
#include <random>

namespace letsir
{

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3 (float xIn, float yIn, float zIn) noexcept : x (xIn), y (yIn), z (zIn) {}

    Vec3 operator+ (const Vec3& o) const noexcept { return { x + o.x, y + o.y, z + o.z }; }
    Vec3 operator- (const Vec3& o) const noexcept { return { x - o.x, y - o.y, z - o.z }; }
    Vec3 operator* (float s) const noexcept { return { x * s, y * s, z * s }; }
    Vec3 operator/ (float s) const noexcept { return { x / s, y / s, z / s }; }
    Vec3& operator+= (const Vec3& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-= (const Vec3& o) noexcept { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*= (float s) noexcept { x *= s; y *= s; z *= s; return *this; }

    float lengthSquared() const noexcept { return x * x + y * y + z * z; }
    float length() const noexcept { return std::sqrt (lengthSquared()); }

    Vec3 normalised() const noexcept
    {
        const float len = length();
        if (len < 1.0e-12f)
            return { 0.0f, 0.0f, 0.0f };
        return *this / len;
    }

    float dot (const Vec3& o) const noexcept { return x * o.x + y * o.y + z * o.z; }

    Vec3 cross (const Vec3& o) const noexcept
    {
        return { y * o.z - z * o.y,
                 z * o.x - x * o.z,
                 x * o.y - y * o.x };
    }

    Vec3 reflected (const Vec3& normal) const noexcept
    {
        return *this - normal * (2.0f * dot (normal));
    }

    /** Azimuth (rad, -pi..pi) and elevation (rad, -pi/2..pi/2).
        Frame: +X = right, +Y = forward, +Z = up. Azimuth 0 = forward. */
    void toSpherical (float& azimuth, float& elevation) const noexcept
    {
        const float len = length();
        if (len < 1.0e-12f)
        {
            azimuth = 0.0f;
            elevation = 0.0f;
            return;
        }

        const float zClamped = std::max (-1.0f, std::min (1.0f, z / len));
        elevation = std::asin (zClamped);
        azimuth = std::atan2 (x, y);
    }
};

inline Vec3 operator* (float s, const Vec3& v) noexcept { return v * s; }

inline Vec3 randomUnitVector (std::mt19937& rng)
{
    constexpr float kPi = 3.14159265358979323846f;
    std::uniform_real_distribution<float> dist (0.0f, 1.0f);
    const float z = 2.0f * dist (rng) - 1.0f;
    const float t = 2.0f * kPi * dist (rng);
    const float r = std::sqrt (std::max (0.0f, 1.0f - z * z));
    return { r * std::cos (t), r * std::sin (t), z };
}

inline Vec3 randomCosineHemisphere (const Vec3& normal, std::mt19937& rng)
{
    constexpr float kPi = 3.14159265358979323846f;
    const Vec3 up = (std::abs (normal.z) < 0.999f) ? Vec3 { 0.0f, 0.0f, 1.0f }
                                                   : Vec3 { 1.0f, 0.0f, 0.0f };
    const Vec3 tangent = normal.cross (up).normalised();
    const Vec3 bitangent = normal.cross (tangent);

    std::uniform_real_distribution<float> dist (0.0f, 1.0f);
    const float u1 = dist (rng);
    const float u2 = dist (rng);
    const float r = std::sqrt (u1);
    const float phi = 2.0f * kPi * u2;
    const float x = r * std::cos (phi);
    const float y = r * std::sin (phi);
    const float z = std::sqrt (std::max (0.0f, 1.0f - u1));

    return (tangent * x + bitangent * y + normal * z).normalised();
}

} // namespace letsir

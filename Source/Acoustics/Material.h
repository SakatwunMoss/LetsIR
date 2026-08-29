#pragma once

#include <array>
#include <string>

namespace letsir
{

inline constexpr int kNumOctaveBands = 6;
inline constexpr std::array<float, kNumOctaveBands> kOctaveBandCentreHz {
    125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f
};

/** Absorption coefficients per octave band (0..1) plus a broadband scattering coefficient. */
struct Material
{
    std::string id;
    std::string displayName;
    std::array<float, kNumOctaveBands> absorption {}; // 125..4k Hz
    float scattering = 0.1f; // 0 = pure specular, 1 = pure diffuse
};

} // namespace letsir

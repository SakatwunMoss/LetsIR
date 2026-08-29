#include "MaterialLibrary.h"

namespace letsir
{

MaterialLibrary::MaterialLibrary()
{
    // Representative octave-band absorption (125 / 250 / 500 / 1k / 2k / 4k Hz)
    // and scattering coefficients for common architectural finishes.
    materials_ = {
        { "concrete", "コンクリート",
          { 0.01f, 0.01f, 0.02f, 0.02f, 0.02f, 0.03f }, 0.05f },
        { "gypsum", "石膏ボード",
          { 0.08f, 0.10f, 0.08f, 0.06f, 0.06f, 0.07f }, 0.15f },
        { "wood_floor", "木材（フローリング）",
          { 0.15f, 0.11f, 0.10f, 0.07f, 0.06f, 0.07f }, 0.20f },
        { "carpet", "カーペット",
          { 0.08f, 0.24f, 0.57f, 0.69f, 0.71f, 0.73f }, 0.55f },
        { "glass", "ガラス",
          { 0.15f, 0.06f, 0.04f, 0.03f, 0.02f, 0.02f }, 0.05f },
        { "curtain", "カーテン",
          { 0.15f, 0.35f, 0.55f, 0.70f, 0.70f, 0.65f }, 0.70f },
        { "brick", "レンガ",
          { 0.03f, 0.03f, 0.03f, 0.04f, 0.05f, 0.07f }, 0.25f },
        { "acoustic_panel", "吸音パネル",
          { 0.25f, 0.55f, 0.85f, 0.90f, 0.90f, 0.85f }, 0.60f },
        { "plaster", "漆喰",
          { 0.02f, 0.02f, 0.03f, 0.04f, 0.05f, 0.05f }, 0.10f },
        { "tile", "タイル",
          { 0.01f, 0.01f, 0.01f, 0.02f, 0.02f, 0.02f }, 0.05f },
    };
}

const Material* MaterialLibrary::findById (const std::string& id) const
{
    for (const auto& m : materials_)
        if (m.id == id)
            return &m;
    return nullptr;
}

const Material& MaterialLibrary::getDefault() const
{
    return materials_.front();
}

std::vector<std::string> MaterialLibrary::getDisplayNames() const
{
    std::vector<std::string> names;
    names.reserve (materials_.size());
    for (const auto& m : materials_)
        names.push_back (m.displayName);
    return names;
}

std::optional<size_t> MaterialLibrary::indexOfId (const std::string& id) const
{
    for (size_t i = 0; i < materials_.size(); ++i)
        if (materials_[i].id == id)
            return i;
    return std::nullopt;
}

} // namespace letsir

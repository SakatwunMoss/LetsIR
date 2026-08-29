#pragma once

#include "Material.h"

#include <optional>
#include <string>
#include <vector>

namespace letsir
{

/** Data-driven material preset table. Easy to extend or later load from JSON. */
class MaterialLibrary
{
public:
    MaterialLibrary();

    const std::vector<Material>& getMaterials() const noexcept { return materials_; }

    const Material* findById (const std::string& id) const;
    const Material& getDefault() const;

    std::vector<std::string> getDisplayNames() const;
    std::optional<size_t> indexOfId (const std::string& id) const;

private:
    std::vector<Material> materials_;
};

} // namespace letsir

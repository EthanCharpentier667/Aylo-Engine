/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MaterialAssets
*/

#include "plugins/shader/MaterialAssets.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/shader/Material.hpp"

namespace aylo::plugins::shader {

std::size_t MaterialAssets::getCount() const noexcept { return _materials.getCount(); }

core::asset::AssetId MaterialAssets::add(const std::string& name, Material material) {
    const auto stored = _materials.add(name, std::move(material));
    if (stored.added != nullptr) {
        markChanged(stored.id);
    }
    return stored.id;
}

core::asset::AssetId MaterialAssets::define(const std::string& name, Material material) {
    // Nothing when the name is known already: the material is replaced. A hash collision throws.
    const core::asset::AssetId id = _materials.reserve(name).value_or(core::asset::AssetId::fromPath(name));
    static_cast<void>(_materials.store(id, name, std::move(material)));
    markChanged(id);
    return id;
}

bool MaterialAssets::setShader(core::asset::AssetId id, core::asset::AssetId shader) {
    const Entry* entry = _materials.getEntry(id);
    if (entry == nullptr || !entry->value) {
        return false;
    }
    Material material = *entry->value;
    material.shader = shader;
    static_cast<void>(_materials.store(id, entry->path, std::move(material)));
    markChanged(id);
    return true;
}

const Material* MaterialAssets::get(core::asset::AssetId id) const { return _materials.get(id); }

const MaterialAssets::Entry* MaterialAssets::getEntry(core::asset::AssetId id) const { return _materials.getEntry(id); }

bool MaterialAssets::setValue(core::asset::AssetId id, const std::string& name, MaterialValue value) {
    const Entry* entry = _materials.getEntry(id);
    if (entry == nullptr || !entry->value) {
        return false;
    }
    Material material = *entry->value;
    material.set(name, value);
    static_cast<void>(_materials.store(id, entry->path, std::move(material)));
    markChanged(id);
    return true;
}

bool MaterialAssets::setTexture(core::asset::AssetId id, const std::string& name, core::asset::AssetId texture) {
    const Entry* entry = _materials.getEntry(id);
    if (entry == nullptr || !entry->value) {
        return false;
    }
    Material material = *entry->value;
    material.setTexture(name, texture);
    static_cast<void>(_materials.store(id, entry->path, std::move(material)));
    markChanged(id);
    return true;
}

bool MaterialAssets::setRenderState(core::asset::AssetId id, const RenderStateOverride& renderState) {
    const Entry* entry = _materials.getEntry(id);
    if (entry == nullptr || !entry->value) {
        return false;
    }
    Material material = *entry->value;
    material.renderState = renderState;
    static_cast<void>(_materials.store(id, entry->path, std::move(material)));
    markChanged(id);
    return true;
}

bool MaterialAssets::setNextPass(core::asset::AssetId id, core::asset::AssetId nextPass) {
    const Entry* entry = _materials.getEntry(id);
    if (entry == nullptr || !entry->value) {
        return false;
    }
    Material material = *entry->value;
    material.nextPass = nextPass;
    static_cast<void>(_materials.store(id, entry->path, std::move(material)));
    markChanged(id);
    return true;
}

bool MaterialAssets::unload(core::asset::AssetId id) {
    if (!_materials.unload(id)) {
        return false;
    }
    std::erase(_changed, id);
    _unloaded.push_back(id);
    return true;
}

std::vector<core::asset::AssetId> MaterialAssets::takeChanged() { return std::exchange(_changed, {}); }

std::vector<core::asset::AssetId> MaterialAssets::takeUnloaded() { return std::exchange(_unloaded, {}); }

void MaterialAssets::markChanged(core::asset::AssetId id) {
    if (std::ranges::find(_changed, id) == _changed.end()) {
        _changed.push_back(id);
    }
}

}  // namespace aylo::plugins::shader

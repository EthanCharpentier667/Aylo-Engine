/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MeshAssets
*/

#include "plugins/mesh/MeshAssets.hpp"

#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "aylo/contract/renderer/drawing/StaticMesh.hpp"
#include "core/asset/AssetId.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/vfs/FileSource.hpp"
#include "plugins/mesh/detail/GltfLoader.hpp"  // IWYU pragma: keep

namespace aylo::plugins::mesh {

void MeshAssets::setSource(std::shared_ptr<const core::vfs::FileSource> source) { _source = std::move(source); }

const core::vfs::FileSource* MeshAssets::getSource() const noexcept { return _source.get(); }

core::asset::AssetId MeshAssets::load(const std::filesystem::path& path) {
    std::string name = path.lexically_normal().generic_string();
    const core::asset::AssetId id = core::asset::AssetId::fromPath(name);
    if (const Entry* known = get(id)) {
        if (known->path != name) {
            throw core::asset::exceptions::AssetLoadException{
                std::format("Mesh '{}' has the same id as '{}': rename one of them", name, known->path)};
        }
        return id;
    }
    _entries.emplace(id, Entry{.path = std::move(name)});
    return id;
}

const MeshAssets::Entry* MeshAssets::get(core::asset::AssetId id) const {
    const auto found = _entries.find(id);
    return found != _entries.end() ? &found->second : nullptr;
}

MeshAssets::Entry* MeshAssets::getMut(core::asset::AssetId id) {
    const auto found = _entries.find(id);
    return found != _entries.end() ? &found->second : nullptr;
}

std::optional<contract::renderer::StaticMeshId> MeshAssets::getMesh(core::asset::AssetId id) const {
    const Entry* entry = get(id);
    if (entry == nullptr || entry->state != MeshState::kLoaded) {
        return std::nullopt;
    }
    return entry->mesh;
}

std::vector<core::asset::AssetId> MeshAssets::getIds() const {
    std::vector<core::asset::AssetId> ids;
    ids.reserve(_entries.size());
    for (const auto& [id, entry] : _entries) {
        ids.push_back(id);
    }
    return ids;
}

}  // namespace aylo::plugins::mesh

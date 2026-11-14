/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MaterialReloadable
*/

#include "plugins/shader/MaterialReloadable.hpp"

#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/core/reload/ChangeSet.hpp"
#include "aylo/core/reload/ReloadOutcome.hpp"
#include "aylo/core/reload/Reloadable.hpp"
#include "core/asset/AssetId.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialFile.hpp"

namespace aylo::plugins::shader {

MaterialReloadable::MaterialReloadable(ShaderAssets* shaders, MaterialAssets* materials,
                                       graphics::TextureAssets* textures, std::filesystem::path assetDirectory)
    : _shaders{shaders}, _materials{materials}, _textures{textures}, _assets{std::move(assetDirectory)} {}

std::string_view MaterialReloadable::name() const noexcept { return "materials"; }

std::vector<core::reload::WatchedPath> MaterialReloadable::watched() const {
    return {core::reload::WatchedPath{.path = _assets, .extensions = {std::string{kMaterialExtension}}}};
}

core::reload::ReloadOutcome MaterialReloadable::reload(const core::reload::ChangeSet& changes) {
    std::vector<std::string> done;
    std::vector<std::string> failed;
    const auto reloadAll = [&](const std::vector<std::string>& files) {
        for (const std::string& file : files) {
            const std::filesystem::path path = std::filesystem::path{file}.lexically_relative(_assets);
            if (_materials->getEntry(core::asset::AssetId::fromPath(path)) == nullptr) {
                continue;  // the game did not load it
            }
            try {
                static_cast<void>(loadMaterialFile(path, *_shaders, *_materials, *_textures));
                done.push_back(std::format("material {}", path.generic_string()));
            } catch (const core::asset::exceptions::AssetLoadException& error) {
                failed.emplace_back(error.what());
            }
        }
    };
    reloadAll(changes.added);
    reloadAll(changes.modified);
    if (!failed.empty()) {
        std::string reasons;
        for (const std::string& reason : failed) {
            reasons += std::format("{}{}", reasons.empty() ? "" : "; ", reason);
        }
        return core::reload::ReloadOutcome::failed(std::format("{} (the last version is kept)", reasons));
    }
    return core::reload::ReloadOutcome::applied(std::move(done));
}

}  // namespace aylo::plugins::shader

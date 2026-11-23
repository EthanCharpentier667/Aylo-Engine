/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ShaderReloadable
*/

#include "plugins/shader/ShaderReloadable.hpp"

#include <algorithm>
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
#include "plugins/shader/ShaderAssets.hpp"

namespace aylo::plugins::shader {

ShaderReloadable::ShaderReloadable(ShaderAssets* shaders, std::filesystem::path assetDirectory)
    : _shaders{shaders}, _assets{std::move(assetDirectory)} {}

std::string_view ShaderReloadable::name() const noexcept { return "shaders"; }

std::vector<core::reload::WatchedPath> ShaderReloadable::watched() const {
    return {core::reload::WatchedPath{.path = _assets / kDirectory, .extensions = {}}};
}

namespace {

/// @brief Reloads the shader @p id of @p shaders named @p name into @p done or @p failed, unless it is in one already.
void reloadOnce(ShaderAssets& shaders, core::asset::AssetId id, const std::string& name, std::vector<std::string>& done,
                std::vector<std::string>& failed) {
    if (std::ranges::contains(done, name) || std::ranges::contains(failed, name)) {
        return;
    }
    (shaders.reload(id) ? done : failed).push_back(name);
}

}  // namespace

core::reload::ReloadOutcome ShaderReloadable::reload(const core::reload::ChangeSet& changes) {
    std::vector<std::string> done;
    std::vector<std::string> failed;
    const auto reloadAll = [&](const std::vector<std::string>& files) {
        for (const std::string& file : files) {
            // A path in the assets' directory, as the game loaded the shader.
            const std::filesystem::path path = std::filesystem::path{file}.lexically_relative(_assets);
            const core::asset::AssetId id = core::asset::AssetId::fromPath(path);
            if (_shaders->getEntry(id) != nullptr) {
                reloadOnce(*_shaders, id, path.generic_string(), done, failed);
            }
            // The shaders that include it.
            for (const core::asset::AssetId includer : _shaders->getIncluders(path.generic_string())) {
                reloadOnce(*_shaders, includer, _shaders->getEntry(includer)->path, done, failed);
            }
        }
    };
    reloadAll(changes.added);
    reloadAll(changes.modified);

    if (!failed.empty()) {
        std::string names;
        for (const std::string& name : failed) {
            names += std::format("{}{}", names.empty() ? "" : ", ", name);
        }
        return core::reload::ReloadOutcome::failed(
            std::format("{} did not compile, the last version is kept (see the shader log)", names));
    }
    for (std::string& name : done) {
        name = std::format("shader {}", name);
    }
    return core::reload::ReloadOutcome::applied(std::move(done));
}

}  // namespace aylo::plugins::shader

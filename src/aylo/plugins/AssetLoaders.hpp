/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AssetLoaders
*/

#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/resource/Resources.hpp"

namespace aylo::plugins {
/// @brief Who loads each kind of asset file, by its extension, as the ECS resource "aylo.AssetLoaders": a plugin that
/// loads a kind of file (models, materials, images) adds its extensions when it is built, and a plugin that only
/// names assets (capsules) asks it to load a path without knowing which plugin does. Several plugins fill it: none
/// owns it, and a world without asset plugins (a server) has an empty one or none.
struct AssetLoaders {
    /// @brief Loads the asset at a path of the assets, as its plugin's setup form would; throws if it cannot.
    using Load = std::function<void(const std::filesystem::path& path)>;

    std::vector<std::pair<std::string, Load>> loaders;  ///< Lowercase extensions with their dot (".obj"), and loads.

    /// @brief Makes `load` the loader of the files ending in `extension` (".obj"), whatever its case.
    void add(std::string_view extension, Load load) { loaders.emplace_back(lowercase(extension), std::move(load)); }

    /// @brief Loads `path` with the loader of its extension.
    /// @return Whether a loader took it; false: no plugin loads that kind of file here.
    /// @throws What the loader throws when the file cannot be loaded.
    [[nodiscard]] bool load(const std::filesystem::path& path) const {
        const std::string extension = lowercase(path.extension().string());
        const auto found = std::ranges::find(loaders, extension, &std::pair<std::string, Load>::first);
        if (found == loaders.end()) {
            return false;
        }
        found->second(path);
        return true;
    }

    /// @return `text` with its ASCII letters in lowercase.
    [[nodiscard]] static std::string lowercase(std::string_view text) {
        std::string lower{text};
        std::ranges::transform(lower, lower.begin(),
                               [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
        return lower;
    }
};
AYLO_ECS_RESOURCE(AssetLoaders, "aylo.AssetLoaders");

/// @brief Adds `load` as the loader of each of `extensions` in the AssetLoaders @p resources hold, which it inserts
/// when they hold none.
inline void addAssetLoader(ecs::Resources& resources, std::initializer_list<std::string_view> extensions,
                           const AssetLoaders::Load& load) {
    resources.insertIfAbsent<AssetLoaders>();
    ecs::Mut<AssetLoaders> loaders = resources.requireMut<AssetLoaders>();
    for (const std::string_view extension : extensions) {
        loaders->add(extension, load);
    }
}
}  // namespace aylo::plugins

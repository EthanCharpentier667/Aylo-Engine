/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AssetDirectory
*/

#pragma once

#include <filesystem>
#include <memory>

#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "core/vfs/DirectorySource.hpp"
#include "core/vfs/FileSource.hpp"

namespace aylo::plugins {

/// @brief Where a game's assets are, as the ECS resource "rtype.AssetDirectory" the Engine inserts before the plugins
/// are built: `<game>/assets`, or the assets of the game's pack. Every asset path is relative to it and cannot leave it
/// (core::vfs::FileSource). A program without an Engine has none: its paths are relative to the working directory.
struct AssetDirectory {
    std::filesystem::path path;  ///< The directory on disk; empty when the assets are in a pack.
    /// @brief Where the assets are read; null: the files of `path`.
    std::shared_ptr<const core::vfs::FileSource> source;
};
AYLO_ECS_RESOURCE(AssetDirectory, "rtype.AssetDirectory");

/// @return The directory on disk @p resources hold, or an empty path if they hold none or the assets are in a pack.
[[nodiscard]] inline std::filesystem::path assetDirectoryOf(const ecs::Resources& resources) {
    const auto* directory = resources.get<AssetDirectory>();
    return directory != nullptr ? directory->path : std::filesystem::path{};
}

/// @return Where the assets @p resources hold are read: their source, the files of their directory, or the working
/// directory, unchecked, if they hold none. Never null.
[[nodiscard]] inline std::shared_ptr<const core::vfs::FileSource> assetSourceOf(const ecs::Resources& resources) {
    const auto* directory = resources.get<AssetDirectory>();
    if (directory != nullptr && directory->source) {
        return directory->source;
    }
    return std::make_shared<core::vfs::DirectorySource>(directory != nullptr ? directory->path
                                                                             : std::filesystem::path{});
}

}  // namespace aylo::plugins

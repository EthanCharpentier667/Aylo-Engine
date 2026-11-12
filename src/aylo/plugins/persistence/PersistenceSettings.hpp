/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PersistenceSettings
*/

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "core/config/Settings.hpp"
#include "core/layout/Layout.hpp"
#include "plugins/persistence/Export.hpp"

namespace aylo::plugins::persistence {

/// @brief The `persistence` section of `project.luau`. Every key is optional.
struct PersistenceSettings {
    std::string directory;  ///< Absolute folder of the saves; empty: `<user data directory>/<folder>`.
    /// The game's folder name in the user's data directory: by default the game's name.
    std::string folder{core::layout::kUserFolder};
    /// Whether the project left both keys to their default: the saves an earlier engine kept in its folder shared by
    /// every game (`aylo`, `rtype` before it) are then copied to `folder` the first time it is used.
    bool adoptSharedSaves = false;

    /// @param settings The `persistence` section.
    /// @param gameName The game's name (the `name` setting): the default `folder`; empty: kUserFolder.
    /// @throws core::config::exceptions::SettingsException If a key is unknown, has the wrong type or is invalid.
    [[nodiscard]] AYLO_PLUGIN_PERSISTENCE_API static PersistenceSettings fromSettings(
        const core::config::Settings& settings, std::string_view gameName = {});

    /// @return The folders of @p userData whose saves are copied to `folder` the first time it is used, in order: the
    /// shared folders (`aylo`, then `rtype`) other than `folder` itself, when adoptSharedSaves; none otherwise, or
    /// when @p userData is empty.
    [[nodiscard]] AYLO_PLUGIN_PERSISTENCE_API std::vector<std::filesystem::path> foldersToAdopt(
        const std::filesystem::path& userData) const;
};

}  // namespace aylo::plugins::persistence

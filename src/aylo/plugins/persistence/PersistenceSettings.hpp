/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PersistenceSettings
*/

#pragma once

#include <string>

#include "core/config/Settings.hpp"
#include "core/layout/Layout.hpp"
#include "plugins/persistence/Export.hpp"

namespace aylo::plugins::persistence {

/// @brief The `persistence` section of `project.luau`. Every key is optional.
struct PersistenceSettings {
    std::string directory;  ///< Absolute folder of the saves; empty: `<user data directory>/<folder>`.
    /// The game's folder name in the user's data directory.
    std::string folder{core::layout::kUserFolder};

    /// @throws core::config::exceptions::SettingsException If a key is unknown, has the wrong type or is invalid.
    [[nodiscard]] AYLO_PLUGIN_PERSISTENCE_API static PersistenceSettings fromSettings(
        const core::config::Settings& settings);
};

}  // namespace aylo::plugins::persistence

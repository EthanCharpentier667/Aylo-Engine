/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PersistenceSettings
*/

#include "plugins/persistence/PersistenceSettings.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"
#include "core/layout/Layout.hpp"

namespace aylo::plugins::persistence {

namespace {
/// The folders every game shared before each had its own, newest first. `rtype` is the engine's name before its rename.
constexpr std::array<std::string_view, 2> kSharedFolders{core::layout::kUserFolder, "rtype"};
}  // namespace

PersistenceSettings PersistenceSettings::fromSettings(const core::config::Settings& settings,
                                                      const std::string_view gameName) {
    settings.checkKeys({"directory", "folder"}, "persistence");
    PersistenceSettings config;
    if (!gameName.empty()) {
        config.folder = gameName;
    }
    config.adoptSharedSaves = !settings.has("directory") && !settings.has("folder");
    config.directory = settings.getString("directory", config.directory);
    config.folder = settings.getString("folder", config.folder);
    if (config.folder.empty() || config.folder.find_first_of("/\\:") != std::string::npos || config.folder == "." ||
        config.folder == "..") {
        throw core::config::exceptions::SettingsException{
            "Setting 'persistence.folder' must be a plain folder name (no separator)"};
    }
    return config;
}

std::vector<std::filesystem::path> PersistenceSettings::foldersToAdopt(const std::filesystem::path& userData) const {
    std::vector<std::filesystem::path> folders;
    if (!adoptSharedSaves || userData.empty()) {
        return folders;
    }
    for (const std::string_view shared : kSharedFolders) {
        if (shared != folder) {
            folders.push_back(userData / shared);
        }
    }
    return folders;
}

}  // namespace aylo::plugins::persistence

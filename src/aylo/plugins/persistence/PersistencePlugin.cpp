/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PersistencePlugin
*/

#include "plugins/persistence/PersistencePlugin.hpp"

#include <filesystem>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "plugins/persistence/PersistenceCalls.hpp"
#include "plugins/persistence/PersistenceSettings.hpp"
#include "plugins/persistence/SaveStore.hpp"
#include "plugins/persistence/UserDataDirectory.hpp"

namespace aylo::plugins::persistence {

PersistencePlugin::PersistencePlugin(PersistenceSettings settings) : _settings{std::move(settings)} {}

std::string_view PersistencePlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> PersistencePlugin::getDependencies() const noexcept { return {}; }

void PersistencePlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    if (!resources.has<SaveStore>()) {
        std::filesystem::path directory = _settings.directory;
        std::vector<std::filesystem::path> shared;
        if (directory.empty()) {
            const std::filesystem::path userData = getUserDataDirectory();
            directory = userData / _settings.folder;
            shared = _settings.foldersToAdopt(userData);
        }
        SaveStore store{std::move(directory)};
        for (const std::filesystem::path& folder : shared) {
            std::ignore = store.adopt(folder);  // copies only while the game's folder does not exist yet
        }
        resources.insert(std::move(store));
    }
    addPersistenceCalls(app);
}

}  // namespace aylo::plugins::persistence

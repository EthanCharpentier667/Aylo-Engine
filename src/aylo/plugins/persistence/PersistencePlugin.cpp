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
#include <utility>

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
        if (directory.empty()) {
            directory = getUserDataDirectory() / _settings.folder;
        }
        resources.insert(SaveStore{std::move(directory)});
    }
    addPersistenceCalls(app);
}

}  // namespace aylo::plugins::persistence

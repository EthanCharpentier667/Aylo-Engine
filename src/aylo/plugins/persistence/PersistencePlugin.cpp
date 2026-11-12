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

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "core/layout/Layout.hpp"
#include "plugins/persistence/PersistenceCalls.hpp"
#include "plugins/persistence/PersistenceSettings.hpp"
#include "plugins/persistence/SaveStore.hpp"
#include "plugins/persistence/UserDataDirectory.hpp"

namespace aylo::plugins::persistence {

namespace {
/// The default save folder before the engine was renamed: a game that kept the default finds its saves again.
constexpr std::string_view kFolderBeforeRename = "rtype";
}  // namespace

PersistencePlugin::PersistencePlugin(PersistenceSettings settings) : _settings{std::move(settings)} {}

std::string_view PersistencePlugin::getName() const noexcept { return kName; }

std::span<const std::string_view> PersistencePlugin::getDependencies() const noexcept { return {}; }

void PersistencePlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    if (!resources.has<SaveStore>()) {
        std::filesystem::path directory = _settings.directory;
        std::filesystem::path before;
        if (directory.empty()) {
            const std::filesystem::path userData = getUserDataDirectory();
            directory = userData / _settings.folder;
            if (!userData.empty() && _settings.folder == core::layout::kUserFolder) {
                before = userData / kFolderBeforeRename;
            }
        }
        SaveStore store{std::move(directory)};
        if (!before.empty()) {
            std::ignore = store.adopt(before);
        }
        resources.insert(std::move(store));
    }
    addPersistenceCalls(app);
}

}  // namespace aylo::plugins::persistence

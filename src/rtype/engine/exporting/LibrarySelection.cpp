/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LibrarySelection
*/

#include "LibrarySelection.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

#include "core/library/LibraryEntry.hpp"
#include "rtype/core/config/Settings.hpp"
#include "rtype/ecs/app/Plugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/engine/CommandLine.hpp"
#include "rtype/engine/Engine.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/plugin/PluginLoader.hpp"
#include "rtype/engine/plugin/SharedLibrary.hpp"
#include "rtype/engine/scripting/SettingsLoader.hpp"

namespace rtype::engine::exporting {
namespace {
/// @brief The extension of the file that lists what a plugin library links (xmake/modules/plugin_links.lua).
constexpr std::string_view kLinksExtension = ".links";

/// @brief A side of the game, as `project.luau` is read for it.
struct Side {
    ecs::RunMode mode;      ///< What the settings are read for.
    std::string_view name;  ///< How the reasons name it.
};

/// @brief The sides an export runs: a player's game and, with `--server`, a dedicated server.
constexpr std::array kSides{Side{.mode = ecs::RunMode::kServer, .name = "server"},
                            Side{.mode = ecs::RunMode::kClient, .name = "client"}};

/// @brief Why each library is kept, by its file.
using Reasons = std::map<std::filesystem::path, std::set<std::string>>;

/// @brief What the libraries of one side brought, by name.
struct Registered {
    std::map<std::string, std::filesystem::path, std::less<>> plugins;             ///< Library of each plugin.
    std::map<std::string, std::set<std::filesystem::path>, std::less<>> backends;  ///< Libraries of each backend.
};

/// @return The names of @p after that @p before does not hold.
std::vector<std::string> addedNames(std::vector<std::string> before, std::vector<std::string> after) {
    std::ranges::sort(before);
    std::ranges::sort(after);
    std::vector<std::string> added;
    std::ranges::set_difference(after, before, std::back_inserter(added));
    return added;
}

/// @return @p names in quotes, separated by commas, or "none".
std::string quoteNames(const std::vector<std::string>& names) {
    std::string text;
    for (const std::string& name : names) {
        text += std::format("{}'{}'", text.empty() ? "" : ", ", name);
    }
    return text.empty() ? "none" : text;
}

/// @return The plugin libraries @p library links, from its `.links` file; none if it has no such file.
std::vector<std::string> readLinks(const std::filesystem::path& library) {
    std::ifstream file{std::filesystem::path{library}.concat(kLinksExtension)};
    std::vector<std::string> links;
    for (std::string line; std::getline(file, line);) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }
        if (!line.empty()) {
            links.push_back(std::move(line));
        }
    }
    return links;
}

/// @return The libraries directly in @p directory, sorted; none if it does not exist.
std::vector<std::filesystem::path> librariesIn(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator{directory, error}) {
        if (entry.path().extension() == plugin::SharedLibrary::getExtension()) {
            files.push_back(entry.path());
        }
    }
    std::ranges::sort(files);
    return files;
}

/// @brief Keeps the libraries one side of the game needs, with their reasons.
class SideSelection {
  public:
    SideSelection(const SelectionRequest& request, const Side& side, Reasons& reasons)
        : _request{&request}, _side{&side}, _reasons{&reasons} {}

    /// @brief Reads the side's settings, registers its libraries and keeps what it needs.
    void run() {
        const std::vector<std::string> args = _request->args;
        const CommandLine commandLine = CommandLine::parse(std::span<const std::string>{args});
        const core::config::Settings settings = scripting::loadProjectSettings(
            _request->game / Engine::kProjectFile, _side->mode, commandLine, _request->debug);
        const bool client = ecs::runsIn(ecs::Side::kClient, _side->mode);
        const bool backends = client && std::ranges::any_of(Engine::kBackendSettings,
                                                            [&settings](const auto key) { return settings.has(key); });

        plugin::PluginLoader loader;
        const std::filesystem::path plugins = _request->runtime / Engine::kPluginsDirectory;
        loader.load(plugins);
        if (backends) {
            loader.load(plugins / Engine::kBackendsDirectory);
        }
        const std::filesystem::path gamePlugins = _request->game / Engine::kPluginsDirectory;
        loader.load(gamePlugins, _request->verifyGamePlugin);

        ecs::PluginRegistry registry;
        registry.setSettings(settings);
        core::library::LibraryRegistrar registrar;
        registrar.provide(ecs::kPluginRegistryService, registry);
        registerLibraries(loader, registry, registrar);

        keepPlugins(settings, registry);
        if (backends) {
            keepBackends(settings);
        }
        for (const std::filesystem::path& file : librariesIn(gamePlugins)) {
            (*_reasons)[file].insert("a plugin of the game");
        }
    }

  private:
    /// @brief Registers the libraries one at a time, noting what each one adds.
    void registerLibraries(const plugin::PluginLoader& loader, const ecs::PluginRegistry& registry,
                           core::library::LibraryRegistrar& registrar) {
        std::vector<std::string> plugins = registry.getNames();
        auto services = registrar.listNames();
        loader.registerInto(registrar, [&](const std::filesystem::path& file) {
            std::vector<std::string> nowPlugins = registry.getNames();
            for (std::string& name : addedNames(plugins, nowPlugins)) {
                _registered.plugins.emplace(std::move(name), file);
            }
            plugins = std::move(nowPlugins);
            auto nowServices = registrar.listNames();
            for (const auto& [service, names] : nowServices) {
                const auto before = services.find(service);
                for (std::string& name :
                     addedNames(before != services.end() ? before->second : std::vector<std::string>{}, names)) {
                    _registered.backends[std::move(name)].insert(file);
                }
            }
            services = std::move(nowServices);
        });
    }

    /// @brief Keeps the libraries of the plugins the side names, the transform plugin, and what they require.
    void keepPlugins(const core::config::Settings& settings, const ecs::PluginRegistry& registry) {
        // A plugin, why it is wanted, and the plugin that requires it (empty: named by the game).
        std::deque<std::tuple<std::string, std::string, std::string>> queue;
        for (std::string& name : settings.getStringList(Engine::kPluginsSetting, {})) {
            queue.emplace_back(std::move(name), std::format("named in {} ({})", Engine::kPluginsSetting, _side->name),
                               "");
        }
        queue.emplace_back(std::string{Engine::kTransformPlugin}, "every game", "");
        std::set<std::string, std::less<>> seen;
        while (!queue.empty()) {
            auto [name, reason, requiredBy] = std::move(queue.front());
            queue.pop_front();
            if (!seen.insert(name).second) {
                continue;
            }
            const auto owner = _registered.plugins.find(name);
            if (owner == _registered.plugins.end()) {
                throw exceptions::RuntimeError{unknownPlugin(name, requiredBy)};
            }
            (*_reasons)[owner->second].insert(reason);
            const std::unique_ptr<ecs::Plugin> created = registry.create(name);
            for (const std::string_view dependency : created->getDependencies()) {
                queue.emplace_back(std::string{dependency}, std::format("required by {}", name), name);
            }
        }
    }

    /// @brief Keeps the libraries of the backends the side's settings name.
    void keepBackends(const core::config::Settings& settings) {
        for (const std::string_view key : Engine::kBackendSettings) {
            if (!settings.has(key)) {
                continue;
            }
            const std::string name = settings.getString(key, "");
            const auto owners = _registered.backends.find(name);
            if (owners == _registered.backends.end()) {
                std::vector<std::string> known;
                known.reserve(_registered.backends.size());
                for (const auto& [backend, files] : _registered.backends) {
                    known.push_back(backend);
                }
                throw exceptions::RuntimeError{
                    std::format("Invalid setting '{}' ({}): no backend library registers '{}' (known: {})", key,
                                _side->name, name, quoteNames(known))};
            }
            for (const std::filesystem::path& file : owners->second) {
                (*_reasons)[file].insert(std::format("backend '{}' ({})", name, key));
            }
        }
    }

    /// @return Why the plugin @p name cannot be kept: no library registers it.
    [[nodiscard]] std::string unknownPlugin(const std::string& name, const std::string& requiredBy) const {
        std::vector<std::string> known;
        known.reserve(_registered.plugins.size());
        for (const auto& [plugin, file] : _registered.plugins) {
            known.push_back(plugin);
        }
        if (requiredBy.empty()) {
            return std::format("Invalid setting '{}' ({}): unknown plugin '{}' (known: {})", Engine::kPluginsSetting,
                               _side->name, name, quoteNames(known));
        }
        return std::format("Plugin '{}' requires '{}', which no library registers (known: {})", requiredBy, name,
                           quoteNames(known));
    }

    const SelectionRequest* _request;  ///< The game and the runtime; outlives the selection.
    const Side* _side;                 ///< The side read; outlives the selection.
    Reasons* _reasons;                 ///< Where the libraries kept go; outlives the selection.
    Registered _registered;            ///< What the side's libraries brought.
};

/// @brief Adds to @p reasons the plugin libraries the libraries kept link, until nothing is added.
/// @throws exceptions::RuntimeError If one is not in @p plugins.
void keepLinked(Reasons& reasons, const std::filesystem::path& plugins) {
    std::deque<std::filesystem::path> queue;
    for (const auto& [file, why] : reasons) {
        queue.push_back(file);
    }
    while (!queue.empty()) {
        const std::filesystem::path file = std::move(queue.front());
        queue.pop_front();
        for (const std::string& link : readLinks(file)) {
            const std::filesystem::path linked = plugins / link;
            if (!std::filesystem::is_regular_file(linked)) {
                throw exceptions::RuntimeError{std::format("Plugin library '{}' links '{}', which is not in '{}'",
                                                           file.generic_string(), link, plugins.generic_string())};
            }
            const bool added = !reasons.contains(linked);
            reasons[linked].insert(std::format("linked by {}", file.filename().generic_string()));
            if (added) {
                queue.push_back(linked);
            }
        }
    }
}
}  // namespace

std::vector<SelectedLibrary> selectLibraries(const SelectionRequest& request) {
    SelectionRequest absolute = request;
    absolute.game = std::filesystem::absolute(request.game).lexically_normal();
    absolute.runtime = std::filesystem::absolute(request.runtime).lexically_normal();
    if (!std::filesystem::is_regular_file(absolute.game / Engine::kProjectFile)) {
        throw exceptions::RuntimeError{
            std::format("No game in '{}': it has no {}", absolute.game.generic_string(), Engine::kProjectFile)};
    }
    Reasons reasons;
    for (const Side& side : kSides) {
        SideSelection{absolute, side, reasons}.run();
    }
    const std::filesystem::path plugins = absolute.runtime / Engine::kPluginsDirectory;
    keepLinked(reasons, plugins);

    std::vector<SelectedLibrary> selected;
    for (auto& [file, why] : reasons) {
        const std::filesystem::path folder = file.parent_path();
        LibraryKind kind = LibraryKind::kGame;
        if (folder == plugins) {
            kind = LibraryKind::kPlugin;
        } else if (folder == plugins / Engine::kBackendsDirectory) {
            kind = LibraryKind::kBackend;
        }
        selected.push_back(SelectedLibrary{.file = file, .kind = kind, .reasons = {why.begin(), why.end()}});
    }
    std::ranges::sort(selected, [](const SelectedLibrary& left, const SelectedLibrary& right) {
        return std::tie(left.kind, left.file) < std::tie(right.kind, right.file);
    });
    return selected;
}
}  // namespace rtype::engine::exporting

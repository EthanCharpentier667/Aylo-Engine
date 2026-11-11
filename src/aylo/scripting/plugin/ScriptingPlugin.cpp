/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptingPlugin
*/

#include "aylo/scripting/plugin/ScriptingPlugin.hpp"

#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/scripting/ErrorKind.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "aylo/scripting/exceptions/ScriptLoadException.hpp"
#include "aylo/scripting/modules/ScriptResolver.hpp"

namespace aylo::scripting {
namespace {
constexpr std::string_view kDefaultEntry = "main";

/// @brief One entry point to run: the setting that names it and the path it holds.
struct Entry {
    std::string_view setting;  ///< `main`, `server` or `client`.
    std::string path;          ///< The entry, relative to `scripts/`, with no extension.
    bool isDefault{false};     ///< Whether no setting gave it: a missing file is then only a warning.
};
}  // namespace

ScriptingPlugin::ScriptingPlugin(Config config) : _config{std::move(config)} {}

ScriptingPlugin::~ScriptingPlugin() = default;

ScriptingPlugin::ScriptingPlugin(ScriptingPlugin&& other) noexcept = default;

ScriptingPlugin& ScriptingPlugin::operator=(ScriptingPlugin&& other) noexcept = default;

std::string_view ScriptingPlugin::getName() const noexcept { return "aylo.scripting"; }

void ScriptingPlugin::build(ecs::App& app) {
    const modules::ScriptResolver resolver = _config.source
                                                 ? modules::ScriptResolver{_config.source, _config.aliases}
                                                 : modules::ScriptResolver{_config.directory, _config.aliases};
    auto host = ScriptHost::create(app, _config.options, resolver.forLoader());
    if (!host) {
        throw exceptions::ScriptLoadException{
            std::format("Cannot create the script runtime: {}", host.error().message())};
    }
    _host = std::move(*host);
    // The files nobody requires are not part of the game: a changed one is ignored by a reload until one requires it.
    _host->setAdoptsNewFiles(false);

    // main first, then the side(s) of this process; with no setting at all, `main` alone, by its default name.
    std::vector<Entry> entries;
    const bool anySet = _config.main || _config.server || _config.client;
    if (!anySet) {
        entries.push_back({.setting = "main", .path = std::string{kDefaultEntry}, .isDefault = true});
    }
    if (_config.main) {
        entries.push_back({.setting = "main", .path = *_config.main});
    }
    if (_config.options.isServer && _config.server) {
        entries.push_back({.setting = "server", .path = *_config.server});
    }
    if (_config.options.isClient && _config.client) {
        entries.push_back({.setting = "client", .path = *_config.client});
    }
    for (const Entry& entry : entries) {
        const auto found = resolver.resolveEntry(entry.path);
        if (!found && entry.isDefault && found.error().kind() == ErrorKind::kModuleNotFound) {
            std::cerr << std::format("[ScriptingPlugin] No entry script '{}' in '{}', no script loaded\n", entry.path,
                                     _config.directory.string());
            continue;
        }
        if (!found) {
            throw exceptions::ScriptLoadException{
                std::format("Setting 'scripts.{}' = '{}': {}", entry.setting, entry.path, found.error().message())};
        }
        if (auto result = _host->require(std::format("@scripts/{}", entry.path)); !result) {
            throw exceptions::ScriptLoadException{std::format("Entry script '{}' ('scripts.{}') failed: {}",
                                                              found->chunkName, entry.setting,
                                                              result.error().message())};
        }
    }
    if (_config.onBuilt) {
        _config.onBuilt(*_host);
    }
}
}  // namespace aylo::scripting

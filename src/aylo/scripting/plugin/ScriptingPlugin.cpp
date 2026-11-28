/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ScriptingPlugin
*/

#include "aylo/scripting/plugin/ScriptingPlugin.hpp"

#include <cstddef>
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
#include "core/vfs/DirectorySource.hpp"
#include "core/vfs/FileSource.hpp"

namespace aylo::scripting {
namespace {
constexpr std::string_view kDefaultEntry = "main";

/// @brief The folder of `scripts/` whose files run without a `require`, after `main` and before the sides.
constexpr std::string_view kBehavioursFolder = "behaviours/";

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
    // Those of scripts/behaviours/ are, required by the engine: a new one there runs.
    _host->setAdoptsNewFiles(false);
    _host->setAdoptedFolder(std::string{kBehavioursFolder});

    // main first, then the side(s) of this process; with no setting at all, `main` alone, by its default name.
    std::vector<Entry> entries;
    const bool anySet = _config.main || _config.server || _config.client;
    if (!anySet) {
        entries.push_back({.setting = "main", .path = std::string{kDefaultEntry}, .isDefault = true});
    }
    if (_config.main) {
        entries.push_back({.setting = "main", .path = *_config.main});
    }
    const std::size_t mainEntries = entries.size();
    if (_config.options.isServer && _config.server) {
        entries.push_back({.setting = "server", .path = *_config.server});
    }
    if (_config.options.isClient && _config.client) {
        entries.push_back({.setting = "client", .path = *_config.client});
    }
    for (std::size_t index = 0; index < entries.size(); ++index) {
        if (index == mainEntries) {
            requireBehaviours();
        }
        const Entry& entry = entries.at(index);
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
    if (mainEntries == entries.size()) {
        requireBehaviours();
    }
    if (_config.onBuilt) {
        _config.onBuilt(*_host);
    }
}
void ScriptingPlugin::requireBehaviours() const {
    const core::vfs::DirectorySource directory{_config.directory};
    const core::vfs::FileSource& source = _config.source ? *_config.source : directory;
    for (const std::string& file : source.list(kBehavioursFolder)) {
        const std::string_view path{file};
        if (!path.ends_with(".luau")) {
            continue;
        }
        const std::string key{path.substr(0, path.size() - std::string_view{".luau"}.size())};
        const std::string chunkName = std::format("scripts/{}", file);
        if (auto result = _host->require(std::format("@scripts/{}", key)); !result) {
            throw exceptions::ScriptLoadException{
                std::format("Behaviour script '{}' failed: {}", chunkName, result.error().message())};
        }
        if (!_host->declaresBehaviour(chunkName)) {
            std::cerr << std::format(
                "[ScriptingPlugin] '{}' declares no Behaviour: the files of scripts/{} run without a require, for "
                "engine.behaviour; move a module elsewhere in scripts/\n",
                chunkName, kBehavioursFolder);
        }
    }
}
}  // namespace aylo::scripting

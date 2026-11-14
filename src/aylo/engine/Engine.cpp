/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Engine
*/

#include "Engine.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "aylo/core/config/Settings.hpp"
#include "aylo/core/reload/ReloadReport.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/MinimalPlugins.hpp"
#include "aylo/ecs/app/Plugin.hpp"
#include "aylo/ecs/app/PluginEntry.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/engine/CommandLine.hpp"
#include "aylo/engine/EngineConfig.hpp"
#include "aylo/engine/Exceptions.hpp"
#include "aylo/engine/NetworkInput.hpp"
#include "aylo/engine/plugin/BackendLibraries.hpp"
#include "aylo/engine/plugin/PluginSignature.hpp"
#include "aylo/engine/plugin/RequiredPlugin.hpp"
#include "aylo/engine/reload/ProjectReloadable.hpp"
#include "aylo/engine/reload/ScriptsReloadable.hpp"
#include "aylo/engine/scripting/SettingsLoader.hpp"
#include "aylo/network/NetworkSettings.hpp"
#include "aylo/network/RandomSource.hpp"
#include "aylo/network/admin/AdminSettings.hpp"
#include "aylo/network/crypto/Crypto.hpp"
#include "aylo/network/io/Address.hpp"
#include "aylo/plugins/AssetDirectory.hpp"
#include "aylo/plugins/HotReload.hpp"
#include "aylo/replication/ReplicationPlugin.hpp"
#include "aylo/replication/ReplicationSettings.hpp"
#include "aylo/replication/client/ClientPlugin.hpp"
#include "aylo/replication/server/ServerPlugin.hpp"
#include "aylo/replication/standalone/StandalonePlugin.hpp"
#include "aylo/scripting/ScriptHost.hpp"
#include "aylo/scripting/exceptions/ScriptLoadException.hpp"
#include "aylo/scripting/modules/ScriptResolver.hpp"
#include "aylo/scripting/plugin/ScriptingPlugin.hpp"
#include "core/asset/exceptions/AssetExceptions.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"
#include "core/layout/GameName.hpp"
#include "core/library/LibraryEntry.hpp"
#include "core/system/ExecutablePath.hpp"
#include "core/vfs/DirectorySource.hpp"
#include "core/vfs/FileSource.hpp"
#include "core/vfs/Pack.hpp"
#include "core/vfs/exceptions/PackException.hpp"

namespace {
#ifdef NDEBUG
constexpr bool kIsDebug = false;
#else
constexpr bool kIsDebug = true;
#endif

/// @return Whether @p mode runs the client side: a window, inputs and sound.
bool runsClient(const aylo::ecs::RunMode mode) { return aylo::ecs::runsIn(aylo::ecs::Side::kClient, mode); }

/// @return Whether @p mode runs the server side.
bool runsServer(const aylo::ecs::RunMode mode) { return aylo::ecs::runsIn(aylo::ecs::Side::kServer, mode); }

/// @return Whether @p args start with the game directory: a first argument that is not a flag.
bool startsWithGame(const std::vector<std::string>& args) { return !args.empty() && !args.front().starts_with("--"); }

/// @return The game in the pack @p file: its files, and the folder of its native plugins next to it.
/// @throws aylo::engine::exceptions::RuntimeError If it is not a pack, or holds no project file.
aylo::engine::Engine::GameLocation openPack(const std::filesystem::path& file) {
    std::shared_ptr<const aylo::core::vfs::FileSource> pack;
    try {
        pack = std::make_shared<aylo::core::vfs::PackSource>(file);
    } catch (const aylo::core::vfs::exceptions::PackException& error) {
        throw aylo::engine::exceptions::RuntimeError{error.what()};
    }
    if (!pack->exists(aylo::engine::Engine::kProjectFile)) {
        throw aylo::engine::exceptions::RuntimeError{
            std::format("No game in the pack '{}': it has no {}", file.string(), aylo::engine::Engine::kProjectFile)};
    }
    return {.directory = file.parent_path() / file.stem(), .files = std::move(pack), .packed = true};
}

/// @return The game: the first of @p args if it is not a flag (a directory, or a `.pak`), else @p fallback, else the
/// pack next to the program (Engine::kPackFile), made absolute.
/// @throws aylo::engine::exceptions::RuntimeError If none holds a project file.
aylo::engine::Engine::GameLocation findGame(const std::vector<std::string>& args,
                                            const std::filesystem::path& fallback) {
    using aylo::engine::Engine;
    const bool given = startsWithGame(args);
    const std::filesystem::path directory =
        std::filesystem::absolute(given ? std::filesystem::path{args.front()} : fallback).lexically_normal();

    if (given && std::filesystem::is_regular_file(directory) &&
        directory.extension() == aylo::core::vfs::kPackExtension) {
        return openPack(directory);
    }
    if (std::filesystem::is_regular_file(directory / Engine::kProjectFile)) {
        return {.directory = directory,
                .files = std::make_shared<aylo::core::vfs::DirectorySource>(directory),
                .packed = false};
    }
    const std::filesystem::path pack = aylo::core::system::getExecutableDirectory() / Engine::kPackFile;
    if (!given && std::filesystem::is_regular_file(pack)) {
        return openPack(pack);
    }
    throw aylo::engine::exceptions::RuntimeError{
        std::format("No game in '{}': it has no {} (give the game directory or its pack as the first argument)",
                    directory.string(), Engine::kProjectFile)};
}

/// @return The flags of @p args, which are what follows the game directory if there is one.
aylo::engine::CommandLine parseFlags(const std::vector<std::string>& args) {
    return aylo::engine::CommandLine::parse(std::span{args}.subspan(startsWithGame(args) ? 1 : 0));
}

/// @return The `network.port` setting, checked: a whole number from @p lowest to 65535.
/// @throws aylo::engine::exceptions::RuntimeError If it is not.
std::uint16_t readPort(const aylo::core::config::Settings& network, const std::uint16_t lowest) {
    const double port = network.getNumber("port", aylo::network::NetworkSettings{}.port);

    if (std::trunc(port) != port || port < lowest || port > std::numeric_limits<std::uint16_t>::max()) {
        throw aylo::engine::exceptions::RuntimeError{std::format(
            "Invalid setting 'network.port': {} is not a port (a whole number from {} to 65535)", port, lowest)};
    }
    return static_cast<std::uint16_t>(port);
}

/// The default of `scripts.timeout`, in seconds: long enough for a module that loads many assets, short enough that a
/// script stuck in a loop gives the game back.
constexpr double kDefaultScriptTimeout = 5.0;
/// The default of `scripts.memoryLimit`, in megabytes: far above what scripts hold, below what takes a machine down.
constexpr double kDefaultScriptMemoryLimit = 1024.0;
/// The largest `scripts.timeout`, in seconds (an hour), and `scripts.memoryLimit`, in megabytes (a terabyte).
constexpr double kMaxScriptTimeout = 3600.0;
constexpr double kMaxScriptMemoryLimit = 1024.0 * 1024.0;

/// @return The `scripts.<key>` number, checked: from 0 to @p highest (0 turns the limit off).
/// @throws aylo::engine::exceptions::RuntimeError If it is not, naming the setting and @p unit.
double readScriptLimit(const aylo::core::config::Settings& scripts, const std::string_view key, const double fallback,
                       const double highest, const std::string_view unit) {
    const double value = scripts.getNumber(key, fallback);

    if (!std::isfinite(value) || value < 0 || value > highest) {
        throw aylo::engine::exceptions::RuntimeError{
            std::format("Invalid setting 'scripts.{}': {} is not a number of {} from 0 (no limit) to {}", key, value,
                        unit, highest)};
    }
    return value;
}

/// @return `scripts.timeout` (seconds) as the budget of a call into a script; zero: no limit.
std::chrono::milliseconds readScriptTimeout(const aylo::core::config::Settings& scripts) {
    constexpr double kMillisecondsPerSecond = 1000.0;
    const double seconds = readScriptLimit(scripts, "timeout", kDefaultScriptTimeout, kMaxScriptTimeout, "seconds");
    return std::chrono::milliseconds{static_cast<std::int64_t>(std::llround(seconds * kMillisecondsPerSecond))};
}

/// @return `scripts.memoryLimit` (megabytes) in bytes; 0: no limit.
std::size_t readScriptMemoryLimit(const aylo::core::config::Settings& scripts) {
    constexpr double kBytesPerMegabyte = 1024.0 * 1024.0;
    const double megabytes =
        readScriptLimit(scripts, "memoryLimit", kDefaultScriptMemoryLimit, kMaxScriptMemoryLimit, "megabytes");
    return static_cast<std::size_t>(megabytes * kBytesPerMegabyte);
}

/// @return The `scripts.aliases` setting: alias name, then its target folder in `scripts/`.
/// @throws aylo::engine::exceptions::RuntimeError If a value is not a string, or an alias is invalid (`scripts` and
/// `self` are reserved, names are `a-z 0-9 _ . -`, the target stays inside @p scriptsDirectory). Names the setting.
std::map<std::string, std::string> readScriptAliases(
    const aylo::core::config::Settings& scripts, const std::filesystem::path& scriptsDirectory,
    const std::shared_ptr<const aylo::core::vfs::FileSource>& scriptFiles) {
    const aylo::core::config::Settings aliases = scripts.section("aliases");
    std::map<std::string, std::string> result;

    for (const std::string& name : aliases.keys()) {
        try {
            result.emplace(name, aliases.getString(name, ""));
            // Alone, so the error names this alias; duplicates (names ignore case) fail when the plugin builds.
            const std::map<std::string, std::string> alias{{name, result.at(name)}};
            const aylo::scripting::modules::ScriptResolver check =
                scriptFiles ? aylo::scripting::modules::ScriptResolver{scriptFiles, alias}
                            : aylo::scripting::modules::ScriptResolver{scriptsDirectory, alias};
        } catch (const aylo::core::config::exceptions::SettingsException&) {
            throw aylo::engine::exceptions::RuntimeError{
                std::format("Invalid setting 'scripts.aliases.{}': it must be a string, the folder in scripts/", name)};
        } catch (const aylo::scripting::exceptions::ScriptLoadException& error) {
            throw aylo::engine::exceptions::RuntimeError{
                std::format("Invalid setting 'scripts.aliases.{}': {}", name, error.what())};
        }
    }
    return result;
}

/// @return The `scripts.<key>` setting (an entry point of the scripts), or nothing if the game does not set it.
/// @throws aylo::engine::exceptions::RuntimeError If it is not a string. Names the setting.
std::optional<std::string> readScriptEntry(const aylo::core::config::Settings& scripts, const std::string_view key) {
    if (!scripts.has(key)) {
        return std::nullopt;
    }
    try {
        return scripts.getString(key, "");
    } catch (const aylo::core::config::exceptions::SettingsException&) {
        throw aylo::engine::exceptions::RuntimeError{std::format(
            "Invalid setting 'scripts.{}': it must be a string, a path in scripts/ without extension", key)};
    }
}

/// @brief Reads `--admin-port` and `--admin-token` into @p server: the admin channel, for tools such as the editor.
/// @throws aylo::engine::exceptions::RuntimeError If only one flag is given, the port is not a port from 1 to 65535,
/// or the build is not a debug one.
std::optional<aylo::network::admin::AdminSettings> readAdmin(const aylo::engine::CommandLine& commandLine,
                                                             const bool isDebug) {
    const std::optional<std::string> port = commandLine.get("admin-port");
    const std::optional<std::string> token = commandLine.get("admin-token");

    if (!port && !token) {
        return std::nullopt;
    }
    if (!isDebug) {
        throw aylo::engine::exceptions::RuntimeError{
            "The admin channel (--admin-port, --admin-token) is only "
            "available in a debug run"};
    }
    if (!port || !token || token->empty()) {
        throw aylo::engine::exceptions::RuntimeError{"--admin-port and --admin-token go together, the token not empty"};
    }
    std::uint32_t value = 0;
    const std::string_view text{*port};
    const char* const first = text.data();
    const char* const last = first + text.size();  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const auto [stop, error] = std::from_chars(first, last, value);
    if (error != std::errc{} || stop != last || value == 0 || value > std::numeric_limits<std::uint16_t>::max()) {
        throw aylo::engine::exceptions::RuntimeError{
            std::format("Invalid --admin-port '{}': a whole number from 1 to 65535", *port)};
    }
    return aylo::network::admin::AdminSettings{.port = static_cast<std::uint16_t>(value), .token = *token};
}

/// @return The seed of the server's identity kept in the file `network.identityFile` names, made and written there
/// (readable by its owner only) when the file does not exist yet; nothing without the setting.
/// @throws aylo::engine::exceptions::RuntimeError If the file holds no seed, or cannot be written.
std::optional<aylo::network::crypto::Key> readIdentitySeed(const aylo::core::config::Settings& network) {
    namespace crypto = aylo::network::crypto;
    if (!network.has("identityFile")) {
        return std::nullopt;
    }
    const std::filesystem::path file = network.getString("identityFile", "");
    std::error_code error;
    if (!std::filesystem::exists(file, error)) {
        crypto::initialize();
        const crypto::Key seed = crypto::makeSeed(aylo::network::makeSystemRandom());
        if (file.has_parent_path()) {
            std::filesystem::create_directories(file.parent_path(), error);
        }
        std::ofstream{file} << crypto::toHex(seed) << '\n';
        std::filesystem::permissions(file, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                                     error);
        if (!std::filesystem::exists(file)) {
            throw aylo::engine::exceptions::RuntimeError{
                std::format("Invalid setting 'network.identityFile': cannot write '{}'", file.string())};
        }
        std::cerr << std::format("[Network] A new server identity was written to '{}': keep it secret.\n",
                                 file.string());
        return seed;
    }
    std::ifstream stream{file};
    std::string text{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
        text.pop_back();
    }
    const std::optional<crypto::Key> seed = crypto::keyFromHex(text);
    if (!seed) {
        throw aylo::engine::exceptions::RuntimeError{
            std::format("Invalid setting 'network.identityFile': '{}' does not hold a seed (64 hexadecimal digits)",
                        file.string())};
    }
    return seed;
}

/// @return The server key `network.serverKey` pins, nothing without the setting.
/// @throws aylo::engine::exceptions::RuntimeError If it is not a key.
std::optional<aylo::network::crypto::Key> readServerKey(const aylo::core::config::Settings& network) {
    if (!network.has("serverKey")) {
        return std::nullopt;
    }
    const std::string text = network.getString("serverKey", "");
    const std::optional<aylo::network::crypto::Key> key = aylo::network::crypto::keyFromHex(text);
    if (!key) {
        throw aylo::engine::exceptions::RuntimeError{
            std::format("Invalid setting 'network.serverKey': '{}' is not a key (64 hexadecimal digits)", text)};
    }
    return key;
}

/// @return The server or the client plugin the mode needs, from the `network` settings; null when standalone.
/// @throws aylo::engine::exceptions::RuntimeError If the settings are invalid, or the server's host does not resolve.
std::unique_ptr<aylo::ecs::Plugin> makeNetworkPlugin(const aylo::ecs::RunMode mode,
                                                     const aylo::core::config::Settings& settings,
                                                     const aylo::engine::CommandLine& commandLine, const bool isDebug) {
    const aylo::core::config::Settings network = settings.section("network");

    network.checkKeys({"host", "port", "identityFile", "serverKey"}, "network");
    if (mode == aylo::ecs::RunMode::kServer) {
        aylo::network::NetworkSettings server;

        server.port = readPort(network, 0);
        server.identitySeed = readIdentitySeed(network);
        if (server.identitySeed) {
            aylo::network::crypto::initialize();
            std::cerr << std::format("[Network] Server identity: {} (clients pin it with network.serverKey)\n",
                                     aylo::network::crypto::toHex(
                                         aylo::network::crypto::Identity::fromSeed(*server.identitySeed).publicKey()));
        }
        auto plugin =
            std::make_unique<aylo::replication::server::ServerPlugin>(server, aylo::replication::ReplicationSettings{});
        if (auto admin = readAdmin(commandLine, isDebug)) {
            plugin->setAdmin(std::move(*admin));
        }
        return plugin;
    }
    if (mode == aylo::ecs::RunMode::kClient) {
        const std::string host = network.getString("host", aylo::engine::Engine::kDefaultHost);
        const std::uint16_t port = readPort(network, 1);
        const std::optional<aylo::network::io::Address> address = aylo::network::io::Address::resolve(host, port);

        if (!address) {
            throw aylo::engine::exceptions::RuntimeError{
                std::format("Invalid setting 'network.host': '{}' does not resolve to an address", host)};
        }
        aylo::network::NetworkSettings client;
        client.trustedServerKey = readServerKey(network);
        return std::make_unique<aylo::replication::client::ClientPlugin>(*address, client);
    }
    return nullptr;
}

/// The setting naming the game: the default folder of its saves.
constexpr std::string_view kNameSetting = "name";

/// @brief Gives @p settings the game's name when its project sets none: the name of its folder @p game
/// (aylo::core::layout::gameName()), if that can name a folder.
/// @throws aylo::engine::exceptions::RuntimeError If the project sets a name that cannot name a folder.
void nameGame(aylo::core::config::Settings& settings, const std::filesystem::path& game) {
    if (settings.has(kNameSetting)) {
        const std::string name = settings.getString(kNameSetting, "");
        if (!aylo::core::layout::isFileName(name)) {
            throw aylo::engine::exceptions::RuntimeError{
                std::format("Invalid setting 'name': '{}' names the game's folders, so it must be a file name", name)};
        }
        return;
    }
    std::string name = aylo::core::layout::gameName(game);
    if (aylo::core::layout::isFileName(name)) {
        settings.set(std::string{kNameSetting}, std::move(name));
    }
}
}  // namespace

namespace aylo::engine {
Engine::Engine(EngineConfig config)
    : _config{std::move(config)},
      _commandLine{parseFlags(_config.args)},
      _game{findGame(_config.args, _config.game)},
      _gameDirectory{_game.directory},
      _reloader{_gameDirectory},
      _hotReload{_config.debug.value_or(kIsDebug)} {
    build();
}

Engine::~Engine() = default;

ecs::App& Engine::app() noexcept { return *_app; }

const ecs::App& Engine::app() const noexcept { return *_app; }

const CommandLine& Engine::commandLine() const noexcept { return _commandLine; }

const std::filesystem::path& Engine::gameDirectory() const noexcept { return _gameDirectory; }

const std::shared_ptr<const core::vfs::FileSource>& Engine::gameFiles() const noexcept { return _game.files; }

bool Engine::isPacked() const noexcept { return _game.packed; }

core::config::Settings Engine::readSettings() const {
    const bool isDebug = _config.debug.value_or(kIsDebug);
    core::config::Settings settings;
    if (!_game.packed) {
        settings = scripting::loadProjectSettings(_gameDirectory / kProjectFile, _config.mode, _commandLine, isDebug);
    } else {
        std::string source;
        try {
            source = _game.files->read(kProjectFile);
        } catch (const core::asset::exceptions::AssetLoadException& error) {
            throw exceptions::RuntimeError{error.what()};
        }
        settings =
            scripting::loadProjectSettings(std::string{kProjectFile}, source, _config.mode, _commandLine, isDebug);
    }
    nameGame(settings, _gameDirectory);
    return settings;
}

ecs::RunMode Engine::mode() const noexcept { return _config.mode; }

const core::config::Settings& Engine::settings() const noexcept { return _settings; }

void Engine::requestReload() noexcept { _reloadRequested = true; }

void Engine::requestReload(const ecs::RunMode mode, std::vector<std::string> args) {
    CommandLine commandLine = parseFlags(args);  // checked here, so the caller hears of a bad command line
    {
        const std::scoped_lock lock{_pendingMutex};
        _pending = Setup{.mode = mode, .args = std::move(args), .commandLine = std::move(commandLine)};
    }
    _reloadRequested = true;
}

Engine::ScriptEvaluation Engine::evaluateScript(const std::string_view source, const ScriptTarget& target) {
    if (_scriptHost == nullptr) {
        return {.ok = false, .text = "no script is running"};
    }
    auto evaluation = _scriptHost->evaluate(source, {.self = target.self, .writable = target.writable});
    return {.ok = evaluation.ok, .text = std::move(evaluation.text)};
}

Engine::ScriptEvaluation Engine::evaluateScript(const std::string_view source) {
    return evaluateScript(source, ScriptTarget{});
}

std::vector<std::string> Engine::completeScript(const std::string_view word) {
    return completeScript(word, ScriptTarget{});
}

std::vector<std::string> Engine::completeScript(const std::string_view word, const ScriptTarget& target) {
    return _scriptHost == nullptr ? std::vector<std::string>{}
                                  : _scriptHost->complete(word, {.self = target.self, .writable = target.writable});
}

std::optional<Engine::Setup> Engine::adoptPendingSetup() {
    std::optional<Setup> next;
    {
        const std::scoped_lock lock{_pendingMutex};
        next = std::exchange(_pending, std::nullopt);
    }
    if (!next) {
        return std::nullopt;
    }
    Setup previous{.mode = _config.mode, .args = _config.args, .commandLine = _commandLine};
    useSetup(std::move(*next));
    return previous;
}

void Engine::useSetup(Setup setup) {
    _config.mode = setup.mode;
    _config.args = std::move(setup.args);
    _commandLine = std::move(setup.commandLine);
}

int Engine::run(std::optional<std::size_t> maxFrames) {
    std::size_t frames = 0;
    while (true) {
        if (maxFrames && frames >= *maxFrames) {
            return 0;
        }
        if (_app) {
            ++frames;
            _app->runFrame();
            if (const std::optional<int> code = _app->takeExit()) {
                return *code;
            }
        } else {
            // The last reload could not build the game: wait for the files to change.
            std::this_thread::sleep_for(core::reload::HotReloader::kInterval / 5);
        }
        pollHotReload();
        if (_reloadRequested.exchange(false)) {
            reload();
            if (!_app) {
                if (maxFrames) {
                    std::cerr << "[Engine] A run with a frame limit does not wait for a change in the game files: "
                                 "stopping.\n";
                    return kBuildFailedCode;
                }
                std::cerr << "[Engine] Waiting for a change in the game files to try again.\n";
            }
        }
    }
}

void Engine::pollHotReload() {
    if (!_hotReload) {
        return;
    }
    // The reloaders log what they did; a rebuild is the only thing left to do here.
    for (const core::reload::ReloadReport& report : _reloader.update(core::reload::HotReloader::Clock::now())) {
        if (report.needsRebuild()) {
            requestReload();
        }
    }
}

void Engine::reload() {
    std::optional<Setup> previous = adoptPendingSetup();
    std::cerr << "[Engine] Reloading the game: the world is built again, its entities, resources and connections "
                 "are lost.\n";
    if (previous) {
        try {
            build();
            return;
        } catch (const std::exception& error) {
            std::cerr << std::format(
                "[Engine] The game cannot be built in the new mode: {}\n[Engine] Going back to "
                "the previous one.\n",
                error.what());
            useSetup(std::move(*previous));
        }
    }
    try {
        build();
    } catch (const std::exception& error) {
        if (!_hotReload) {
            throw;
        }
        _app.reset();
        _scriptHost = nullptr;
        std::cerr << std::format("[Engine] The game cannot be built: {}\n", error.what());
    }
}

void Engine::addHotReloadSources() {
    _reloader.add(std::make_unique<reload::ProjectReloadable>(std::filesystem::path{kProjectFile}, [this] {
        // The project is read again and its network settings checked, without building anything.
        const core::config::Settings settings = readSettings();
        static_cast<void>(makeNetworkPlugin(_config.mode, settings, _commandLine, _config.debug.value_or(kIsDebug)));
    }));
    _reloader.add(std::make_unique<reload::ScriptsReloadable>(_gameDirectory, std::filesystem::path{kScriptsDirectory},
                                                              [this] { return _scriptHost; }));
}

void Engine::registerLibraries() {
    const std::filesystem::path plugins = core::system::getExecutableDirectory() / kPluginsDirectory;
    _loader.load(plugins);
    // Only a client whose game opens a window or plays sound loads what does it, and only the backends it names: a
    // server never does, and two backends carrying the same library (GLFW) never meet in one process.
    if (runsClient(_config.mode)) {
        _loader.loadFiles(plugin::backendLibraries(plugins / kBackendsDirectory, _settings, kBackendSettings));
    }
    _loader.load(_gameDirectory / kPluginsDirectory, plugin::gamePluginVerifier());
    ecs::PluginRegistry& registry = _app->getPluginRegistry();
    registry.setSettings(_settings);
    core::library::LibraryRegistrar registrar;
    registrar.provide(ecs::kPluginRegistryService, registry);
    _loader.registerInto(registrar);
}

void Engine::addListedPlugins() {
    const ecs::PluginRegistry& plugins = _app->getPluginRegistry();
    std::vector<std::string> names = _settings.getStringList(kPluginsSetting, {});
    for (const std::string& extra : _config.extraPlugins) {
        if (std::ranges::find(names, extra) == names.end()) {
            names.push_back(extra);
        }
    }
    for (const std::string& name : names) {
        std::unique_ptr<ecs::Plugin> plugin = plugins.create(name);
        if (plugin == nullptr) {
            std::string known;
            for (const std::string& registered : plugins.getNames()) {
                known += std::format("{}'{}'", known.empty() ? "" : ", ", registered);
            }
            throw exceptions::RuntimeError{std::format("Invalid setting '{}': unknown plugin '{}' (known: {})",
                                                       kPluginsSetting, name, known.empty() ? "none" : known)};
        }
        _app->addPlugin(std::move(plugin));
    }
}

void Engine::build() {
    // The old App goes first: its window and its socket must be released before the new one opens them again. What
    // follows files may point into it, so it is forgotten first. The project and the scripts are followed before the
    // project is read, so a project that fails to build is still watched.
    _reloader.clear();
    _scriptHost = nullptr;
    _app.reset();
    // A pack has no files to follow: an exported game never reloads itself.
    if (!_game.packed) {
        addHotReloadSources();
    }
    const bool isDebug = _config.debug.value_or(kIsDebug);
    _settings = readSettings();
    _hotReload = _settings.getBool(kHotReloadSetting, isDebug);
    if (_game.packed && _hotReload) {
        std::cerr << "[Engine] Hot reload is off: the game is a pack, which has no files to follow.\n";
        _hotReload = false;
    }
    // Checked before anything is built, so a wrong address fails before a window opens; added last, see the class.
    std::unique_ptr<ecs::Plugin> networkPlugin = makeNetworkPlugin(_config.mode, _settings, _commandLine, isDebug);
    const bool client = runsClient(_config.mode);
    const bool server = runsServer(_config.mode);
    const aylo::core::config::Settings scripts = _settings.section("scripts");
    scripts.checkKeys({"main", "server", "client", "aliases", "native", "timeout", "memoryLimit"}, "scripts");
    const bool nativeCode = scripts.getBool("native", true);
    const std::chrono::milliseconds callBudget = readScriptTimeout(scripts);
    const std::size_t memoryLimit = readScriptMemoryLimit(scripts);
    const std::shared_ptr<const core::vfs::FileSource> scriptFiles =
        _game.packed ? std::make_shared<core::vfs::SubSource>(_game.files, std::string{kScriptsDirectory}) : nullptr;
    std::map<std::string, std::string> aliases =
        readScriptAliases(scripts, _gameDirectory / kScriptsDirectory, scriptFiles);
    std::optional<std::string> mainEntry = readScriptEntry(scripts, "main");
    std::optional<std::string> serverEntry = readScriptEntry(scripts, "server");
    std::optional<std::string> clientEntry = readScriptEntry(scripts, "client");
    const std::filesystem::path assets = _gameDirectory / kAssetsDirectory;
    // A server can run with the scripts alone; a client always draws something, so it is checked before a window opens.
    // A pack holds the assets it was exported with.
    if (client && !_game.packed && !std::filesystem::is_directory(assets)) {
        throw exceptions::RuntimeError{
            std::format("No assets in '{}': it has no {} directory", _gameDirectory.string(), kAssetsDirectory)};
    }
    _app = std::make_unique<ecs::App>(_config.mode);

    ecs::App& app = *_app;
    // Inserted first, so every plugin finds it when it is built.
    if (_game.packed) {
        app.getWorld().getResources().insert(plugins::AssetDirectory{
            .source = std::make_shared<core::vfs::SubSource>(_game.files, std::string{kAssetsDirectory})});
    } else {
        app.getWorld().getResources().insert(
            plugins::AssetDirectory{.path = assets, .source = std::make_shared<core::vfs::DirectorySource>(assets)});
    }
    if (_hotReload) {
        app.getWorld().getResources().insert(plugins::HotReload{.reloader = &_reloader});
    }

    registerLibraries();
    app.addPlugins(ecs::MinimalPlugins{});
    // Every game has it, and the Engine links it no more than the others: it comes from its library like a listed one.
    app.addPlugin(plugin::createRequiredPlugin(
        app.getPluginRegistry(), kTransformPlugin,
        {core::system::getExecutableDirectory() / kPluginsDirectory, _gameDirectory / kPluginsDirectory}));
    if (_config.addPlugins) {
        _config.addPlugins(app, _settings);
    }
    if (_config.mode == ecs::RunMode::kStandalone) {
        app.addPlugin(replication::standalone::StandalonePlugin{});
    } else {
        app.addPlugin(replication::ReplicationPlugin{});
    }
    addListedPlugins();
    if (_config.setup) {
        _config.setup(*this);
    }
    app.addPlugin(aylo::scripting::ScriptingPlugin{{
        .directory = _gameDirectory / kScriptsDirectory,
        .source = scriptFiles,
        .main = std::move(mainEntry),
        .server = std::move(serverEntry),
        .client = std::move(clientEntry),
        .aliases = std::move(aliases),
        .options = {.isClient = client,
                    .isServer = server,
                    .isDebug = isDebug,
                    .nativeCode = nativeCode,
                    .memoryLimit = memoryLimit,
                    .callBudget = callBudget},
        .onBuilt = [this](aylo::scripting::ScriptHost& host) { _scriptHost = &host; },
    }});
    if (networkPlugin) {
        app.addPlugin(std::move(networkPlugin));
    }
    if (_config.mode == ecs::RunMode::kClient || _config.mode == ecs::RunMode::kStandalone) {
        addNetworkInput(app);
    }
}
}  // namespace aylo::engine

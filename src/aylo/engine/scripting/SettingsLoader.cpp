/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SettingsLoader
*/

#include "SettingsLoader.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "aylo/core/config/Settings.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/engine/CommandLine.hpp"
#include "aylo/engine/Exceptions.hpp"
#include "aylo/luau/Function.hpp"
#include "aylo/luau/Result.hpp"
#include "aylo/luau/Runtime.hpp"
#include "aylo/luau/RuntimeConfig.hpp"
#include "aylo/luau/Script.hpp"
#include "aylo/luau/Table.hpp"
#include "aylo/luau/Value.hpp"
#include "aylo/luau/ValuePack.hpp"

namespace {
/// How long `project.luau` may run, and how much it may hold: it only assigns settings, so a loop or an allocation
/// that goes past these is a mistake, reported instead of freezing the start. Fixed: the scripts' own limits are
/// settings it sets.
constexpr std::chrono::milliseconds kProjectTimeout{5000};
constexpr std::size_t kProjectMemoryLimit = std::size_t{256} * 1024 * 1024;

/// @return A runtime with the pure standard libraries (no `os`, no `debug`) and no optimization: project scripts only
/// assign settings, but may use `table`, `string`, `math`, `tonumber`...
aylo::luau::Runtime createRuntime() {
    using Libs = aylo::luau::RuntimeConfig::Libs;
    const aylo::luau::RuntimeConfig config{
        .libs = Libs::kBase | Libs::kMath | Libs::kTable | Libs::kString | Libs::kCoroutine | Libs::kBit32 |
                Libs::kUtf8 | Libs::kBuffer | Libs::kVector,
        .optimizationLevel = aylo::luau::RuntimeConfig::OptimizationLevel::kDisabled,
        .memoryLimit = kProjectMemoryLimit,
        .callBudget = kProjectTimeout,
    };
    return aylo::luau::expect(aylo::luau::Runtime::create(config));
}

/// @return A new empty table of the runtime's state.
aylo::luau::Table createTable(const aylo::luau::Runtime& runtime) {
    return aylo::luau::expect(aylo::luau::Table::create(runtime));
}

/// @throws aylo::engine::exceptions::RuntimeError Always, naming the offending setting.
[[noreturn]] void fail(const std::string_view key, const std::string_view reason) {
    throw aylo::engine::exceptions::RuntimeError{std::format("Invalid setting '{}': {}", key, reason)};
}

void addTable(aylo::core::config::Settings& settings, const aylo::luau::Table& table, const std::string& prefix);

/// Reads the `size` elements of a Luau array, which must all hold a `T`.
template <typename T>
std::vector<T> readList(const aylo::luau::Table& table, const std::size_t size, const std::string& key) {
    std::vector<T> list;

    list.reserve(size);
    for (std::size_t index = 1; index <= size; ++index) {
        const aylo::luau::Value element = aylo::luau::expect(table.getAt(index));
        const T* item = element.tryAs<T>();

        if (!item) {
            fail(key, std::format("element {} does not have the type of the first one", index));
        }
        list.push_back(*item);
    }
    return list;
}

/// Stores a Luau array as a list of numbers or of strings, according to its first element.
void addList(aylo::core::config::Settings& settings, const aylo::luau::Table& table, const std::size_t size,
             const std::string& key) {
    if (aylo::luau::expect(table.count()) != size) {
        fail(key, "an array cannot have named keys or holes");
    }
    if (const aylo::luau::Value first = aylo::luau::expect(table.getAt(1)); first.is<double>()) {
        settings.set(key, readList<double>(table, size, key));
    } else if (first.is<std::string>()) {
        settings.set(key, readList<std::string>(table, size, key));
    } else {
        fail(key, "an array can only hold numbers or strings");
    }
}

/// Stores one value under `key`: a scalar, an array, or the flattened entries of a table.
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the settings table.
void addValue(aylo::core::config::Settings& settings, const aylo::luau::Value& value, const std::string& key) {
    if (const auto* flag = value.tryAs<bool>()) {
        settings.set(key, *flag);
    } else if (const auto* number = value.tryAs<double>()) {
        settings.set(key, *number);
    } else if (const auto* text = value.tryAs<std::string>()) {
        settings.set(key, *text);
    } else if (const auto* table = value.tryAs<aylo::luau::Table>()) {
        if (const std::size_t size = aylo::luau::expect(table->size()); size > 0) {
            addList(settings, *table, size, key);
        } else if (aylo::luau::expect(table->count()) > 0) {
            addTable(settings, *table, key);
        }
        // An empty table is an empty group (one a script only read): nothing to store.
    } else {
        fail(key, "a setting can only be a boolean, a number, a string, an array or a table");
    }
}

/// Flattens the named entries of a table into dotted keys under `prefix`.
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the settings table.
void addTable(aylo::core::config::Settings& settings, const aylo::luau::Table& table, const std::string& prefix) {
    for (const auto& [name, value] : aylo::luau::expect(table.entries())) {
        const auto* part = name.tryAs<std::string>();

        if (part == nullptr) {
            fail(prefix.empty() ? "settings" : prefix,
                 "keys must be strings (an array cannot be mixed with named keys)");
        }
        addValue(settings, value, prefix.empty() ? *part : std::format("{}.{}", prefix, *part));
    }
}
}  // namespace

namespace aylo::engine::scripting {
SettingsLoader::SettingsLoader()
    : _runtime{createRuntime()},
      _engineTable{createTable(_runtime)},
      _settingsTable{createTable(_runtime)},
      _groupMetatable{createTable(_runtime)},
      _createGroup{luau::Function::create(_runtime, "settings.__index")} {
    // `__index(group, key)` runs when a script reads a key a group does not have: it stores an empty group under it
    // and returns it, with the same metatable, so that `a.b.c = 1` works at any depth.
    _createGroup.bind(
        [&runtime = _runtime, metatable = _groupMetatable](const luau::Table& group, const std::string& key) {
            luau::Table parent = group;
            luau::Table child = luau::expect(luau::Table::create(runtime));
            luau::ValuePack result;

            luau::expect(child.setMetatable(metatable));
            luau::expect(parent.set(key, luau::Value{child}));
            result.add(luau::Value{child});
            return result;
        });
    luau::expect(_groupMetatable.set("__index", luau::Value{_createGroup.ref()}));
    luau::expect(_settingsTable.setMetatable(_groupMetatable));
    luau::expect(_engineTable.set("settings", {_settingsTable}));
    luau::expect(_engineTable.set("args", {createTable(_runtime)}));
    luau::expect(_runtime.globals().set("engine", luau::Value{_engineTable}));
}

// ReSharper disable once CppMemberFunctionMayBeConst
core::config::Settings SettingsLoader::build() {
    core::config::Settings settings;

    addTable(settings, _settingsTable, "");
    return settings;
}

void SettingsLoader::setFlag(const std::string_view name, bool enabled) {
    luau::expect(_engineTable.set(name, {enabled}));
}

void SettingsLoader::setArgs(const CommandLine& commandLine) {
    luau::Table args = createTable(_runtime);

    for (const auto& [name, text] : commandLine.flags()) {
        const luau::Value value =
            std::visit([](auto item) { return luau::Value{std::move(item)}; }, CommandLine::toValue(text));

        luau::expect(args.set(name, value));
    }
    luau::expect(_engineTable.set("args", {args}));
}

// ReSharper disable once CppMemberFunctionMayBeConst
void SettingsLoader::load(const std::filesystem::path& path) {
    const luau::Script script = aylo::luau::expect(_runtime.load(path));

    luau::expect(_runtime.run(script));
}

void SettingsLoader::load(std::string name, const std::string& source) {
    const luau::Script script = aylo::luau::expect(_runtime.load(std::move(name), source));

    luau::expect(_runtime.run(script));
}

namespace {
/// @brief Gives @p loader the flags of @p mode, @p isDebug and the command line.
void setUp(SettingsLoader& loader, const ecs::RunMode mode, const CommandLine& commandLine, const bool isDebug) {
    loader.setFlag("isClient", ecs::runsIn(ecs::Side::kClient, mode));
    loader.setFlag("isServer", ecs::runsIn(ecs::Side::kServer, mode));
    loader.setFlag("isDebug", isDebug);
    loader.setArgs(commandLine);
}
}  // namespace

core::config::Settings loadProjectSettings(const std::filesystem::path& projectFile, const ecs::RunMode mode,
                                           const CommandLine& commandLine, const bool isDebug) {
    SettingsLoader loader;
    setUp(loader, mode, commandLine, isDebug);
    loader.load(projectFile);
    return loader.build();
}

core::config::Settings loadProjectSettings(std::string name, const std::string& source, const ecs::RunMode mode,
                                           const CommandLine& commandLine, const bool isDebug) {
    SettingsLoader loader;
    setUp(loader, mode, commandLine, isDebug);
    loader.load(std::move(name), source);
    return loader.build();
}
}  // namespace aylo::engine::scripting

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Runtime
*/

#include "Runtime.hpp"

#include <lua.h>
#include <luacode.h>
#include <luacodegen.h>
#include <lualib.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "Bytecode.hpp"
#include "ErrorKind.hpp"
#include "Exceptions.hpp"
#include "Failure.hpp"
#include "Function.hpp"
#include "FunctionRef.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "RuntimeConfig.hpp"
#include "Script.hpp"
#include "Table.hpp"
#include "ValuePack.hpp"
#include "detail/Budget.hpp"
#include "detail/UsertypeRegistry.hpp"

namespace {
constexpr std::int32_t kLuauGlobalEnv = 0;

/// @brief Tells whether every library of @p flag is set in @p libs.
bool has(const aylo::luau::RuntimeConfig::Libs libs, const aylo::luau::RuntimeConfig::Libs flag) noexcept {
    return (libs & flag) == flag;
}

/// @brief Calls a closure in protected mode, without arguments or results.
/// @param state Lua state (or thread) to run the closure on.
/// @param closure Reference to the closure to call.
/// @return Success, or an `ErrorKind::kRuntime` error carrying the Lua error message.
aylo::luau::Result<> runClosure(lua_State* state, const aylo::luau::Ref& closure) {
    const aylo::luau::detail::CallScope scope{state};
    closure.push(state);
    if (const int status = lua_pcall(state, 0, 0, 0); status != LUA_OK) {
        const char* text = lua_tostring(state, -1);
        const std::string message{text != nullptr ? text : "error object is not a string"};
        lua_pop(state, 1);
        return aylo::luau::Failure{aylo::luau::detail::failureKind(state, status, message),
                                   std::format("unable to run script: {}", message)};
    }
    return {};
}
}  // namespace

namespace aylo::luau {
void Runtime::StateDeleter::operator()(lua_State* state) const noexcept { lua_close(state); }

int Runtime::collectGarbage(lua_State* state) {
    const char* option = luaL_optstring(state, 1, "collect");
    if (std::string_view{option} == "collect") {
        lua_gc(state, LUA_GCCOLLECT, 0);
        lua_pushinteger(state, 0);
        return 1;
    }
    if (std::string_view{option} == "count") {
        // Kilobytes in use, with the bytes beyond as a fraction, as Lua's `collectgarbage("count")`.
        constexpr double kBytesPerKilobyte = 1024.0;
        const double kilobytes = static_cast<double>(lua_gc(state, LUA_GCCOUNT, 0)) +
                                 (static_cast<double>(lua_gc(state, LUA_GCCOUNTB, 0)) / kBytesPerKilobyte);
        lua_pushnumber(state, kilobytes);
        return 1;
    }
    {
        // Built before the error, which unwinds: the string is copied onto the Lua stack and released here.
        const std::string message =
            std::format("collectgarbage: unknown option '{}' (expected 'collect' or 'count')", option);
        luaL_where(state, 1);  // the position of the script, as `error` gives it
        lua_pushlstring(state, message.data(), message.size());
        lua_concat(state, 2);
    }
    lua_error(state);
}

Runtime::Runtime(std::unique_ptr<detail::Budget> budget, lua_State* state, const RuntimeConfig config)
    : _config{config},
      _budget{std::move(budget)},
      _state{state},
      _usertypes{std::make_unique<detail::UsertypeRegistry>(state)} {
    if (_budget->limitsTime()) {
        lua_callbacks(_state.get())->interrupt = &detail::Budget::interrupt;
    }
    _config.nativeCode = _config.nativeCode && luau_codegen_supported() != 0;
    if (_config.nativeCode) {
        luau_codegen_create(_state.get());
    }
    openLibraries();
    if (has(_config.libs, RuntimeConfig::Libs::kBase)) {
        // Luau's base library no longer has `collectgarbage`, which its reference still documents: scripts get ours.
        lua_pushcfunction(_state.get(), &collectGarbage, "collectgarbage");
        lua_setglobal(_state.get(), "collectgarbage");
    }
}

void Runtime::openLibraries() {
    if (_config.libs == RuntimeConfig::Libs::kStandard) {
        luaL_openlibs(_state.get());
        return;
    }
    using Libs = RuntimeConfig::Libs;
    struct Entry {
        Libs flag;
        const char* name;
        lua_CFunction open;
    };
    const std::array<Entry, 11> entries{{
        {.flag = Libs::kBase, .name = "", .open = luaopen_base},
        {.flag = Libs::kMath, .name = LUA_MATHLIBNAME, .open = luaopen_math},
        {.flag = Libs::kTable, .name = LUA_TABLIBNAME, .open = luaopen_table},
        {.flag = Libs::kString, .name = LUA_STRLIBNAME, .open = luaopen_string},
        {.flag = Libs::kCoroutine, .name = LUA_COLIBNAME, .open = luaopen_coroutine},
        {.flag = Libs::kBit32, .name = LUA_BITLIBNAME, .open = luaopen_bit32},
        {.flag = Libs::kUtf8, .name = LUA_UTF8LIBNAME, .open = luaopen_utf8},
        {.flag = Libs::kOs, .name = LUA_OSLIBNAME, .open = luaopen_os},
        {.flag = Libs::kDebug, .name = LUA_DBLIBNAME, .open = luaopen_debug},
        {.flag = Libs::kBuffer, .name = LUA_BUFFERLIBNAME, .open = luaopen_buffer},
        {.flag = Libs::kVector, .name = LUA_VECLIBNAME, .open = luaopen_vector},
    }};
    for (const auto& [flag, name, open] : entries) {
        if (has(_config.libs, flag)) {
            lua_pushcfunction(_state.get(), open, nullptr);
            lua_pushstring(_state.get(), name);
            lua_call(_state.get(), 1, 0);
        }
    }
}

Runtime::~Runtime() noexcept = default;

Runtime::Runtime(Runtime&& other) noexcept = default;

Runtime& Runtime::operator=(Runtime&& other) noexcept {
    if (this != &other) {
        // The handlers of the usertypes go before the state they belong to is closed.
        _usertypes.reset();
        _config = other._config;
        _sandboxed = other._sandboxed;
        _state = std::move(other._state);
        _budget = std::move(other._budget);
        _usertypes = std::move(other._usertypes);
    }
    return *this;
}

Result<Bytecode> Runtime::compile(const std::string& source) const {
    std::size_t size = 0;
    lua_CompileOptions options = {
        .optimizationLevel = std::to_underlying(_config.optimizationLevel),
        .debugLevel = 1,  // Keeps the line numbers, which error messages carry.
    };

    // NOLINTNEXTLINE(*-avoid-c-arrays)
    auto result = CPtr<char[]>{luau_compile(source.data(), source.size(), &options, &size)};

    // On a compilation error, Luau returns a buffer made of a null byte followed by the error message.
    if (size == 0) {
        return Failure{ErrorKind::kCompilation, "unable to compile script"};
    }
    // NOLINTNEXTLINE(*-pro-bounds-avoid-unchecked-container-access)
    if (result[0] == 0) {
        // The message is not null-terminated: its length is the buffer's, minus the null byte.
        // NOLINTNEXTLINE(*-pro-bounds-pointer-arithmetic)
        const std::string_view message{result.get() + 1, size - 1};
        return Failure{ErrorKind::kCompilation, std::format("unable to compile script: {}", message)};
    }
    return Bytecode{std::move(result), size};
}

Result<Runtime> Runtime::create(const RuntimeConfig config) {
    auto budget = std::make_unique<detail::Budget>(config.memoryLimit, config.callBudget);
    lua_State* state = lua_newstate(&detail::Budget::allocate, budget.get());
    if (state == nullptr) {
        return Failure{ErrorKind::kUnknown, "unable to create lua state"};
    }
    try {
        return Runtime{std::move(budget), state, config};
    } catch (const std::exception& e) {
        return Failure{ErrorKind::kUnknown, std::format("unable to create runtime: {}", e.what())};
    } catch (...) {
        return Failure{ErrorKind::kUnknown, "unable to create runtime"};
    }
}

detail::UsertypeRegistry& Runtime::usertypes() const { return *_usertypes; }

Result<std::string> Runtime::precompile(const std::string& source) const {
    Result<Bytecode> bytecode = compile(source);

    if (!bytecode) {
        return forwardError<std::string>(std::move(bytecode));
    }
    std::string chunk{kBytecodeHeader};
    chunk.append(bytecode->data().get(), bytecode->size());
    return chunk;
}

Result<Script> Runtime::load(std::string name, const std::string& source) const {
    if (std::string_view{source}.starts_with(kBytecodeHeader)) {
        // Precompiled (precompile()): loaded as it is. Luau does not check bytecode, so it must come from where the
        // engine itself comes from, such as the pack of an exported game.
        const std::string_view bytecode = std::string_view{source}.substr(kBytecodeHeader.size());
        return loadBytecode(std::move(name), bytecode.data(), bytecode.size());
    }
    Result<Bytecode> bytecode = compile(source);

    if (!bytecode) {
        return forwardError<Script>(std::move(bytecode));
    }
    return loadBytecode(std::move(name), bytecode->data().get(), bytecode->size());
}

Result<Script> Runtime::loadBytecode(std::string name, const char* bytecode, const std::size_t size) const {
    lua_State* threadState = lua_newthread(_state.get());
    Ref thread = Ref::pop(_state.get());

    if (_sandboxed) {
        // The thread gets its own global table, which falls back on the shared (frozen) one.
        luaL_sandboxthread(threadState);
    }

    // The '=' prefix makes Luau use the name as it is in messages (`Shield.luau:3:`), not as `[string
    // "Shield.luau"]:3:`.
    const std::string chunkName{"=" + name};
    const std::int32_t result = luau_load(threadState, chunkName.c_str(), bytecode, size, kLuauGlobalEnv);

    if (result != 0) {
        const std::string message{lua_tostring(threadState, -1)};
        lua_pop(threadState, 1);
        return Failure{ErrorKind::kRuntime, std::format("unable to load script '{}': {}", name, message)};
    }

    if (_config.nativeCode) {
        luau_codegen_compile(threadState, -1);
    }
    Ref closure = Ref::pop(threadState);
    return Script{threadState, std::move(thread), std::move(closure), std::move(name)};
}

Result<Script> Runtime::load(const std::filesystem::path& path) const {
    std::ifstream file{path, std::ios::binary};

    if (!file) {
        return Failure{ErrorKind::kUnknown, std::format("unable to open script '{}'", path.string())};
    }
    const std::string source{std::istreambuf_iterator{file}, std::istreambuf_iterator<char>{}};

    if (file.bad()) {
        return Failure{ErrorKind::kUnknown, std::format("unable to read script '{}'", path.string())};
    }
    return load(path.filename().string(), source);
}

Result<> Runtime::run(const Ref& closure) const { return runClosure(_state.get(), closure); }

// ReSharper disable once CppMemberFunctionMayBeStatic
// NOLINTNEXTLINE(*-convert-member-functions-to-static)
Result<> Runtime::run(const Script& script) const { return runClosure(script.state(), script.closure()); }

// NOLINTNEXTLINE(*-convert-member-functions-to-static)
Result<ValuePack> Runtime::call(const Script& script, const ValuePack& args) const {
    lua_State* const state = script.state();
    if (lua_checkstack(state, 2) == 0) {
        return Failure{ErrorKind::kStackOverflow, "Lua stack overflow"};
    }
    script.closure().push(state);
    auto function = detail::readFunctionRef(state);
    lua_pop(state, 1);
    if (!function) {
        return forwardError<ValuePack>(std::move(function));
    }
    return Function::fromRef(*function, std::string{script.name()}).call(args);
}

void Runtime::sandbox() {
    if (_sandboxed) {
        return;
    }
    luaL_sandbox(_state.get());
    _sandboxed = true;
}

bool Runtime::isNative() const noexcept { return _config.nativeCode; }

std::size_t Runtime::memoryInUse() const noexcept { return _budget->used(); }

bool Runtime::isSandboxed() const noexcept { return _sandboxed; }

Table Runtime::globals() const {
    lua_pushvalue(_state.get(), LUA_GLOBALSINDEX);
    auto result = detail::readTable(_state.get());
    lua_pop(_state.get(), 1);
    if (!result) {
        // if globals' table is nil, there is a big problem
        throw exceptions::InvalidState{};
    }
    return *result;
}
}  // namespace aylo::luau

namespace aylo::luau::detail {
lua_State* rawState(const Runtime& runtime) noexcept { return runtime._state.get(); }
}  // namespace aylo::luau::detail

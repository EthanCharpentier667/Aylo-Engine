/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StackAccess
*/

#include "StackAccess.hpp"

#include <lua.h>
#include <luaconf.h>

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "Exceptions.hpp"
#include "Value.hpp"
#include "detail/Stack.hpp"
#include "detail/UsertypeData.hpp"

namespace aylo::luau {
// Luau guarantees a C function LUA_MINSTACK free slots when it is called: the first pushes need no check.
Pusher::Pusher(lua_State* state) noexcept : _state{state}, _room{LUA_MINSTACK} {}

void Pusher::reserve(const int slots) const {
    if (_count + slots <= _room) {
        return;
    }
    if (lua_checkstack(_state, slots) == 0) {
        throw exceptions::RuntimeError{"stack overflow: too many results"};
    }
}

void Pusher::number(const double number) {
    reserve(1);
    lua_pushnumber(_state, number);
    ++_count;
}

void Pusher::boolean(const bool flag) {
    reserve(1);
    lua_pushboolean(_state, flag ? 1 : 0);
    ++_count;
}

void Pusher::nil() {
    reserve(1);
    lua_pushnil(_state);
    ++_count;
}

void Pusher::value(const Value& value) {
    reserve(1);
    if (const auto pushed = detail::pushValue(_state, value); !pushed) {
        throw exceptions::RuntimeError{std::string{pushed.error().message()}};
    }
    ++_count;
}

void Pusher::record(const std::span<const std::string_view> keys, const std::span<const double> numbers) {
    if (keys.size() != numbers.size()) {
        throw exceptions::InvalidOperation{"a record needs one number per key"};
    }
    reserve(2);
    lua_createtable(_state, 0, static_cast<int>(keys.size()));
    auto number = numbers.begin();
    for (const std::string_view key : keys) {
        lua_pushlstring(_state, key.data(), key.size());
        lua_pushnumber(_state, *number++);
        lua_rawset(_state, -3);
    }
    ++_count;
}

void Pusher::pushUserdata(const detail::UsertypeData& data, void (*initialize)(void* storage, void* source),
                          void* source) {
    reserve(1);
    data.push(_state, initialize, source);
    ++_count;
}

int Pusher::count() const noexcept { return _count; }

StackSlot::StackSlot(lua_State* state, const int index) noexcept : _state{state}, _index{lua_absindex(state, index)} {}

bool StackSlot::isTable() const noexcept { return lua_istable(_state, _index); }

std::optional<double> StackSlot::number() const noexcept {
    if (lua_type(_state, _index) != LUA_TNUMBER) {
        return std::nullopt;
    }
    return lua_tonumber(_state, _index);
}

std::optional<bool> StackSlot::boolean() const noexcept {
    if (lua_type(_state, _index) != LUA_TBOOLEAN) {
        return std::nullopt;
    }
    return lua_toboolean(_state, _index) != 0;
}

Value StackSlot::value() const {
    auto value = detail::readValue(_state, _index);
    if (!value) {
        throw exceptions::RuntimeError{std::string{value.error().message()}};
    }
    return std::move(*value);
}

void StackSlot::visitEntries(const Visitor visitor, void* context) const {
    if (!isTable()) {
        return;
    }
    if (lua_checkstack(_state, 2) == 0) {
        throw exceptions::RuntimeError{"stack overflow: cannot visit a table"};
    }
    lua_pushnil(_state);
    while (lua_next(_state, _index) != 0) {
        std::optional<std::string_view> key;
        // lua_tolstring would turn a number key into a string in place, which breaks lua_next: strings only.
        if (lua_type(_state, -2) == LUA_TSTRING) {
            std::size_t length = 0;
            const char* text = lua_tolstring(_state, -2, &length);
            key = std::string_view{text, length};
        }
        const StackSlot value{_state, -1};
        if (!visitor(context, key, value)) {
            lua_pop(_state, 2);
            return;
        }
        lua_pop(_state, 1);
    }
}
}  // namespace aylo::luau

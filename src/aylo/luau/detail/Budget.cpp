/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Budget
*/

#include "Budget.hpp"

#include <lua.h>

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <string>
#include <string_view>

#include "aylo/luau/ErrorKind.hpp"

namespace aylo::luau::detail {
Budget::Budget(const std::size_t memoryLimit, const Clock::duration callBudget) noexcept
    : _memoryLimit{memoryLimit}, _callBudget{callBudget} {}

void* Budget::allocate(void* userdata, void* block, const std::size_t oldSize, const std::size_t newSize) noexcept {
    auto* budget = static_cast<Budget*>(userdata);
    const std::size_t previous = block != nullptr ? oldSize : 0;

    if (newSize == 0) {
        std::free(block);  // NOLINT(*-no-malloc, *-owning-memory): the contract of lua_Alloc
        budget->_used -= previous;
        return nullptr;
    }
    if (budget->_memoryLimit != 0 && newSize > previous && budget->_used - previous + newSize > budget->_memoryLimit) {
        return nullptr;
    }
    void* moved = std::realloc(block, newSize);  // NOLINT(*-no-malloc, *-owning-memory): the contract of lua_Alloc
    if (moved != nullptr) {
        budget->_used = budget->_used - previous + newSize;
    }
    return moved;
}

void Budget::interrupt(lua_State* state, const int gc) {
    if (gc >= 0) {
        return;
    }
    Budget* budget = of(state);
    if (budget == nullptr || budget->_depth == 0 || budget->_callBudget == Clock::duration::zero()) {
        return;
    }
    if (!budget->_timedOut) {
        if (++budget->_sinceClock < kClockPeriod) {
            return;
        }
        budget->_sinceClock = 0;
        if (Clock::now() < budget->_deadline) {
            return;
        }
        budget->_timedOut = true;
    }
    {
        // In a block of its own: the message is on the Lua stack, and nothing of C++ is left to unwind, when
        // lua_error leaves the function.
        const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(budget->_callBudget).count();
        std::string message = std::format("the script ran longer than its budget of {} ms", milliseconds);
        lua_Debug where{};
        if (lua_getinfo(state, 0, "sl", &where) != 0 && where.currentline > 0) {
            message = std::format("{}:{}: {}", where.short_src, where.currentline, message);
        }
        lua_pushlstring(state, message.data(), message.size());
    }
    lua_error(state);
}

Budget* Budget::of(lua_State* state) noexcept {
    void* userdata = nullptr;
    const lua_Alloc allocator = lua_getallocf(state, &userdata);
    return allocator == &allocate ? static_cast<Budget*>(userdata) : nullptr;
}

bool Budget::limitsTime() const noexcept { return _callBudget != Clock::duration::zero(); }

std::size_t Budget::used() const noexcept { return _used; }

Budget::Clock::duration Budget::callBudget() const noexcept { return _callBudget; }

bool Budget::timedOut() const noexcept { return _timedOut; }

void Budget::enter() noexcept {
    if (_depth++ == 0) {
        _deadline = Clock::now() + _callBudget;
        _sinceClock = 0;
        _timedOut = false;
    }
}

void Budget::leave() noexcept {
    if (_depth == 0) {
        return;
    }
    --_depth;
    if (_depth == 0) {
        _timedOut = false;
    }
}

CallScope::CallScope(lua_State* state) noexcept : _budget{Budget::of(state)} {
    if (_budget != nullptr) {
        _budget->enter();
    }
}

CallScope::~CallScope() noexcept {
    if (_budget != nullptr) {
        _budget->leave();
    }
}

ErrorKind failureKind(lua_State* state, const int status, const std::string_view message) {
    if (const Budget* budget = Budget::of(state); budget != nullptr && budget->timedOut()) {
        return ErrorKind::kTimeout;
    }
    if (status == LUA_ERRMEM) {
        lua_gc(state, LUA_GCCOLLECT, 0);
        return ErrorKind::kOutOfMemory;
    }
    if (status == LUA_ERRERR || message.ends_with("stack overflow")) {
        return ErrorKind::kStackOverflow;
    }
    return ErrorKind::kRuntime;
}
}  // namespace aylo::luau::detail

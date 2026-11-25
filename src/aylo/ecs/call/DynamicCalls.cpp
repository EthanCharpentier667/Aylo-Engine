/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DynamicCalls
*/

#include "DynamicCalls.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "aylo/ecs/call/CallInfo.hpp"
#include "aylo/ecs/call/Calls.hpp"
#include "aylo/ecs/exceptions/CallException.hpp"
#include "aylo/ecs/system/ClientWrites.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/system/SystemParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
DynamicCalls::DynamicCalls(DynamicCallsState& state, const SystemContext& context) noexcept
    : _state{&state}, _context{context} {}

std::size_t DynamicCalls::getCount() const noexcept { return _state->infos.size(); }

const CallInfo& DynamicCalls::getInfo(const std::size_t index) const noexcept {
    assert(index < _state->infos.size());
    // NOLINTNEXTLINE(*-avoid-unchecked-container-access): index < getCount(), asserted above.
    return *_state->infos[index];
}

std::optional<std::size_t> DynamicCalls::find(const CallId id) const noexcept {
    const auto found = std::ranges::find(_state->infos, id, [](const CallInfo* info) { return info->id; });
    if (found == _state->infos.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(_state->infos.begin(), found));
}

std::size_t DynamicCalls::add(const CallId id) const {
    if (const std::optional<std::size_t> known = find(id)) {
        return *known;
    }
    const CallInfo* info = _context.world->getCalls().findById(id);
    if (info == nullptr) {
        throw exceptions::CallException{std::format("Call id {} is not registered", id)};
    }
    if (_context.access == nullptr) {
        throw exceptions::CallException{
            std::format("Call '{}' cannot be added: the calls are not made from a system's run", info->name)};
    }
    if (_context.side == Side::kClient) {
        if (const auto write = findClientWrite(_context.world->getComponents().getRegistry(), info->access)) {
            throw exceptions::CallException{
                std::format("Call '{}' writes component '{}', which is {}: a system running on clients cannot make it",
                            info->name, write->component, write->reason)};
        }
    }
    _state->owned.push_back(info->body->bind(*_context.world));
    _state->bodies.push_back(_state->owned.back().get());
    _state->infos.push_back(info);
    _context.access->merge(info->access);
    return _state->infos.size() - 1;
}

void DynamicCalls::invoke(const std::size_t index, const std::span<const std::byte> arguments,
                          const std::span<std::byte> result) const {
    assert(index < _state->bodies.size());
    checkCallBytes(getInfo(index), arguments, result);
    _state->bodies.at(index)->invoke(_context, arguments, result);
}

SystemParam<DynamicCalls>::State SystemParam<DynamicCalls>::init(World& world, Config config) {
    State state;
    const std::vector<CallId> ids = std::move(config.calls);
    for (const CallId id : ids) {
        const CallInfo* info = world.getCalls().findById(id);
        if (info == nullptr) {
            throw exceptions::CallException{std::format("A call of the system (id {}) is not registered", id)};
        }
        state.infos.push_back(info);
        const auto shared = std::ranges::find(state.infos, info);
        if (shared != std::prev(state.infos.end())) {
            state.bodies.push_back(state.bodies.at(static_cast<std::size_t>(shared - state.infos.begin())));
            continue;
        }
        state.owned.push_back(info->body->bind(world));
        state.bodies.push_back(state.owned.back().get());
    }
    return state;
}

DynamicCalls SystemParam<DynamicCalls>::get(State& state, const SystemContext& context) noexcept {
    return DynamicCalls{state, context};
}

void SystemParam<DynamicCalls>::addAccess(const State& state, SystemAccess& access) {
    for (const CallInfo* info : state.infos) {
        access.merge(info->access);
    }
}
}  // namespace aylo::ecs

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DynamicEvents
*/

#include "DynamicEvents.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <utility>

#include "aylo/ecs/event/EventInfo.hpp"
#include "aylo/ecs/event/EventRange.hpp"
#include "aylo/ecs/event/Events.hpp"
#include "aylo/ecs/exceptions/TypeExceptions.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/system/SystemParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
DynamicEvents::DynamicEvents(Events& events, DynamicEventsState& state, SystemAccess* access) noexcept
    : _events{&events}, _state{&state}, _access{access} {
    assert(state.uses.size() == state.cursors.size());
}

std::size_t DynamicEvents::getCount() const noexcept { return _state->uses.size(); }

const EventUse& DynamicEvents::getUse(const std::size_t index) const noexcept {
    assert(index < _state->uses.size());
    // NOLINTNEXTLINE(*-avoid-unchecked-container-access): index < getCount(), asserted above.
    return _state->uses[index];
}

std::size_t DynamicEvents::add(const EventId id, const bool read, const bool send) const {
    const EventInfo* info = _events->getRegistry().findById(id);
    if (info == nullptr) {
        throw exceptions::TypeRegistrationException{std::format("Event id {} is not registered", id)};
    }
    if (_access == nullptr) {
        throw exceptions::TypeRegistrationException{
            std::format("Event '{}' cannot be added: the events are not used by a system's run", info->type.name)};
    }
    if (read || send) {
        _access->addEvent(id, send);
    }
    auto& uses = _state->uses;
    const auto known = std::ranges::find(uses, id, &EventUse::id);
    if (known != uses.end()) {
        known->read = known->read || read;
        known->send = known->send || send;
        return static_cast<std::size_t>(std::distance(uses.begin(), known));
    }
    uses.push_back({.id = id, .read = read, .send = send});
    _state->cursors.emplace_back();
    return uses.size() - 1;
}

RawEventRange DynamicEvents::read(const std::size_t index) const noexcept {
    const EventUse& use = getUse(index);
    if (!use.read) {
        return {};
    }
    // Uses of the same event share the cursor of the first one, so an event is consumed once whichever index reads it.
    const auto first = std::ranges::find(_state->uses, use.id, &EventUse::id);
    const auto cursor = static_cast<std::size_t>(std::distance(_state->uses.begin(), first));
    // NOLINTNEXTLINE(*-avoid-unchecked-container-access): use.id is in uses, which cursors mirrors.
    return _events->read(use.id, _state->cursors[cursor]);
}

bool DynamicEvents::send(const std::size_t index, const std::span<const std::byte> value) const {
    const EventUse& use = getUse(index);
    return use.send && _events->send(use.id, value);
}

SystemParam<DynamicEvents>::State SystemParam<DynamicEvents>::init(World& world, Config config) {
    for (const EventUse& use : config.events) {
        if (world.getEvents().getRegistry().findById(use.id) == nullptr) {
            throw exceptions::TypeRegistrationException{
                std::format("An event of the system (id {}) is not registered", use.id)};
        }
    }
    State state;
    state.cursors.resize(config.events.size());
    state.uses = std::move(config.events);
    return state;
}

DynamicEvents SystemParam<DynamicEvents>::get(State& state, const SystemContext& context) noexcept {
    return DynamicEvents{context.world->getEvents(), state, context.access};
}

void SystemParam<DynamicEvents>::addAccess(const State& state, SystemAccess& access) {
    for (const EventUse& use : state.uses) {
        if (!use.read && !use.send) {
            continue;  // an event the system neither reads nor sends is not accessed
        }
        access.addEvent(use.id, use.send);
    }
}
}  // namespace aylo::ecs

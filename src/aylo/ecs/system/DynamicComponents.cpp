/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DynamicComponents
*/

#include "DynamicComponents.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <iterator>
#include <optional>
#include <span>
#include <utility>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/exceptions/ComponentExceptions.hpp"
#include "aylo/ecs/system/ClientWrites.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/system/SystemParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
namespace {
/// @return The component `id`.
/// @throws exceptions::ComponentRegistrationException When it is not registered.
const ComponentInfo& checkRegistered(const Components& components, const ComponentId id) {
    const ComponentInfo* info = components.getRegistry().findById(id);
    if (info == nullptr) {
        throw exceptions::ComponentRegistrationException{
            std::format("A component of the system (id {}) is not registered", id)};
    }
    return *info;
}
}  // namespace

DynamicComponents::DynamicComponents(ComponentList& list, const SystemContext& context) noexcept
    : _list{&list}, _context{context} {}

std::size_t DynamicComponents::getCount() const noexcept { return _list->components.size(); }

const ComponentUse& DynamicComponents::getUse(const std::size_t index) const noexcept {
    assert(index < _list->components.size());
    // NOLINTNEXTLINE(*-avoid-unchecked-container-access): index < getCount(), asserted above.
    return _list->components[index];
}

std::optional<std::size_t> DynamicComponents::find(const ComponentId id) const noexcept {
    const auto& uses = _list->components;
    const auto found = std::ranges::find(uses, id, &ComponentUse::id);
    if (found == uses.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(uses.begin(), found));
}

std::size_t DynamicComponents::add(const ComponentId id, const bool write) const {
    const Components& components = _context.world->getComponents();
    const ComponentInfo& info = checkRegistered(components, id);
    if (_context.access == nullptr) {
        throw exceptions::ComponentRegistrationException{std::format(
            "Component '{}' cannot be added: the components are not used by a system's run", info.type.name)};
    }
    if (write && _context.side == Side::kClient) {
        SystemAccess wanted;
        wanted.addComponent(id, true);
        if (const auto refused = findClientWrite(components.getRegistry(), wanted)) {
            throw exceptions::ComponentRegistrationException{
                std::format("a client system cannot write '{}': it is {}", refused->component, refused->reason)};
        }
    }
    _context.access->addComponent(id, write);
    auto& uses = _list->components;
    if (const std::optional<std::size_t> known = find(id)) {
        // NOLINTNEXTLINE(*-avoid-unchecked-container-access): find() returned an index of uses.
        uses[*known].write = uses[*known].write || write;
        return *known;
    }
    uses.push_back({.id = id, .write = write});
    return uses.size() - 1;
}

bool DynamicComponents::has(const Entity entity, const std::size_t index) const noexcept {
    return _context.world->isAlive(entity) && _context.world->getComponents().has(entity, getUse(index).id);
}

std::span<const std::byte> DynamicComponents::get(const Entity entity, const std::size_t index) const noexcept {
    if (!_context.world->isAlive(entity)) {
        return {};
    }
    return std::as_const(*_context.world).getComponents().get(entity, getUse(index).id);
}

std::span<std::byte> DynamicComponents::getMut(const Entity entity, const std::size_t index) const noexcept {
    const ComponentUse& use = getUse(index);
    if (!use.write || !_context.world->isAlive(entity)) {
        return {};
    }
    return _context.world->getComponents().getMut(entity, use.id);
}

SystemParam<DynamicComponents>::State SystemParam<DynamicComponents>::init(World& world, Config config) {
    for (const ComponentUse& use : config.components) {
        static_cast<void>(checkRegistered(world.getComponents(), use.id));
    }
    return config;
}

DynamicComponents SystemParam<DynamicComponents>::get(State& state, const SystemContext& context) noexcept {
    return DynamicComponents{state, context};
}

void SystemParam<DynamicComponents>::addAccess(const State& state, SystemAccess& access) {
    for (const ComponentUse& use : state.components) {
        access.addComponent(use.id, use.write);
    }
}
}  // namespace aylo::ecs

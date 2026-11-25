/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DynamicResources
*/

#include "DynamicResources.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <utility>

#include "aylo/ecs/exceptions/ResourceExceptions.hpp"
#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/system/SystemParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
namespace {
/// @return The resource `id`, checked to be usable by a dynamic system.
/// @throws exceptions::ResourceRegistrationException When it is not registered, or is main-thread only.
const ResourceInfo& checkUsable(const Resources& resources, const ResourceId id) {
    const ResourceInfo* info = resources.getRegistry().findById(id);
    if (info == nullptr) {
        throw exceptions::ResourceRegistrationException{
            std::format("A resource of the system (id {}) is not registered", id)};
    }
    if (info->nonSend) {
        throw exceptions::ResourceRegistrationException{
            std::format("Resource '{}' is main-thread only: a dynamic system cannot use it", info->type.name)};
    }
    return *info;
}
}  // namespace

DynamicResources::DynamicResources(Resources& resources, ResourceList& list, SystemAccess* access) noexcept
    : _resources{&resources}, _list{&list}, _access{access} {}

std::size_t DynamicResources::getCount() const noexcept { return _list->resources.size(); }

const ResourceUse& DynamicResources::getUse(const std::size_t index) const noexcept {
    assert(index < _list->resources.size());
    // NOLINTNEXTLINE(*-avoid-unchecked-container-access): index < getCount(), asserted above.
    return _list->resources[index];
}

std::size_t DynamicResources::add(const ResourceId id, const bool write) const {
    const ResourceInfo& info = checkUsable(*_resources, id);
    if (_access == nullptr) {
        throw exceptions::ResourceRegistrationException{
            std::format("Resource '{}' cannot be added: the resources are not used by a system's run", info.type.name)};
    }
    auto& uses = _list->resources;
    const auto known = std::ranges::find(uses, id, &ResourceUse::id);
    _access->addResource(id, write);
    if (known != uses.end()) {
        known->write = known->write || write;
        return static_cast<std::size_t>(std::distance(uses.begin(), known));
    }
    uses.push_back({.id = id, .write = write});
    return uses.size() - 1;
}

bool DynamicResources::has(const std::size_t index) const noexcept { return _resources->has(getUse(index).id); }

std::span<const std::byte> DynamicResources::get(const std::size_t index) const noexcept {
    return std::as_const(*_resources).get(getUse(index).id);
}

std::span<std::byte> DynamicResources::getMut(const std::size_t index) noexcept {
    const ResourceUse& use = getUse(index);
    return use.write ? _resources->getMut(use.id) : std::span<std::byte>{};
}

SystemParam<DynamicResources>::State SystemParam<DynamicResources>::init(World& world, Config config) {
    for (const ResourceUse& use : config.resources) {
        static_cast<void>(checkUsable(world.getResources(), use.id));
    }
    return config;
}

DynamicResources SystemParam<DynamicResources>::get(State& state, const SystemContext& context) noexcept {
    return DynamicResources{context.world->getResources(), state, context.access};
}

void SystemParam<DynamicResources>::addAccess(const State& state, SystemAccess& access) {
    for (const ResourceUse& use : state.resources) {
        access.addResource(use.id, use.write);
    }
}
}  // namespace aylo::ecs

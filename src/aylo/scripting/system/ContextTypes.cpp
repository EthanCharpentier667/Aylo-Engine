/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ContextTypes
*/

#include "ContextTypes.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "PlainData.hpp"
#include "SystemRun.hpp"
#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/system/DynamicResources.hpp"
#include "aylo/ecs/world/World.hpp"
#include "aylo/luau/Exceptions.hpp"
#include "aylo/luau/Result.hpp"
#include "aylo/luau/Runtime.hpp"
#include "aylo/luau/Usertype.hpp"
#include "aylo/luau/Value.hpp"
#include "aylo/scripting/view/ComponentView.hpp"

namespace aylo::scripting::system {
namespace {
/// @return The index of the system's resource called `name`; one it did not declare is discovered and added, read.
/// @throws luau::exceptions::RuntimeError If no resource has this name, or a script cannot use it.
[[nodiscard]] std::size_t findOrDiscover(const ResourcesHandle& handle, const std::string_view name) {
    auto& terms = handle.system->resources;
    const auto found = std::ranges::find(terms, name, [](const ResourceTerm& term) { return term.type->name; });
    if (found != terms.end()) {
        return static_cast<std::size_t>(std::distance(terms.begin(), found));
    }
    const ecs::ResourceInfo* info = handle.system->world->getResources().getRegistry().findByName(name);
    if (info == nullptr) {
        throw luau::exceptions::RuntimeError{
            std::format("unknown resource '{}' (system '{}')", name, handle.system->name)};
    }
    if (const std::string why = whyNotPlainData(info->type, info->nonSend); !why.empty()) {
        throw luau::exceptions::RuntimeError{std::format("resource '{}' cannot be used by a script: {}", name, why)};
    }
    const std::size_t index = handle.resources->add(info->type.id, false);
    assert(index == terms.size());
    terms.push_back({.type = &info->type, .write = false, .discovered = true});
    return index;
}

/// @return What a read-only view of a discovered resource does when a field is written: the system now writes the
/// resource, which is marked changed.
[[nodiscard]] std::shared_ptr<const std::function<void()>> makeUpgrade(const ResourcesHandle& handle,
                                                                       const std::size_t index) {
    return std::make_shared<const std::function<void()>>([system = handle.system, resources = handle.resources, index] {
        ResourceTerm& term = system->resources.at(index);
        static_cast<void>(resources->add(term.type->id, true));
        term.write = true;
        static_cast<void>(resources->getMut(index));  // marks it changed
    });
}

/// @return The view of the resource called `name`, or `nil` while it has no value.
/// @throws luau::exceptions::RuntimeError If the handle is stale, or no resource a script can use has this name.
[[nodiscard]] luau::Value viewOf(const luau::Runtime& runtime, const ResourcesHandle& handle,
                                 const std::string_view name) {
    const auto scope = handle.scope.lock();
    if (scope == nullptr || scope->generation != handle.generation) {
        throw luau::exceptions::RuntimeError{
            "ctx.resources is no longer valid: it only lives during the run that received it"};
    }
    const std::size_t index = findOrDiscover(handle, name);
    if (!handle.resources->has(index)) {
        return luau::Value{luau::Value::Nil{}};
    }
    const ResourceTerm& term = handle.system->resources.at(index);
    std::span<std::byte> bytes;
    if (term.write) {
        bytes = handle.resources->getMut(index);
    } else {
        const std::span<const std::byte> read = handle.resources->get(index);
        // The view refuses writes (or, for a discovered resource, asks for write access first), so the bytes are never
        // written through it while the system only reads the resource.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        bytes = {const_cast<std::byte*>(read.data()), read.size()};
    }
    auto view = runtime.usertype<view::ComponentView>().create(view::ComponentView{
        .type = term.type,
        .bytes = bytes,
        .scope = scope,
        .generation = handle.generation,
        .readOnly = !term.write,
        .onWrite = term.discovered && !term.write ? makeUpgrade(handle, index) : nullptr,
    });
    if (!view) {
        throw luau::exceptions::RuntimeError{std::string{view.error().message()}};
    }
    return luau::Value{std::move(*view)};
}
}  // namespace

luau::Result<> registerContextTypes(luau::Runtime& runtime) {
    auto resourcesType = runtime.usertype<ResourcesHandle>("Resources");
    if (!resourcesType) {
        return tl::unexpected{std::move(resourcesType.error())};
    }
    resourcesType
        ->index([&runtime](const ResourcesHandle& handle, const std::string_view name) {
            return viewOf(runtime, handle, name);
        })
        .tostring([](const ResourcesHandle& /*handle*/) { return std::string{"Resources"}; });
    return {};
}
}  // namespace aylo::scripting::system

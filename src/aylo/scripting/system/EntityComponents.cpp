/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EntityComponents
*/

#include "EntityComponents.hpp"

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
#include <utility>

#include "SystemRun.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/exceptions/ComponentExceptions.hpp"
#include "aylo/ecs/hierarchy/Children.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/system/DynamicComponents.hpp"
#include "aylo/ecs/type/TypeTraits.hpp"
#include "aylo/luau/CallContext.hpp"
#include "aylo/luau/Exceptions.hpp"
#include "aylo/luau/Runtime.hpp"
#include "aylo/luau/Table.hpp"
#include "aylo/luau/Userdata.hpp"
#include "aylo/luau/Usertype.hpp"
#include "aylo/luau/Value.hpp"
#include "aylo/luau/ValuePack.hpp"
#include "aylo/scripting/view/ComponentView.hpp"

namespace aylo::scripting::system {
namespace {
/// @brief The entity and the component a call of `ctx:get` or `ctx:has` names.
struct Target {
    ecs::Entity entity;  ///< The entity.
    std::size_t index;   ///< The component's index in `system.entityComponents`.
};

/// @return The index of the system's component called `name`; one it did not declare is discovered and added, read.
/// @throws luau::exceptions::RuntimeError If no component has this name.
[[nodiscard]] std::size_t findOrDiscover(ScriptSystem& system, const ecs::DynamicComponents& components,
                                         const std::string_view name) {
    auto& terms = system.entityComponents;
    const auto found = std::ranges::find(terms, name, [](const ComponentTerm& term) { return term.info->type.name; });
    if (found != terms.end()) {
        return static_cast<std::size_t>(std::distance(terms.begin(), found));
    }
    const ecs::ComponentInfo* info = system.components->findByName(name);
    if (info == nullptr) {
        throw luau::exceptions::RuntimeError{std::format("unknown component '{}' (system '{}')", name, system.name)};
    }
    const std::size_t index = components.add(info->type.id, false);
    assert(index == terms.size());
    terms.push_back({.info = info, .write = false, .discovered = true});
    return index;
}

/// @return The entity given as the first argument after `ctx`, or null when it is not one.
[[nodiscard]] const ecs::Entity* findEntity(const std::span<const luau::Value> args) {
    const auto* userdata = args.size() > 1 ? std::next(args.begin(), 1)->tryAs<luau::Userdata>() : nullptr;
    return userdata != nullptr ? userdata->tryAs<ecs::Entity>() : nullptr;
}

/// @return The entity and component of `ctx:<function>(entity, name)`.
/// @throws luau::exceptions::RuntimeError If the arguments are not `ctx`, an entity and a name, or no component has
/// this name.
[[nodiscard]] Target readTarget(ScriptSystem& system, const ecs::DynamicComponents& components,
                                const luau::CallContext& arguments, const std::string_view function) {
    const std::span<const luau::Value> args = arguments.args();
    const auto* entity = findEntity(args);
    const auto* name = args.size() > 2 ? std::next(args.begin(), 2)->tryAs<std::string>() : nullptr;
    if (entity == nullptr || name == nullptr) {
        throw luau::exceptions::RuntimeError{
            std::format("ctx:{} expects an entity and the name of a component: ctx:{}(entity, \"game.Health\")",
                        function, function)};
    }
    return {.entity = *entity, .index = findOrDiscover(system, components, *name)};
}

/// @return The entity of `ctx:<function>(entity)`.
/// @throws luau::exceptions::RuntimeError If the arguments are not `ctx` and an entity.
[[nodiscard]] ecs::Entity readEntity(const luau::CallContext& arguments, const std::string_view function) {
    const auto* entity = findEntity(arguments.args());
    if (entity == nullptr) {
        throw luau::exceptions::RuntimeError{
            std::format("ctx:{} expects an entity: ctx:{}(entity)", function, function)};
    }
    return *entity;
}

/// @return The hierarchy component `T` of `entity`, read by the system (discovered like any component), or null when
/// the entity is not alive, does not have it, or no entity has one yet (the world registers it at the first parent).
template <typename T>
[[nodiscard]] const T* findHierarchy(ScriptSystem& system, const ecs::DynamicComponents& components,
                                     const ecs::Entity entity) {
    if (system.components->findByName(ecs::TypeTraits<T>::kName) == nullptr) {
        return nullptr;
    }
    return components.get<T>(entity, findOrDiscover(system, components, ecs::TypeTraits<T>::kName));
}

/// @return Why a field of the component of `term` cannot be written by the system, or an empty string when it can be.
[[nodiscard]] std::string whyNotWritable(const ScriptSystem& system, const ComponentTerm& term) {
    const std::string_view name = term.info->type.name;
    if (name == ecs::TypeTraits<ecs::Parent>::kName || name == ecs::TypeTraits<ecs::Children>::kName) {
        return std::format("'{}' cannot be written: use commands:setParent()", name);
    }
    if (!term.discovered && !term.write) {
        return std::format(
            "the view of '{}' is read-only: system '{}' declared it under 'components.read' (list it under "
            "'components.write')",
            name, system.name);
    }
    return {};
}

/// @return What a view does when a field is written: the system now writes the component (refused by the ECS for a
/// client system and a replicated or server-only one), which is marked changed on the entity.
[[nodiscard]] std::shared_ptr<const std::function<void()>> makeOnWrite(ScriptSystem& system,
                                                                       const ecs::DynamicComponents& components,
                                                                       const Target target) {
    return std::make_shared<const std::function<void()>>([&system, components, target] {
        ComponentTerm& term = system.entityComponents.at(target.index);
        if (!term.write) {
            try {
                static_cast<void>(components.add(term.info->type.id, true));
            } catch (const ecs::exceptions::ComponentRegistrationException& error) {
                throw luau::exceptions::RuntimeError{error.what()};
            }
            term.write = true;
        }
        static_cast<void>(components.getMut(target.entity, target.index));  // marks it changed
    });
}
}  // namespace

luau::ValuePack getComponent(ScriptSystem& system, const luau::Runtime& runtime,
                             const ecs::DynamicComponents& components, const luau::CallContext& arguments) {
    const Target target = readTarget(system, components, arguments, "get");
    luau::ValuePack pack;
    if (!components.has(target.entity, target.index)) {
        pack.add(luau::Value{luau::Value::Nil{}});
        return pack;
    }
    const ComponentTerm& term = system.entityComponents.at(target.index);
    const std::span<const std::byte> read = components.get(target.entity, target.index);
    std::string refused = whyNotWritable(system, term);
    auto onWrite = refused.empty() ? makeOnWrite(system, components, target)
                                   : std::make_shared<const std::function<void()>>([refused = std::move(refused)] {
                                         throw luau::exceptions::RuntimeError{refused};
                                     });
    auto view = runtime.usertype<view::ComponentView>().create(view::ComponentView{
        .type = &term.info->type,
        // A write goes through onWrite first, which takes write access and marks the component changed, or raises why
        // the view cannot be written. So the bytes are never written while the system only reads them.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        .bytes = {const_cast<std::byte*>(read.data()), read.size()},
        .scope = system.scope,
        .generation = system.scope->generation,
        .readOnly = true,
        .onWrite = std::move(onWrite),
    });
    if (!view) {
        throw luau::exceptions::RuntimeError{std::string{view.error().message()}};
    }
    pack.add(luau::Value{std::move(*view)});
    return pack;
}

luau::ValuePack getParent(ScriptSystem& system, const luau::Runtime& runtime, const ecs::DynamicComponents& components,
                          const luau::CallContext& arguments) {
    const ecs::Entity entity = readEntity(arguments, "getParent");
    luau::ValuePack pack;
    const auto* parent = findHierarchy<ecs::Parent>(system, components, entity);
    if (parent == nullptr) {
        pack.add(luau::Value{luau::Value::Nil{}});
        return pack;
    }
    auto handle = runtime.usertype<ecs::Entity>().create(parent->entity);
    if (!handle) {
        throw luau::exceptions::RuntimeError{std::string{handle.error().message()}};
    }
    pack.add(luau::Value{std::move(*handle)});
    return pack;
}

luau::ValuePack getChildren(ScriptSystem& system, const luau::Runtime& runtime,
                            const ecs::DynamicComponents& components, const luau::CallContext& arguments) {
    const ecs::Entity entity = readEntity(arguments, "getChildren");
    const auto* children = findHierarchy<ecs::Children>(system, components, entity);
    const std::span<const ecs::Entity> entities =
        children != nullptr ? std::span<const ecs::Entity>{children->entities} : std::span<const ecs::Entity>{};
    auto list = luau::Table::create(runtime, static_cast<int>(entities.size()));
    if (!list) {
        throw luau::exceptions::RuntimeError{std::string{list.error().message()}};
    }
    const auto handles = runtime.usertype<ecs::Entity>();
    double key = 1;
    for (const ecs::Entity child : entities) {
        auto handle = handles.create(child);
        if (!handle) {
            throw luau::exceptions::RuntimeError{std::string{handle.error().message()}};
        }
        if (auto set = list->set(luau::Value{key}, luau::Value{std::move(*handle)}); !set) {
            throw luau::exceptions::RuntimeError{std::string{set.error().message()}};
        }
        ++key;
    }
    luau::ValuePack pack;
    pack.add(luau::Value{std::move(*list)});
    return pack;
}

luau::ValuePack hasComponent(ScriptSystem& system, const ecs::DynamicComponents& components,
                             const luau::CallContext& arguments) {
    const Target target = readTarget(system, components, arguments, "has");
    luau::ValuePack pack;
    pack.add(luau::Value{components.has(target.entity, target.index)});
    return pack;
}
}  // namespace aylo::scripting::system

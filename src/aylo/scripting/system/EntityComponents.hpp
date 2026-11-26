/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EntityComponents
*/

#pragma once

#include "SystemRun.hpp"
#include "aylo/ecs/system/DynamicComponents.hpp"
#include "aylo/luau/CallContext.hpp"
#include "aylo/luau/Runtime.hpp"
#include "aylo/luau/ValuePack.hpp"

namespace aylo::scripting::system {
/// @brief `ctx:get(entity, name)`: a view of the entity's component, or `nil` when the entity is dead or lacks it.
///
/// @details A component the system declared under `components` keeps what it declared: one declared read gives a
/// read-only view. One it did not declare is discovered, read, and turns into a write the first time a field of its
/// view is written. A view marks the component changed when a field is written, not when it is given. Writing is
/// refused for `aylo.Parent` and `aylo.Children` (commands:setParent() changes them), and, for a system running on
/// clients, for a replicated or server-only component.
/// @param system The system being run; it records the components it discovers.
/// @param runtime The runtime that makes the view.
/// @param components The components of the run, in the order of `system.entityComponents`.
/// @param arguments `ctx`, the entity and the component's name.
/// @return The view, or `nil`.
/// @throws luau::exceptions::RuntimeError If the arguments are wrong or no component has this name.
[[nodiscard]] luau::ValuePack getComponent(ScriptSystem& system, const luau::Runtime& runtime,
                                           const ecs::DynamicComponents& components,
                                           const luau::CallContext& arguments);

/// @brief `ctx:has(entity, name)`: whether the entity is alive and has the component. The component is discovered,
/// read, as by getComponent().
/// @throws luau::exceptions::RuntimeError If the arguments are wrong or no component has this name.
[[nodiscard]] luau::ValuePack hasComponent(ScriptSystem& system, const ecs::DynamicComponents& components,
                                           const luau::CallContext& arguments);

/// @brief `ctx:getParent(entity)`: the entity's parent, or `nil` when it has none or is dead. Reads `aylo.Parent`,
/// discovered like any component.
/// @throws luau::exceptions::RuntimeError If the argument is not an entity.
[[nodiscard]] luau::ValuePack getParent(ScriptSystem& system, const luau::Runtime& runtime,
                                        const ecs::DynamicComponents& components, const luau::CallContext& arguments);

/// @brief `ctx:getChildren(entity)`: a new list of the entity's children, in the order of `aylo.Children`; empty when
/// it has none or is dead. Reads `aylo.Children`, discovered like any component. The list is a copy: it stays valid
/// after the run, but does not follow later changes of the hierarchy.
/// @throws luau::exceptions::RuntimeError If the argument is not an entity.
[[nodiscard]] luau::ValuePack getChildren(ScriptSystem& system, const luau::Runtime& runtime,
                                          const ecs::DynamicComponents& components, const luau::CallContext& arguments);
}  // namespace aylo::scripting::system

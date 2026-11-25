/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ContextTypes
*/

#pragma once

#include <cstdint>
#include <memory>

#include "SystemRun.hpp"
#include "aylo/ecs/system/DynamicResources.hpp"
#include "aylo/luau/Result.hpp"
#include "aylo/luau/Runtime.hpp"
#include "aylo/scripting/Export.hpp"
#include "aylo/scripting/view/ViewScope.hpp"

namespace aylo::scripting::system {
/// @brief What `ctx.resources` holds: the resources a system declared, reached by name.
///
/// @details Like a view, it only lives during the run that received it: the scope's generation moves on when `run`
/// returns, and every access through a handle of an older generation raises a Luau error.
struct AYLO_SCRIPTING_API ResourcesHandle {
    ScriptSystem* system{nullptr};              ///< The system, for its resources; it adds those it discovers.
    ecs::DynamicResources* resources{nullptr};  ///< The bytes of those resources, for the run.
    std::weak_ptr<view::ViewScope> scope;       ///< The scope that invalidates the handle and the views it gives.
    std::uint64_t generation{0};                ///< The scope's generation when the handle was created.
};

/// @brief Registers the `Resources` usertype of `ctx.resources` in a runtime.
///
/// @details `ctx.resources["aylo.Input"]` gives a view of the resource, like those of components: a resource declared
/// under `resources.read` stays read-only, so writing one of its fields raises a Luau error. A name the system did not
/// declare is discovered, read, and becomes written at the first write to its view; an unknown name raises a Luau
/// error. The view is `nil` while the resource has no value. A declared writable view marks the resource changed when
/// it is given, not when a field is written, since the ECS only has bytes.
/// @param runtime The runtime to register the usertype in; it must have the `Entity`, `AssetId` and `ComponentView`
/// usertypes (see view::registerViewTypes()).
/// @return Nothing, or the error of the registration.
[[nodiscard]] AYLO_SCRIPTING_API luau::Result<> registerContextTypes(luau::Runtime& runtime);
}  // namespace aylo::scripting::system

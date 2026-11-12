/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ComponentRegistration
*/

#pragma once

#include "ComponentDeclaration.hpp"
#include "ComponentLayout.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/scripting/Export.hpp"
#include "aylo/scripting/Result.hpp"

namespace aylo::scripting::component {
/// @brief Registers a script component: sorts its fields by alignment (largest first) then by name, computes the
/// offsets, the size and the alignment, and calls Components::registerType.
///
/// @details The layout depends only on the declaration, so the server and every client agree on it whatever the
/// order the fields were written in. Registering the same declaration twice returns the same id.
/// @return The layout and id, or an ErrorKind::kInvalidDeclaration error (empty name, empty or duplicate field name,
/// default value of the wrong size), kLayoutMismatch (the name is registered with another layout) or kRegistration
/// (the ECS refused it for another reason).
[[nodiscard]] AYLO_SCRIPTING_API Result<ComponentLayout> registerComponent(ecs::Components& components,
                                                                           const ComponentDeclaration& declaration);
}  // namespace aylo::scripting::component

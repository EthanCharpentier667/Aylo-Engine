/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** LightGizmos
*/

#pragma once

#include <vector>

#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "editor/Export.hpp"

namespace aylo::ecs {
class World;
}  // namespace aylo::ecs

namespace aylo::editor::scene {

/// @return The lines that show each aylo.Light3D of @p world in the Scene tab, in its colour: an arrow along a
/// directional light's -Z, three circles of a point light's range (1 without one), a spot light's cone to its range
/// (5 without one), an area light's rectangle and the way it faces. A disabled light is grey.
[[nodiscard]] AYLO_EDITOR_API std::vector<contract::renderer::Lines3D> lightGizmos(const ecs::World& world);

}  // namespace aylo::editor::scene

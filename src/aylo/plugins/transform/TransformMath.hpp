/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TransformMath
*/

#pragma once

#include "Transform.hpp"
#include "plugins/transform/Export.hpp"

namespace aylo::plugins::transform {
/// @return Where `local`, relative to `parent`, is in the world.
/// @details Exact with a uniform scale or a mirror (a negative scale on one axis, which also reverses the child's
/// rotation). A rotated child of a parent scaled differently on each axis would need a shear, which GlobalTransform
/// cannot hold: its position stays right, its rotation and scale are approximate. TODO
[[nodiscard]] AYLO_PLUGIN_TRANSFORM_API GlobalTransform combine(const GlobalTransform& parent,
                                                                const Transform& local) noexcept;

/// @return `world` expressed relative to `parent`: the inverse of combine().
/// @details A parent scaled to 0 on an axis loses that axis: the result is 0 on it, not a division by 0.
[[nodiscard]] AYLO_PLUGIN_TRANSFORM_API Transform toLocal(const GlobalTransform& parent,
                                                          const GlobalTransform& world) noexcept;

/// @return @p from blended toward @p to by @p alpha (0: @p from, 1: @p to). The rotation turns the short way, so a pose
/// that crossed a full turn does not spin back.
[[nodiscard]] AYLO_PLUGIN_TRANSFORM_API Transform interpolate(const Transform& from, const Transform& to,
                                                              float alpha) noexcept;
}  // namespace aylo::plugins::transform

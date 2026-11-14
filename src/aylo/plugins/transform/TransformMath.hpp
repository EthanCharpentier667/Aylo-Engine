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
/// rotation). A rotated child of a parent scaled differently on each axis would need a shear, which GlobalTransform2D
/// cannot hold: its position stays right, its rotation and scale are approximate. TODO
[[nodiscard]] AYLO_PLUGIN_TRANSFORM_API GlobalTransform2D combine(const GlobalTransform2D& parent,
                                                                  const Transform2D& local) noexcept;

/// @return `world` expressed relative to `parent`: the inverse of combine().
/// @details A parent scaled to 0 on an axis loses that axis: the result is 0 on it, not a division by 0.
[[nodiscard]] AYLO_PLUGIN_TRANSFORM_API Transform2D toLocal(const GlobalTransform2D& parent,
                                                            const GlobalTransform2D& world) noexcept;

/// @return @p from blended toward @p to by @p alpha (0: @p from, 1: @p to). The rotation turns the short way, so a pose
/// that crossed a full turn does not spin back.
[[nodiscard]] AYLO_PLUGIN_TRANSFORM_API Transform2D interpolate(const Transform2D& from, const Transform2D& to,
                                                                float alpha) noexcept;
/// @return Where `local`, relative to `parent`, is in the 3D world: the parent's scale, then its rotation, then its
/// position, applied to the child's position; the rotations and the scales multiplied.
/// @details Exact with a uniform scale. A rotated child of a parent scaled differently on each axis would need a shear,
/// which GlobalTransform3D cannot hold: its position stays right, its rotation and scale are approximate.
[[nodiscard]] AYLO_PLUGIN_TRANSFORM_API GlobalTransform3D combine(const GlobalTransform3D& parent,
                                                                  const Transform3D& local) noexcept;

/// @return `world` expressed relative to `parent`: the inverse of the 3D combine(). A scale of 0 on an axis loses it.
[[nodiscard]] AYLO_PLUGIN_TRANSFORM_API Transform3D toLocal(const GlobalTransform3D& parent,
                                                            const GlobalTransform3D& world) noexcept;

/// @return @p from blended toward @p to by @p alpha: the positions and scales linearly, the rotations along the short
/// arc (spherical interpolation).
[[nodiscard]] AYLO_PLUGIN_TRANSFORM_API Transform3D interpolate(const Transform3D& from, const Transform3D& to,
                                                                float alpha) noexcept;
}  // namespace aylo::plugins::transform

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Gizmo
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <span>

#include "editor/Export.hpp"
#include "plugins/transform/Transform.hpp"

namespace aylo::editor::scene {

/// @brief What the Scene panel's gizmo does, as Unity's W, E and R.
enum class GizmoTool : std::uint8_t {
    kMove,    ///< W: drags the position along an axis.
    kRotate,  ///< E: turns around an axis.
    kScale,   ///< R: stretches along an axis.
};

/// @brief One handle of the gizmo, as the screen shows it.
struct GizmoHandle {
    std::uint8_t axis{0};     ///< 0 X, 1 Y, 2 Z.
    glm::vec2 from{0.0F};     ///< The selection's origin, in panel pixels.
    glm::vec2 to{0.0F};       ///< The handle's end, in panel pixels.
    float worldLength{1.0F};  ///< How long the handle is in the world.
};

/// @return The handle nearest @p mouse (panel pixels) within @p tolerance pixels of its segment; nothing when none is.
[[nodiscard]] AYLO_EDITOR_API std::optional<std::uint8_t> hitGizmo(std::span<const GizmoHandle> handles,
                                                                   glm::vec2 mouse, float tolerance);

/// @return @p start dragged by @p tool along @p handle's axis, the mouse having moved @p mouse panel pixels since the
/// drag began.
[[nodiscard]] AYLO_EDITOR_API plugins::transform::Transform3D dragGizmo(GizmoTool tool, const GizmoHandle& handle,
                                                                        const plugins::transform::Transform3D& start,
                                                                        glm::vec2 mouse);

/// @return @p start dragged by @p tool along @p handle's axis (X or Y; rotation ignores it), in 2D.
[[nodiscard]] AYLO_EDITOR_API plugins::transform::Transform2D dragGizmo2D(GizmoTool tool, const GizmoHandle& handle,
                                                                          const plugins::transform::Transform2D& start,
                                                                          glm::vec2 mouse);

}  // namespace aylo::editor::scene

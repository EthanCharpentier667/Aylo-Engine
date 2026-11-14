/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SceneCamera
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

#include "aylo/contract/renderer/view/View.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"
#include "editor/Export.hpp"

namespace aylo::editor::scene {

/// @brief Which world the Scene panel looks at.
enum class SceneMode : std::uint8_t {
    k2D,  ///< The 2D world, from above: pan and zoom.
    k3D,  ///< The 3D world: fly, orbit, pan, dolly.
};

/// @brief The Scene panel's camera, an editor's own (not an entity), moved as Unity's Scene view moves.
/// @details In 3D it turns around a pivot: its position is the pivot, `distance` back along its forward. Flying moves
/// the pivot with it; orbiting turns around the pivot; panning moves both across the view; the wheel brings the camera
/// closer to the pivot. In 2D it is a centre and a zoom (pixels per world unit). Pure data and math, so tests drive it.
struct AYLO_EDITOR_API SceneCamera {
    SceneMode mode{SceneMode::k3D};  ///< The world shown.
    glm::vec3 pivot{0.0F};           ///< 3D: what it turns around and looks at.
    float yaw{0.7F};                 ///< 3D: radians around the world's Y; 0 looks along -Z.
    float pitch{-0.45F};             ///< 3D: radians up (positive) or down; kept within a quarter turn.
    float distance{12.0F};           ///< 3D: from the pivot back to the camera, in world units.
    float fieldOfView{1.0471976F};   ///< 3D: the vertical angle shown, radians (60 degrees).
    glm::vec2 center{0.0F};          ///< 2D: the world point in the middle.
    float zoom{1.0F};                ///< 2D: pixels per world unit.

    /// @return 3D: the orientation, a quaternion x, y, z, w (yaw, then pitch).
    [[nodiscard]] glm::vec4 rotation() const;
    /// @return 3D: where the camera is.
    [[nodiscard]] glm::vec3 position() const;
    /// @return 3D: where it looks (unit).
    [[nodiscard]] glm::vec3 forward() const;
    /// @return 3D: its right (unit).
    [[nodiscard]] glm::vec3 right() const;
    /// @return 3D: its up (unit).
    [[nodiscard]] glm::vec3 up() const;

    /// @return The 3D view it shows.
    [[nodiscard]] contract::renderer::View3D view3D() const;
    /// @return The 2D view it shows, at @p pixelScale framebuffer pixels per window pixel.
    [[nodiscard]] contract::renderer::View view2D(float pixelScale) const;

    /// @brief 3D, fly mode: turns where it looks by a mouse move of @p pixels, the camera staying in place.
    void look(glm::vec2 pixels);
    /// @brief 3D, fly mode: moves by @p direction (x right, y up, z forward, each -1 to 1) for @p seconds; @p fast is
    /// Shift.
    void fly(glm::vec3 direction, float seconds, bool fast);
    /// @brief 3D: turns around the pivot by a mouse move of @p pixels.
    void orbit(glm::vec2 pixels);
    /// @brief Moves across the view by a mouse move of @p pixels, in a view @p viewHeight pixels high: the world
    /// follows the mouse.
    void pan(glm::vec2 pixels, float viewHeight);
    /// @brief The wheel: 3D, closer to the pivot (positive) or farther; 2D, zooms in or out.
    void dolly(float wheel);
    /// @brief Looks at @p target, a thing about @p radius big: 3D, the pivot there and a distance that shows it; 2D,
    /// the centre there.
    void frame(glm::vec3 target, float radius);
};

}  // namespace aylo::editor::scene

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CameraView3D
*/

#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <optional>

#include "aylo/contract/renderer/Export.hpp"
#include "aylo/contract/renderer/view/CameraView.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"

namespace aylo::contract::renderer {

/// @brief What a View3D shows in a window: its matrices, its viewport, and the conversions between the screen and the
/// world, in the conventions of the 2D view (CameraView).
///
/// @details The viewport is the 2D view's: the largest rectangle of the view size's aspect ratio that fits the
/// framebuffer (letterboxing), or the whole framebuffer without a view size. The projection keeps that aspect ratio.
/// @code
/// const CameraView3D shown{view, {1920.0F, 1080.0F}, platform.getFramebufferSize()};
/// const Ray3D pick = shown.screenToRay(mousePosition);
/// @endcode
class AYLO_CONTRACT_RENDERER_API CameraView3D {
  public:
    /// @param view What to show.
    /// @param viewSize The resolution the game is designed for; 0x0 for the framebuffer's size.
    /// @param framebufferSize The window's framebuffer, in pixels; a 0x0 one (minimized) gives an empty viewport.
    CameraView3D(const View3D& view, glm::vec2 viewSize, glm::uvec2 framebufferSize);
    ~CameraView3D() = default;
    CameraView3D(const CameraView3D& other) = default;
    CameraView3D& operator=(const CameraView3D& other) = default;
    CameraView3D(CameraView3D&& other) noexcept = default;
    CameraView3D& operator=(CameraView3D&& other) noexcept = default;

    /// @return World to camera space: right-handed, the camera at the origin looking along -Z, Y up.
    [[nodiscard]] const glm::mat4& getViewMatrix() const noexcept { return _view; }
    /// @return Camera to clip space: X right, Y down, depth 0 to 1 (Vulkan's conventions, as CameraView's).
    [[nodiscard]] const glm::mat4& getProjection() const noexcept { return _projection; }
    /// @return World to clip space: getProjection() * getViewMatrix().
    [[nodiscard]] const glm::mat4& getViewProjection() const noexcept { return _viewProjection; }
    /// @return Where the frame goes in the framebuffer.
    [[nodiscard]] const Viewport& getViewport() const noexcept { return _viewport; }

    /// @return The ray the point @p screen (framebuffer pixels from the top-left corner) sees, from the near plane.
    [[nodiscard]] Ray3D screenToRay(glm::vec2 screen) const;
    /// @return Where @p world appears, in framebuffer pixels from the top-left corner; nothing when it is behind the
    /// camera.
    [[nodiscard]] std::optional<glm::vec2> worldToScreen(glm::vec3 world) const;

  private:
    glm::mat4 _view{1.0F};            ///< World to camera space.
    glm::mat4 _projection{1.0F};      ///< Camera to clip space.
    glm::mat4 _viewProjection{1.0F};  ///< World to clip space.
    glm::mat4 _inverse{1.0F};         ///< Clip space to world.
    Viewport _viewport;               ///< Letterboxed rectangle of the framebuffer.
};

}  // namespace aylo::contract::renderer

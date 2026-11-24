/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** FrameUniforms
*/

#pragma once

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>

#include "render/vulkan/Export.hpp"

namespace aylo::render::vulkan::rendering {

/// @brief What every shader can read during a frame: the CPU side of the uniform block at set 0, binding 0.
///
/// @details Copied as raw bytes into the uniform buffer, so its layout must follow std140 and match
/// shaders/include/FrameUniforms.glsl field by field: change both files together. In std140, a float is aligned on
/// 4 bytes, a vec2 on 8, a vec3, a vec4 and a mat4 on 16, and the whole block is a multiple of 16 bytes. The
/// static_asserts below check it at compile time.
struct FrameUniforms {
    float time = 0.0F;                           ///< Seconds since the renderer's init(). Offset 0.
    float deltaTime = 0.0F;                      ///< Seconds since the previous frame, 0 on the first one. Offset 4.
    glm::vec2 framebufferSize{0.0F};             ///< Size of the rendered images, in pixels. Offset 8.
    alignas(16) glm::mat4 viewProjection{1.0F};  ///< World to clip space: the frame's camera. Offset 16.
    glm::mat4 view{1.0F};                        ///< World to camera space (identity in 2D). Offset 80.
    glm::mat4 projection{1.0F};                  ///< Camera to clip space. Offset 144.
    glm::mat4 inverseView{1.0F};                 ///< Camera to world space. Offset 208.
    glm::mat4 inverseProjection{1.0F};           ///< Clip to camera space. Offset 272.
    glm::vec4 cameraPosition{
        0.0F};  ///< xyz: the camera in the world; w: 1 for a perspective, 0 orthographic. Offset 336.
};

static_assert(offsetof(FrameUniforms, deltaTime) == 4, "FrameUniforms::deltaTime must be at offset 4 (std140)");
static_assert(offsetof(FrameUniforms, framebufferSize) == 8,
              "FrameUniforms::framebufferSize must be at offset 8 (std140)");
static_assert(offsetof(FrameUniforms, viewProjection) == 16,
              "FrameUniforms::viewProjection must be at offset 16 (std140)");
static_assert(offsetof(FrameUniforms, view) == 80 && offsetof(FrameUniforms, projection) == 144 &&
                  offsetof(FrameUniforms, inverseView) == 208 && offsetof(FrameUniforms, inverseProjection) == 272 &&
                  offsetof(FrameUniforms, cameraPosition) == 336 && sizeof(FrameUniforms) == 352,
              "FrameUniforms' camera must match AyloFrame of common.glsl and FrameUniforms.glsl (std140)");
static_assert(sizeof(FrameUniforms) % 16 == 0, "FrameUniforms' size must be a multiple of 16 bytes (std140)");

/// @brief Sets @p uniforms' camera: @p view and @p projection, their product, their inverses and the camera's place
/// (from the inverse view), @p perspective telling a perspective projection from an orthographic one. A matrix that
/// cannot be inverted keeps an identity inverse.
AYLO_RENDER_VULKAN_API void setCamera(FrameUniforms& uniforms, const glm::mat4& view, const glm::mat4& projection,
                                      bool perspective) noexcept;

}  // namespace aylo::render::vulkan::rendering

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** SceneCamera
*/

#include "editor/scene/SceneCamera.hpp"

#include <algorithm>
#include <cmath>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

#include "aylo/contract/renderer/view/View.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"

namespace aylo::editor::scene {
namespace {
constexpr float kTurnPerPixel = 0.005F;    ///< Radians a pixel of mouse move turns the camera.
constexpr float kMaxPitch = 1.55F;         ///< Just under a quarter turn: never upside down.
constexpr float kFlySpeed = 6.0F;          ///< World units per second, flying.
constexpr float kFastFactor = 4.0F;        ///< How much faster Shift flies.
constexpr float kDollyStep = 0.85F;        ///< How much one notch of the wheel brings the camera closer.
constexpr float kZoomStep = 1.15F;         ///< How much one notch of the wheel zooms in 2D.
constexpr float kMinDistance = 0.1F;       ///< The closest the camera comes to its pivot.
constexpr float kMaxDistance = 10000.0F;   ///< The farthest.
constexpr float kMinZoom = 0.01F;          ///< 2D: the farthest.
constexpr float kMaxZoom = 64.0F;          ///< 2D: the closest.
constexpr float kFrameFactor = 3.0F;       ///< How many sizes away framing puts the camera.
constexpr float kMinFrameDistance = 2.0F;  ///< The closest framing puts it.

/// @return The rotation of a yaw and a pitch.
glm::quat orientation(float yaw, float pitch) {
    return glm::angleAxis(yaw, glm::vec3{0.0F, 1.0F, 0.0F}) * glm::angleAxis(pitch, glm::vec3{1.0F, 0.0F, 0.0F});
}
}  // namespace

glm::vec4 SceneCamera::rotation() const {
    const glm::quat value = orientation(yaw, pitch);
    return {value.x, value.y, value.z, value.w};
}

glm::vec3 SceneCamera::forward() const { return orientation(yaw, pitch) * glm::vec3{0.0F, 0.0F, -1.0F}; }

glm::vec3 SceneCamera::right() const { return orientation(yaw, pitch) * glm::vec3{1.0F, 0.0F, 0.0F}; }

glm::vec3 SceneCamera::up() const { return orientation(yaw, pitch) * glm::vec3{0.0F, 1.0F, 0.0F}; }

glm::vec3 SceneCamera::position() const { return pivot - (forward() * distance); }

contract::renderer::View3D SceneCamera::view3D() const {
    return contract::renderer::View3D{
        .position = position(),
        .rotation = rotation(),
        .projection =
            contract::renderer::Perspective{.fieldOfView = fieldOfView, .nearPlane = 0.05F, .farPlane = 5000.0F},
    };
}

contract::renderer::View SceneCamera::view2D(float pixelScale) const {
    return contract::renderer::View{
        .position = center,
        .projection = contract::renderer::Orthographic{.zoom = zoom * pixelScale},
    };
}

void SceneCamera::look(glm::vec2 pixels) {
    const glm::vec3 eye = position();
    yaw -= pixels.x * kTurnPerPixel;
    pitch = std::clamp(pitch - (pixels.y * kTurnPerPixel), -kMaxPitch, kMaxPitch);
    pivot = eye + (forward() * distance);
}

void SceneCamera::fly(glm::vec3 direction, float seconds, bool fast) {
    const float speed = kFlySpeed * (fast ? kFastFactor : 1.0F) * seconds;
    pivot +=
        ((right() * direction.x) + (glm::vec3{0.0F, 1.0F, 0.0F} * direction.y) + (forward() * direction.z)) * speed;
}

void SceneCamera::orbit(glm::vec2 pixels) {
    yaw -= pixels.x * kTurnPerPixel;
    pitch = std::clamp(pitch - (pixels.y * kTurnPerPixel), -kMaxPitch, kMaxPitch);
}

void SceneCamera::pan(glm::vec2 pixels, float viewHeight) {
    if (mode == SceneMode::k2D) {
        center -= pixels / zoom;
        return;
    }
    // A pixel at the pivot's distance covers this many world units.
    const float perPixel = 2.0F * distance * std::tan(fieldOfView / 2.0F) / std::max(viewHeight, 1.0F);
    pivot += ((-right() * pixels.x) + (up() * pixels.y)) * perPixel;
}

void SceneCamera::dolly(float wheel) {
    if (mode == SceneMode::k2D) {
        zoom = std::clamp(zoom * std::pow(kZoomStep, wheel), kMinZoom, kMaxZoom);
        return;
    }
    distance = std::clamp(distance * std::pow(kDollyStep, wheel), kMinDistance, kMaxDistance);
}

void SceneCamera::frame(glm::vec3 target, float radius) {
    if (mode == SceneMode::k2D) {
        center = glm::vec2{target};
        return;
    }
    pivot = target;
    distance = std::max(radius * kFrameFactor, kMinFrameDistance);
}

}  // namespace aylo::editor::scene

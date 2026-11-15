/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MoveAndSlide
*/

#include "plugins/physics/MoveAndSlide.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <optional>

#include "aylo/ecs/entity/Entity.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/Collider3D.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/QueryFilter.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"

namespace aylo::plugins::physics {
namespace {
/// @brief Below this length, in world units, a move is over.
constexpr float kMinMove = 1e-4F;
/// @brief Below this cosine between the move and the surface, the shape grazes it: the skin is kept along the move.
constexpr float kMinFacing = 0.1F;
}  // namespace

glm::vec2 moveAndSlide(const PhysicsWorld& world, const Collider& shape, glm::vec2 from, glm::vec2 delta,
                       const QueryFilter& filter, const SlideSettings& settings) {
    if (!(settings.skin >= 0.0F)) {
        throw exceptions::PhysicsException{
            std::format("moveAndSlide: the skin must not be negative, got {}", settings.skin)};
    }
    glm::vec2 position = from;
    glm::vec2 remaining = delta;
    for (std::uint32_t slide = 0; slide <= settings.maxSlides; ++slide) {
        const float distance = glm::length(remaining);
        if (distance <= kMinMove) {
            break;
        }
        const glm::vec2 direction = remaining / distance;
        const std::optional<PhysicsWorld::RayHit> hit =
            world.shapeCast(shape, position, direction, distance + settings.skin, filter);
        if (!hit) {
            position += remaining;
            break;
        }
        const float facing = -glm::dot(direction, hit->normal);
        const float backOff = facing > kMinFacing ? settings.skin / facing : settings.skin;
        const float travel = std::clamp(hit->distance - backOff, 0.0F, distance);
        position += direction * travel;
        if (glm::length(hit->normal) <= kMinMove || slide == settings.maxSlides) {
            break;
        }
        const glm::vec2 left = direction * (distance - travel);
        remaining = left - (hit->normal * glm::dot(left, hit->normal));
    }
    return position;
}

glm::vec2 moveAndSlide(const PhysicsWorld& world, ecs::Entity entity, const Collider& collider, glm::vec2 from,
                       glm::vec2 delta, const SlideSettings& settings) {
    return moveAndSlide(world, collider, from, delta, QueryFilter{.mask = collider.mask, .ignore = entity}, settings);
}

namespace {
/// @brief The up axis of the 3D bodies.
constexpr glm::vec3 kUp{0.0F, 1.0F, 0.0F};
/// @brief The identity rotation: a controller's shape is swept upright.
constexpr glm::vec4 kUpright{0.0F, 0.0F, 0.0F, 1.0F};
/// @brief The shortest cast down that tells a shape with no skin it is grounded, in metres.
constexpr float kMinGroundProbe = 1e-3F;

/// @brief What one sliding pass of moveAndSlide3D() saw.
struct Pass3D {
    glm::vec3 position{0.0F};  ///< Where it ended.
    bool hitWall = false;      ///< Whether a surface too steep to stand on stopped it.
};

/// @brief The shape, the bodies and the settings of one moveAndSlide3D().
struct Mover3D {
    const PhysicsWorld& world;
    const Collider3D& shape;
    const QueryFilter& filter;
    const Slide3DSettings& settings;
    float minGroundY;  ///< The smallest Y of a normal the shape can stand on: cos(maxSlope).

    /// @return Whether a surface of this normal is ground, not a wall.
    [[nodiscard]] bool isGround(const glm::vec3& normal) const noexcept { return normal.y >= minGroundY; }

    /// @brief Moves the shape by @p motion, sliding along what it hits. A wall is never climbed: what would gain height
    /// against it slides along it level instead. With @p stopOnGround, ground stops the move (a fall lands, it does not
    /// slide down a slope it can stand on).
    [[nodiscard]] Pass3D slide(glm::vec3 position, const glm::vec3& motion, bool stopOnGround) const {
        Pass3D pass{.position = position};
        glm::vec3 remaining = motion;
        for (std::uint32_t slide = 0; slide <= settings.maxSlides; ++slide) {
            const float distance = glm::length(remaining);
            if (distance <= kMinMove) {
                break;
            }
            const glm::vec3 direction = remaining / distance;
            const std::optional<PhysicsWorld::RayHit3D> hit =
                world.shapeCast3D(shape, pass.position, kUpright, direction, distance + settings.skin, filter);
            if (!hit) {
                pass.position += remaining;
                break;
            }
            const float facing = -glm::dot(direction, hit->normal);
            const float backOff = facing > kMinFacing ? settings.skin / facing : settings.skin;
            const float travel = std::clamp(hit->distance - backOff, 0.0F, distance);
            pass.position += direction * travel;
            const bool ground = isGround(hit->normal);
            pass.hitWall = pass.hitWall || !ground;
            if (glm::length(hit->normal) <= kMinMove || slide == settings.maxSlides || (stopOnGround && ground)) {
                break;
            }
            const glm::vec3 left = direction * (distance - travel);
            remaining = left - (hit->normal * glm::dot(left, hit->normal));
            const glm::vec3 level{hit->normal.x, 0.0F, hit->normal.z};
            if (!ground && remaining.y > std::max(left.y, 0.0F) && glm::length(level) > kMinMove) {
                const glm::vec3 across = glm::normalize(level);
                remaining = left - (across * glm::dot(left, across));  // along the wall, no higher than it was going
            }
        }
        return pass;
    }

    /// @brief Tries to step over what stopped @p blocked: up by the step height, across by @p across, down onto
    /// ground. @return Where the step lands, or nothing if it goes no further or lands on no ground.
    [[nodiscard]] std::optional<glm::vec3> step(const glm::vec3& from, const glm::vec3& across,
                                                const glm::vec3& blocked) const {
        const Pass3D up = slide(from, kUp * settings.stepHeight, false);
        const Pass3D side = slide(up.position, across, false);
        const float raised = up.position.y - from.y;
        const std::optional<PhysicsWorld::RayHit3D> below =
            world.shapeCast3D(shape, side.position, kUpright, -kUp, raised + settings.skin, filter);
        if (!below || !isGround(below->normal)) {
            return std::nullopt;  // the step is a ledge over nothing, or a slope too steep
        }
        const glm::vec3 landed = side.position - (kUp * std::max(below->distance - settings.skin, 0.0F));
        const auto level = [&from](const glm::vec3& at) {
            return glm::length(glm::vec2{at.x - from.x, at.z - from.z});
        };
        if (level(landed) <= level(blocked) + kMinMove) {
            return std::nullopt;
        }
        return landed;
    }
};
}  // namespace

Slide3DResult moveAndSlide3D(const PhysicsWorld& world, const Collider3D& shape, const glm::vec3& from,
                             const glm::vec3& delta, const QueryFilter& filter, const Slide3DSettings& settings) {
    if (!(settings.skin >= 0.0F) || !(settings.stepHeight >= 0.0F)) {
        throw exceptions::PhysicsException{
            std::format("moveAndSlide3D: the skin and the step height must not be negative, got {} and {}",
                        settings.skin, settings.stepHeight)};
    }
    if (!(settings.maxSlope >= 0.0F && settings.maxSlope < 90.0F)) {
        throw exceptions::PhysicsException{
            std::format("moveAndSlide3D: the slope must be in [0, 90) degrees, got {}", settings.maxSlope)};
    }
    const Mover3D mover{.world = world,
                        .shape = shape,
                        .filter = filter,
                        .settings = settings,
                        .minGroundY = std::cos(glm::radians(settings.maxSlope))};
    const glm::vec3 vertical = kUp * glm::dot(delta, kUp);
    const glm::vec3 across = delta - vertical;

    const Pass3D walked = mover.slide(from, across, false);
    glm::vec3 position = walked.position;
    if (walked.hitWall && settings.stepHeight > 0.0F) {
        position = mover.step(from, across, walked.position).value_or(walked.position);
    }
    position = mover.slide(position, vertical, true).position;

    Slide3DResult result{.position = position};
    const float probe = std::max(settings.skin * 2.0F, kMinGroundProbe);
    if (const std::optional<PhysicsWorld::RayHit3D> below =
            world.shapeCast3D(shape, position, kUpright, -kUp, probe, filter);
        below && mover.isGround(below->normal)) {
        result.grounded = true;
        result.groundNormal = below->normal;
        result.ground = below->entity;
    }
    return result;
}

Slide3DResult moveAndSlide3D(const PhysicsWorld& world, ecs::Entity entity, const Collider3D& collider,
                             const glm::vec3& from, const glm::vec3& delta, const Slide3DSettings& settings) {
    return moveAndSlide3D(world, collider, from, delta, QueryFilter{.mask = collider.mask, .ignore = entity}, settings);
}

}  // namespace aylo::plugins::physics

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** MoveAndSlide
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

#include "aylo/ecs/entity/Entity.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/Collider3D.hpp"
#include "plugins/physics/Export.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/QueryFilter.hpp"

namespace aylo::plugins::physics {

/// @brief How moveAndSlide() moves a shape.
struct SlideSettings {
    float skin = 0.5F;             ///< The gap kept between the shape and what it touches, in world units.
    std::uint32_t maxSlides = 4U;  ///< How many surfaces one move can slide along before it stops.
};

/// @brief Moves a shape by a delta without stepping the world: it stops against what it hits and slides along it.
/// @details A kinematic move for predicted systems: built on PhysicsWorld::shapeCast(), it gives the same result on the
/// server and on a client whose bodies follow the same Transforms, so a ship replaying its inputs agrees with the
/// server. Each hit stops the shape a skin away from the surface, and the rest of the delta goes on along it; a shape
/// that starts inside a body stays where it is. The shape is swept without rotation, as by shapeCast().
/// @param world The world whose bodies block the shape.
/// @param shape The shape to move: only its shape fields are read (shape, size, radius, halfHeight, offset).
/// @param from Where the shape starts, in world units, Y down.
/// @param delta How far it would move without obstacles, in world units, Y down.
/// @param filter Which bodies block it: ignore the entity that moves, or it blocks itself.
/// @param settings The skin and the number of slides.
/// @return Where the shape ends, in world units, Y down.
/// @throws exceptions::PhysicsException When the shape is invalid (as for a Collider) or the skin is negative.
/// @code
/// transform->position = aylo::plugins::physics::moveAndSlide(*world, collider, transform->position, delta,
///                                                             {.mask = collider.mask, .ignore = ship});
/// @endcode
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API glm::vec2 moveAndSlide(const PhysicsWorld& world, const Collider& shape,
                                                             glm::vec2 from, glm::vec2 delta,
                                                             const QueryFilter& filter = {},
                                                             const SlideSettings& settings = {});

/// @brief Moves an entity's collider by a delta, blocked by the bodies its mask collides with and never by its own.
/// @details moveAndSlide() with a filter seeing Collider::mask and ignoring the entity.
/// @code
/// transform->position = aylo::plugins::physics::moveAndSlide(*world, ship, collider, transform->position, delta);
/// @endcode
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API glm::vec2 moveAndSlide(const PhysicsWorld& world, ecs::Entity entity,
                                                             const Collider& collider, glm::vec2 from, glm::vec2 delta,
                                                             const SlideSettings& settings = {});

/// @brief How moveAndSlide3D() moves a shape: a character controller's settings, in metres.
struct Slide3DSettings {
    float skin = 0.02F;            ///< The gap kept between the shape and what it touches.
    std::uint32_t maxSlides = 4U;  ///< How many surfaces one move can slide along before it stops.
    float stepHeight = 0.0F;       ///< The highest step climbed without jumping (a stair); 0: none.
    float maxSlope = 45.0F;        ///< The steepest ground walked on, in degrees; a steeper surface is a wall.
};

/// @brief Where moveAndSlide3D() left the shape, and what it stands on.
struct Slide3DResult {
    glm::vec3 position{0.0F};      ///< Where the shape ends, in metres.
    bool grounded = false;         ///< Whether it stands on ground no steeper than maxSlope, within twice the skin.
    glm::vec3 groundNormal{0.0F};  ///< That ground's normal, unit, pointing up out of it; zero when not grounded.
    ecs::Entity ground;            ///< The entity it stands on; null when not grounded.
};

/// @brief Moves a 3D shape by a delta without stepping the world: a kinematic character controller.
/// @details Built on PhysicsWorld::shapeCast3D(), without rotation (a capsule stands along Y), in metres, Y up. The
/// horizontal part of the delta goes first: the shape stops a skin away from what it hits and slides along it. A
/// surface steeper than maxSlope is a wall: the shape slides along it but never climbs it, and when stepHeight is
/// above 0 it tries to step over it (up, across, down onto ground no steeper than maxSlope). The vertical part goes
/// next: falling stops on the ground, and slides down a surface too steep to stand on. Last, a short cast down tells
/// whether the shape is grounded. As moveAndSlide(), the same bodies give the same result on the server and on a
/// client. Add the gravity to the delta yourself every tick: the function keeps no velocity.
/// @param world The world whose 3D bodies block the shape.
/// @param shape The shape to move: only its shape fields are read (shape, size, radius, halfHeight, offset).
/// @param from Where the shape starts (its Transform3D's position), in metres.
/// @param delta How far it would move without obstacles, in metres.
/// @param filter Which bodies block it: ignore the entity that moves, or it blocks itself.
/// @param settings The skin, the slides, the step and the slope.
/// @return Where the shape ends and whether it stands on the ground.
/// @throws exceptions::PhysicsException When the shape is invalid (as for a Collider3D), or the skin or the step height
/// is negative, or the slope is not in [0, 90).
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API Slide3DResult moveAndSlide3D(const PhysicsWorld& world, const Collider3D& shape,
                                                                   const glm::vec3& from, const glm::vec3& delta,
                                                                   const QueryFilter& filter = {},
                                                                   const Slide3DSettings& settings = {});

/// @brief Moves an entity's Collider3D by a delta, blocked by the bodies its mask collides with and never by its own.
/// @details moveAndSlide3D() with a filter seeing Collider3D::mask and ignoring the entity.
[[nodiscard]] AYLO_PLUGIN_PHYSICS_API Slide3DResult moveAndSlide3D(const PhysicsWorld& world, ecs::Entity entity,
                                                                   const Collider3D& collider, const glm::vec3& from,
                                                                   const glm::vec3& delta,
                                                                   const Slide3DSettings& settings = {});

}  // namespace aylo::plugins::physics

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CollisionEvents
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/event/EventTraits.hpp"
#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::physics {

/// @brief Two solid colliders started touching: sent once per pair, whatever the number of contact points.
/// @details Sent by the physics plugin after the step that saw it. Only pairs whose collision layers and masks let them
/// collide are reported. Two 2D bodies fill point and normal, two 3D bodies point3D and normal3D; the others are
/// zero.
/// @code
/// void onHit(aylo::ecs::EventReader<aylo::plugins::physics::CollisionStarted> hits) {
///     for (const aylo::plugins::physics::CollisionStarted& hit : hits) { /* damage hit.first and hit.second */ }
/// }
/// @endcode
struct CollisionStarted {
    ecs::Entity first;             ///< One entity of the pair.
    ecs::Entity second;            ///< The other one.
    glm::vec2 point{0.0F, 0.0F};   ///< Where they touched, in world units, Y down.
    glm::vec2 normal{0.0F, 0.0F};  ///< Unit vector from first to second, Y down.
    glm::vec3 point3D{0.0F};       ///< Between 3D bodies: where they touched, in metres.
    glm::vec3 normal3D{0.0F};      ///< Between 3D bodies: unit vector from first to second.
};
AYLO_ECS_EVENT(CollisionStarted, "aylo.CollisionStarted", first, second, point, normal, point3D, normal3D);

/// @brief Two solid colliders stopped touching. Not sent when one of them was despawned, lost its collider or was
/// rebuilt.
struct CollisionEnded {
    ecs::Entity first;   ///< One entity of the pair.
    ecs::Entity second;  ///< The other one.
};
AYLO_ECS_EVENT(CollisionEnded, "aylo.CollisionEnded", first, second);

/// @brief A collider entered a sensor (Collider::sensor): a pickup, a level trigger.
struct TriggerEntered {
    ecs::Entity trigger;  ///< The sensor's entity.
    ecs::Entity other;    ///< The entity that entered it.
};
AYLO_ECS_EVENT(TriggerEntered, "aylo.TriggerEntered", trigger, other);

/// @brief A collider left a sensor. Not sent when one of them was despawned, lost its collider or was rebuilt.
struct TriggerExited {
    ecs::Entity trigger;  ///< The sensor's entity.
    ecs::Entity other;    ///< The entity that left it.
};
AYLO_ECS_EVENT(TriggerExited, "aylo.TriggerExited", trigger, other);

}  // namespace aylo::plugins::physics

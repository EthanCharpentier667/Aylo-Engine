/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** WorldImpl
*/

#pragma once

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Core/Core.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Math/MathTypes.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/Physics/Body/BodyType.h>
#include <Jolt/Physics/Body/MotionProperties.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <unordered_map>
#include <vector>

#include "aylo/ecs/entity/Entity.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/QueryFilter.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/Velocity3D.hpp"
#include "plugins/physics/detail/ContactCollector.hpp"
#include "plugins/physics/detail/JoltMath.hpp"
#include "plugins/physics/detail/Layers.hpp"

/// @file
/// @brief The Jolt objects behind a PhysicsWorld. Internal: only the PhysicsWorld sources include it.

namespace aylo::plugins::physics {

namespace detail {
/// @return The entity whose Entity::getBits() gave these bits.
inline ecs::Entity entityFromBits(std::uint64_t bits) noexcept {
    return ecs::Entity{static_cast<std::uint32_t>(bits), static_cast<std::uint32_t>(bits >> 32U)};
}

}  // namespace detail

/// @brief The world's Jolt objects. Destroyed in reverse order: the system first, then what it points to.
// The member order is what keeps destruction safe, so it is not reordered to save the padding (a few dozen bytes per
// world): the field order clang-analyzer suggests would destroy the layer filters before the system that uses them.
// NOLINTNEXTLINE(clang-analyzer-optin.performance.Padding)
struct PhysicsWorld::Impl {
    /// @brief What the world knows of an entity's body.
    struct BodyEntry {
        JPH::BodyID body;                ///< The body.
        bool hasRigidBody = false;       ///< Whether it was built with a RigidBody: catches one added or removed.
        bool spatial = false;            ///< Whether it is a 3D body (Collider3D): its pose is in the 3D fields.
        float gravityScale = 1.0F;       ///< A 3D body's RigidBody::gravityScale, for the 3D gravity.
        glm::vec3 lastPosition3D{0.0F};  ///< A 3D body's last position given or read, in metres.
        glm::vec4 lastRotation3D{0.0F, 0.0F, 0.0F, 1.0F};  ///< A 3D body's last rotation given or read.
        std::optional<Velocity3D> lastVelocity3D;          ///< A 3D body's last velocity given or read.
        std::uint64_t seenPass = 0;                        ///< The last body pass that saw the entity.
        glm::vec2 lastPosition{0.0F, 0.0F};  ///< The last position given to the body or read from it, in world units.
        float lastRotation = 0.0F;           ///< The last rotation given to the body or read from it.
        std::optional<glm::vec2> lastLocalPosition;  ///< A child's last relative position; nothing if not recorded.
        float lastLocalRotation = 0.0F;              ///< A child's last relative rotation.
        std::optional<Velocity>
            lastVelocity;                     ///< The last velocity given to the body or read from it; none if unknown.
        std::uint64_t kinematicMoveStep = 0;  ///< The step (as getStepCount() counts it after it) of the last
                                              ///< moveKinematic(): its pose is the Transform2D's, not written back.

        /// @return Whether this is the last pose given or read. Exact float equality on purpose: the Transform2D holds
        /// exactly the floats read from the body.
        [[nodiscard]] bool isAt(glm::vec2 position, float rotation) const noexcept {
            return lastPosition == position && lastRotation == rotation;
        }
        /// @brief Records the pose given to the body or read from it.
        void remember(glm::vec2 position, float rotation) noexcept {
            lastPosition = position;
            lastRotation = rotation;
        }
        /// @return Whether this is the last 3D pose given or read, exactly.
        [[nodiscard]] bool isAt3D(const glm::vec3& position, const glm::vec4& rotation) const noexcept {
            return lastPosition3D == position && lastRotation3D == rotation;
        }
        /// @brief Records the 3D pose given to the body or read from it.
        void remember3D(const glm::vec3& position, const glm::vec4& rotation) noexcept {
            lastPosition3D = position;
            lastRotation3D = rotation;
        }
    };

    /// @brief What a push does to a body.
    enum class PushKind : std::uint8_t {
        kForce,    ///< JPH::BodyInterface::AddForce.
        kImpulse,  ///< JPH::BodyInterface::AddImpulse.
        kTorque,   ///< JPH::BodyInterface::AddTorque.
    };
    /// @brief A force, impulse or torque queued for the next step.
    struct Push {
        std::uint64_t entity;  ///< Entity::getBits() of the pushed entity.
        PushKind kind;         ///< What the push does.
        JPH::Vec3 value;       ///< Already in Jolt's units and axes.
        bool spatial;          ///< Whether it pushes a 3D body: a push of the other family is ignored.
    };

    explicit Impl(const PhysicsSettings& settings)
        : unitsPerMeter{settings.unitsPerMeter},
          gravity3D{settings.gravity3D},
          tempAllocator{static_cast<JPH::uint>(settings.tempAllocatorBytes)},
          jobSystem{JPH::cMaxPhysicsJobs} {
        system.Init(settings.maxBodies, 0, settings.maxBodyPairs, settings.maxContactConstraints, broadPhaseLayers,
                    objectVsBroadPhaseFilter, objectLayerPairFilter);
        system.SetContactListener(&contacts);
        system.SetGravity(detail::toJolt2D(settings.gravity / unitsPerMeter));
    }
    ~Impl() = default;
    Impl(const Impl& other) = delete;
    Impl& operator=(const Impl& other) = delete;
    Impl(Impl&& other) noexcept = delete;
    Impl& operator=(Impl&& other) noexcept = delete;

    float unitsPerMeter;                                        ///< World units in one meter.
    std::uint64_t steps = 0;                                    ///< Steps done.
    std::uint64_t pass = 0;                                     ///< The current body pass.
    bool droppedContacts = false;                               ///< Whether the last step ignored contacts.
    bool broadPhaseDirty = false;                               ///< Whether bodies came or went since the last release.
    JPH::TempAllocatorImpl tempAllocator;                       ///< Scratch memory for the steps.
    JPH::JobSystemSingleThreaded jobSystem;                     ///< Runs Jolt's jobs on the caller's thread.
    detail::BroadPhaseLayers broadPhaseLayers;                  ///< Object layer to broad phase layer.
    detail::ObjectVsBroadPhaseFilter objectVsBroadPhaseFilter;  ///< Object layer vs broad phase layer.
    detail::ObjectLayerPairFilter objectLayerPairFilter;        ///< Object layer vs object layer.
    detail::ContactCollector contacts;                          ///< Records the contacts during the steps.
    JPH::PhysicsSystem system;                                  ///< Points to the four above: declared after them.
    std::unordered_map<std::uint64_t, BodyEntry> bodies;        ///< The bodies by Entity::getBits().
    glm::vec3 gravity3D;                                        ///< The 3D bodies' gravity, in m/s².
    std::vector<Push> pushes;                                   ///< Queued for the next step, in call order.

    /// @return The entity's body if it is of the family asked for, else bodies.end().
    [[nodiscard]] std::unordered_map<std::uint64_t, BodyEntry>::iterator find(ecs::Entity entity, bool spatial) {
        const auto found = bodies.find(entity.getBits());
        return found != bodies.end() && found->second.spatial == spatial ? found : bodies.end();
    }
    /// @return The entity's body if it is of the family asked for, else bodies.end().
    [[nodiscard]] std::unordered_map<std::uint64_t, BodyEntry>::const_iterator find(ecs::Entity entity,
                                                                                    bool spatial) const {
        const auto found = bodies.find(entity.getBits());
        return found != bodies.end() && found->second.spatial == spatial ? found : bodies.end();
    }

    /// @brief Queues a push for the next step.
    void queue(ecs::Entity entity, PushKind kind, JPH::Vec3Arg value, bool spatial = false) {
        pushes.push_back(Push{.entity = entity.getBits(), .kind = kind, .value = value, .spatial = spatial});
    }

    /// @brief Applies the queued pushes to the dynamic bodies they name, then forgets them.
    void applyPushes() {
        JPH::BodyInterface& bodyInterface = system.GetBodyInterface();
        for (const Push& push : pushes) {
            const auto found = bodies.find(push.entity);
            if (found == bodies.end() || found->second.spatial != push.spatial ||
                bodyInterface.GetMotionType(found->second.body) != JPH::EMotionType::Dynamic) {
                continue;
            }
            switch (push.kind) {
                case PushKind::kForce:
                    bodyInterface.AddForce(found->second.body, push.value);
                    break;
                case PushKind::kImpulse:
                    bodyInterface.AddImpulse(found->second.body, push.value);
                    break;
                case PushKind::kTorque:
                    bodyInterface.AddTorque(found->second.body, push.value);
                    break;
            }
        }
        pushes.clear();
    }

    /// @return The body of the entity a query ignores; an invalid id when none is ignored or it has no body.
    [[nodiscard]] JPH::BodyID ignoredBody(const QueryFilter& filter) const {
        if (!filter.ignore) {
            return JPH::BodyID{};
        }
        const auto found = bodies.find(filter.ignore->getBits());
        return found == bodies.end() ? JPH::BodyID{} : found->second.body;
    }

    /// @return What a query hit, in world units: the entity reads the body's user data, the normal (in meters, unit)
    /// only flips Y; nothing if the body went away.
    [[nodiscard]] std::optional<RayHit> makeHit(JPH::BodyID body, JPH::RVec3Arg point, JPH::Vec3Arg normal,
                                                float distance) const {
        const JPH::BodyLockRead lock{system.GetBodyLockInterface(), body};
        if (!lock.Succeeded()) {
            return std::nullopt;
        }
        return RayHit{.entity = detail::entityFromBits(lock.GetBody().GetUserData()),
                      .point = detail::fromJolt2D(point) * unitsPerMeter,
                      .normal = detail::fromJolt2D(normal),
                      .distance = distance};
    }

    /// @brief Pushes the active dynamic 3D bodies with the 3D gravity (times their gravityScale) for the next step.
    /// @details Jolt's gravity is the 2D one, so the 3D bodies get theirs as a force; sleeping bodies get none, as
    /// with Jolt's gravity.
    void applyGravity3D() {
        if (gravity3D == glm::vec3{0.0F}) {
            return;
        }
        JPH::BodyIDVector active;
        system.GetActiveBodies(JPH::EBodyType::RigidBody, active);
        const JPH::BodyLockInterface& locks = system.GetBodyLockInterfaceNoLock();
        for (const JPH::BodyID id : active) {
            const JPH::BodyLockWrite lock{locks, id};
            if (!lock.Succeeded() || !lock.GetBody().IsDynamic() ||
                !detail::layers::isSpatial(lock.GetBody().GetObjectLayer())) {
                continue;
            }
            JPH::Body& body = lock.GetBody();
            const auto found = bodies.find(body.GetUserData());
            const float inverseMass = body.GetMotionProperties()->GetInverseMass();
            if (found == bodies.end() || !(inverseMass > 0.0F)) {
                continue;
            }
            body.AddForce(detail::toJolt(gravity3D * (found->second.gravityScale / inverseMass)));
        }
    }

    /// @brief Removes a body from the simulation and frees it.
    void destroy(JPH::BodyID id) {
        contacts.forget(id);
        JPH::BodyInterface& bodyInterface = system.GetBodyInterface();
        bodyInterface.RemoveBody(id);
        bodyInterface.DestroyBody(id);
        broadPhaseDirty = true;
    }
};

}  // namespace aylo::plugins::physics

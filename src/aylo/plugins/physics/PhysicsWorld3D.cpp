/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsWorld3D
*/

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Core/Reference.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyType.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/EActivation.h>

#include <algorithm>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <optional>
#include <vector>

#include "aylo/ecs/entity/Entity.hpp"
#include "plugins/physics/Collider3D.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/QueryFilter.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity3D.hpp"
#include "plugins/physics/detail/BodySettings.hpp"
#include "plugins/physics/detail/JoltMath.hpp"
#include "plugins/physics/detail/QueryFilters.hpp"
#include "plugins/physics/detail/WorldImpl.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/transform/Transform.hpp"

namespace aylo::plugins::physics {

namespace {
/// @return A 3D body's velocity.
Velocity3D fromJoltVelocity3D(JPH::Vec3Arg linear, JPH::Vec3Arg angular) noexcept {
    return Velocity3D{.linear = detail::fromJolt(linear), .angular = detail::fromJolt(angular)};
}

/// @return The ray of a 3D query; nothing when its direction has no length or it has no reach.
std::optional<JPH::RRayCast> rayOf(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) noexcept {
    const float length = glm::length(direction);
    if (!(length > 0.0F) || !(maxDistance > 0.0F)) {
        return std::nullopt;
    }
    return JPH::RRayCast{detail::toJolt(origin), detail::toJolt(direction * (maxDistance / length))};
}
}  // namespace

glm::vec3 PhysicsWorld::getGravity3D() const noexcept { return _impl->gravity3D; }

void PhysicsWorld::setGravity3D(const glm::vec3& gravity) noexcept { _impl->gravity3D = gravity; }

PhysicsHandle PhysicsWorld::createBody3D(ecs::Entity entity, const Collider3D& collider, const RigidBody* body,
                                         const transform::Transform3D& at) {
    const JPH::BodyCreationSettings settings = detail::makeBodySettings3D(collider, body, at, entity.getBits());
    JPH::BodyInterface& bodyInterface = _impl->system.GetBodyInterface();

    JPH::Vec3 linearVelocity = JPH::Vec3::sZero();
    JPH::Vec3 angularVelocity = JPH::Vec3::sZero();
    if (const auto old = _impl->bodies.find(entity.getBits()); old != _impl->bodies.end()) {
        if (old->second.spatial && bodyInterface.GetMotionType(old->second.body) != JPH::EMotionType::Static) {
            linearVelocity = bodyInterface.GetLinearVelocity(old->second.body);
            angularVelocity = bodyInterface.GetAngularVelocity(old->second.body);
        }
        _impl->destroy(old->second.body);
        _impl->bodies.erase(old);
    }

    const JPH::Body* const created = bodyInterface.CreateBody(settings);
    if (created == nullptr) {
        throw exceptions::PhysicsException{"The physics world is full: raise PhysicsSettings::maxBodies"};
    }
    const JPH::BodyID id = created->GetID();
    const bool moving = settings.mMotionType != JPH::EMotionType::Static;
    _impl->broadPhaseDirty = true;
    bodyInterface.AddBody(id, moving ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);
    if (moving) {
        bodyInterface.SetLinearAndAngularVelocity(id, linearVelocity, angularVelocity);
    }
    Impl::BodyEntry entry{
        .body = id,
        .hasRigidBody = body != nullptr,
        .spatial = true,
        .gravityScale = body != nullptr ? body->gravityScale : 1.0F,
        .seenPass = _impl->pass,
    };
    entry.remember3D(at.position, at.rotation);
    _impl->bodies.insert_or_assign(entity.getBits(), entry);
    return PhysicsHandle{.body = id.GetIndexAndSequenceNumber()};
}

bool PhysicsWorld::touchBody3D(ecs::Entity entity, bool hasRigidBody) noexcept {
    const auto found = _impl->find(entity, /*spatial=*/true);
    if (found == _impl->bodies.end() || found->second.hasRigidBody != hasRigidBody) {
        return false;
    }
    found->second.seenPass = _impl->pass;
    return true;
}

std::optional<PhysicsWorld::BodyPose3D> PhysicsWorld::getBodyPose3D(ecs::Entity entity) const {
    const auto found = _impl->find(entity, /*spatial=*/true);
    if (found == _impl->bodies.end()) {
        return std::nullopt;
    }
    const JPH::BodyInterface& bodyInterface = _impl->system.GetBodyInterface();
    return BodyPose3D{.entity = entity,
                      .position = detail::fromJolt(bodyInterface.GetPosition(found->second.body)),
                      .rotation = detail::fromJoltRotation3D(bodyInterface.GetRotation(found->second.body))};
}

std::optional<Velocity3D> PhysicsWorld::getBodyVelocity3D(ecs::Entity entity) const {
    const auto found = _impl->find(entity, /*spatial=*/true);
    if (found == _impl->bodies.end()) {
        return std::nullopt;
    }
    const JPH::BodyInterface& bodyInterface = _impl->system.GetBodyInterface();
    return fromJoltVelocity3D(bodyInterface.GetLinearVelocity(found->second.body),
                              bodyInterface.GetAngularVelocity(found->second.body));
}

bool PhysicsWorld::isOutOfSync3D(ecs::Entity entity, const transform::Transform3D& transform) const {
    const auto found = _impl->find(entity, /*spatial=*/true);
    return found != _impl->bodies.end() && !found->second.isAt3D(transform.position, transform.rotation);
}

bool PhysicsWorld::isVelocityOutOfSync3D(ecs::Entity entity, const Velocity3D& velocity) const {
    const auto found = _impl->find(entity, /*spatial=*/true);
    return found != _impl->bodies.end() &&
           _impl->system.GetBodyInterface().GetMotionType(found->second.body) != JPH::EMotionType::Static &&
           found->second.lastVelocity3D != velocity;
}

void PhysicsWorld::moveKinematic3D(ecs::Entity entity, const transform::Transform3D& transform, float deltaSeconds) {
    const auto found = _impl->find(entity, /*spatial=*/true);
    if (found == _impl->bodies.end()) {
        return;
    }
    JPH::BodyInterface& bodyInterface = _impl->system.GetBodyInterface();
    if (bodyInterface.GetMotionType(found->second.body) != JPH::EMotionType::Kinematic) {
        return;
    }
    bodyInterface.MoveKinematic(found->second.body, detail::toJolt(transform.position),
                                detail::toJoltRotation3D(transform.rotation), deltaSeconds);
    found->second.remember3D(transform.position, transform.rotation);
    found->second.lastVelocity3D.reset();
    found->second.kinematicMoveStep = _impl->steps + 1;
}

void PhysicsWorld::teleport3D(ecs::Entity entity, const transform::Transform3D& transform) {
    const auto found = _impl->find(entity, /*spatial=*/true);
    if (found == _impl->bodies.end()) {
        return;
    }
    _impl->system.GetBodyInterface().SetPositionAndRotation(found->second.body, detail::toJolt(transform.position),
                                                            detail::toJoltRotation3D(transform.rotation),
                                                            JPH::EActivation::Activate);
    found->second.remember3D(transform.position, transform.rotation);
}

void PhysicsWorld::setVelocity3D(ecs::Entity entity, const Velocity3D& velocity) {
    const auto found = _impl->find(entity, /*spatial=*/true);
    if (found == _impl->bodies.end()) {
        return;
    }
    JPH::BodyInterface& bodyInterface = _impl->system.GetBodyInterface();
    if (bodyInterface.GetMotionType(found->second.body) == JPH::EMotionType::Static) {
        return;
    }
    bodyInterface.SetLinearAndAngularVelocity(found->second.body, detail::toJolt(velocity.linear),
                                              detail::toJolt(velocity.angular));
    found->second.lastVelocity3D = velocity;
}

void PhysicsWorld::applyForce3D(ecs::Entity entity, const glm::vec3& force) {
    _impl->queue(entity, Impl::PushKind::kForce, detail::toJolt(force), /*spatial=*/true);
}

void PhysicsWorld::applyImpulse3D(ecs::Entity entity, const glm::vec3& impulse) {
    _impl->queue(entity, Impl::PushKind::kImpulse, detail::toJolt(impulse), /*spatial=*/true);
}

void PhysicsWorld::applyTorque3D(ecs::Entity entity, const glm::vec3& torque) {
    _impl->queue(entity, Impl::PushKind::kTorque, detail::toJolt(torque), /*spatial=*/true);
}

std::vector<PhysicsWorld::BodyPose3D> PhysicsWorld::collectMovedBodies3D() {
    JPH::BodyIDVector active;
    _impl->system.GetActiveBodies(JPH::EBodyType::RigidBody, active);
    std::vector<BodyPose3D> poses;
    const JPH::BodyLockInterface& locks = _impl->system.GetBodyLockInterface();
    for (const JPH::BodyID id : active) {
        const JPH::BodyLockRead lock{locks, id};
        if (!lock.Succeeded() || lock.GetBody().IsStatic()) {
            continue;
        }
        const JPH::Body& body = lock.GetBody();
        const auto found = _impl->bodies.find(body.GetUserData());
        if (found == _impl->bodies.end() || !found->second.spatial ||
            (body.IsKinematic() && found->second.kinematicMoveStep == _impl->steps)) {
            continue;
        }
        const glm::vec3 position = detail::fromJolt(body.GetPosition());
        const glm::vec4 rotation = detail::fromJoltRotation3D(body.GetRotation());
        if (found->second.isAt3D(position, rotation)) {
            continue;
        }
        found->second.remember3D(position, rotation);
        poses.push_back(
            BodyPose3D{.entity = detail::entityFromBits(found->first), .position = position, .rotation = rotation});
    }
    return poses;
}

std::vector<PhysicsWorld::BodyVelocity3D> PhysicsWorld::collectChangedVelocities3D() {
    JPH::BodyIDVector active;
    _impl->system.GetActiveBodies(JPH::EBodyType::RigidBody, active);
    std::vector<BodyVelocity3D> velocities;
    const JPH::BodyLockInterface& locks = _impl->system.GetBodyLockInterface();
    for (const JPH::BodyID id : active) {
        const JPH::BodyLockRead lock{locks, id};
        if (!lock.Succeeded() || !lock.GetBody().IsDynamic()) {
            continue;
        }
        const JPH::Body& body = lock.GetBody();
        const auto found = _impl->bodies.find(body.GetUserData());
        if (found == _impl->bodies.end() || !found->second.spatial) {
            continue;
        }
        const Velocity3D velocity = fromJoltVelocity3D(body.GetLinearVelocity(), body.GetAngularVelocity());
        if (found->second.lastVelocity3D == velocity) {
            continue;
        }
        found->second.lastVelocity3D = velocity;
        velocities.push_back(BodyVelocity3D{.entity = detail::entityFromBits(found->first), .velocity = velocity});
    }
    return velocities;
}

namespace {
/// @return The surface normal of @p body at @p point, or zero if the body went away.
JPH::Vec3 normalAt(const JPH::BodyLockInterface& locks, const JPH::RayCastResult& result, JPH::RVec3Arg point) {
    const JPH::BodyLockRead lock{locks, result.mBodyID};
    return lock.Succeeded() ? lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, point)
                            : JPH::Vec3::sZero();
}

/// @return The entity of a hit body; nothing if it went away.
std::optional<ecs::Entity> entityOf(const JPH::BodyLockInterface& locks, JPH::BodyID body) {
    const JPH::BodyLockRead lock{locks, body};
    if (!lock.Succeeded()) {
        return std::nullopt;
    }
    return detail::entityFromBits(lock.GetBody().GetUserData());
}
}  // namespace

std::optional<PhysicsWorld::RayHit3D> PhysicsWorld::raycast3D(const glm::vec3& origin, const glm::vec3& direction,
                                                              float maxDistance, const QueryFilter& filter) const {
    const std::optional<JPH::RRayCast> ray = rayOf(origin, direction, maxDistance);
    if (!ray) {
        return std::nullopt;
    }
    const detail::MaskLayerFilter layerFilter{filter.mask, /*spatial=*/true};
    const detail::QueryBodyFilter bodyFilter{_impl->ignoredBody(filter), filter.includeSensors};
    JPH::RayCastResult result;
    if (!_impl->system.GetNarrowPhaseQuery().CastRay(*ray, result, {}, layerFilter, bodyFilter)) {
        return std::nullopt;
    }
    const JPH::BodyLockInterface& locks = _impl->system.GetBodyLockInterface();
    const JPH::RVec3 point = ray->GetPointOnRay(result.mFraction);
    const JPH::Vec3 normal = normalAt(locks, result, point);
    const std::optional<ecs::Entity> entity = entityOf(locks, result.mBodyID);
    if (!entity) {
        return std::nullopt;
    }
    return RayHit3D{.entity = *entity,
                    .point = detail::fromJolt(point),
                    .normal = detail::fromJolt(normal),
                    .distance = result.mFraction * maxDistance};
}

std::vector<PhysicsWorld::RayHit3D> PhysicsWorld::raycastAll3D(const glm::vec3& origin, const glm::vec3& direction,
                                                               float maxDistance, const QueryFilter& filter) const {
    const std::optional<JPH::RRayCast> ray = rayOf(origin, direction, maxDistance);
    if (!ray) {
        return {};
    }
    const detail::MaskLayerFilter layerFilter{filter.mask, /*spatial=*/true};
    const detail::QueryBodyFilter bodyFilter{_impl->ignoredBody(filter), filter.includeSensors};
    JPH::AllHitCollisionCollector<JPH::CastRayCollector> collector;
    _impl->system.GetNarrowPhaseQuery().CastRay(*ray, JPH::RayCastSettings{}, collector, {}, layerFilter, bodyFilter);
    collector.Sort();
    std::vector<RayHit3D> hits;
    std::vector<JPH::BodyID> seen;
    const JPH::BodyLockInterface& locks = _impl->system.GetBodyLockInterface();
    for (const JPH::RayCastResult& result : collector.mHits) {
        if (std::ranges::find(seen, result.mBodyID) != seen.end()) {
            continue;
        }
        seen.push_back(result.mBodyID);
        const JPH::RVec3 point = ray->GetPointOnRay(result.mFraction);
        const JPH::Vec3 normal = normalAt(locks, result, point);
        if (const std::optional<ecs::Entity> entity = entityOf(locks, result.mBodyID)) {
            hits.push_back(RayHit3D{.entity = *entity,
                                    .point = detail::fromJolt(point),
                                    .normal = detail::fromJolt(normal),
                                    .distance = result.mFraction * maxDistance});
        }
    }
    return hits;
}

}  // namespace aylo::plugins::physics

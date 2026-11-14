/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BodySettings
*/

#include "plugins/physics/detail/BodySettings.hpp"

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Core/Reference.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Body/AllowedDOFs.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/MotionQuality.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>

#include <algorithm>
#include <cstdint>
#include <format>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/Collider3D.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/detail/JoltMath.hpp"
#include "plugins/physics/detail/Layers.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/transform/Transform.hpp"

namespace aylo::plugins::physics::detail {
namespace {
/// @brief Turns Jolt's result into the shape, or throws its error.
/// @throws exceptions::PhysicsException When Jolt refused the shape.
JPH::RefConst<JPH::Shape> unwrap(const JPH::Shape::ShapeResult& result) {
    if (result.HasError()) {
        throw exceptions::PhysicsException{std::format("Invalid collider shape: {}", result.GetError())};
    }
    return result.Get();
}

/// @throws exceptions::PhysicsException When the value is not positive.
void requirePositive(float value, const char* name) {
    if (!(value > 0.0F)) {
        throw exceptions::PhysicsException{std::format("Collider::{} must be positive, got {}", name, value)};
    }
}

/// @return The shape before its offset, in meters.
JPH::RefConst<JPH::Shape> makeCenteredShape(const Collider& collider, float unitsPerMeter) {
    switch (collider.shape) {
        case ShapeType::kBox: {
            requirePositive(collider.size.x, "size.x");
            requirePositive(collider.size.y, "size.y");
            const glm::vec2 half = collider.size / (2.0F * unitsPerMeter);
            const float depth = std::min(half.x, half.y);
            const float convexRadius = std::min(JPH::cDefaultConvexRadius, depth);
            return unwrap(JPH::BoxShapeSettings{JPH::Vec3{half.x, half.y, depth}, convexRadius}.Create());
        }
        case ShapeType::kCircle:
            requirePositive(collider.radius, "radius");
            return unwrap(JPH::SphereShapeSettings{collider.radius / unitsPerMeter}.Create());
        case ShapeType::kCapsule:
            requirePositive(collider.radius, "radius");
            requirePositive(collider.halfHeight, "halfHeight");
            return unwrap(
                JPH::CapsuleShapeSettings{collider.halfHeight / unitsPerMeter, collider.radius / unitsPerMeter}
                    .Create());
    }
    throw exceptions::PhysicsException{"Unknown Collider::shape"};
}

/// @return The 3D shape before its offset, in metres.
JPH::RefConst<JPH::Shape> makeCenteredShape3D(const Collider3D& collider) {
    switch (collider.shape) {
        case Shape3DType::kBox: {
            requirePositive(collider.size.x, "size.x");
            requirePositive(collider.size.y, "size.y");
            requirePositive(collider.size.z, "size.z");
            const glm::vec3 half = collider.size * 0.5F;
            const float convexRadius = std::min({JPH::cDefaultConvexRadius, half.x, half.y, half.z});
            return unwrap(JPH::BoxShapeSettings{toJolt(half), convexRadius}.Create());
        }
        case Shape3DType::kSphere:
            requirePositive(collider.radius, "radius");
            return unwrap(JPH::SphereShapeSettings{collider.radius}.Create());
        case Shape3DType::kCapsule:
            requirePositive(collider.radius, "radius");
            requirePositive(collider.halfHeight, "halfHeight");
            return unwrap(JPH::CapsuleShapeSettings{collider.halfHeight, collider.radius}.Create());
    }
    throw exceptions::PhysicsException{"Unknown Collider3D::shape"};
}

/// @return The Jolt motion type of a body type.
JPH::EMotionType toMotionType(BodyType type) noexcept {
    switch (type) {
        case BodyType::kStatic:
            return JPH::EMotionType::Static;
        case BodyType::kKinematic:
            return JPH::EMotionType::Kinematic;
        case BodyType::kDynamic:
            return JPH::EMotionType::Dynamic;
    }
    return JPH::EMotionType::Static;
}
}  // namespace

JPH::RefConst<JPH::Shape> makeShape(const Collider& collider, float unitsPerMeter) {
    JPH::RefConst<JPH::Shape> shape = makeCenteredShape(collider, unitsPerMeter);
    if (collider.offset == glm::vec2{0.0F, 0.0F}) {
        return shape;
    }
    return unwrap(
        JPH::RotatedTranslatedShapeSettings{toJolt2D(collider.offset / unitsPerMeter), JPH::Quat::sIdentity(), shape}
            .Create());
}

namespace {
/// @return The object layer of a collider's layer and mask.
/// @throws exceptions::PhysicsException When they do not fit (layers::isValid()).
JPH::ObjectLayer layerOf(std::uint32_t categories, std::uint32_t mask, bool isStatic, bool spatial) {
    if (!layers::isValid(categories, mask)) {
        throw exceptions::PhysicsException{
            std::format("Collider::layer must name at least one category and neither layer nor mask may go beyond "
                        "kAllLayers ({:#x}), "
                        "got layer {:#x} and mask {:#x}",
                        kAllLayers, categories, mask)};
    }
    return layers::encode(categories, mask, isStatic, spatial);
}

/// @brief Gives a moving body the motion its RigidBody asks for; @p allowedDOFs are the axes it moves along.
/// @throws exceptions::PhysicsException When a dynamic body's mass is not positive.
void applyMotion(JPH::BodyCreationSettings& settings, const RigidBody& body, JPH::EAllowedDOFs allowedDOFs) {
    settings.mAllowedDOFs = allowedDOFs;
    settings.mLinearDamping = body.linearDamping;
    settings.mAngularDamping = body.angularDamping;
    settings.mGravityFactor = body.gravityScale;
    settings.mMotionQuality =
        body.continuousCollision ? JPH::EMotionQuality::LinearCast : JPH::EMotionQuality::Discrete;
    if (settings.mMotionType == JPH::EMotionType::Dynamic) {
        if (!(body.mass > 0.0F)) {
            throw exceptions::PhysicsException{std::format("RigidBody::mass must be positive, got {}", body.mass)};
        }
        settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = body.mass;
    }
}
}  // namespace

JPH::BodyCreationSettings makeBodySettings(const Collider& collider, const RigidBody* body,
                                           const transform::Transform2D& transform, float unitsPerMeter,
                                           std::uint64_t userData) {
    const JPH::EMotionType motion = body != nullptr ? toMotionType(body->type) : JPH::EMotionType::Static;
    const JPH::ObjectLayer layer =
        layerOf(collider.layer, collider.mask, motion == JPH::EMotionType::Static, /*spatial=*/false);
    JPH::BodyCreationSettings settings{makeShape(collider, unitsPerMeter), toJolt2D(transform.position / unitsPerMeter),
                                       toJoltRotation2D(transform.rotation), motion, layer};
    settings.mUserData = userData;
    settings.mIsSensor = collider.sensor;
    settings.mFriction = collider.friction;
    settings.mRestitution = collider.restitution;
    if (motion != JPH::EMotionType::Static) {
        applyMotion(settings, *body,
                    body->fixedRotation ? JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY
                                        : JPH::EAllowedDOFs::Plane2D);
    }
    return settings;
}

JPH::RefConst<JPH::Shape> makeShape3D(const Collider3D& collider) {
    JPH::RefConst<JPH::Shape> shape = makeCenteredShape3D(collider);
    if (collider.offset == glm::vec3{0.0F}) {
        return shape;
    }
    return unwrap(JPH::RotatedTranslatedShapeSettings{toJolt(collider.offset), JPH::Quat::sIdentity(), shape}.Create());
}

JPH::BodyCreationSettings makeBodySettings3D(const Collider3D& collider, const RigidBody* body,
                                             const transform::Transform3D& transform, std::uint64_t userData) {
    const JPH::EMotionType motion = body != nullptr ? toMotionType(body->type) : JPH::EMotionType::Static;
    const JPH::ObjectLayer layer =
        layerOf(collider.layer, collider.mask, motion == JPH::EMotionType::Static, /*spatial=*/true);
    JPH::BodyCreationSettings settings{makeShape3D(collider), toJolt(transform.position),
                                       toJoltRotation3D(transform.rotation), motion, layer};
    settings.mUserData = userData;
    settings.mIsSensor = collider.sensor;
    settings.mFriction = collider.friction;
    settings.mRestitution = collider.restitution;
    if (motion != JPH::EMotionType::Static) {
        // The world applies the 3D gravity itself (PhysicsWorld::step): Jolt's own is the 2D one.
        applyMotion(settings, *body,
                    body->fixedRotation ? JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY |
                                              JPH::EAllowedDOFs::TranslationZ
                                        : JPH::EAllowedDOFs::All);
        settings.mGravityFactor = 0.0F;
    }
    return settings;
}

}  // namespace aylo::plugins::physics::detail

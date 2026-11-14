/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsCalls
*/

#include "plugins/physics/PhysicsCalls.hpp"

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/MoveAndSlide.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/QueryFilter.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"

namespace aylo::plugins::physics {
namespace {
/// @return The QueryFilter the call fields name; a null @p ignore skips no entity.
[[nodiscard]] QueryFilter filterOf(const std::uint32_t mask, const ecs::Entity ignore, const bool includeSensors) {
    QueryFilter filter{.mask = mask, .includeSensors = includeSensors};
    if (!ignore.isNull()) {
        filter.ignore = ignore;
    }
    return filter;
}

/// @return A Collider with the shape fields of call arguments, the others left at their defaults.
template <typename Arguments>
[[nodiscard]] Collider shapeOf(const Arguments& arguments) {
    return Collider{
        .shape = arguments.shape,
        .size = arguments.size,
        .radius = arguments.radius,
        .halfHeight = arguments.halfHeight,
        .offset = arguments.offset,
    };
}

/// @return The result of a ray or shape cast.
[[nodiscard]] HitResult resultOf(const std::optional<PhysicsWorld::RayHit>& hit) {
    if (!hit) {
        return HitResult{};
    }
    return HitResult{
        .hit = true,
        .entity = hit->entity,
        .point = hit->point,
        .normal = hit->normal,
        .distance = hit->distance,
    };
}

HitResult raycast(ecs::Res<PhysicsWorld> world, const RaycastArguments& arguments) {
    return resultOf(world->raycast(arguments.origin, arguments.direction, arguments.maxDistance,
                                   filterOf(arguments.mask, arguments.ignore, arguments.includeSensors)));
}

HitResult shapeCast(ecs::Res<PhysicsWorld> world, const ShapeCastArguments& arguments) {
    return resultOf(world->shapeCast(shapeOf(arguments), arguments.from, arguments.direction, arguments.maxDistance,
                                     filterOf(arguments.mask, arguments.ignore, arguments.includeSensors)));
}

OverlapResult overlap(ecs::Res<PhysicsWorld> world, const OverlapArguments& arguments) {
    const std::vector<ecs::Entity> entities = world->overlap(
        shapeOf(arguments), arguments.at, filterOf(arguments.mask, arguments.ignore, arguments.includeSensors));
    if (entities.empty()) {
        return OverlapResult{};
    }
    return OverlapResult{.count = static_cast<std::uint32_t>(entities.size()), .first = entities.front()};
}

MoveAndSlideResult slide(ecs::Res<PhysicsWorld> world, ecs::Query<const Collider> colliders,
                         const MoveAndSlideArguments& arguments) {
    if (const auto found = colliders.get(arguments.entity)) {
        const auto& [entity, collider] = *found;
        return MoveAndSlideResult{
            .position = physics::moveAndSlide(*world, entity, collider, arguments.from, arguments.delta,
                                              SlideSettings{.skin = arguments.skin, .maxSlides = arguments.maxSlides}),
        };
    }
    throw exceptions::PhysicsException{
        std::format("{}: entity {} has no Collider", kMoveAndSlideCall, arguments.entity.getIndex())};
}

void applyForce(ecs::ResMut<PhysicsWorld> world, const PushArguments& arguments) {
    world->applyForce(arguments.entity, arguments.value);
}

void applyImpulse(ecs::ResMut<PhysicsWorld> world, const PushArguments& arguments) {
    world->applyImpulse(arguments.entity, arguments.value);
}

void applyTorque(ecs::ResMut<PhysicsWorld> world, const TorqueArguments& arguments) {
    world->applyTorque(arguments.entity, arguments.torque);
}

void setGravity(ecs::ResMut<PhysicsWorld> world, const GravityArguments& arguments) {
    world->setGravity(arguments.gravity);
}

Hit3DResult raycast3D(ecs::Res<PhysicsWorld> world, const Raycast3DArguments& arguments) {
    const std::optional<PhysicsWorld::RayHit3D> hit =
        world->raycast3D(arguments.origin, arguments.direction, arguments.maxDistance,
                         filterOf(arguments.mask, arguments.ignore, arguments.includeSensors));
    if (!hit) {
        return Hit3DResult{};
    }
    return Hit3DResult{
        .hit = true, .entity = hit->entity, .point = hit->point, .normal = hit->normal, .distance = hit->distance};
}

void applyForce3D(ecs::ResMut<PhysicsWorld> world, const Push3DArguments& arguments) {
    world->applyForce3D(arguments.entity, arguments.value);
}

void applyImpulse3D(ecs::ResMut<PhysicsWorld> world, const Push3DArguments& arguments) {
    world->applyImpulse3D(arguments.entity, arguments.value);
}

void applyTorque3D(ecs::ResMut<PhysicsWorld> world, const Push3DArguments& arguments) {
    world->applyTorque3D(arguments.entity, arguments.value);
}

void setGravity3D(ecs::ResMut<PhysicsWorld> world, const Gravity3DArguments& arguments) {
    world->setGravity3D(arguments.gravity);
}
}  // namespace

void addPhysicsCalls(ecs::App& app) {
    app.addCall(std::string{kRaycastCall}, &raycast);
    app.addCall(std::string{kShapeCastCall}, &shapeCast);
    app.addCall(std::string{kOverlapCall}, &overlap);
    app.addCall(std::string{kMoveAndSlideCall}, &slide);
    app.addCall(std::string{kApplyForceCall}, &applyForce);
    app.addCall(std::string{kApplyImpulseCall}, &applyImpulse);
    app.addCall(std::string{kApplyTorqueCall}, &applyTorque);
    app.addCall(std::string{kSetGravityCall}, &setGravity);
    app.addCall(std::string{kRaycast3DCall}, &raycast3D);
    app.addCall(std::string{kApplyForce3DCall}, &applyForce3D);
    app.addCall(std::string{kApplyImpulse3DCall}, &applyImpulse3D);
    app.addCall(std::string{kApplyTorque3DCall}, &applyTorque3D);
    app.addCall(std::string{kSetGravity3DCall}, &setGravity3D);
}

}  // namespace aylo::plugins::physics

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TransformPlugin
*/

#include "plugins/transform/TransformPlugin.hpp"

#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/change/Mut.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/hierarchy/Children.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryTerms.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/time/Time.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformMath.hpp"

namespace aylo::plugins::transform {
namespace {
/// @brief What tells the two families apart: the world pose of a local one, and the poses an interpolation keeps.
template <typename Local>
struct Family;

template <>
struct Family<Transform2D> {
    using Global = GlobalTransform2D;
    static InterpolatedPoses<Transform2D>& poses(TransformInterpolation& state) { return state.poses2D; }
    static const InterpolatedPoses<Transform2D>& poses(const TransformInterpolation& state) { return state.poses2D; }
};

template <>
struct Family<Transform3D> {
    using Global = GlobalTransform3D;
    static InterpolatedPoses<Transform3D>& poses(TransformInterpolation& state) { return state.poses3D; }
    static const InterpolatedPoses<Transform3D>& poses(const TransformInterpolation& state) { return state.poses3D; }
};

/// @return The world pose of an entity without a parent: its local pose.
template <typename Local>
Family<Local>::Global toGlobal(const Local& transform) {
    return typename Family<Local>::Global{
        .position = transform.position, .rotation = transform.rotation, .scale = transform.scale};
}

/// @return Whether @p first and @p second are the same pose (local or world).
template <typename Pose>
bool samePose(const Pose& first, const Pose& second) {
    return first.position == second.position && first.rotation == second.rotation && first.scale == second.scale;
}

/// @brief Gives a world pose to the entities of one family spawned this frame, combined with their parents.
template <typename Local>
void insertFamily(
    ecs::Commands& commands,
    ecs::Query<const Local, ecs::Optional<const ecs::Parent>, ecs::Without<typename Family<Local>::Global>>& fresh,
    ecs::Query<const typename Family<Local>::Global>& placed) {
    using Global = Family<Local>::Global;
    std::unordered_map<ecs::Entity, Global> known;
    std::vector<std::tuple<ecs::Entity, const Local*, ecs::Entity>> waiting;
    for (auto [entity, transform, parent] : fresh) {
        if (parent == nullptr) {
            known.emplace(entity, toGlobal(transform));
            commands.entity(entity).add(toGlobal(transform));
        } else {
            waiting.emplace_back(entity, &transform, parent->entity);
        }
    }
    if (waiting.empty()) {
        return;
    }
    for (auto [entity, global] : placed) {
        known.emplace(entity, global);
    }
    bool progressed = true;
    while (progressed && !waiting.empty()) {
        progressed = false;
        std::erase_if(waiting, [&](const auto& entry) {
            const auto& [entity, transform, parent] = entry;
            const auto found = known.find(parent);
            if (found == known.end()) {
                return false;
            }
            const Global global = combine(found->second, *transform);
            known.emplace(entity, global);
            commands.entity(entity).add(global);
            progressed = true;
            return true;
        });
    }
    // A parent of the other family, or none placed yet: a root.
    for (const auto& [entity, transform, parent] : waiting) {
        commands.entity(entity).add(toGlobal(*transform));
    }
}

/// @brief PostUpdate: gives a world pose to the entities that have a transform of either family but none yet.
void insertGlobalTransforms(
    ecs::Commands& commands,
    ecs::Query<const Transform2D, ecs::Optional<const ecs::Parent>, ecs::Without<GlobalTransform2D>> fresh2D,
    ecs::Query<const GlobalTransform2D> placed2D,
    ecs::Query<const Transform3D, ecs::Optional<const ecs::Parent>, ecs::Without<GlobalTransform3D>> fresh3D,
    ecs::Query<const GlobalTransform3D> placed3D) {
    insertFamily<Transform2D>(commands, fresh2D, placed2D);
    insertFamily<Transform3D>(commands, fresh3D, placed3D);
}

/// @brief Records the pose of each interpolated entity of one family after the fixed ticks of the frame and blends the
/// last two by Time::alpha.
/// @details A tick ran since the last record: the pose then becomes `previous` and the transform now `current`. No tick
/// ran but the transform changed: it was moved out of the ticks (a teleport, the editor), and is drawn there at once.
template <typename Local>
void interpolateFamily(ecs::Query<TransformInterpolation, const Local>& entities, const ecs::Time& time) {
    for (auto [entity, state, transform] : entities) {
        InterpolatedPoses<Local>& poses = Family<Local>::poses(*state);
        if (!state->recorded) {
            poses = InterpolatedPoses<Local>{.previous = transform, .current = transform, .drawn = transform};
            state->tick = time.tick;
            state->recorded = true;
            continue;
        }
        if (state->tick != time.tick) {
            poses.previous = poses.current;
            poses.current = transform;
            state->tick = time.tick;
        } else if (!samePose(poses.current, transform)) {
            poses.previous = transform;
            poses.current = transform;
        }
        poses.drawn = interpolate(poses.previous, poses.current, time.alpha);
    }
}

/// @brief PostUpdate, on the client: blends the fixed ticks of the interpolated entities of both families.
void interpolateTransforms(ecs::Query<TransformInterpolation, const Transform2D> entities2D,
                           ecs::Query<TransformInterpolation, const Transform3D> entities3D, ecs::Res<ecs::Time> time) {
    interpolateFamily<Transform2D>(entities2D, *time);
    interpolateFamily<Transform3D>(entities3D, *time);
}

/// @return What the world pose is computed from: the blended pose of an entity interpolated on this side, its
/// transform otherwise.
template <typename Local>
const Local* drawnLocal(const Local* transform, const TransformInterpolation* interpolation) {
    return interpolation != nullptr && interpolation->recorded ? &Family<Local>::poses(*interpolation).drawn
                                                               : transform;
}

/// @brief What the walk down a hierarchy needs of one entity.
template <typename Local>
struct Node {
    ecs::Mut<typename Family<Local>::Global> global;  ///< Written by the walk.
    const Local* local;                               ///< Relative to its parent.
    const ecs::Parent* parent;                        ///< Null for a root.
    const ecs::Children* children;                    ///< Null without children.
};

/// @brief A child to compute: the entity, and where its parent is in the world.
template <typename Local>
struct Step {
    ecs::Entity entity;
    Family<Local>::Global parent;
};

/// @brief Writes `value` only if it differs, so an entity that did not move is not marked as changed.
template <typename Global>
void update(ecs::Mut<Global>& global, const Global& value) {
    if (!samePose(global.get(), value)) {
        *global = value;
    }
}

/// @brief Queues the children of `node`, which sits at `global` in the world.
template <typename Local>
void pushChildren(const Node<Local>& node, const typename Family<Local>::Global& global,
                  std::vector<Step<Local>>& steps) {
    if (node.children != nullptr) {
        for (const ecs::Entity child : node.children->entities) {
            steps.push_back(Step<Local>{.entity = child, .parent = global});
        }
    }
}

/// @brief The query of one family that propagation walks.
template <typename Local>
using FamilyQuery = ecs::Query<typename Family<Local>::Global, const Local, ecs::Optional<const ecs::Parent>,
                               ecs::Optional<const ecs::Children>, ecs::Optional<const TransformInterpolation>>;

/// @brief Computes every world pose of one family, from the roots down. A child whose parent is not of the family
/// (another family's entity, or one without a transform) is a root.
template <typename Local>
void propagateFamily(FamilyQuery<Local>& transforms) {
    using Global = Family<Local>::Global;
    std::unordered_map<ecs::Entity, Node<Local>> nodes;
    for (auto [entity, global, transform, parent, children, interpolation] : transforms) {
        const Local* local = drawnLocal(&transform, interpolation);
        if (parent == nullptr && children == nullptr) {
            update(global, toGlobal(*local));
            continue;
        }
        nodes.emplace(entity, Node<Local>{.global = global, .local = local, .parent = parent, .children = children});
    }
    std::vector<Step<Local>> steps;
    for (auto& [entity, node] : nodes) {
        if (node.parent == nullptr || !nodes.contains(node.parent->entity)) {
            const Global global = toGlobal(*node.local);
            update(node.global, global);
            pushChildren(node, global, steps);
        }
    }
    while (!steps.empty()) {
        const Step<Local> step = steps.back();
        steps.pop_back();
        const auto found = nodes.find(step.entity);
        if (found != nodes.end()) {
            const Global global = combine(step.parent, *found->second.local);
            update(found->second.global, global);
            pushChildren(found->second, global, steps);
        }
    }
}

/// @brief PostUpdate: computes every world pose of both families.
void propagateTransforms(FamilyQuery<Transform2D> transforms2D, FamilyQuery<Transform3D> transforms3D) {
    propagateFamily<Transform2D>(transforms2D);
    propagateFamily<Transform3D>(transforms3D);
}
}  // namespace

std::string_view TransformPlugin::getName() const noexcept { return kName; }

void TransformPlugin::build(ecs::App& app) {
    static_cast<void>(app.getWorld().getComponents().registerType<Transform2D>());
    static_cast<void>(app.getWorld().getComponents().registerType<GlobalTransform2D>());
    static_cast<void>(app.getWorld().getComponents().registerType<Transform3D>());
    static_cast<void>(app.getWorld().getComponents().registerType<GlobalTransform3D>());
    static_cast<void>(app.getWorld().getComponents().registerType<TransformInterpolation>());
    static_cast<void>(app.getWorld().getComponents().registerType<ecs::Parent>(ecs::kParentOptions));
    static_cast<void>(app.getWorld().getComponents().registerType<ecs::Children>());
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kInsertSystem}, &insertGlobalTransforms)
        .setSide(ecs::Side::kBoth);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kInterpolateSystem}, &interpolateTransforms)
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kPropagateSystem}, &propagateTransforms)
        .after(std::string{kInsertSystem})
        .after(std::string{kInterpolateSystem})
        .setSide(ecs::Side::kBoth);
}

}  // namespace aylo::plugins::transform

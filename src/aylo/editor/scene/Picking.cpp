/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Picking
*/

#include "editor/scene/Picking.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include "aylo/contract/renderer/view/CameraView.hpp"
#include "aylo/contract/renderer/view/CameraView3D.hpp"
#include "aylo/contract/renderer/view/View3D.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/entity/Name.hpp"
#include "aylo/ecs/hierarchy/Children.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/world/World.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/transform/Transform.hpp"

namespace aylo::editor::scene {
namespace {
constexpr float kMinPickRadius = 0.25F;  ///< The smallest sphere a 3D entity is picked by, in world units.

/// @return The entities that have the component @p Component.
template <typename Component>
std::span<const ecs::Entity> entitiesWith(const ecs::World& world) {
    const ecs::Components& components = world.getComponents();
    const std::optional<ecs::ComponentId> id = components.find(ecs::ComponentTraits<Component>::kName);
    const auto* pool = id ? components.findPool(*id) : nullptr;
    return pool != nullptr ? pool->getEntities() : std::span<const ecs::Entity>{};
}

/// @return How far along @p ray it meets the sphere, or nothing.
std::optional<float> hitSphere(const contract::renderer::Ray3D& ray, glm::vec3 center, float radius) {
    const glm::vec3 toCenter = center - ray.origin;
    const float along = glm::dot(toCenter, ray.direction);
    const float across = glm::dot(toCenter, toCenter) - (along * along);
    if (across > radius * radius) {
        return std::nullopt;
    }
    const float half = std::sqrt((radius * radius) - across);
    const float near = along - half;
    const float far = along + half;
    if (far < 0.0F) {
        return std::nullopt;
    }
    return near >= 0.0F ? near : far;
}

/// @return The name node @p index of @p capsule gives its entity: its own Name if it has one, else its name.
std::string_view expectedName(const ecs::World& world, const plugins::capsule::Capsule& capsule, std::uint32_t index) {
    const plugins::capsule::CapsuleNode& node = *std::next(capsule.getNodes().begin(), index);
    const std::optional<ecs::ComponentId> id = world.getComponents().find(ecs::ComponentTraits<ecs::Name>::kName);
    if (id && node.values.has(*id)) {
        const std::span<const std::byte> bytes = node.values.get(*id);
        if (bytes.size() >= sizeof(ecs::Name)) {
            return reinterpret_cast<const ecs::Name*>(bytes.data())->value.view();  // NOLINT(*-reinterpret-cast)
        }
    }
    return node.name;
}

/// @return The child of @p entity named @p name.
std::optional<ecs::Entity> childNamed(const ecs::World& world, ecs::Entity entity, std::string_view name) {
    const auto* children = world.getComponents().get<ecs::Children>(entity);
    if (children == nullptr) {
        return std::nullopt;
    }
    for (const ecs::Entity child : children->entities) {
        const auto* label = world.getComponents().get<ecs::Name>(child);
        if (label != nullptr && label->value.view() == name) {
            return child;
        }
    }
    return std::nullopt;
}
}  // namespace

std::optional<ecs::Entity> pick3D(const ecs::World& world, const contract::renderer::CameraView3D& view,
                                  glm::vec2 screen) {
    const contract::renderer::Ray3D ray = view.screenToRay(screen);
    std::optional<ecs::Entity> best;
    float nearest = 0.0F;
    for (const ecs::Entity entity : entitiesWith<plugins::transform::GlobalTransform3D>(world)) {
        const auto* pose = world.getComponents().get<plugins::transform::GlobalTransform3D>(entity);
        if (pose == nullptr) {
            continue;
        }
        const float radius =
            std::max(kMinPickRadius,
                     0.5F * std::max({std::abs(pose->scale.x), std::abs(pose->scale.y), std::abs(pose->scale.z)}));
        const std::optional<float> hit = hitSphere(ray, pose->position, radius);
        if (hit && (!best || *hit < nearest)) {
            best = entity;
            nearest = *hit;
        }
    }
    return best;
}

std::optional<ecs::Entity> pick2D(const ecs::World& world, const contract::renderer::CameraView& view, glm::vec2 screen,
                                  float tolerance) {
    std::optional<ecs::Entity> best;
    float nearest = tolerance;
    for (const ecs::Entity entity : entitiesWith<plugins::transform::GlobalTransform2D>(world)) {
        const auto* pose = world.getComponents().get<plugins::transform::GlobalTransform2D>(entity);
        if (pose == nullptr) {
            continue;
        }
        const float distance = glm::length(view.worldToScreen(pose->position) - screen);
        if (distance <= nearest) {
            best = entity;
            nearest = distance;
        }
    }
    return best;
}

std::optional<ecs::Entity> rootOf(const ecs::World& world, const plugins::capsule::Capsule& capsule) {
    for (const ecs::Entity entity : entitiesWith<plugins::capsule::CapsuleInstance>(world)) {
        const auto* instance = world.getComponents().get<plugins::capsule::CapsuleInstance>(entity);
        if (instance != nullptr && !instance->nested && instance->capsule == capsule.getId()) {
            return entity;
        }
    }
    return std::nullopt;
}

std::optional<ecs::Entity> entityOfNode(const ecs::World& world, const plugins::capsule::Capsule& capsule,
                                        std::uint32_t node) {
    const auto nodes = capsule.getNodes();
    if (node >= nodes.size()) {
        return std::nullopt;
    }
    std::vector<std::uint32_t> path;  // from the node up to the root's child
    for (std::uint32_t index = node; index != 0; index = std::next(nodes.begin(), index)->parent) {
        path.push_back(index);
    }
    std::optional<ecs::Entity> entity = rootOf(world, capsule);
    for (auto step = path.rbegin(); entity && step != path.rend(); ++step) {
        entity = childNamed(world, *entity, expectedName(world, capsule, *step));
    }
    return entity;
}

std::optional<std::uint32_t> nodeOfEntity(const ecs::World& world, const plugins::capsule::Capsule& capsule,
                                          ecs::Entity entity) {
    const std::optional<ecs::Entity> root = rootOf(world, capsule);
    if (!root) {
        return std::nullopt;
    }
    std::vector<ecs::Entity> chain;  // from the entity up to, not including, the root
    ecs::Entity current = entity;
    while (current != *root) {
        chain.push_back(current);
        const auto* parent = world.getComponents().get<ecs::Parent>(current);
        if (parent == nullptr) {
            return std::nullopt;  // not under the capsule's root
        }
        current = parent->entity;
    }
    std::uint32_t node = 0;
    const auto nodes = capsule.getNodes();
    for (const ecs::Entity step : std::views::reverse(chain)) {
        const auto* label = world.getComponents().get<ecs::Name>(step);
        std::optional<std::uint32_t> found;
        for (std::uint32_t index = 1; label != nullptr && index < nodes.size(); ++index) {
            if (std::next(nodes.begin(), index)->parent == node &&
                expectedName(world, capsule, index) == label->value.view()) {
                found = index;
                break;
            }
        }
        if (!found) {
            return node;  // inside an instance of another capsule: its node
        }
        node = *found;
    }
    return node;
}

}  // namespace aylo::editor::scene

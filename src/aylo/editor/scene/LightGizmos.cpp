/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** LightGizmos
*/

#include "editor/scene/LightGizmos.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/trigonometric.hpp>
#include <numbers>
#include <optional>
#include <vector>

#include "aylo/contract/renderer/drawing/Lines3D.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/world/World.hpp"
#include "plugins/graphics/Light.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformMath.hpp"

namespace aylo::editor::scene {

namespace {

using plugins::graphics::LightKind;

constexpr int kCircleSegments = 24;         ///< Segments of a circle.
constexpr float kDefaultPointRange = 1.0F;  ///< The circles of a point light without a range.
constexpr float kDefaultSpotRange = 5.0F;   ///< The cone of a spot light without a range.
constexpr float kArrowLength = 1.5F;        ///< A directional light's arrow.

/// @brief Adds a circle of @p radius around the origin in the plane of @p first and @p second, in the light's space.
void addCircle(std::vector<glm::vec3>& points, glm::vec3 center, glm::vec3 first, glm::vec3 second, float radius) {
    for (int segment = 0; segment < kCircleSegments; ++segment) {
        const auto angle = [](int index) {
            return 2.0F * std::numbers::pi_v<float> * static_cast<float>(index) / kCircleSegments;
        };
        for (const int index : {segment, segment + 1}) {
            points.push_back(center +
                             (radius * ((std::cos(angle(index)) * first) + (std::sin(angle(index)) * second))));
        }
    }
}

/// @return The lines of @p light, in its own space (it shines along -Z).
std::vector<glm::vec3> shapeOf(const plugins::graphics::Light3D& light) {
    const glm::vec3 x{1.0F, 0.0F, 0.0F};
    const glm::vec3 y{0.0F, 1.0F, 0.0F};
    const glm::vec3 z{0.0F, 0.0F, 1.0F};
    std::vector<glm::vec3> points;
    switch (light.kind) {
        case LightKind::kDirectional:
            points = {glm::vec3{0.0F},   -kArrowLength * z,
                      -kArrowLength * z, (-0.8F * kArrowLength * z) + (0.2F * x),
                      -kArrowLength * z, (-0.8F * kArrowLength * z) - (0.2F * x)};
            addCircle(points, glm::vec3{0.0F}, x, y, 0.3F);
            break;
        case LightKind::kPoint: {
            const float radius = light.range > 0.0F ? light.range : kDefaultPointRange;
            addCircle(points, glm::vec3{0.0F}, x, y, radius);
            addCircle(points, glm::vec3{0.0F}, x, z, radius);
            addCircle(points, glm::vec3{0.0F}, y, z, radius);
            break;
        }
        case LightKind::kSpot: {
            const float length = light.range > 0.0F ? light.range : kDefaultSpotRange;
            const float radius = length * std::tan(glm::radians(std::min(light.outerConeAngle, 89.0F)));
            addCircle(points, -length * z, x, y, radius);
            for (const glm::vec3 side : {x, -x, y, -y}) {
                points.emplace_back(0.0F);
                points.emplace_back((-length * z) + (radius * side));
            }
            break;
        }
        case LightKind::kArea: {
            const glm::vec3 halfX = 0.5F * light.size.x * x;
            const glm::vec3 halfY = 0.5F * light.size.y * y;
            const std::vector<glm::vec3> corners{halfX + halfY, halfX - halfY, -halfX - halfY, -halfX + halfY};
            for (std::size_t corner = 0; corner < corners.size(); ++corner) {
                points.push_back(corners.at(corner));
                points.push_back(corners.at((corner + 1) % corners.size()));
            }
            points.emplace_back(0.0F);
            points.emplace_back(-0.5F * z);
            break;
        }
    }
    return points;
}

}  // namespace

std::vector<contract::renderer::Lines3D> lightGizmos(const ecs::World& world) {
    const ecs::Components& components = world.getComponents();
    const std::optional<ecs::ComponentId> id = components.find(ecs::ComponentTraits<plugins::graphics::Light3D>::kName);
    const auto* pool = id ? components.findPool(*id) : nullptr;
    std::vector<contract::renderer::Lines3D> gizmos;
    if (pool == nullptr) {
        return gizmos;
    }
    for (const ecs::Entity entity : pool->getEntities()) {
        const auto* light = components.get<plugins::graphics::Light3D>(entity);
        const auto* global = components.get<plugins::transform::GlobalTransform3D>(entity);
        if (light == nullptr || global == nullptr) {
            continue;
        }
        const glm::vec4 color = light->enabled ? glm::vec4{glm::min(light->color, glm::vec3{1.0F}), 1.0F}
                                               : glm::vec4{0.5F, 0.5F, 0.5F, 1.0F};
        // Unscaled: the gizmo shows the light's own range and size.
        plugins::transform::GlobalTransform3D placed = *global;
        placed.scale = glm::vec3{1.0F};
        gizmos.push_back(contract::renderer::Lines3D{
            .points = shapeOf(*light), .color = color, .world = plugins::transform::toMatrix(placed)});
    }
    return gizmos;
}

}  // namespace aylo::editor::scene

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Picking
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <optional>

#include "aylo/contract/renderer/view/CameraView.hpp"
#include "aylo/contract/renderer/view/CameraView3D.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "editor/Export.hpp"
#include "plugins/capsule/Capsule.hpp"

namespace aylo::ecs {
class World;
}  // namespace aylo::ecs

/// What the Scene panel clicks on: the entity under the mouse, and the capsule node it was spawned from.
namespace aylo::editor::scene {

/// @return The 3D entity whose sphere (half its largest scale across, at least kMinPickRadius) the ray under @p screen
/// (pixels of @p view's framebuffer) meets first; nothing when it meets none.
[[nodiscard]] AYLO_EDITOR_API std::optional<ecs::Entity> pick3D(const ecs::World& world,
                                                                const contract::renderer::CameraView3D& view,
                                                                glm::vec2 screen);

/// @return The 2D entity whose origin is shown nearest @p screen, within @p tolerance pixels; nothing when none is.
[[nodiscard]] AYLO_EDITOR_API std::optional<ecs::Entity> pick2D(const ecs::World& world,
                                                                const contract::renderer::CameraView& view,
                                                                glm::vec2 screen, float tolerance);

/// @return The entity the capsule's root was spawned as, by itself (not nested); nothing when it is not in the world.
[[nodiscard]] AYLO_EDITOR_API std::optional<ecs::Entity> rootOf(const ecs::World& world,
                                                                const plugins::capsule::Capsule& capsule);

/// @return The entity node @p node of @p capsule was spawned as: found from the root down, by the names of the nodes.
[[nodiscard]] AYLO_EDITOR_API std::optional<ecs::Entity> entityOfNode(const ecs::World& world,
                                                                      const plugins::capsule::Capsule& capsule,
                                                                      std::uint32_t node);

/// @return The node of @p capsule @p entity was spawned from; for an entity inside an instance of another capsule, the
/// instance's node. Nothing when it does not belong to the capsule.
[[nodiscard]] AYLO_EDITOR_API std::optional<std::uint32_t> nodeOfEntity(const ecs::World& world,
                                                                        const plugins::capsule::Capsule& capsule,
                                                                        ecs::Entity entity);

}  // namespace aylo::editor::scene

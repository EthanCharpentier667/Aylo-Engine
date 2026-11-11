/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Isolation
*/

#include "Isolation.hpp"

#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string>

#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/entity/Name.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/type/String.hpp"
#include "aylo/ecs/world/World.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "plugins/transform/Transform.hpp"

namespace aylo::editor::scene {

std::optional<Isolation> isolate(ecs::World& world, plugins::capsule::CapsuleId id, std::string& error) {
    const auto* capsules = world.getResources().get<plugins::capsule::Capsules>();
    if (capsules == nullptr) {
        error = "The game has no capsules";
        return std::nullopt;
    }
    Isolation isolation{.capsule = id};
    try {
        ecs::Commands commands{world};
        isolation.anchor = commands.spawn()
                               .add(plugins::transform::Transform{.position = kIsolatedOrigin})
                               .add(ecs::Name{.value = ecs::String{"Prefab mode"}})
                               .getEntity();
        isolation.root = plugins::capsule::spawnCapsule(commands, *capsules, id)
                             .add(ecs::Parent{.entity = isolation.anchor})
                             .getEntity();
        commands.flush();
    } catch (const plugins::capsule::exceptions::CapsuleException& failure) {
        error = failure.what();
        return std::nullopt;
    }
    return isolation;
}

glm::vec2 centerOf(const ecs::World& world, const Isolation& isolation) {
    const auto* root = world.getComponents().get<plugins::transform::GlobalTransform>(isolation.root);
    return root != nullptr ? root->position : kIsolatedOrigin;
}

void endIsolation(ecs::World& world, const Isolation& isolation) {
    if (world.isAlive(isolation.anchor)) {
        static_cast<void>(world.despawn(isolation.anchor));
    }
}

}  // namespace aylo::editor::scene

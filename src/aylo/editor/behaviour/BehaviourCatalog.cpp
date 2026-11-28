/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BehaviourCatalog
*/

#include "BehaviourCatalog.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#include "BehaviourFiles.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/world/World.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"

namespace aylo::editor::behaviour {
namespace {
/// @return The capsule nodes of @p capsules that carry the component @p id.
std::vector<BehaviourCarrier> carriersOf(const plugins::capsule::Capsules* capsules, ecs::ComponentId id) {
    std::vector<BehaviourCarrier> carriers;
    if (capsules == nullptr) {
        return carriers;
    }
    for (const plugins::capsule::Capsule* capsule : capsules->list()) {
        const auto nodes = capsule->getNodes();
        for (std::size_t index = 0; index < nodes.size(); ++index) {
            if (std::next(nodes.begin(), static_cast<std::ptrdiff_t>(index))->values.has(id)) {
                carriers.push_back(BehaviourCarrier{.capsule = std::string{capsule->getPath()},
                                                    .node = static_cast<std::uint32_t>(index),
                                                    .nodePath = capsule->getNodePath(index)});
            }
        }
    }
    return carriers;
}
}  // namespace

std::vector<BehaviourEntry> catalogBehaviours(const ecs::App& app, const std::filesystem::path& assets) {
    const ecs::World& world = app.getWorld();
    const ecs::Components& components = world.getComponents();
    const ecs::ComponentRegistry& registry = components.getRegistry();
    const auto* capsules = world.getResources().get<plugins::capsule::Capsules>();
    std::vector<BehaviourEntry> entries;
    for (std::size_t index = 0; index < registry.getCount(); ++index) {
        const auto id = static_cast<ecs::ComponentId>(index);
        const ecs::ComponentInfo* info = registry.findById(id);
        if (info == nullptr || !isBehaviour(app, info->type.name)) {
            continue;
        }
        const auto* pool = components.findPool(id);
        entries.push_back(BehaviourEntry{
            .name = info->type.name,
            .hooks = hooksOf(app, info->type.name),
            .file = assets.empty() ? std::nullopt : findBehaviourFile(behavioursFolder(assets), info->type.name),
            .carriers = carriersOf(capsules, id),
            .live = pool != nullptr ? pool->getEntities().size() : 0,
        });
    }
    std::ranges::sort(entries, {}, &BehaviourEntry::name);
    return entries;
}
}  // namespace aylo::editor::behaviour

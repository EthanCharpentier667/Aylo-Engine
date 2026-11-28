/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CapsuleSpawner
*/

#include "CapsuleSpawner.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <iterator>
#include <string>
#include <vector>

#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/entity/Name.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/type/String.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/ComponentValues.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"

namespace aylo::plugins::capsule::detail {
namespace {
/// @brief Gives `entity` the values of `values` a spawn keeps on the registry's side, in their order.
/// @param replicated Whether to give the values of replicated components: false for a root the server spawned, whose
/// replicated components come from the server only.
void addKeptValues(const Capsules& capsules, const ComponentValues& values, ecs::Entity entity, ecs::Commands& commands,
                   bool replicated = true) {
    for (std::size_t index = 0; index < values.getCount(); ++index) {
        if (capsules.keeps(values.getPresence(index)) && (replicated || !values.isReplicated(index))) {
            // The id was registered when the value was set.
            static_cast<void>(commands.add(entity, values.getId(index), values.getValue(index)));
        }
    }
}

// NOLINTNEXTLINE(misc-no-recursion): the stack stops a capsule that contains itself.
void checkNodes(const Capsules& capsules, const Capsule& capsule, std::vector<const Capsule*>& stack) {
    stack.push_back(&capsule);
    const auto nodes = capsule.getNodes();
    std::vector<bool> kept(nodes.size(), false);
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const CapsuleNode& node = *std::next(nodes.begin(), static_cast<std::ptrdiff_t>(index));
        if (index != 0 && (!kept.at(node.parent) || !capsules.loads(node.side))) {
            continue;  // skipped with its subtree, as the spawn does
        }
        kept.at(index) = true;
        if (!node.instance) {
            continue;
        }
        const Capsule* inner = capsules.find(node.instance->id);
        if (inner == nullptr) {
            throw exceptions::CapsuleException{std::format("Capsule '{}' spawns '{}' at '{}', which no capsule has",
                                                           capsule.getPath(), node.instance->path,
                                                           capsule.getNodePath(index))};
        }
        if (std::ranges::find(stack, inner) != stack.end()) {
            std::string chain;
            for (const Capsule* outer : stack) {
                chain += std::format("'{}' -> ", outer->getPath());
            }
            throw exceptions::CapsuleException{
                std::format("Capsule '{}' contains itself: {}'{}'", inner->getPath(), chain, inner->getPath())};
        }
        checkNodes(capsules, *inner, stack);
    }
    stack.pop_back();
}

/// @return The name a capsule's root gets: its file's name, without its folders and extension ("boss" for
/// "actors/boss.actor").
std::string rootName(const Capsule& capsule) { return std::filesystem::path{capsule.getPath()}.stem().string(); }
}  // namespace

void checkCapsule(const Capsules& capsules, const Capsule& capsule) {
    std::vector<const Capsule*> stack;
    checkNodes(capsules, capsule, stack);
}

// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the instances, which checkCapsule() keeps acyclic.
bool rootReplicates(const Capsules& capsules, const Capsule& capsule) {
    const CapsuleNode& root = capsule.getNodes().front();
    for (std::size_t index = 0; index < root.values.getCount(); ++index) {
        if (root.values.isReplicated(index) && capsules.keeps(root.values.getPresence(index))) {
            return true;
        }
    }
    const Capsule* inner = root.instance ? capsules.find(root.instance->id) : nullptr;
    return inner != nullptr && rootReplicates(capsules, *inner);
}

// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the instances, which checkCapsule() keeps acyclic.
ecs::Entity spawnTree(const Capsules& capsules, const Capsule& capsule, ecs::Commands& commands, ecs::Entity root,
                      bool nested, bool clientPart) {
    const auto nodes = capsule.getNodes();
    std::vector<ecs::Entity> entities(nodes.size());  // the null entity for a node skipped
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        const CapsuleNode& node = *std::next(nodes.begin(), static_cast<std::ptrdiff_t>(index));
        ecs::Entity parent;
        if (index != 0) {
            parent = entities.at(node.parent);
            if (parent == ecs::Entity{} || !capsules.loads(node.side)) {
                continue;  // its parent was skipped, or it is the other side's: it is skipped with its subtree
            }
            if (clientPart && node.parent == 0 && node.side != ecs::Side::kClient) {
                continue;  // the server spawned it: it comes through replication, if at all
            }
        }
        const ecs::Entity into = index == 0 ? root : ecs::Entity{};
        // Only the root is the server's: what the client part spawns under it is the client's own.
        const bool serverRoot = clientPart && index == 0;
        ecs::Entity entity;
        if (node.instance) {
            entity = spawnTree(capsules, *capsules.find(node.instance->id), commands, into, true, serverRoot);
        } else {
            entity = into != ecs::Entity{} ? into : commands.spawn().getEntity();
        }
        commands.add(entity, ecs::Name{.value = ecs::String{index == 0 ? rootName(capsule) : node.name}});
        addKeptValues(capsules, node.values, entity, commands, !serverRoot);
        if (index != 0) {
            commands.add(entity, ecs::Parent{.entity = parent});
        }
        entities.at(index) = entity;
    }
    const ecs::Entity spawned = entities.front();
    commands.add(spawned,
                 CapsuleInstance{.capsule = capsule.getId(), .version = capsule.getVersion(), .nested = nested});
    return spawned;
}
}  // namespace aylo::plugins::capsule::detail

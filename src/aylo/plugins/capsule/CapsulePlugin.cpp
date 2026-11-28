/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CapsulePlugin
*/

#include "CapsulePlugin.hpp"

#include <algorithm>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "aylo/core/reload/HotReloader.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/change/Mut.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/component/Components.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/hierarchy/Children.hpp"
#include "aylo/ecs/hierarchy/Parent.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryTerms.hpp"
#include "aylo/ecs/resource/Resources.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"
#include "plugins/AssetDirectory.hpp"
#include "plugins/HotReload.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleCalls.hpp"
#include "plugins/capsule/CapsuleFiles.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsuleReloadable.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/ReloadedCapsules.hpp"
#include "plugins/capsule/SpawnedCapsule.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"

namespace aylo::plugins::capsule {
namespace {
/// @brief A spawned root as the hot reload sees it.
struct Root {
    CapsuleInstance instance;           ///< Its capsule.
    std::vector<ecs::Entity> children;  ///< Its children now.
};

/// @return The nearest ancestor of `entity` that is a capsule's root, or the null entity.
ecs::Entity rootAbove(ecs::Entity entity, const std::unordered_map<ecs::Entity, Root>& roots,
                      const std::unordered_map<ecs::Entity, ecs::Entity>& parents) {
    for (auto parent = parents.find(entity); parent != parents.end(); parent = parents.find(parent->second)) {
        if (roots.contains(parent->second)) {
            return parent->second;
        }
    }
    return ecs::Entity{};
}

/// @return The outermost capsule root `entity` belongs to: up through the capsules that spawned it as a node, so their
/// nodes' values stay on it when it is reloaded.
ecs::Entity outermostRoot(ecs::Entity entity, const std::unordered_map<ecs::Entity, Root>& roots,
                          const std::unordered_map<ecs::Entity, ecs::Entity>& parents) {
    while (roots.at(entity).instance.nested) {
        const ecs::Entity above = rootAbove(entity, roots, parents);
        if (above == ecs::Entity{}) {
            break;
        }
        entity = above;
    }
    return entity;
}

/// @brief Render, with hot reload: reloads what was spawned from the capsules the CapsuleReloadable read again, nested
/// roots through the outermost capsule they belong to.
void reloadCapsules(
    ecs::Commands& commands, ecs::Res<Capsules> capsules, ecs::ResMut<ReloadedCapsules> reloaded,
    ecs::Query<const CapsuleInstance, ecs::Optional<const ecs::Children>, ecs::Optional<const SpawnedCapsule>> roots,
    ecs::Query<const ecs::Parent> parents) {
    if (reloaded->capsules.empty()) {
        return;
    }
    const std::vector<CapsuleId> reread = std::exchange(reloaded->capsules, {});
    std::unordered_map<ecs::Entity, ecs::Entity> parentOf;
    for (auto [entity, parent] : parents) {
        parentOf.emplace(entity, parent.entity);
    }
    std::unordered_map<ecs::Entity, Root> rootOf;
    // On a client, a root the server spawned is the server's to reload: its children came through replication.
    const bool client = !capsules->keeps(ecs::Presence::kServerOnly);
    for (auto [entity, instance, children, spawned] : roots) {
        if (client && spawned != nullptr) {
            continue;
        }
        rootOf.emplace(entity, Root{.instance = instance,
                                    .children = children != nullptr ? children->entities : std::vector<ecs::Entity>{}});
    }
    std::vector<ecs::Entity> outermost;
    for (const auto& [entity, root] : rootOf) {
        if (std::ranges::find(reread, root.instance.capsule) != reread.end()) {
            const ecs::Entity top = outermostRoot(entity, rootOf, parentOf);
            if (std::ranges::find(outermost, top) == outermost.end()) {
                outermost.push_back(top);
            }
        }
    }
    for (const ecs::Entity entity : outermost) {
        const Root& root = rootOf.at(entity);
        try {
            reloadCapsule(commands, *capsules, entity, root.instance, root.children);
        } catch (const exceptions::CapsuleException& error) {  // a file instancing a capsule that is gone
            std::cerr << std::format("[capsule] entity {} not reloaded: {}\n", entity.getIndex(), error.what());
        }
    }
}

/// @brief Update, on a client: spawns into each root the server spawned (a SpawnedCapsule, no CapsuleInstance yet) the
/// part of its capsule the server cannot send. A capsule this client does not know is reported, once.
void spawnReplicated(ecs::Commands& commands, ecs::Res<Capsules> capsules,
                     ecs::Query<const SpawnedCapsule, ecs::Without<CapsuleInstance>> roots) {
    for (auto [entity, spawned] : roots) {
        try {
            spawnClientPart(commands, *capsules, entity, spawned.capsule);
        } catch (const exceptions::CapsuleException& error) {  // bytes from the server: never more than a message
            std::cerr << std::format("[capsule] entity {} the server spawned has no client part: {}\n",
                                     entity.getIndex(), error.what());
            commands.add(entity, CapsuleInstance{.capsule = spawned.capsule});  // not tried again
        }
    }
}
}  // namespace

std::string_view CapsulePlugin::getName() const noexcept { return kName; }

void CapsulePlugin::build(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    resources.insertIfAbsent<Capsules>();
    resources.requireMut<Capsules>()->setRunMode(app.getSchedules().getRunMode());  // a spawn keeps this side's
    resources.insertIfAbsent<LoadedCapsule>();
    ecs::Components& components = app.getWorld().getComponents();
    if (components.getRegistry().find<CapsuleInstance>() == nullptr) {
        static_cast<void>(components.registerType<CapsuleInstance>());
    }
    if (components.getRegistry().find<SpawnedCapsule>() == nullptr) {
        static_cast<void>(components.registerType<SpawnedCapsule>());  // replicated, at spawn
    }
    addCapsuleCalls(app);
    // What to respawn in the world at the next frame: the files the hot reload read again, or what a tool (the
    // editor) changed. It runs whether or not hot reload is on.
    resources.insertIfAbsent<ReloadedCapsules>();
    app.addSystem(ecs::Phase::kRender, std::string{kHotReloadSystem}, &reloadCapsules).setSide(ecs::Side::kBoth);
    app.addSystem(ecs::Phase::kUpdate, std::string{kSpawnReplicatedSystem}, &spawnReplicated)
        .setSide(ecs::Side::kClient);
    // A game has assets (a folder or a pack); a program without an Engine has none, and reads no capsule file.
    const auto* assetDirectory = resources.get<AssetDirectory>();
    if (assetDirectory == nullptr || (assetDirectory->path.empty() && !assetDirectory->source)) {
        return;
    }
    resources.insert(CapsuleFiles{assetSourceOf(resources), components});
    // Only files on disk are followed: a pack has none to watch.
    const std::filesystem::path directory = assetDirectory->path;
    if (directory.empty()) {
        return;
    }
    core::reload::HotReloader* reloader = hotReloaderOf(resources);
    if (reloader == nullptr) {
        return;
    }
    // The reloader's paths are relative to the game; the assets' directory is a folder of it.
    const std::filesystem::path assets = directory.lexically_relative(reloader->root());
    if (assets.empty() || *assets.begin() == "..") {
        return;
    }
    reloader->add(std::make_unique<CapsuleReloadable>(&*resources.getMut<CapsuleFiles>(),
                                                      &*resources.requireMut<Capsules>(),
                                                      &*resources.requireMut<ReloadedCapsules>(), &resources, assets));
}

void CapsulePlugin::finish(ecs::App& app) {
    ecs::Resources& resources = app.getWorld().getResources();
    ecs::Mut<CapsuleFiles> files = resources.getMut<CapsuleFiles>();
    if (files) {
        static_cast<void>(files->loadAll(*resources.requireMut<Capsules>()));
        loadNamedAssets(*resources.get<Capsules>(), resources);
    }
}
}  // namespace aylo::plugins::capsule

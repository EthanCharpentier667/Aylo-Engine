/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** CapsuleCalls
*/

#include "CapsuleCalls.hpp"

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/command/Commands.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/hierarchy/Children.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/query/QueryTerms.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"

namespace aylo::plugins::capsule {
namespace {
CapsuleRoot spawn(ecs::Commands& commands, ecs::Res<Capsules> capsules, const CapsuleArguments& arguments) {
    return CapsuleRoot{.root = spawnCapsule(commands, *capsules, arguments.capsule).getEntity()};
}

CapsuleRoot load(ecs::Commands& commands, ecs::Res<Capsules> capsules, ecs::ResMut<LoadedCapsule> loaded,
                 const CapsuleArguments& arguments) {
    return CapsuleRoot{.root = loadCapsule(commands, *capsules, *loaded, arguments.capsule).getEntity()};
}

ReloadResult reload(ecs::Commands& commands, ecs::Res<Capsules> capsules,
                    ecs::Query<const CapsuleInstance, ecs::Optional<const ecs::Children>> roots,
                    const ReloadArguments& arguments) {
    for (auto [entity, instance, children] : roots) {  // no lookup by entity yet: a few roots at most
        if (entity == arguments.root) {
            const std::vector<ecs::Entity> old = children != nullptr ? children->entities : std::vector<ecs::Entity>{};
            reloadCapsule(commands, *capsules, entity, instance, old);
            return ReloadResult{.reloaded = true};
        }
    }
    return ReloadResult{.reloaded = false};
}
}  // namespace

void addCapsuleForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "CapsuleForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addCapsuleForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addCapsuleCalls(ecs::App& app) {
    addCapsuleForms(app.getPluginRegistry());
    app.addCall(std::string{kSpawnCall}, &spawn);
    app.addCall(std::string{kLoadCall}, &load);
    app.addCall(std::string{kReloadCall}, &reload);
}
}  // namespace aylo::plugins::capsule

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClientWrites
*/

#include "ClientWrites.hpp"

#include <optional>
#include <string>

#include "aylo/ecs/component/ComponentInfo.hpp"
#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"

namespace aylo::ecs {
std::optional<ClientWrite> findClientWrite(const ComponentRegistry& registry, const SystemAccess& access) {
    for (const TypeAccess& use : access.getComponents()) {
        const ComponentInfo* info = registry.findById(use.id);
        if (!use.write || info == nullptr) {
            continue;
        }
        if (info->type.replication != ReplicationMode::kNone) {
            return ClientWrite{.component = std::string{info->type.name},
                               .reason = "replicated: the server would overwrite it"};
        }
        if (info->type.presence == Presence::kServerOnly) {
            return ClientWrite{.component = std::string{info->type.name},
                               .reason = "server-only: clients do not have it"};
        }
    }
    return std::nullopt;
}
}  // namespace aylo::ecs

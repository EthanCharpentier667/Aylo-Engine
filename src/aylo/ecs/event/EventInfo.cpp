/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EventInfo
*/

#include "EventInfo.hpp"

#include <format>

#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"

namespace aylo::ecs {
Result<void> validate(const EventInfo& info) {
    if (Result<void> valid = validateType(info.type, "event"); !valid) {
        return valid;
    }
    if (info.type.replication != ReplicationMode::kNone) {
        return fail(ErrorKind::kInvalidType,
                    std::format("Event '{}': an event is local to a world and cannot be replicated", info.type.name));
    }
    return {};
}
}  // namespace aylo::ecs

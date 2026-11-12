/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ComponentInfo
*/

#include "ComponentInfo.hpp"

#include <format>

#include "aylo/ecs/error/Error.hpp"
#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"

namespace aylo::ecs {
Result<void> validate(const ComponentInfo& info) {
    if (Result<void> valid = validateType(info.type, "component"); !valid) {
        return valid;
    }
    if (!info.type.copyable) {
        return fail(ErrorKind::kInvalidType,
                    std::format("Component '{}': a component must be copyable", info.type.name));
    }
    return {};
}
}  // namespace aylo::ecs

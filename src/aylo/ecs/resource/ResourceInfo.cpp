/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ResourceInfo
*/

#include "ResourceInfo.hpp"

#include "aylo/ecs/error/Result.hpp"
#include "aylo/ecs/type/TypeInfo.hpp"

namespace aylo::ecs {
Result<void> validate(const ResourceInfo& info) { return validateType(info.type, "resource"); }
}  // namespace aylo::ecs

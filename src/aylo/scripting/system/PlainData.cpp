/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PlainData
*/

#include "PlainData.hpp"

#include <string>

#include "aylo/ecs/type/TypeInfo.hpp"

namespace aylo::scripting::system {
std::string whyNotPlainData(const ecs::TypeInfo& type, const bool mainThread) {
    if (mainThread) {
        return "it is main-thread only";
    }
    if (type.hooks.destruct != nullptr || type.hooks.move != nullptr || type.hooks.copy != nullptr) {
        return "it is not plain data (it owns memory of its own)";
    }
    return {};
}
}  // namespace aylo::scripting::system

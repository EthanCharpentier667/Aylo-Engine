/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClientWrites
*/

#pragma once

#include <optional>
#include <string>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/component/ComponentRegistry.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"

namespace aylo::ecs {
/// @brief What a system running on clients writes that it may not: a replicated component (the server would overwrite
/// it) or a server-only one (clients do not have it).
struct ClientWrite {
    std::string component;  ///< The component's name.
    std::string reason;     ///< "replicated: the server would overwrite it" or "server-only: clients do not have it".
};

/// @return The first component `access` writes that a system running on clients may not write, or nothing.
[[nodiscard]] AYLO_ECS_API std::optional<ClientWrite> findClientWrite(const ComponentRegistry& registry,
                                                                      const SystemAccess& access);
}  // namespace aylo::ecs

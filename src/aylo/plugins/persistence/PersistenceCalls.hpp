/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PersistenceCalls
*/

#pragma once

#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"

namespace aylo::plugins::persistence {

inline constexpr std::string_view kReadCall = "aylo.persistence.read";      ///< SaveStore::read().
inline constexpr std::string_view kWriteCall = "aylo.persistence.write";    ///< SaveStore::write().
inline constexpr std::string_view kDeleteCall = "aylo.persistence.delete";  ///< SaveStore::remove().
inline constexpr std::string_view kExistsCall = "aylo.persistence.exists";  ///< SaveStore::exists().

/// @brief The arguments of `read`, `delete` and `exists`: the save's name.
struct NameArguments {
    std::string name;  ///< 1 to 64 letters, digits, `_`, `-` or `.`, not starting with a dot.
};
AYLO_ECS_CALL_DATA(NameArguments, "aylo.persistence.NameArguments", name);

/// @brief The arguments of `write`.
struct WriteArguments {
    std::string name;  ///< As NameArguments.
    std::string data;  ///< The opaque text to save (the game serialises its table itself).
};
AYLO_ECS_CALL_DATA(WriteArguments, "aylo.persistence.WriteArguments", name, data);

/// @brief The result of `read`, `write` and `delete`: a failure is a value, never a script error.
struct SaveOutcome {
    bool ok{false};     ///< The operation succeeded.
    std::string data;   ///< `read`: the saved text.
    std::string error;  ///< `invalid_name`, `not_found`, `corrupt` or `io: ...`; empty on success.
};
AYLO_ECS_CALL_DATA(SaveOutcome, "aylo.persistence.SaveOutcome", ok, data, error);

/// @brief The result of `exists`.
struct ExistsResult {
    bool exists{false};  ///< A file exists for the name (it may be corrupt).
};
AYLO_ECS_CALL_DATA(ExistsResult, "aylo.persistence.ExistsResult", exists);

/// @brief Registers the four calls on @p app, and the forms. Called by the PersistencePlugin.
void addPersistenceCalls(ecs::App& app);

/// @brief Registers `engine.persistence.read/write/delete/exists(ctx, ...)` on @p registry.
void addPersistenceForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::persistence

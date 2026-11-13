/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StateCalls
*/

#pragma once

#include <string_view>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/type/String.hpp"

namespace aylo::ecs {
class App;

inline constexpr std::string_view kGetStateCall = "aylo.state.get";  ///< The value a state machine is in.
inline constexpr std::string_view kSetStateCall = "aylo.state.set";  ///< Asks a state machine for another value.

/// @brief The arguments of `aylo.state.get`: the machine to read.
struct GetStateArguments {
    String state;  ///< The machine's name, as declared ("game.Phase").
};
AYLO_ECS_CALL_DATA(GetStateArguments, "aylo.state.GetStateArguments", state);

/// @brief The result of `aylo.state.get`.
struct StateValue {
    String value;  ///< The value the machine is in; empty before it entered its first one.
};
AYLO_ECS_CALL_DATA(StateValue, "aylo.state.StateValue", value);

/// @brief The arguments of `aylo.state.set`: the machine and the value to go to, at the next frame.
struct SetStateArguments {
    String state;  ///< The machine's name.
    String value;  ///< One of its values.
};
AYLO_ECS_CALL_DATA(SetStateArguments, "aylo.state.SetStateArguments", state, value);

/// @brief Registers `aylo.state.get` and `aylo.state.set` on the States resource of @p app. Both sides have them: a
/// machine lives on each side, apart (States). The App registers them when it is made.
AYLO_ECS_API void addStateCalls(App& app);
}  // namespace aylo::ecs

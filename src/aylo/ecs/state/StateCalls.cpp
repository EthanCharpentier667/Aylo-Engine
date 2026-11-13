/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** StateCalls
*/

#include "aylo/ecs/state/StateCalls.hpp"

#include <optional>
#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/state/States.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/type/String.hpp"

namespace aylo::ecs {
namespace {
/// @brief `aylo.state.get`: the value the machine is in, empty before its first.
/// @throws exceptions::StateException When no machine has the name.
StateValue getState(Res<States> states, const GetStateArguments& arguments) {
    const std::optional<std::string_view> value = states->get(arguments.state.view());
    return StateValue{.value = String{value.value_or(std::string_view{})}};
}

/// @brief `aylo.state.set`: the machine goes to the value at the next frame.
/// @throws exceptions::StateException When no machine has the name, or it has no such value.
void setState(ResMut<States> states, const SetStateArguments& arguments) {
    states->request(arguments.state.view(), arguments.value.view());
}
}  // namespace

void addStateCalls(App& app) {
    app.addCall(std::string{kGetStateCall}, &getState);
    app.addCall(std::string{kSetStateCall}, &setState);
}
}  // namespace aylo::ecs

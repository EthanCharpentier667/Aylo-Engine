/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** InputCalls
*/

#include "aylo/replication/input/InputCalls.hpp"

#include <array>
#include <format>
#include <string>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/query/Query.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/replication/component/PlayerInput.hpp"
#include "aylo/replication/exceptions/ReplicationExceptions.hpp"
#include "aylo/replication/input/InputKind.hpp"
#include "aylo/replication/input/InputLayout.hpp"

namespace aylo::replication::input {
namespace {
/// @return The kind named @p name, as the input plugin names its action types.
/// @throws exceptions::InputLayoutException If no kind has this name.
[[nodiscard]] InputKind kindNamed(const std::string_view name) {
    if (name == "button") {
        return InputKind::kButton;
    }
    if (name == "axis") {
        return InputKind::kAxis;
    }
    if (name == "vector2") {
        return InputKind::kVector2;
    }
    throw exceptions::InputLayoutException{
        std::format(R"(Unknown input kind '{}' (expected "button", "axis" or "vector2"))", name)};
}

void addInput(ecs::ResMut<InputLayout> layout, const AddInputArguments& arguments) {
    layout->add(std::string{arguments.name.view()}, kindNamed(arguments.kind));
}

InputState readInput(ecs::Res<InputLayout> layout, ecs::Query<const component::PlayerInput> inputs,
                     const ReadInputArguments& arguments) {
    const InputHandle action = layout->find(arguments.action);
    // A Query has no lookup by entity: the inputs are walked until the entity's.
    for (auto [entity, input] : inputs) {
        if (entity != arguments.entity) {
            continue;
        }
        InputState state;
        switch (action.kind) {
            case InputKind::kButton:
                state.held = layout->isHeld(input, action);
                state.pressed = layout->isPressed(input, action);
                state.released = layout->isReleased(input, action);
                break;
            case InputKind::kAxis:
                state.axis = layout->readAxis(input, action);
                break;
            case InputKind::kVector2: {
                const std::array<float, 2> vector = layout->readVector(input, action);
                state.vector = InputVector{.x = vector.at(0), .y = vector.at(1)};
                break;
            }
        }
        return state;
    }
    throw exceptions::InputLayoutException{std::format("Cannot read input '{}': entity {} has no PlayerInput",
                                                       arguments.action.view(), arguments.entity.getIndex())};
}
}  // namespace

void addInputCalls(ecs::App& app) {
    app.addCall(std::string{kAddInputCall}, &addInput);
    app.addCall(std::string{kReadInputCall}, &readInput);
}
}  // namespace aylo::replication::input

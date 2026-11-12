/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** InputCalls
*/

#pragma once

#include <string_view>
#include <type_traits>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/type/Field.hpp"
#include "aylo/ecs/type/FieldTypeOf.hpp"
#include "aylo/ecs/type/String.hpp"
#include "aylo/replication/Export.hpp"

namespace aylo::replication::input {
/// @brief Two floats, the value of a vector action: replication does not depend on glm, so the call results use this
/// instead of glm::vec2. Reflected as ecs::FieldType::kVec2, a script reads it as `{x, y}`.
struct InputVector {
    float x{0.0F};  ///< The horizontal value, in [-1, 1].
    float y{0.0F};  ///< The vertical value, in [-1, 1], up positive.
};
}  // namespace aylo::replication::input

/// @brief An InputVector field is reflected as kVec2.
template <>
struct aylo::ecs::FieldTypeOf<aylo::replication::input::InputVector>
    : std::integral_constant<aylo::ecs::FieldType, aylo::ecs::FieldType::kVec2> {};
static_assert(sizeof(aylo::replication::input::InputVector) == 8 && alignof(aylo::replication::input::InputVector) == 4,
              "InputVector must match FieldType::kVec2");

namespace aylo::replication::input {
inline constexpr std::string_view kAddInputCall = "aylo.replication.addInput";    ///< InputLayout::add().
inline constexpr std::string_view kReadInputCall = "aylo.replication.readInput";  ///< Reads a PlayerInput by name.

/// @brief The arguments of `aylo.replication.addInput`: declares an action of the network InputLayout.
struct AddInputArguments {
    ecs::String name;            ///< The action's name, 1 to 255 bytes, not declared yet.
    ecs::String kind{"button"};  ///< What it carries: "button", "axis" or "vector2".
};
AYLO_ECS_CALL_DATA(AddInputArguments, "aylo.replication.AddInputArguments", name, kind);

/// @brief The arguments of `aylo.replication.readInput`: an action of an entity's PlayerInput.
struct ReadInputArguments {
    ecs::Entity entity;  ///< The entity, with a PlayerInput (the server's ships, filled from their client's inputs).
    ecs::String action;  ///< The action's name, as declared in the InputLayout.
};
AYLO_ECS_CALL_DATA(ReadInputArguments, "aylo.replication.ReadInputArguments", entity, action);

/// @brief The result of `aylo.replication.readInput`: the fields of the action's kind are set, the others are zero.
struct InputState {
    bool held{false};      ///< A button: held now (InputLayout::isHeld()).
    bool pressed{false};   ///< A button: held now and not at the input before (InputLayout::isPressed()).
    bool released{false};  ///< A button: held at the input before and not now (InputLayout::isReleased()).
    float axis{0.0F};      ///< An axis: its value, in [-1, 1] (InputLayout::readAxis()).
    InputVector vector;    ///< A vector: its value (InputLayout::readVector()).
};
AYLO_ECS_CALL_DATA(InputState, "aylo.replication.InputState", held, pressed, released, axis, vector);

/// @brief Registers the input engine calls on @p app, for scripts, on the `net.InputLayout` resource:
/// `aylo.replication.addInput`, which declares an action while the layout is not frozen (scripts declare them while
/// they load, through `engine.input.action`), and `aylo.replication.readInput`, which reads an action of an entity's
/// PlayerInput. Both throw an exceptions::InputLayoutException on an unknown action or kind, a frozen layout, or an
/// entity without a PlayerInput. Called by the ReplicationPlugin.
AYLO_REPLICATION_API void addInputCalls(ecs::App& app);
}  // namespace aylo::replication::input

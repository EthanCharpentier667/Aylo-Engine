/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** InputCalls
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <string_view>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/app/PluginRegistry.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/type/String.hpp"
#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep

namespace aylo::plugins::input {

inline constexpr std::string_view kAddActionCall = "aylo.input.addAction";    ///< Input::addAction().
inline constexpr std::string_view kBindCall = "aylo.input.bind";              ///< InputAction::bind().
inline constexpr std::string_view kBindAxisCall = "aylo.input.bindAxis";      ///< InputAction::bindAxis().
inline constexpr std::string_view kBindVectorCall = "aylo.input.bindVector";  ///< InputAction::bindVector().
inline constexpr std::string_view kReadActionCall = "aylo.input.read";        ///< The state of an action.

/// @brief The arguments of `aylo.input.addAction`: creates (or replaces) an action.
struct AddActionArguments {
    ecs::String name;            ///< The action's name, as gameplay reads it ("move", "shoot").
    ecs::String type{"button"};  ///< "button", "axis" or "vector2".
};
AYLO_ECS_CALL_DATA(AddActionArguments, "aylo.input.AddActionArguments", name, type);

/// @brief The arguments of `aylo.input.bind`: binds one control to an action.
struct BindArguments {
    ecs::String action;   ///< The action.
    ecs::String control;  ///< The control, named as controlFromName() reads it ("Space", "MouseLeft", "GamepadLeftX").
    float scale{1.0F};    ///< Multiplies the control's raw value (sensitivity, inversion).
};
AYLO_ECS_CALL_DATA(BindArguments, "aylo.input.BindArguments", action, control, scale);

/// @brief The arguments of `aylo.input.bindAxis`: binds a 1D composite (-1 for `negative`, +1 for `positive`).
struct BindAxisArguments {
    ecs::String action;    ///< The action.
    ecs::String negative;  ///< The control that gives -1.
    ecs::String positive;  ///< The control that gives +1.
};
AYLO_ECS_CALL_DATA(BindAxisArguments, "aylo.input.BindAxisArguments", action, negative, positive);

/// @brief The arguments of `aylo.input.bindVector`: binds a 2D vector, either from four controls (`up`, `down`,
/// `left`, `right`: WASD, the arrows, the d-pad) or from two analog ones (`x`, `y`: a stick).
struct BindVectorArguments {
    ecs::String action;  ///< The action.
    ecs::String up;      ///< Four controls: up.
    ecs::String down;    ///< Four controls: down.
    ecs::String left;    ///< Four controls: left.
    ecs::String right;   ///< Four controls: right.
    ecs::String x;       ///< Two analog controls: the horizontal one.
    ecs::String y;       ///< Two analog controls: the vertical one.
};
AYLO_ECS_CALL_DATA(BindVectorArguments, "aylo.input.BindVectorArguments", action, up, down, left, right, x, y);

/// @brief The arguments of `aylo.input.read`.
struct ReadActionArguments {
    ecs::String action;  ///< The action.
};
AYLO_ECS_CALL_DATA(ReadActionArguments, "aylo.input.ReadActionArguments", action);

/// @brief The result of `aylo.input.read`: an action's state this frame.
struct ActionState {
    glm::vec2 value{0.0F};  ///< Its value: the X alone for a button or an axis.
    bool held{false};       ///< Down this frame.
    bool pressed{false};    ///< Went down this frame.
    bool released{false};   ///< Went up this frame.
};
AYLO_ECS_CALL_DATA(ActionState, "aylo.input.ActionState", value, held, pressed, released);

/// @brief Registers the input engine calls on @p app, for scripts: `aylo.input.addAction`, `aylo.input.bind`,
/// `aylo.input.bindAxis`, `aylo.input.bindVector` and `aylo.input.read`, each on the `aylo.Input` resource. They
/// throw an InputException naming an unknown action, control or action type. Called by the InputPlugin.
void addInputCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin on @p registry: its library does it on every side, so a server gets
/// the ids of what it does not load, and so does building the plugin, for a program without libraries.
void addInputForms(ecs::PluginRegistry& registry);

}  // namespace aylo::plugins::input

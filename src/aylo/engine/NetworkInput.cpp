/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** NetworkInput
*/

#include "NetworkInput.hpp"

#include <exception>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "Exceptions.hpp"
#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/call/CallInfo.hpp"
#include "aylo/ecs/call/CallTraits.hpp"
#include "aylo/ecs/call/DynamicCalls.hpp"
#include "aylo/ecs/exceptions/MissingResourceException.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/Side.hpp"
#include "aylo/ecs/type/String.hpp"
#include "aylo/ecs/type/TypeTraits.hpp"
#include "aylo/plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "aylo/replication/client/ClientPlugin.hpp"
#include "aylo/replication/client/ClientState.hpp"
#include "aylo/replication/component/PlayerInput.hpp"
#include "aylo/replication/input/InputKind.hpp"
#include "aylo/replication/input/InputLayout.hpp"
#include "aylo/replication/standalone/LocalSession.hpp"

namespace aylo::engine {
namespace {
namespace input_call {
/// @brief The arguments of `rtype.input.read`, as the Engine builds them. It names the same data as the input
/// plugin's `ReadActionArguments`: addNetworkInput() checks that the plugin's layout is this one.
struct ReadActionArguments {
    ecs::String action;  ///< The action.
};
AYLO_ECS_CALL_DATA(ReadActionArguments, "rtype.input.ReadActionArguments", action);

/// @brief The result of `rtype.input.read`, as the Engine reads it (the input plugin's `ActionState`).
struct ActionState {
    glm::vec2 value{0.0F};  ///< Its value: the X alone for a button or an axis.
    bool held{false};       ///< Down this frame.
    bool pressed{false};    ///< Went down this frame.
    bool released{false};   ///< Went up this frame.
};
AYLO_ECS_CALL_DATA(ActionState, "rtype.input.ActionState", value, held, pressed, released);
}  // namespace input_call

/// @brief Reads one input action through the call at index 0 of @p calls.
[[nodiscard]] std::optional<ActionReading> readThroughCall(const ecs::DynamicCalls& calls,
                                                           const std::string_view name) {
    const input_call::ReadActionArguments arguments{.action = ecs::String{name}};
    input_call::ActionState state;
    try {
        calls.invoke(0, std::as_bytes(std::span{&arguments, 1}), std::as_writable_bytes(std::span{&state, 1}));
    } catch (const ecs::exceptions::MissingResourceException&) {
        throw;  // the input plugin's resource is gone: that is not "no such action"
    } catch (const std::exception&) {
        return std::nullopt;  // the input plugin throws for an action it does not have
    }
    return ActionReading{.held = state.held, .value = state.value};
}

void fillNetworkInput(ecs::DynamicCalls calls, ecs::Res<replication::input::InputLayout> layout,
                      ecs::ResMut<replication::client::ClientState> client) {
    if (!layout->actions().empty()) {
        client->setInput(
            readNetworkInput(*layout, [&calls](const std::string_view name) { return readThroughCall(calls, name); }));
    }
}

/// @brief Standalone: the same input, kept for the FixedUpdate to hand to the entity of the local client.
void fillLocalInput(ecs::DynamicCalls calls, ecs::Res<replication::input::InputLayout> layout,
                    ecs::ResMut<replication::standalone::LocalSession> session) {
    if (!layout->actions().empty()) {
        session->setInput(
            readNetworkInput(*layout, [&calls](const std::string_view name) { return readThroughCall(calls, name); }));
    }
}

/// @throws exceptions::RuntimeError When @p call does not take and return what the Engine builds and reads.
void checkReadCall(const ecs::CallInfo& call) {
    const bool argumentsMatch =
        call.arguments.has_value() &&
        call.arguments->layoutHash == ecs::TypeTraits<input_call::ReadActionArguments>::kLayoutHash;
    const bool resultMatches =
        call.result.has_value() && call.result->layoutHash == ecs::TypeTraits<input_call::ActionState>::kLayoutHash;
    if (!argumentsMatch || !resultMatches) {
        throw exceptions::RuntimeError{std::format(
            "The engine call '{}' does not take and return what this Engine expects: the input plugin library and the "
            "Engine come from different versions",
            call.name)};
    }
}
}  // namespace

replication::component::PlayerInput readNetworkInput(const replication::input::InputLayout& layout,
                                                     const ActionReader& read) {
    replication::component::PlayerInput result;
    for (const replication::input::InputAction& declared : layout.actions()) {
        const std::optional<ActionReading> action = read(declared.name);
        if (!action) {
            continue;
        }
        switch (declared.kind) {
            case replication::input::InputKind::kButton:
                layout.setButton(result, declared.name, action->held);
                break;
            case replication::input::InputKind::kAxis:
                layout.setAxis(result, declared.name, action->value.x);
                break;
            case replication::input::InputKind::kVector2:
                layout.setVector(result, declared.name, {action->value.x, action->value.y});
                break;
        }
    }
    return result;
}

void addNetworkInput(ecs::App& app) {
    const ecs::CallInfo* call = app.getWorld().getCalls().findByName(kReadActionCall);
    if (call == nullptr) {
        return;
    }
    checkReadCall(*call);
    if (app.getSchedules().getRunMode() == ecs::RunMode::kStandalone) {
        app.addSystem(ecs::Phase::kPostUpdate, std::string{kNetworkInputSystem}, &fillLocalInput,
                      ecs::CallList{{call->id}})
            .setSide(ecs::Side::kClient);
        return;
    }
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kNetworkInputSystem}, &fillNetworkInput,
                  ecs::CallList{{call->id}})
        .before(std::string{replication::client::ClientPlugin::kSendInputSystem})
        .setSide(ecs::Side::kClient);
}
}  // namespace aylo::engine

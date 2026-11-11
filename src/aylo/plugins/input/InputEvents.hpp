/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** InputEvents
*/

#pragma once

#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/ecs/event/EventTraits.hpp"

// The platform events Input reads, declared as ECS events: one queue per alternative of contract::platform::Event. The
// window plugin sends them after polling the platform; the InputPlugin reads them. The declaring macro must sit in the
// namespace of the type, so this reopens the engine's. Their fields are not reflected: they never reach Luau or the
// network, which see actions instead.
namespace aylo::contract::platform::event {
AYLO_ECS_EVENT(KeyPressed, "aylo.KeyPressed");
AYLO_ECS_EVENT(KeyReleased, "aylo.KeyReleased");
AYLO_ECS_EVENT(MouseButtonPressed, "aylo.MouseButtonPressed");
AYLO_ECS_EVENT(MouseButtonReleased, "aylo.MouseButtonReleased");
AYLO_ECS_EVENT(MouseMoved, "aylo.MouseMoved");
AYLO_ECS_EVENT(MouseScrolled, "aylo.MouseScrolled");
AYLO_ECS_EVENT(FocusChanged, "aylo.FocusChanged");
AYLO_ECS_EVENT(GamepadConnected, "aylo.GamepadConnected");
AYLO_ECS_EVENT(GamepadDisconnected, "aylo.GamepadDisconnected");
AYLO_ECS_EVENT(GamepadButtonPressed, "aylo.GamepadButtonPressed");
AYLO_ECS_EVENT(GamepadButtonReleased, "aylo.GamepadButtonReleased");
AYLO_ECS_EVENT(GamepadAxisMoved, "aylo.GamepadAxisMoved");
}  // namespace aylo::contract::platform::event

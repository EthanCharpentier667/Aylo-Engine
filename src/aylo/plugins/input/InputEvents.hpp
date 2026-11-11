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
AYLO_ECS_EVENT(KeyPressed, "rtype.KeyPressed");
AYLO_ECS_EVENT(KeyReleased, "rtype.KeyReleased");
AYLO_ECS_EVENT(MouseButtonPressed, "rtype.MouseButtonPressed");
AYLO_ECS_EVENT(MouseButtonReleased, "rtype.MouseButtonReleased");
AYLO_ECS_EVENT(MouseMoved, "rtype.MouseMoved");
AYLO_ECS_EVENT(MouseScrolled, "rtype.MouseScrolled");
AYLO_ECS_EVENT(FocusChanged, "rtype.FocusChanged");
AYLO_ECS_EVENT(GamepadConnected, "rtype.GamepadConnected");
AYLO_ECS_EVENT(GamepadDisconnected, "rtype.GamepadDisconnected");
AYLO_ECS_EVENT(GamepadButtonPressed, "rtype.GamepadButtonPressed");
AYLO_ECS_EVENT(GamepadButtonReleased, "rtype.GamepadButtonReleased");
AYLO_ECS_EVENT(GamepadAxisMoved, "rtype.GamepadAxisMoved");
}  // namespace aylo::contract::platform::event

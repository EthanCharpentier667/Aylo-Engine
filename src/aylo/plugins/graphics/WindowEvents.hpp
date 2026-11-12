/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** WindowEvents
*/

#pragma once

#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/ecs/event/EventTraits.hpp"

// The window events of contract::platform::Event that are not input, declared as ECS events: the GraphicsPlugin sends
// them after polling the platform (the input ones are in plugins/input/InputEvents.hpp). The declaring macro must sit
// in the namespace of the type, so this reopens the engine's.
namespace aylo::contract::platform::event {
AYLO_ECS_EVENT(Closed, "aylo.Closed");
AYLO_ECS_EVENT(Resized, "aylo.Resized");
AYLO_ECS_EVENT(TextEntered, "aylo.TextEntered");
}  // namespace aylo::contract::platform::event

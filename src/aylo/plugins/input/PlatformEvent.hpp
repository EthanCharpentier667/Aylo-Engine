/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PlatformEvent
*/

#pragma once

#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/ecs/event/EventTraits.hpp"

namespace aylo::plugins::input {
/// @brief Any event of the platform, as one ECS event: the GraphicsPlugin sends one per event polled, in the order the
/// platform produced them.
/// @details Readers that need the exact order of a frame's events (Input: a key released then pressed again within
/// one frame) read this queue. Gameplay code that wants one kind only reads its typed queue instead
/// (`EventReader<KeyPressed>`...), filled from the same events. Its field is not reflected: a variant is not.
struct PlatformEvent {
    contract::platform::Event event;  ///< The platform's event.
};
AYLO_ECS_EVENT(PlatformEvent, "aylo.PlatformEvent");
}  // namespace aylo::plugins::input

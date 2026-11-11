/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PlatformEventFilter
*/

#pragma once

#include <functional>

#include "aylo/contract/platform/event/Event.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"

namespace aylo::plugins::graphics {

/// @brief What stands between the window and the game, as the main-thread ECS resource "aylo.PlatformEventFilter": a
/// tool (an editor) that shows the game inside its own window reads the events first, and decides what the game sees.
///
/// @details The GraphicsPlugin's PreUpdate system "aylo.graphics.events" calls the filter once per event, in order,
/// before the event becomes a PlatformEvent and its typed ECS event. The filter may change the event (to place the
/// mouse in the game's own coordinates) and returns false to hide it from the game. The renderer is resized by the
/// real Resized events whatever the filter does. No filter, the default, lets every event through.
struct PlatformEventFilter {
    std::function<bool(contract::platform::Event&)> filter;  ///< Keeps the (possibly changed) event if it returns true.
};
AYLO_ECS_RESOURCE(PlatformEventFilter, "aylo.PlatformEventFilter");

}  // namespace aylo::plugins::graphics

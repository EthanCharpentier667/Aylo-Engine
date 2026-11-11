/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EventWriter
*/

#pragma once

#include "aylo/ecs/event/EventTraits.hpp"
#include "aylo/ecs/event/Events.hpp"

namespace aylo::ecs {
/// @brief An event type as a system parameter, written: sends events of type `T`.
/// @details Sending appends to the queue at once (events are not structural: no flush needed); readers see the event
/// in their next run, or later in this update if they run after this system.
/// @code
/// void detect(aylo::ecs::Query<const Position, aylo::ecs::With<Bullet>> bullets,
///             aylo::ecs::EventWriter<Collision> collisions) {
///     collisions.send(Collision{.a = bullet, .b = enemy});
/// }
/// @endcode
template <Event T>
class EventWriter {
  public:
    using Type = T;  ///< The event type.

    /// @param events The world's events.
    explicit EventWriter(Events& events) noexcept;
    ~EventWriter() = default;
    EventWriter(const EventWriter& other) noexcept = default;
    EventWriter& operator=(const EventWriter& other) noexcept = default;
    EventWriter(EventWriter&& other) noexcept = default;
    EventWriter& operator=(EventWriter&& other) noexcept = default;

    /// @brief Sends an event, moved in (Events::send()).
    void send(T event);

  private:
    Events* _events;  ///< The world's events.
};
}  // namespace aylo::ecs

#include "EventWriter.tpp"

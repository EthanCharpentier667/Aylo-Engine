/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** DynamicEvents
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/event/EventCursor.hpp"
#include "aylo/ecs/event/EventInfo.hpp"
#include "aylo/ecs/event/EventRange.hpp"
#include "aylo/ecs/event/Events.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/system/SystemParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::ecs {
/// @brief One event type a system uses, by id, and what it does with it.
struct EventUse {
    EventId id{kInvalidEventId};  ///< The event type, registered before the system is added.
    bool read{false};             ///< Whether the system reads the events (it gets a cursor in the queue).
    bool send{false};             ///< Whether the system sends events of this type.
};

/// @brief The configuration of a DynamicEvents parameter: the event types the system uses, given when it is added.
/// @details They are numbered in this order (DynamicEvents::read(0) reads the first).
struct EventList {
    std::vector<EventUse> events;  ///< The event types, in order.
};

/// @brief What a DynamicEvents parameter keeps between runs: the event types and where the system is in each queue.
struct DynamicEventsState {
    std::vector<EventUse> uses;        ///< The event types, in order.
    std::vector<EventCursor> cursors;  ///< One cursor per use.
};

/// @brief A system parameter reading and sending events chosen when the system is added, by id, or found while it
/// runs: for events the system does not know at compile time (those a script names).
///
/// @details Configured with an EventList (see SystemParam): the system keeps one EventCursor per event type, as an
/// EventReader does, so it reads each event once, and its access set gets every type as read, or as written when it
/// sends (it then orders against the systems reading or sending the same type). Reading a type declared only to be
/// sent, or sending one declared only to be read, does nothing. add() gives the system an event type while it runs,
/// or lets it read or send one it already has: its access grows, and the schedule checks the system again after the
/// run. A type added to be read starts at the oldest event its queue still holds, as a declared one does.
/// @code
/// void run(aylo::ecs::DynamicEvents events) {
///     for (const std::span<const std::byte> bytes : events.read(0)) {
///         // ...
///     }
/// }
/// app.addSystem(aylo::ecs::Phase::kUpdate, "game.read", &run, aylo::ecs::EventList{{{collisionId, true, false}}});
/// @endcode
class AYLO_ECS_API DynamicEvents {
  public:
    /// @param events The world's events; they must outlive the view.
    /// @param state The event types of the system and its cursors; it must outlive the view.
    /// @param access The system's access, grown by add(); null when the view is not built by a system's run.
    DynamicEvents(Events& events, DynamicEventsState& state, SystemAccess* access) noexcept;

    ~DynamicEvents() = default;
    DynamicEvents(const DynamicEvents& other) noexcept = default;
    DynamicEvents& operator=(const DynamicEvents& other) noexcept = default;
    DynamicEvents(DynamicEvents&& other) noexcept = default;
    DynamicEvents& operator=(DynamicEvents&& other) noexcept = default;

    /// @return The number of event types: those of the EventList, then those add() found.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The use at `index`; `index` must be below getCount().
    [[nodiscard]] const EventUse& getUse(std::size_t index) const noexcept;

    /// @brief Gives the system the event type `id` while it runs, or lets it also read or send one it has: adds it to
    /// the system's access (SystemContext::access).
    /// @return The index of the type.
    /// @throws exceptions::TypeRegistrationException When `id` is not a registered event, or the view is not built by
    /// a system's run (no access to grow).
    [[nodiscard]] std::size_t add(EventId id, bool read, bool send) const;

    /// @brief Takes the events of the type at `index` that the system has not read yet.
    /// @return The unread events as raw bytes; empty if the type was not declared read. Invalidated by the next update
    /// of the queue.
    [[nodiscard]] RawEventRange read(std::size_t index) const noexcept;

    /// @brief Sends a copy of an event of the type at `index`.
    /// @param value The bytes of a live value of the event, exactly its size (empty for an event without data).
    /// @return False, sending nothing, if the type was not declared sent, `value` has the wrong size, or the event is
    /// not copyable.
    [[nodiscard]] bool send(std::size_t index, std::span<const std::byte> value) const;

  private:
    Events* _events;             ///< The world's events.
    DynamicEventsState* _state;  ///< The system's event types and cursors.
    SystemAccess* _access;       ///< The system's access; null outside a system's run.
};

/// @brief `DynamicEvents`: configured with an EventList; the system keeps its cursors.
template <>
struct AYLO_ECS_API SystemParam<DynamicEvents> {
    using Config = EventList;          ///< The event types to use, given when the system is added.
    using State = DynamicEventsState;  ///< The event types and where the system is in each queue.

    /// @return The list checked against the world, with a cursor at the start of each queue.
    /// @throws exceptions::TypeRegistrationException When an id is not a registered event.
    [[nodiscard]] static State init(World& world, Config config);

    /// @return A view of the events for this run.
    [[nodiscard]] static DynamicEvents get(State& state, const SystemContext& context) noexcept;

    /// @brief Adds reading, or sending, every event type of the list.
    static void addAccess(const State& state, SystemAccess& access);
};
}  // namespace aylo::ecs

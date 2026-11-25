/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RemovalLog
*/

#pragma once

#include <memory>
#include <span>
#include <vector>

#include "aylo/ecs/Export.hpp"
#include "aylo/ecs/change/Tick.hpp"
#include "aylo/ecs/entity/Entity.hpp"

namespace aylo::ecs::storage {
/// @brief One entity that lost a component, and when.
struct Removal {
    Entity entity;  ///< The entity, which may be dead now.
    Tick tick{0};   ///< The world's change tick when it lost the component.
};

/// @brief The entities that lost one component (removed, or despawned), kept for the queries with a `removed` term.
///
/// @details A pool keeps one. It records only while a query watches it (watch()), and drops a removal once every
/// watching query has run past it: each reader is a tick, the `lastRun` of its last pass, which the query sets. A
/// reader is dropped with the query that holds it. Removals are kept in the order they happened, so their ticks only
/// grow.
/// @code
/// std::shared_ptr<aylo::ecs::Tick> reader = log.watch(world.getChangeTick());
/// log.record(entity, world.getChangeTick());
/// *reader = lastRun;  // at each pass: the removals not newer than every reader are dropped at the next record
/// @endcode
class AYLO_ECS_API RemovalLog {
  public:
    /// @return A new reader that has seen everything up to `seen`; recording starts if it is the first.
    [[nodiscard]] std::shared_ptr<Tick> watch(Tick seen);

    /// @brief Records that `entity` lost the component at `tick`, if a query watches the log, after dropping the
    /// removals every reader has seen.
    void record(Entity entity, Tick tick);

    /// @return The removals kept, oldest first.
    [[nodiscard]] std::span<const Removal> getRemovals() const noexcept;

    /// @return Whether `entity` lost the component after `lastRun` (in (lastRun, thisRun]).
    [[nodiscard]] bool contains(Entity entity, Tick lastRun, Tick thisRun) const noexcept;

    /// @brief Clamps the tick of every removal kept, as the world does for the pools (see clampTick()).
    void clampTicks(Tick now) noexcept;

  private:
    /// @brief Forgets the readers whose query is gone, and the removals that no reader can still see.
    void dropSeen(Tick now) noexcept;

    std::vector<Removal> _removals;             ///< The removals, oldest first.
    std::vector<std::weak_ptr<Tick>> _readers;  ///< What each watching query has seen.
};
}  // namespace aylo::ecs::storage

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** QueryMatcher
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <span>

#include "aylo/ecs/change/Tick.hpp"
#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/query/QueryTerm.hpp"
#include "aylo/ecs/storage/SparseSetStorage.hpp"

// The join runs once per visited entity: it lives in this header, inline, so that a typed Query, whose terms are
// known at compile time, can have it inlined and simplified in its loop instead of calling into the library.
namespace aylo::ecs::detail {
/// @brief Position recorded for a term whose component the matched entity does not have (absent optional,
/// without).
inline constexpr std::uint32_t kNoPosition = std::numeric_limits<std::uint32_t>::max();

/// @return The index of the term a query pass walks: the removed term with the fewest removals when there is one (the
/// entities that lost a component are no longer in its pool), else the required term with the fewest entities.
/// @param terms Checked terms, with at least one required or removed term (DynamicQuery::create() guarantees it).
/// @param pools The pool of each term, in term order.
[[nodiscard]] inline std::size_t findDriver(std::span<const QueryTerm> terms,
                                            std::span<storage::SparseSetStorage* const> pools) noexcept {
    std::size_t driver = 0;
    std::size_t driverSize = std::numeric_limits<std::size_t>::max();
    bool removing = false;
    std::size_t index = 0;
    auto pool = pools.begin();
    for (const QueryTerm& term : terms) {
        const bool removed = term.kind == TermKind::kRemoved;
        const std::size_t size = removed ? (*pool)->getRemovalLog().getRemovals().size() : (*pool)->getSize();
        if (isDriverTerm(term.kind) && (removed || !removing) && (removed != removing || size < driverSize)) {
            driver = index;
            driverSize = size;
            removing = removing || removed;
        }
        ++index;
        ++pool;
    }
    return driver;
}

/// @brief Checks one term against an entity: what matchEntity() does for each term.
/// @param isDriver Whether the term drives the pass: its pool is not searched, the entity is at `driverPosition` in
/// it (in its removals for a removed term, already checked by the pass).
/// @param position Set to the entity's position in the term's pool, or kNoPosition.
/// @return Whether the entity satisfies the term.
[[nodiscard]] inline bool matchTerm(const QueryTerm& term, const storage::SparseSetStorage& storage, Entity entity,
                                    bool isDriver, std::uint32_t driverPosition, std::uint32_t& position, Tick lastRun,
                                    Tick thisRun) noexcept {
    if (term.kind == TermKind::kRemoved) {
        position = kNoPosition;  // it gives no value: the entity may still have the component, or be dead
        return isDriver || storage.getRemovalLog().contains(entity, lastRun, thisRun);
    }
    const std::optional<std::uint32_t> found =
        isDriver ? std::optional<std::uint32_t>{driverPosition} : storage.find(entity);
    position = found.value_or(kNoPosition);
    if (term.kind == TermKind::kWithout) {
        return !found;
    }
    if (!isRequiredTerm(term.kind)) {
        return true;  // an optional term
    }
    if (!found) {
        return false;
    }
    if (term.kind == TermKind::kAdded) {
        return isNewerThan(*std::next(storage.getAddedTicks().begin(), *found), lastRun, thisRun);
    }
    if (term.kind == TermKind::kChanged) {
        return isNewerThan(*std::next(storage.getChangedTicks().begin(), *found), lastRun, thisRun);
    }
    return true;
}

/// @brief Checks every term against one entity of the driver pool: required pools contain it (with their tick
/// filters), without pools do not, removed terms list it. The one join algorithm of every query.
/// @param driver The index of the driver term; its pool is not searched: the entity is at `driverPosition` in it (in
/// its removals for a removed term, already checked by the pass).
/// @param driverPosition The entity's position in the driver pool, where the pass found it.
/// @param pools The pool of each term, in term order.
/// @param positions Filled with the entity's position in each term's pool, or kNoPosition.
/// @return Whether the entity matches; `positions` is only complete when it does.
[[nodiscard]] inline bool matchEntity(std::span<const QueryTerm> terms, Entity entity, std::size_t driver,
                                      std::uint32_t driverPosition, std::span<storage::SparseSetStorage* const> pools,
                                      std::span<std::uint32_t> positions, Tick lastRun, Tick thisRun) noexcept {
    std::size_t index = 0;
    auto pool = pools.begin();
    auto position = positions.begin();
    for (const QueryTerm& term : terms) {
        if (!matchTerm(term, **pool, entity, index == driver, driverPosition, *position, lastRun, thisRun)) {
            return false;
        }
        ++index;
        ++pool;
        ++position;
    }
    return true;
}
}  // namespace aylo::ecs::detail

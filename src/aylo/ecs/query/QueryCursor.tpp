/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** QueryCursor
*/

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <utility>

#include "aylo/ecs/query/QueryMatcher.hpp"
#include "aylo/ecs/storage/SparseSetStorage.hpp"

namespace aylo::ecs::detail {
template <typename Pools, typename Positions>
QueryCursor<Pools, Positions>::QueryCursor(std::span<const QueryTerm> terms, Pools pools, Positions positions,
                                           const MatchContext context) noexcept
    : _terms{terms},
      _pools{std::move(pools)},
      _positions{std::move(positions)},
      _driver{findDriver(_terms, _pools)},
      _removing{std::next(_terms.begin(), static_cast<std::ptrdiff_t>(_driver))->kind == TermKind::kRemoved},
      _context{context} {
    skipToMatch();
}

template <typename Pools, typename Positions>
const Pools& QueryCursor<Pools, Positions>::getPools() const noexcept {
    return _pools;
}

template <typename Pools, typename Positions>
const Positions& QueryCursor<Pools, Positions>::getPositions() const noexcept {
    return _positions;
}

template <typename Pools, typename Positions>
Tick QueryCursor<Pools, Positions>::getThisRun() const noexcept {
    return _context.thisRun;
}

template <typename Pools, typename Positions>
Entity QueryCursor<Pools, Positions>::getEntity() const noexcept {
    return _entity;
}

template <typename Pools, typename Positions>
bool QueryCursor<Pools, Positions>::isDone() const noexcept {
    const storage::SparseSetStorage& driver = **std::next(_pools.begin(), static_cast<std::ptrdiff_t>(_driver));
    return _index >= (_removing ? driver.getRemovalLog().getRemovals().size() : driver.getSize());
}

template <typename Pools, typename Positions>
void QueryCursor<Pools, Positions>::advance() noexcept {
    ++_index;
    skipToMatch();
}

template <typename Pools, typename Positions>
void QueryCursor<Pools, Positions>::skipToMatch() noexcept {
    if (_removing) {
        skipToRemoval();
        return;
    }
    const std::span<const Entity> entities =
        (*std::next(_pools.begin(), static_cast<std::ptrdiff_t>(_driver)))->getEntities();
    for (; _index < entities.size(); ++_index) {
        const Entity entity = *std::next(entities.begin(), static_cast<std::ptrdiff_t>(_index));
        if (matchEntity(_terms, entity, _driver, static_cast<std::uint32_t>(_index), _pools, _positions, _context)) {
            _entity = entity;
            return;
        }
    }
}

template <typename Pools, typename Positions>
void QueryCursor<Pools, Positions>::skipToRemoval() noexcept {
    const std::span<const storage::Removal> removals =
        (*std::next(_pools.begin(), static_cast<std::ptrdiff_t>(_driver)))->getRemovalLog().getRemovals();
    for (; _index < removals.size(); ++_index) {
        const storage::Removal& removal = *std::next(removals.begin(), static_cast<std::ptrdiff_t>(_index));
        if (isNewerThan(removal.tick, _context.lastRun, _context.thisRun) &&
            matchEntity(_terms, removal.entity, _driver, kNoPosition, _pools, _positions, _context)) {
            _entity = removal.entity;
            return;
        }
    }
}
}  // namespace aylo::ecs::detail

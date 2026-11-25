/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RemovalLog
*/

#include "RemovalLog.hpp"

#include <algorithm>
#include <memory>
#include <span>

#include "aylo/ecs/change/Tick.hpp"
#include "aylo/ecs/entity/Entity.hpp"

namespace aylo::ecs::storage {
std::shared_ptr<Tick> RemovalLog::watch(const Tick seen) {
    auto reader = std::make_shared<Tick>(seen);
    _readers.push_back(reader);
    return reader;
}

void RemovalLog::record(const Entity entity, const Tick tick) {
    dropSeen(tick);
    if (!_readers.empty()) {
        _removals.push_back({.entity = entity, .tick = tick});
    }
}

std::span<const Removal> RemovalLog::getRemovals() const noexcept { return _removals; }

bool RemovalLog::contains(const Entity entity, const Tick lastRun, const Tick thisRun) const noexcept {
    return std::ranges::any_of(_removals, [&](const Removal& removal) {
        return removal.entity == entity && isNewerThan(removal.tick, lastRun, thisRun);
    });
}

void RemovalLog::clampTicks(const Tick now) noexcept {
    for (Removal& removal : _removals) {
        clampTick(removal.tick, now);
    }
}

void RemovalLog::dropSeen(const Tick now) noexcept {
    std::erase_if(_readers, [](const std::weak_ptr<Tick>& reader) { return reader.expired(); });
    const auto unseen = std::ranges::find_if(_removals, [&](const Removal& removal) {
        return std::ranges::any_of(_readers, [&](const std::weak_ptr<Tick>& reader) {
            const std::shared_ptr<Tick> seen = reader.lock();
            return seen != nullptr && isNewerThan(removal.tick, *seen, now);
        });
    });
    _removals.erase(_removals.begin(), unseen);
}
}  // namespace aylo::ecs::storage

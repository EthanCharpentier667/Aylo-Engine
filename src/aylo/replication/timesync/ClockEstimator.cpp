/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClockEstimator
*/

#include "aylo/replication/timesync/ClockEstimator.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>

namespace aylo::replication::timesync {
void ClockEstimator::reset() noexcept {
    _used = 0;
    _rtt = 0.0;
}

bool ClockEstimator::addSample(const double sent, const double received, const double serverTime) noexcept {
    const double rtt = received - sent;
    if (!std::isfinite(rtt) || !std::isfinite(serverTime) || rtt < 0.0 || rtt > kMaxRoundTrip) {
        return false;
    }
    _rtt = _used == 0 ? rtt : _rtt + ((rtt - _rtt) * kRttSmoothing);
    _samples.at(_used % kWindow) = Sample{.rtt = rtt, .offset = serverTime + (rtt / 2.0) - received};
    ++_used;
    return true;
}

bool ClockEstimator::isSynced() const noexcept { return _used > 0; }

std::size_t ClockEstimator::sampleCount() const noexcept { return _used; }

std::optional<double> ClockEstimator::serverTime(const double local) const noexcept {
    if (_used == 0) {
        return std::nullopt;
    }
    const auto end = _samples.begin() + static_cast<std::ptrdiff_t>(std::min(_used, kWindow));
    const auto best =
        std::min_element(_samples.begin(), end, [](const Sample& a, const Sample& b) { return a.rtt < b.rtt; });
    return local + best->offset;
}

double ClockEstimator::rtt() const noexcept { return _rtt; }
}  // namespace aylo::replication::timesync

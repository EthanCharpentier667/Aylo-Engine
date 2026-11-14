/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ClockEstimator
*/

#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "aylo/replication/Export.hpp"

namespace aylo::replication::timesync {
/// @brief Estimates the server's clock from the ping/pong exchanges, on the client.
///
/// @details Each exchange gives the round trip (the time between the ping and its pong) and an offset between the two
/// clocks, assuming the pong travelled for half of it. The estimate uses the offset of the exchange with the smallest
/// round trip among the last kWindow: a delayed packet is never the best one, so a spike does not move the clock. It
/// knows no socket and no clock: the caller gives it the times, which makes it testable.
class AYLO_REPLICATION_API ClockEstimator {
  public:
    static constexpr std::size_t kWindow = 8;     ///< How many exchanges are kept.
    static constexpr double kMaxRoundTrip = 5.0;  ///< Seconds; a slower exchange is ignored.
    static constexpr double kRttSmoothing = 0.2;  ///< Weight of a new exchange in rtt().

    /// @brief Forgets every exchange (a new connection: the server may be another one).
    void reset() noexcept;

    /// @brief Adds the result of one ping/pong exchange.
    /// @param sent The local clock when the ping was sent, in seconds.
    /// @param received The local clock when the pong arrived, in seconds.
    /// @param serverTime The server's clock in the pong, in seconds.
    /// @return Whether it was used: false for a pong that came before its ping, or took more than kMaxRoundTrip.
    bool addSample(double sent, double received, double serverTime) noexcept;

    /// @return Whether at least one exchange was used.
    [[nodiscard]] bool isSynced() const noexcept;

    /// @return How many exchanges were used since the last reset().
    [[nodiscard]] std::size_t sampleCount() const noexcept;

    /// @param local The local clock, in seconds.
    /// @return The estimated server clock at that moment, or nothing before the first exchange.
    [[nodiscard]] std::optional<double> serverTime(double local) const noexcept;

    /// @return The smoothed round trip in seconds, 0 before the first exchange.
    [[nodiscard]] double rtt() const noexcept;

  private:
    /// @brief One exchange.
    struct Sample {
        double rtt{0.0};     ///< Round trip, in seconds.
        double offset{0.0};  ///< Server clock minus local clock, in seconds.
    };

    std::array<Sample, kWindow> _samples{};  ///< A ring of the last exchanges.
    std::size_t _used{0};                    ///< Exchanges used since reset().
    double _rtt{0.0};                        ///< The smoothed round trip.
};
}  // namespace aylo::replication::timesync

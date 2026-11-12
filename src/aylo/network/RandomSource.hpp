/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RandomSource
*/

#pragma once

#include <cstdint>
#include <functional>

#include "aylo/network/Export.hpp"

namespace aylo::network {

/// @brief Where challenges and tokens come from; tests inject a fixed sequence.
using RandomSource = std::function<std::uint64_t()>;

/// @brief Builds a random source backed by `std::random_device`, never by a seeded pseudo-random generator.
/// @note Not thread-safe: the returned source and all its copies share one `std::random_device`, so they must be
/// invoked from a single thread. No locking is done.
[[nodiscard]] AYLO_NETWORK_API RandomSource makeSystemRandom();

}  // namespace aylo::network

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RandomSource
*/

#include "aylo/network/RandomSource.hpp"

#include <cstdint>
#include <memory>
#include <random>

namespace aylo::network {

RandomSource makeSystemRandom() {
    // std::function needs a copyable callable, and std::random_device is not.
    const auto device = std::make_shared<std::random_device>();
    return [device] {
        const std::uint64_t high = (*device)();
        const std::uint64_t low = (*device)();
        return (high << 32U) | (low & 0xFFFFFFFFU);
    };
}

}  // namespace aylo::network

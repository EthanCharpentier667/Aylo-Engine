/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** RandomSourceTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <set>

#include "aylo/network/RandomSource.hpp"

TEST(RandomSource, SystemRandomIsCallable) {
    const aylo::network::RandomSource random = aylo::network::makeSystemRandom();

    EXPECT_TRUE(static_cast<bool>(random));
}

TEST(RandomSource, SystemRandomReturnsDifferentValues) {
    const aylo::network::RandomSource random = aylo::network::makeSystemRandom();

    EXPECT_NE(random(), random());
}

TEST(RandomSource, SystemRandomFillsTheHighBits) {
    const aylo::network::RandomSource random = aylo::network::makeSystemRandom();
    std::set<std::uint64_t> highWords;

    for (int i = 0; i < 16; ++i) {
        highWords.insert(random() >> 32U);
    }
    EXPECT_GT(highWords.size(), 1U);
}

TEST(RandomSource, CopiesShareTheSameDevice) {
    const aylo::network::RandomSource random = aylo::network::makeSystemRandom();
    const aylo::network::RandomSource copy = random;

    EXPECT_NE(random(), copy());
}

TEST(RandomSource, AFixedSequenceCanBeInjected) {
    std::uint64_t next = 0;
    const aylo::network::RandomSource fixed = [&next]() { return ++next; };

    EXPECT_EQ(fixed(), 1U);
    EXPECT_EQ(fixed(), 2U);
}

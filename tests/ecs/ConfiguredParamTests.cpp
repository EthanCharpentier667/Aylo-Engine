/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ConfiguredParamTests
*/

#include <gtest/gtest.h>

#include <memory>

#include "aylo/ecs/app/App.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/schedule/Phase.hpp"
#include "aylo/ecs/schedule/RunMode.hpp"
#include "aylo/ecs/schedule/Schedule.hpp"
#include "aylo/ecs/schedule/Schedules.hpp"
#include "aylo/ecs/system/ResourceParam.hpp"
#include "aylo/ecs/system/System.hpp"
#include "aylo/ecs/system/SystemAccess.hpp"
#include "aylo/ecs/system/SystemParam.hpp"
#include "aylo/ecs/world/World.hpp"

namespace aylo::test::configured {
struct Score {
    int value;
};
AYLO_ECS_RESOURCE(Score, "test.configured.Score", value);

/// @brief A parameter configured when the system is added: the bonus each run adds.
struct Bonus {
    int value;
};

/// @brief The configuration of Bonus. Move-only, so a copy anywhere on the way fails to compile.
struct BonusConfig {
    std::unique_ptr<int> value;
};

/// @brief A second configured parameter, with another configuration type.
struct Factor {
    int value;
};

/// @brief The configuration of Factor.
struct FactorConfig {
    int value;
};
}  // namespace aylo::test::configured

namespace aylo::ecs {
/// @brief Bonus keeps the value its configuration gave, and accesses nothing.
template <>
struct SystemParam<test::configured::Bonus> {
    using Config = test::configured::BonusConfig;  ///< Given when the system is added.

    /// @brief The configured bonus.
    struct State {
        int value;  ///< Added at each run.
    };

    static State init(World& /*world*/, Config config) { return State{.value = *config.value}; }

    static test::configured::Bonus get(State& state, const SystemContext& /*context*/) noexcept {
        return test::configured::Bonus{.value = state.value};
    }

    static void addAccess(const State& /*state*/, SystemAccess& /*access*/) noexcept {}
};

/// @brief Factor keeps the value its configuration gave, and accesses nothing.
template <>
struct SystemParam<test::configured::Factor> {
    using Config = test::configured::FactorConfig;  ///< Given when the system is added.
    using State = int;                              ///< The configured factor.

    static State init(World& /*world*/, Config config) noexcept { return config.value; }

    static test::configured::Factor get(State& state, const SystemContext& /*context*/) noexcept {
        return test::configured::Factor{.value = state};
    }

    static void addAccess(const State& /*state*/, SystemAccess& /*access*/) noexcept {}
};
}  // namespace aylo::ecs

namespace {
using aylo::ecs::Phase;
using aylo::ecs::ResMut;
using aylo::ecs::RunMode;
using aylo::ecs::System;
using aylo::ecs::World;
using aylo::test::configured::Bonus;
using aylo::test::configured::BonusConfig;
using aylo::test::configured::Factor;
using aylo::test::configured::FactorConfig;
using aylo::test::configured::Score;

void addBonus(ResMut<Score> score, Bonus bonus) { score->value += bonus.value; }

void addScaledBonus(Factor factor, ResMut<Score> score, Bonus bonus) { score->value += bonus.value * factor.value; }

/// @return A configuration of Bonus.
BonusConfig bonusOf(int value) { return BonusConfig{.value = std::make_unique<int>(value)}; }

/// @return The world's score.
int scoreOf(const World& world) { return world.getResources().require<Score>().value; }
}  // namespace

static_assert(aylo::ecs::ConfiguredParameter<Bonus>);
static_assert(!aylo::ecs::ConfiguredParameter<ResMut<Score>>);

TEST(ConfiguredParam, ReceivesItsConfiguration) {
    World world;
    world.getResources().insert(Score{.value = 0});
    System system{world, "test.addBonus", &addBonus, bonusOf(5)};
    system.run();
    system.run();
    EXPECT_EQ(scoreOf(world), 10);
}

TEST(ConfiguredParam, TakesConfigurationsInAnyOrder) {
    World world;
    world.getResources().insert(Score{.value = 0});
    System system{world, "test.addScaledBonus", &addScaledBonus, bonusOf(5), FactorConfig{.value = 3}};
    system.run();
    EXPECT_EQ(scoreOf(world), 15);  // the parameters are Factor then Bonus: each found its configuration by type
}

TEST(ConfiguredParam, IsConfiguredPerSystem) {
    World world;
    world.getResources().insert(Score{.value = 0});
    System small{world, "test.small", &addBonus, bonusOf(1)};
    System large{world, "test.large", &addBonus, bonusOf(100)};
    small.run();
    large.run();
    EXPECT_EQ(scoreOf(world), 101);  // one function, two configurations
}

TEST(ConfiguredParam, WorksWithALambda) {
    World world;
    world.getResources().insert(Score{.value = 0});
    System system{world, "test.lambda", [](ResMut<Score> score, Bonus bonus) { score->value -= bonus.value; },
                  bonusOf(4)};
    system.run();
    EXPECT_EQ(scoreOf(world), -4);
}

TEST(ConfiguredParam, IsForwardedByAddSystem) {
    World world;
    world.getResources().insert(Score{.value = 0});
    aylo::ecs::Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("test.schedule", &addBonus, bonusOf(1));
    schedule.run();
    EXPECT_EQ(scoreOf(world), 1);

    aylo::ecs::Schedules schedules{world, RunMode::kStandalone};
    schedules.addSystem(Phase::kUpdate, "test.schedules", &addBonus, bonusOf(10));
    schedules.run(Phase::kUpdate);
    EXPECT_EQ(scoreOf(world), 11);
}

TEST(ConfiguredParam, IsForwardedByTheApp) {
    aylo::ecs::App app;
    app.insertResource(Score{.value = 0});
    app.addSystem(Phase::kUpdate, "test.app", &addScaledBonus, FactorConfig{.value = 2}, bonusOf(7));
    app.runFrame();
    EXPECT_EQ(scoreOf(app.getWorld()), 14);
}

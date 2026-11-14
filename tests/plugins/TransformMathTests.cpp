/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** TransformMathTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <glm/ext/vector_float2.hpp>
#include <numbers>

#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformMath.hpp"

using aylo::plugins::transform::combine;
using aylo::plugins::transform::GlobalTransform2D;
using aylo::plugins::transform::toLocal;
using aylo::plugins::transform::Transform2D;

namespace {
constexpr float kQuarterTurn = std::numbers::pi_v<float> / 2.0F;
constexpr float kTolerance = 1e-3F;

void expectNear(glm::vec2 actual, glm::vec2 expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}
}  // namespace

TEST(Combine, IsTheLocalTransformUnderAnIdentityParent) {
    const GlobalTransform2D global =
        combine(GlobalTransform2D{}, Transform2D{.position = {3.0F, 4.0F}, .rotation = 0.5F, .scale = {2.0F, 2.0F}});
    expectNear(global.position, {3.0F, 4.0F});
    EXPECT_NEAR(global.rotation, 0.5F, kTolerance);
    expectNear(global.scale, {2.0F, 2.0F});
}

TEST(Combine, AddsThePositions) {
    expectNear(
        combine(GlobalTransform2D{.position = {400.0F, 200.0F}}, Transform2D{.position = {-30.0F, 10.0F}}).position,
        {370.0F, 210.0F});
}

TEST(Combine, TurnsThePositionClockwiseOnScreen) {
    const GlobalTransform2D parent{.rotation = kQuarterTurn};
    expectNear(combine(parent, Transform2D{.position = {10.0F, 0.0F}}).position, {0.0F, 10.0F});
}

TEST(Combine, ScalesThePositionBeforeTurningIt) {
    const GlobalTransform2D parent{.rotation = kQuarterTurn, .scale = {2.0F, 3.0F}};
    expectNear(combine(parent, Transform2D{.position = {10.0F, 0.0F}}).position, {0.0F, 20.0F});
}

TEST(Combine, MultipliesTheScalesAndAddsTheRotations) {
    const GlobalTransform2D global = combine(GlobalTransform2D{.rotation = 0.25F, .scale = {2.0F, 3.0F}},
                                             Transform2D{.rotation = 0.5F, .scale = {4.0F, 5.0F}});
    EXPECT_NEAR(global.rotation, 0.75F, kTolerance);
    expectNear(global.scale, {8.0F, 15.0F});
}

TEST(Combine, MirrorsThePositionAndReversesTheRotation) {
    const GlobalTransform2D global =
        combine(GlobalTransform2D{.scale = {-1.0F, 1.0F}}, Transform2D{.position = {10.0F, 5.0F}, .rotation = 0.5F});
    expectNear(global.position, {-10.0F, 5.0F});
    EXPECT_NEAR(global.rotation, -0.5F, kTolerance);
    expectNear(global.scale, {-1.0F, 1.0F});
}

TEST(Combine, KeepsTheRotationUnderAMirrorOnBothAxes) {
    EXPECT_NEAR(combine(GlobalTransform2D{.scale = {-1.0F, -1.0F}}, Transform2D{.rotation = 0.5F}).rotation, 0.5F,
                kTolerance);
}

TEST(Combine, ChainsThroughSeveralLevels) {
    const GlobalTransform2D root{.position = {400.0F, 200.0F}};
    const GlobalTransform2D turret = combine(root, Transform2D{.position = {30.0F, 10.0F}});
    expectNear(combine(turret, Transform2D{.position = {0.0F, -8.0F}}).position, {430.0F, 202.0F});
}

TEST(ToLocal, UndoesCombine) {
    constexpr std::array kParents{
        GlobalTransform2D{},
        GlobalTransform2D{.position = {100.0F, -50.0F}},
        GlobalTransform2D{.rotation = 1.2F},
        GlobalTransform2D{.scale = {2.0F, 2.0F}},
        GlobalTransform2D{.scale = {-1.0F, 1.0F}},
        GlobalTransform2D{.scale = {1.0F, -1.0F}},
        GlobalTransform2D{.scale = {-2.0F, -2.0F}},
        GlobalTransform2D{.position = {10.0F, 20.0F}, .rotation = -0.7F, .scale = {3.0F, 3.0F}},
        GlobalTransform2D{.position = {-5.0F, 8.0F}, .rotation = 2.1F, .scale = {-1.5F, 1.5F}},
        GlobalTransform2D{.position = {7.0F, 7.0F}, .rotation = 0.4F, .scale = {2.0F, 0.5F}},
    };
    const Transform2D local{.position = {12.0F, -7.0F}, .rotation = 0.6F, .scale = {1.5F, 0.75F}};
    for (const GlobalTransform2D& parent : kParents) {
        const Transform2D back = toLocal(parent, combine(parent, local));
        expectNear(back.position, local.position);
        EXPECT_NEAR(back.rotation, local.rotation, kTolerance);
        expectNear(back.scale, local.scale);
    }
}

TEST(ToLocal, GivesZeroOnAnAxisTheParentFlattens) {
    const Transform2D local = toLocal(GlobalTransform2D{.scale = {0.0F, 2.0F}},
                                      GlobalTransform2D{.position = {10.0F, 10.0F}, .scale = {1.0F, 1.0F}});
    EXPECT_FALSE(std::isnan(local.position.x));
    expectNear(local.position, {0.0F, 5.0F});
    expectNear(local.scale, {0.0F, 0.5F});
}

TEST(Interpolate, BlendsPositionRotationAndScale) {
    const Transform2D from{.position = {0.0F, 10.0F}, .rotation = 0.0F, .scale = {1.0F, 1.0F}};
    const Transform2D to{.position = {10.0F, 30.0F}, .rotation = kQuarterTurn, .scale = {3.0F, 1.0F}};
    const Transform2D half = aylo::plugins::transform::interpolate(from, to, 0.5F);
    expectNear(half.position, {5.0F, 20.0F});
    EXPECT_NEAR(half.rotation, kQuarterTurn / 2.0F, kTolerance);
    expectNear(half.scale, {2.0F, 1.0F});
}

TEST(Interpolate, GivesItsEndsAtZeroAndOne) {
    const Transform2D from{.position = {1.0F, 2.0F}, .rotation = 0.25F};
    const Transform2D to{.position = {7.0F, -4.0F}, .rotation = 0.75F};
    expectNear(aylo::plugins::transform::interpolate(from, to, 0.0F).position, from.position);
    expectNear(aylo::plugins::transform::interpolate(from, to, 1.0F).position, to.position);
    EXPECT_NEAR(aylo::plugins::transform::interpolate(from, to, 1.0F).rotation, to.rotation, kTolerance);
}

TEST(Interpolate, TurnsTheShortWayAcrossAFullTurn) {
    const float almostTurn = (2.0F * std::numbers::pi_v<float>)-0.1F;
    const Transform2D from{.rotation = almostTurn};
    const Transform2D to{.rotation = 0.1F};  // 0.2 radians further clockwise, not 6.08 back
    EXPECT_NEAR(aylo::plugins::transform::interpolate(from, to, 0.5F).rotation, almostTurn + 0.1F, kTolerance);
}

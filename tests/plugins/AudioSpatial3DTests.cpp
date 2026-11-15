/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** AudioSpatial3DTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

#include "plugins/audio/Spatial3D.hpp"

using aylo::plugins::audio::ListenerPose3D;
using aylo::plugins::audio::SoundPlace3D;
using aylo::plugins::audio::spatialize;

namespace {
constexpr float kTolerance = 1e-4F;

/// @return A rotation of @p degrees around Y: a positive one turns the listener to its left.
glm::vec4 turnedLeft(float degrees) {
    const float half = degrees * 3.14159265F / 360.0F;
    return {0.0F, std::sin(half), 0.0F, std::cos(half)};
}
}  // namespace

TEST(AudioSpatial3D, PansFromTheListenersRightAndLeft) {
    const ListenerPose3D listener;  // at the origin, looking along -Z, +X on its right
    EXPECT_NEAR(spatialize({.position = {5.0F, 0.0F, 0.0F}}, listener).pan, 1.0F, kTolerance);
    EXPECT_NEAR(spatialize({.position = {-5.0F, 0.0F, 0.0F}}, listener).pan, -1.0F, kTolerance);
    EXPECT_NEAR(spatialize({.position = {0.0F, 0.0F, -5.0F}}, listener).pan, 0.0F, kTolerance);  // in front
    EXPECT_NEAR(spatialize({.position = {0.0F, 0.0F, 5.0F}}, listener).pan, 0.0F, kTolerance);   // behind
    EXPECT_NEAR(spatialize({.position = {3.0F, 0.0F, -3.0F}}, listener).pan, std::sqrt(0.5F), kTolerance);
}

TEST(AudioSpatial3D, FollowsWhereTheListenerFaces) {
    // Turned a quarter to its left, it faces -X: a sound in front of the world (-Z) is now on its right.
    const ListenerPose3D turned{.rotation = turnedLeft(90.0F)};
    EXPECT_NEAR(spatialize({.position = {0.0F, 0.0F, -5.0F}}, turned).pan, 1.0F, kTolerance);
    EXPECT_NEAR(spatialize({.position = {-5.0F, 0.0F, 0.0F}}, turned).pan, 0.0F, kTolerance);
    // Moved, it hears from where it is.
    const ListenerPose3D moved{.position = {10.0F, 0.0F, 0.0F}};
    EXPECT_NEAR(spatialize({.position = {5.0F, 0.0F, 0.0F}}, moved).pan, -1.0F, kTolerance);
}

TEST(AudioSpatial3D, FadesWithTheInverseOfTheDistanceBetweenTheTwoDistances) {
    const ListenerPose3D listener;
    const auto at = [&listener](float distance) {
        return spatialize({.position = {distance, 0.0F, 0.0F}, .minDistance = 2.0F, .maxDistance = 40.0F}, listener)
            .gain;
    };
    EXPECT_NEAR(at(1.0F), 1.0F, kTolerance);  // closer than the minimum: full
    EXPECT_NEAR(at(2.0F), 1.0F, kTolerance);
    EXPECT_NEAR(at(4.0F), 0.5F, kTolerance);     // twice the minimum: half
    EXPECT_NEAR(at(40.0F), 0.05F, kTolerance);   // the maximum
    EXPECT_NEAR(at(400.0F), 0.05F, kTolerance);  // no quieter beyond it
}

TEST(AudioSpatial3D, ASoundAtTheListenersPlaceIsCentredAndFull) {
    const auto heard = spatialize({.position = {1.0F, 2.0F, 3.0F}}, ListenerPose3D{.position = {1.0F, 2.0F, 3.0F}});
    EXPECT_EQ(heard.pan, 0.0F);
    EXPECT_EQ(heard.gain, 1.0F);
}

TEST(AudioSpatial3D, ReadsAZeroRotationAsNoneAndBadDistancesSafely) {
    const ListenerPose3D zero{.rotation = {0.0F, 0.0F, 0.0F, 0.0F}};
    EXPECT_NEAR(spatialize({.position = {5.0F, 0.0F, 0.0F}}, zero).pan, 1.0F, kTolerance);
    // A maximum below the minimum is the minimum; a minimum of 0 is 1 cm: the gain stays in (0, 1].
    const auto odd = spatialize({.position = {5.0F, 0.0F, 0.0F}, .minDistance = 0.0F, .maxDistance = -1.0F}, {});
    EXPECT_GT(odd.gain, 0.0F);
    EXPECT_LE(odd.gain, 1.0F);
}

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** PhysicsCharacter3DTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <optional>
#include <vector>

#include "aylo/ecs/entity/Entity.hpp"
#include "plugins/physics/Collider3D.hpp"
#include "plugins/physics/MoveAndSlide.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/QueryFilter.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/transform/Transform.hpp"

using aylo::ecs::Entity;
using aylo::plugins::physics::Collider3D;
using aylo::plugins::physics::moveAndSlide3D;
using aylo::plugins::physics::PhysicsSettings;
using aylo::plugins::physics::PhysicsWorld;
using aylo::plugins::physics::QueryFilter;
using aylo::plugins::physics::Shape3DType;
using aylo::plugins::physics::Slide3DResult;
using aylo::plugins::physics::Slide3DSettings;
using aylo::plugins::physics::exceptions::PhysicsException;
using aylo::plugins::transform::Transform3D;

namespace {
constexpr float kTolerance = 0.01F;
constexpr glm::vec4 kUpright{0.0F, 0.0F, 0.0F, 1.0F};
constexpr float kSkin = 0.02F;

/// @brief The player: a capsule 1.8 m tall (radius 0.3, straight part 1.2), its Transform3D at its centre.
const Collider3D kPlayer{.shape = Shape3DType::kCapsule, .radius = 0.3F, .halfHeight = 0.6F};
/// @brief The height of the player's centre when it stands on the floor (y = 0), a skin above it.
constexpr float kStanding = 0.9F + kSkin;

/// @return A rotation of @p degrees around Z: a positive one turns +X toward +Y.
glm::vec4 aroundZ(float degrees) {
    const float half = degrees * 3.14159265F / 360.0F;
    return {0.0F, 0.0F, std::sin(half), std::cos(half)};
}

/// @brief A small level: a floor whose top is y = 0, and a wall whose face toward the origin is x = 2.
struct Level {
    PhysicsWorld world{PhysicsSettings{
        .maxBodies = 32, .maxBodyPairs = 32, .maxContactConstraints = 32, .tempAllocatorBytes = 1024 * 1024}};
    Entity floor{1, 1};
    Entity wall{2, 1};
    std::uint32_t next = 3;

    Level() {
        add(floor, Collider3D{.size = {40.0F, 1.0F, 40.0F}}, {0.0F, -0.5F, 0.0F});
        add(wall, Collider3D{.size = {1.0F, 6.0F, 40.0F}}, {2.5F, 3.0F, 0.0F});
    }

    /// @brief Adds a static 3D body.
    Entity add(Entity entity, const Collider3D& collider, const glm::vec3& at, const glm::vec4& rotation = kUpright) {
        static_cast<void>(
            world.createBody3D(entity, collider, nullptr, Transform3D{.position = at, .rotation = rotation}));
        return entity;
    }

    /// @brief Adds a static box of @p size at @p at.
    Entity box(const glm::vec3& size, const glm::vec3& at, const glm::vec4& rotation = kUpright) {
        return add(Entity{next++, 1}, Collider3D{.size = size}, at, rotation);
    }

    /// @brief Moves the player for @p ticks ticks of @p perTick, gravity included, as a script does.
    [[nodiscard]] Slide3DResult walk(glm::vec3 from, const glm::vec3& perTick, int ticks,
                                     const Slide3DSettings& settings = {.skin = kSkin}) const {
        Slide3DResult result{.position = from};
        for (int tick = 0; tick < ticks; ++tick) {
            result =
                moveAndSlide3D(world, kPlayer, result.position, perTick + glm::vec3{0.0F, -0.05F, 0.0F}, {}, settings);
        }
        return result;
    }
};
}  // namespace

TEST(PhysicsShapeCast3D, StopsWhereTheShapeFirstTouches) {
    const Level level;
    const auto hit = level.world.shapeCast3D(Collider3D{.shape = Shape3DType::kSphere, .radius = 0.5F},
                                             {0.0F, 2.0F, 0.0F}, kUpright, {1.0F, 0.0F, 0.0F}, 10.0F);
    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->entity, level.wall);
    EXPECT_NEAR(hit->distance, 1.5F, kTolerance);  // the sphere's surface reaches the face at x = 2
    EXPECT_NEAR(hit->normal.x, -1.0F, kTolerance);
    EXPECT_NEAR(hit->point.x, 2.0F, kTolerance);
}

TEST(PhysicsShapeCast3D, FindsNothingOutOfReachOrWithoutADirection) {
    const Level level;
    const Collider3D sphere{.shape = Shape3DType::kSphere, .radius = 0.5F};
    EXPECT_FALSE(level.world.shapeCast3D(sphere, {0.0F, 2.0F, 0.0F}, kUpright, {1.0F, 0.0F, 0.0F}, 1.0F));
    EXPECT_FALSE(level.world.shapeCast3D(sphere, {0.0F, 2.0F, 0.0F}, kUpright, {0.0F, 0.0F, 0.0F}, 10.0F));
}

TEST(PhysicsShapeCast3D, TheRotationTurnsTheShape) {
    const Level level;
    // A plank 3 m long along X, 1.5 m from the wall: it reaches it at once, but turned along Z it travels 1.4 m.
    const Collider3D plank{.size = {3.0F, 0.2F, 0.2F}};
    const glm::vec4 alongZ{0.0F, std::sin(0.785398F), 0.0F, std::cos(0.785398F)};
    const auto flat = level.world.shapeCast3D(plank, {0.5F, 2.0F, 0.0F}, kUpright, {1.0F, 0.0F, 0.0F}, 5.0F);
    const auto turned = level.world.shapeCast3D(plank, {0.5F, 2.0F, 0.0F}, alongZ, {1.0F, 0.0F, 0.0F}, 5.0F);
    ASSERT_TRUE(flat);
    ASSERT_TRUE(turned);
    EXPECT_NEAR(flat->distance, 0.0F, kTolerance);
    EXPECT_NEAR(turned->distance, 1.4F, kTolerance);
}

TEST(PhysicsOverlap3D, ListsTheEntitiesUnderAShapeAndFollowsTheFilter) {
    Level level;
    const Entity crate = level.box({1.0F, 1.0F, 1.0F}, {0.0F, 0.5F, 0.0F});
    const Collider3D probe{.shape = Shape3DType::kSphere, .radius = 0.6F};
    const std::vector<Entity> found = level.world.overlap3D(probe, {0.0F, 0.3F, 0.0F}, kUpright);
    EXPECT_EQ(found.size(), 2U);  // the crate and the floor under it
    EXPECT_NE(std::ranges::find(found, crate), found.end());
    EXPECT_EQ(level.world.overlap3D(probe, {0.0F, 0.3F, 0.0F}, kUpright, QueryFilter{.ignore = level.floor}),
              std::vector<Entity>{crate});
    EXPECT_TRUE(level.world.overlap3D(probe, {0.0F, 10.0F, 0.0F}, kUpright).empty());
}

TEST(PhysicsOverlap3D, TheRotationTurnsTheShape) {
    const Level level;
    const Collider3D plank{.size = {3.0F, 0.2F, 0.2F}};
    const glm::vec4 alongZ{0.0F, std::sin(0.785398F), 0.0F, std::cos(0.785398F)};
    EXPECT_EQ(level.world.overlap3D(plank, {0.6F, 2.0F, 0.0F}, kUpright), std::vector<Entity>{level.wall});
    EXPECT_TRUE(level.world.overlap3D(plank, {0.6F, 2.0F, 0.0F}, alongZ).empty());
}

TEST(PhysicsCharacter3D, FallsOntoTheFloorAndStandsAskinAboveIt) {
    const Level level;
    const Slide3DResult fall = moveAndSlide3D(level.world, kPlayer, {0.0F, 3.0F, 0.0F}, {0.0F, -5.0F, 0.0F}, {},
                                              Slide3DSettings{.skin = kSkin});
    EXPECT_NEAR(fall.position.y, kStanding, kTolerance);
    EXPECT_TRUE(fall.grounded);
    EXPECT_EQ(fall.ground, level.floor);
    EXPECT_NEAR(fall.groundNormal.y, 1.0F, kTolerance);
}

TEST(PhysicsCharacter3D, IsNotGroundedInTheAir) {
    const Level level;
    const Slide3DResult jump = moveAndSlide3D(level.world, kPlayer, {0.0F, 3.0F, 0.0F}, {0.0F, 0.1F, 0.0F}, {},
                                              Slide3DSettings{.skin = kSkin});
    EXPECT_NEAR(jump.position.y, 3.1F, kTolerance);
    EXPECT_FALSE(jump.grounded);
    EXPECT_TRUE(jump.ground.isNull());
    EXPECT_EQ(jump.groundNormal, glm::vec3{0.0F});
}

TEST(PhysicsCharacter3D, SlidesAlongAWallInsteadOfStopping) {
    const Level level;
    const Slide3DResult moved = level.walk({1.0F, kStanding, 0.0F}, {0.2F, 0.0F, 0.2F}, 10);
    EXPECT_NEAR(moved.position.x, 2.0F - 0.3F - kSkin, kTolerance);  // stopped a skin from the wall
    EXPECT_NEAR(moved.position.z, 2.0F, kTolerance);                 // but went on along it
    EXPECT_NEAR(moved.position.y, kStanding, kTolerance);            // neither climbed nor sank
    EXPECT_TRUE(moved.grounded);
}

TEST(PhysicsCharacter3D, StepsUpAStairOnlyWithAStepHeight) {
    Level level;
    level.box({2.0F, 0.25F, 40.0F}, {-3.0F, 0.125F, 0.0F});  // a step 0.25 m high, from x = -4 to x = -2
    const glm::vec3 start{-1.0F, kStanding, 0.0F};

    const Slide3DResult blocked = level.walk(start, {-0.1F, 0.0F, 0.0F}, 20);
    EXPECT_NEAR(blocked.position.x, -2.0F + 0.3F + kSkin, 0.05F);
    EXPECT_NEAR(blocked.position.y, kStanding, kTolerance);

    const Slide3DResult stepped = level.walk(start, {-0.1F, 0.0F, 0.0F}, 20, {.skin = kSkin, .stepHeight = 0.3F});
    EXPECT_LT(stepped.position.x, -2.5F);
    EXPECT_NEAR(stepped.position.y, kStanding + 0.25F, kTolerance);
    EXPECT_TRUE(stepped.grounded);
}

TEST(PhysicsCharacter3D, DoesNotStepUpWhatIsHigherThanTheStepHeight) {
    Level level;
    level.box({2.0F, 0.5F, 40.0F}, {-3.0F, 0.25F, 0.0F});  // 0.5 m: a ledge, not a stair
    const Slide3DResult moved =
        level.walk({-1.0F, kStanding, 0.0F}, {-0.1F, 0.0F, 0.0F}, 20, {.skin = kSkin, .stepHeight = 0.3F});
    EXPECT_GT(moved.position.x, -2.0F);
    EXPECT_NEAR(moved.position.y, kStanding, kTolerance);
}

TEST(PhysicsCharacter3D, WalksUpAGentleSlopeButNotASteepOne) {
    // A ramp centred at the origin, rising toward -X: it comes out of the floor near x = 0.3.
    Level gentle;
    gentle.box({6.0F, 0.2F, 40.0F}, {0.0F, 0.0F, 0.0F}, aroundZ(-20.0F));
    const Slide3DResult up = gentle.walk({1.5F, kStanding, 0.0F}, {-0.1F, 0.0F, 0.0F}, 30);
    EXPECT_LT(up.position.x, -0.5F);
    EXPECT_GT(up.position.y, kStanding + 0.2F);  // it climbed
    EXPECT_TRUE(up.grounded);
    EXPECT_NEAR(up.groundNormal.y, std::cos(20.0F * 3.14159265F / 180.0F), kTolerance);

    Level steep;
    steep.box({6.0F, 0.2F, 40.0F}, {0.0F, 0.0F, 0.0F}, aroundZ(-60.0F));  // 60 degrees: a wall to the player
    const Slide3DResult stopped = steep.walk({1.5F, kStanding, 0.0F}, {-0.1F, 0.0F, 0.0F}, 30);
    EXPECT_GT(stopped.position.x, 0.0F);              // stopped at its foot
    EXPECT_LT(stopped.position.y, kStanding + 0.1F);  // never walked up it
}

TEST(PhysicsCharacter3D, SlidesDownASlopeTooSteepToStandOn) {
    Level level;
    level.box({6.0F, 0.2F, 40.0F}, {0.0F, 0.0F, 0.0F}, aroundZ(-60.0F));
    // Dropped on the ramp's face, it does not stay there: the fall slides it down, away from the ramp.
    const Slide3DResult first = level.walk({-0.8F, 3.5F, 0.0F}, {0.0F, 0.0F, 0.0F}, 1);
    const Slide3DResult later = level.walk({-0.8F, 3.5F, 0.0F}, {0.0F, 0.0F, 0.0F}, 60);
    EXPECT_FALSE(first.grounded);
    EXPECT_GT(later.position.x, first.position.x + 0.3F);
    EXPECT_LT(later.position.y, first.position.y - 0.5F);
}

TEST(PhysicsCharacter3D, AnEntityIsNotBlockedByItsOwnBody) {
    Level level;
    const Entity player{20, 1};
    level.add(player, kPlayer, {0.0F, kStanding, 0.0F});  // its own body, where it stands
    const Slide3DResult moved = moveAndSlide3D(level.world, player, kPlayer, {0.0F, kStanding, 0.0F},
                                               {0.5F, 0.0F, 0.0F}, Slide3DSettings{.skin = kSkin});
    EXPECT_NEAR(moved.position.x, 0.5F, kTolerance);
}

TEST(PhysicsCharacter3D, RefusesSettingsThatCannotBe) {
    const Level level;
    const auto move = [&level](const Slide3DSettings& settings) {
        return moveAndSlide3D(level.world, kPlayer, {0.0F, 2.0F, 0.0F}, {0.0F, 0.0F, 0.0F}, {}, settings);
    };
    EXPECT_THROW(static_cast<void>(move({.skin = -0.1F})), PhysicsException);
    EXPECT_THROW(static_cast<void>(move({.stepHeight = -0.1F})), PhysicsException);
    EXPECT_THROW(static_cast<void>(move({.maxSlope = 90.0F})), PhysicsException);
    EXPECT_THROW(
        static_cast<void>(moveAndSlide3D(level.world, Collider3D{.shape = Shape3DType::kSphere, .radius = -1.0F},
                                         {0.0F, 2.0F, 0.0F}, {1.0F, 0.0F, 0.0F})),
        PhysicsException);
}

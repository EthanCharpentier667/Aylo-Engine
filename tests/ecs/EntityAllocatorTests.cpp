/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** EntityAllocatorTests
*/

#include <gtest/gtest.h>

#include <cstdint>

#include "aylo/ecs/entity/Entity.hpp"
#include "aylo/ecs/entity/EntityAllocator.hpp"

TEST(EntityAllocator, AllocatesFreshIndicesWithGenerationOne) {
    aylo::ecs::EntityAllocator allocator;
    for (std::uint32_t index = 0; index < 3; ++index) {
        const aylo::ecs::Entity entity = allocator.allocate();
        EXPECT_EQ(entity.getIndex(), index);
        EXPECT_EQ(entity.getGeneration(), 1U);
        EXPECT_TRUE(allocator.isAlive(entity));
    }
    EXPECT_EQ(allocator.getAliveCount(), 3U);
}

TEST(EntityAllocator, ReleaseMakesTheHandleDead) {
    aylo::ecs::EntityAllocator allocator;
    const aylo::ecs::Entity entity = allocator.allocate();
    EXPECT_TRUE(allocator.release(entity));
    EXPECT_FALSE(allocator.isAlive(entity));
    EXPECT_EQ(allocator.getAliveCount(), 0U);
}

TEST(EntityAllocator, ReusesTheLastReleasedSlotWithANewGeneration) {
    aylo::ecs::EntityAllocator allocator;
    const aylo::ecs::Entity first = allocator.allocate();
    const aylo::ecs::Entity second = allocator.allocate();
    ASSERT_TRUE(allocator.release(first));
    ASSERT_TRUE(allocator.release(second));

    const aylo::ecs::Entity reused = allocator.allocate();
    EXPECT_EQ(reused.getIndex(), second.getIndex());
    EXPECT_EQ(reused.getGeneration(), second.getGeneration() + 1);
    EXPECT_NE(reused, second);
    EXPECT_TRUE(allocator.isAlive(reused));
    EXPECT_FALSE(allocator.isAlive(second));

    EXPECT_EQ(allocator.allocate().getIndex(), first.getIndex());
}

TEST(EntityAllocator, RefusesToReleaseAnEntityThatIsNotAlive) {
    aylo::ecs::EntityAllocator allocator;
    const aylo::ecs::Entity entity = allocator.allocate();
    ASSERT_TRUE(allocator.release(entity));

    EXPECT_FALSE(allocator.release(entity));                    // already released
    EXPECT_FALSE(allocator.release(aylo::ecs::Entity{}));       // null
    EXPECT_FALSE(allocator.release(aylo::ecs::Entity{99, 1}));  // unknown index

    const aylo::ecs::Entity reused = allocator.allocate();
    EXPECT_FALSE(allocator.release(entity));  // stale: the slot now holds `reused`
    EXPECT_TRUE(allocator.isAlive(reused));
    EXPECT_EQ(allocator.getAliveCount(), 1U);
}

TEST(EntityAllocator, NullAndUnknownEntitiesAreNotAlive) {
    aylo::ecs::EntityAllocator allocator;
    static_cast<void>(allocator.allocate());
    EXPECT_FALSE(allocator.isAlive(aylo::ecs::Entity{}));
    EXPECT_FALSE(allocator.isAlive(aylo::ecs::Entity{5, 1}));
    EXPECT_FALSE(allocator.isAlive(aylo::ecs::Entity{0, 2}));  // right index, wrong generation
}

TEST(EntityAllocator, ReservedEntitiesBecomeAliveOnFlush) {
    aylo::ecs::EntityAllocator allocator;
    const aylo::ecs::Entity entity = allocator.reserve();
    EXPECT_FALSE(entity.isNull());
    EXPECT_TRUE(allocator.isReserved(entity));
    EXPECT_FALSE(allocator.isAlive(entity));
    EXPECT_EQ(allocator.getAliveCount(), 0U);

    allocator.flushReserved();
    EXPECT_TRUE(allocator.isAlive(entity));
    EXPECT_FALSE(allocator.isReserved(entity));
    EXPECT_EQ(allocator.getAliveCount(), 1U);
}

TEST(EntityAllocator, ReleasingAReservedEntityCancelsIt) {
    aylo::ecs::EntityAllocator allocator;
    const aylo::ecs::Entity cancelled = allocator.reserve();
    const aylo::ecs::Entity kept = allocator.reserve();
    EXPECT_TRUE(allocator.release(cancelled));
    EXPECT_FALSE(allocator.isReserved(cancelled));

    allocator.flushReserved();
    EXPECT_FALSE(allocator.isAlive(cancelled));
    EXPECT_TRUE(allocator.isAlive(kept));
    EXPECT_EQ(allocator.getAliveCount(), 1U);

    const aylo::ecs::Entity reused = allocator.allocate();
    EXPECT_EQ(reused.getIndex(), cancelled.getIndex());
    EXPECT_NE(reused, cancelled);
}

TEST(EntityAllocator, ASlotReusedBeforeTheFlushIsCountedOnce) {
    aylo::ecs::EntityAllocator allocator;
    const aylo::ecs::Entity cancelled = allocator.reserve();
    ASSERT_TRUE(allocator.release(cancelled));
    const aylo::ecs::Entity reserved = allocator.reserve();  // same slot, reserved twice before the flush
    ASSERT_EQ(reserved.getIndex(), cancelled.getIndex());

    allocator.flushReserved();
    EXPECT_TRUE(allocator.isAlive(reserved));
    EXPECT_EQ(allocator.getAliveCount(), 1U);
}

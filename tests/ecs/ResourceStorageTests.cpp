/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ResourceStorageTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>

#include "aylo/ecs/change/Mut.hpp"
#include "aylo/ecs/change/Tick.hpp"
#include "aylo/ecs/resource/ResourceInfo.hpp"
#include "aylo/ecs/resource/ResourceTraits.hpp"
#include "aylo/ecs/storage/ResourceStorage.hpp"
#include "aylo/ecs/storage/ViewAs.hpp"

namespace {
/// @brief Number of Counter instances alive, to detect leaks and double destruction.
int liveCounters = 0;

/// @brief Counts its own instances.
struct Counter {
    Counter() noexcept { ++liveCounters; }
    ~Counter() { --liveCounters; }
    Counter(const Counter& /*other*/) noexcept { ++liveCounters; }
    Counter& operator=(const Counter& /*other*/) noexcept = default;
    Counter(Counter&& /*other*/) noexcept { ++liveCounters; }
    Counter& operator=(Counter&& /*other*/) noexcept = default;
};

/// @brief Makes every Fragile copy throw while true.
bool throwOnCopy = false;
}  // namespace

namespace aylo::test::resource_storage {
struct Time {
    float delta;
    float fixedDelta;
};
AYLO_ECS_RESOURCE(Time, "aylo.Time");

struct Tracked {
    std::int32_t value;
    Counter counter;
};
AYLO_ECS_RESOURCE(Tracked, "test.Tracked");

struct Backend {
    std::unique_ptr<std::int32_t> window;
};
AYLO_ECS_RESOURCE(Backend, "aylo.Backend");

struct Paused {};
AYLO_ECS_RESOURCE(Paused, "aylo.Paused");

/// @brief A resource whose copy throws on demand, counting its instances.
struct Fragile {
    explicit Fragile(std::int32_t newValue) noexcept : value{newValue} {}
    ~Fragile() = default;
    Fragile(const Fragile& other) : value{other.value} {
        if (throwOnCopy) {
            throw std::runtime_error{"copy failed"};
        }
    }
    Fragile& operator=(const Fragile& other) = default;
    Fragile(Fragile&& other) noexcept = default;
    Fragile& operator=(Fragile&& other) noexcept = default;

    std::int32_t value;  ///< Payload.
    Counter counter;     ///< Detects leaks and double destruction.
};
AYLO_ECS_RESOURCE(Fragile, "test.Fragile");

struct alignas(16) Aligned16 {
    std::array<float, 4> values;
};
AYLO_ECS_RESOURCE(Aligned16, "test.Aligned16");
}  // namespace aylo::test::resource_storage

namespace res = aylo::test::resource_storage;

namespace {
/// @return The description of `T`, kept alive for the whole test run: a storage only points to it.
template <typename T>
const aylo::ecs::ResourceInfo& infoOf() {
    static const aylo::ecs::ResourceInfo info = aylo::ecs::ResourceTraits<T>::makeInfo();
    return info;
}

template <typename T>
aylo::ecs::storage::ResourceStorage makeStorage() {
    return aylo::ecs::storage::ResourceStorage{infoOf<T>()};
}

template <typename T>
std::span<const std::byte> bytesOf(const T& value) {
    return std::as_bytes(std::span{&value, 1});
}

template <typename T>
std::span<std::byte> bytesOf(T& value) {
    return std::as_writable_bytes(std::span{&value, 1});
}
}  // namespace

TEST(ResourceStorage, StartsEmpty) {
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Time>();
    EXPECT_FALSE(storage.hasValue());
    EXPECT_TRUE(storage.get().empty());
    EXPECT_FALSE(storage.remove());
    EXPECT_EQ(storage.getInfo().type.name, "aylo.Time");
}

TEST(ResourceStorage, InsertMoveThenGet) {
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Time>();
    res::Time time{.delta = 0.5F, .fixedDelta = 0.25F};
    storage.insertMove(bytesOf(time), 3);
    ASSERT_TRUE(storage.hasValue());
    const res::Time* stored = aylo::ecs::storage::viewAs<res::Time>(storage.get());
    EXPECT_FLOAT_EQ(stored->delta, 0.5F);
    EXPECT_FLOAT_EQ(stored->fixedDelta, 0.25F);
    EXPECT_EQ(storage.getTicks().added, 3U);
    EXPECT_EQ(storage.getTicks().changed, 3U);
}

TEST(ResourceStorage, InsertCopyLeavesSourceIntact) {
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Time>();
    const res::Time time{.delta = 1.F, .fixedDelta = 2.F};
    storage.insertCopy(bytesOf(time), 1);
    EXPECT_FLOAT_EQ(aylo::ecs::storage::viewAs<res::Time>(storage.get())->fixedDelta, 2.F);
    EXPECT_FLOAT_EQ(time.fixedDelta, 2.F);
}

TEST(ResourceStorage, MutReadDoesNotMarkChanged) {
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Time>();
    res::Time time{.delta = 2.F, .fixedDelta = 0.F};
    storage.insertMove(bytesOf(time), 1);
    const aylo::ecs::Mut<res::Time> mut{aylo::ecs::storage::viewAs<res::Time>(storage.get()),
                                        &storage.getTicks().changed, 7};
    EXPECT_FLOAT_EQ(mut.get().delta, 2.F);
    EXPECT_EQ(storage.getTicks().changed, 1U);
}

TEST(ResourceStorage, MutWriteMarksChanged) {
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Time>();
    res::Time time{.delta = 0.F, .fixedDelta = 0.F};
    storage.insertMove(bytesOf(time), 1);
    aylo::ecs::Mut<res::Time> mut{aylo::ecs::storage::viewAs<res::Time>(storage.get()), &storage.getTicks().changed, 7};
    mut->delta = 4.F;
    EXPECT_FLOAT_EQ(aylo::ecs::storage::viewAs<res::Time>(storage.get())->delta, 4.F);
    EXPECT_EQ(storage.getTicks().added, 1U);
    EXPECT_EQ(storage.getTicks().changed, 7U);
    EXPECT_TRUE(storage.getTicks().isChanged(5, 8));
    EXPECT_FALSE(storage.getTicks().isAdded(5, 8));
}

TEST(ResourceStorage, ReplaceKeepsAddedTickAndDestroysOldValue) {
    liveCounters = 0;
    {
        aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Tracked>();
        res::Tracked first{.value = 1, .counter = {}};
        storage.insertMove(bytesOf(first), 2);
        res::Tracked second{.value = 2, .counter = {}};
        storage.insertMove(bytesOf(second), 5);
        EXPECT_EQ(liveCounters, 3);  // first, second and the stored copy
        EXPECT_EQ(aylo::ecs::storage::viewAs<res::Tracked>(storage.get())->value, 2);
        EXPECT_EQ(storage.getTicks().added, 2U);
        EXPECT_EQ(storage.getTicks().changed, 5U);
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(ResourceStorage, RemoveDestroysValue) {
    liveCounters = 0;
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Tracked>();
    {
        res::Tracked value{.value = 1, .counter = {}};
        storage.insertMove(bytesOf(value), 1);
    }
    EXPECT_EQ(liveCounters, 1);
    EXPECT_TRUE(storage.remove());
    EXPECT_EQ(liveCounters, 0);
    EXPECT_FALSE(storage.hasValue());
    EXPECT_TRUE(storage.get().empty());
}

TEST(ResourceStorage, InsertAfterRemoveStampsNewAddedTick) {
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Time>();
    res::Time time{.delta = 0.F, .fixedDelta = 0.F};
    storage.insertMove(bytesOf(time), 1);
    storage.remove();
    storage.insertMove(bytesOf(time), 9);
    EXPECT_EQ(storage.getTicks().added, 9U);
}

TEST(ResourceStorage, DestructorDestroysValue) {
    liveCounters = 0;
    {
        aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Tracked>();
        res::Tracked value{.value = 1, .counter = {}};
        storage.insertCopy(bytesOf(std::as_const(value)), 1);
        EXPECT_EQ(liveCounters, 2);
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(ResourceStorage, StoresMoveOnlyType) {
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Backend>();
    res::Backend backend{.window = std::make_unique<std::int32_t>(42)};
    storage.insertMove(bytesOf(backend), 1);
    EXPECT_EQ(backend.window, nullptr);
    const res::Backend* stored = aylo::ecs::storage::viewAs<res::Backend>(storage.get());
    ASSERT_NE(stored->window, nullptr);
    EXPECT_EQ(*stored->window, 42);
}

TEST(ResourceStorage, EmptyTypeHasValueButNoBytes) {
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Paused>();
    storage.insertMove({}, 1);
    EXPECT_TRUE(storage.hasValue());
    EXPECT_TRUE(storage.get().empty());
    EXPECT_TRUE(storage.remove());
    EXPECT_FALSE(storage.hasValue());
}

TEST(ResourceStorage, ValueIsAligned) {
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Aligned16>();
    res::Aligned16 value{.values = {1.F, 2.F, 3.F, 4.F}};
    storage.insertMove(bytesOf(value), 1);
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(storage.get().data()) % 16, 0U);
}

TEST(ResourceStorage, MoveTransfersValue) {
    liveCounters = 0;
    {
        aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Tracked>();
        res::Tracked value{.value = 6, .counter = {}};
        storage.insertMove(bytesOf(value), 1);
        aylo::ecs::storage::ResourceStorage moved{std::move(storage)};
        EXPECT_EQ(aylo::ecs::storage::viewAs<res::Tracked>(moved.get())->value, 6);
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(ResourceStorage, ReplaceByCopyKeepsAddedTickAndDestroysOldValue) {
    liveCounters = 0;
    {
        aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Fragile>();
        const res::Fragile first{1};
        storage.insertCopy(bytesOf(first), 2);
        const res::Fragile second{2};
        storage.insertCopy(bytesOf(second), 5);
        EXPECT_EQ(liveCounters, 3);  // first, second and the stored copy
        EXPECT_EQ(aylo::ecs::storage::viewAs<res::Fragile>(storage.get())->value, 2);
        EXPECT_EQ(storage.getTicks().added, 2U);
        EXPECT_EQ(storage.getTicks().changed, 5U);
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(ResourceStorage, ThrowingCopyKeepsOldValue) {
    liveCounters = 0;
    {
        aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Fragile>();
        const res::Fragile first{1};
        storage.insertCopy(bytesOf(first), 2);
        const res::Fragile second{2};
        throwOnCopy = true;
        EXPECT_THROW(storage.insertCopy(bytesOf(second), 5), std::runtime_error);
        throwOnCopy = false;
        ASSERT_TRUE(storage.hasValue());
        EXPECT_EQ(aylo::ecs::storage::viewAs<res::Fragile>(storage.get())->value, 1);
        EXPECT_EQ(storage.getTicks().added, 2U);
        EXPECT_EQ(storage.getTicks().changed, 2U);
        EXPECT_EQ(liveCounters, 3);  // first, second and the stored copy: no leak from the failed copy
        const res::Fragile third{3};
        storage.insertCopy(bytesOf(third), 6);
        EXPECT_EQ(aylo::ecs::storage::viewAs<res::Fragile>(storage.get())->value, 3);
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(ResourceStorage, ThrowingFirstCopyLeavesStorageEmpty) {
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Fragile>();
    const res::Fragile value{1};
    throwOnCopy = true;
    EXPECT_THROW(storage.insertCopy(bytesOf(value), 1), std::runtime_error);
    throwOnCopy = false;
    EXPECT_FALSE(storage.hasValue());
}

TEST(ResourceStorage, ClampTicksMovesOnlyOldTicks) {
    aylo::ecs::storage::ResourceStorage storage = makeStorage<res::Time>();
    res::Time time{.delta = 0.F, .fixedDelta = 0.F};
    storage.insertMove(bytesOf(time), 0);
    const aylo::ecs::Tick thisRun = aylo::ecs::kMaxChangeAge + 100;
    storage.getTicks().changed = thisRun - 10;
    storage.clampTicks(thisRun);
    EXPECT_EQ(storage.getTicks().added, thisRun - aylo::ecs::kMaxChangeAge);
    EXPECT_EQ(storage.getTicks().changed, thisRun - 10);
}

/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** ByteColumnTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory_resource>
#include <span>
#include <utility>
#include <vector>

#include "aylo/ecs/component/ComponentTraits.hpp"
#include "aylo/ecs/storage/ByteColumn.hpp"
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

/// @brief A memory resource that records every allocation and checks it is given back with the same size and
/// alignment.
class CountingResource : public std::pmr::memory_resource {
  public:
    int allocations{0};
    int deallocations{0};
    bool mismatch{false};

  private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        ++allocations;
        void* data = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        _live[data] = {bytes, alignment};
        return data;
    }

    void do_deallocate(void* data, std::size_t bytes, std::size_t alignment) override {
        ++deallocations;
        const auto found = _live.find(data);
        if (found == _live.end() || found->second != std::pair{bytes, alignment}) {
            mismatch = true;
        } else {
            _live.erase(found);
        }
        std::pmr::new_delete_resource()->deallocate(data, bytes, alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    std::map<void*, std::pair<std::size_t, std::size_t>> _live;
};
}  // namespace

namespace aylo::test::column {
struct Plain {
    std::int32_t a;
    float b;
};
AYLO_ECS_COMPONENT(Plain, "test.Plain", a, b);

struct Scaled {
    float scale{1.0F};
};
AYLO_ECS_COMPONENT(Scaled, "test.Scaled", scale);

struct Tracked {
    std::vector<std::int32_t> items;
    Counter counter;
};
AYLO_ECS_COMPONENT(Tracked, "test.Tracked");

struct alignas(16) Aligned16 {
    std::array<float, 4> values;
};
AYLO_ECS_COMPONENT(Aligned16, "test.Aligned16", values);

struct Dead {};
AYLO_ECS_COMPONENT(Dead, "test.Dead");
}  // namespace aylo::test::column

namespace {
template <typename T>
aylo::ecs::storage::ByteColumn makeColumn() {
    return aylo::ecs::storage::ByteColumn{aylo::ecs::ComponentTraits<T>::makeInfo().type};
}

template <typename T>
T& valueAt(aylo::ecs::storage::ByteColumn& column, std::size_t position) {
    return *aylo::ecs::storage::viewAs<T>(column.at(position));
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

TEST(ByteColumn, StartsEmpty) {
    const aylo::ecs::storage::ByteColumn column = makeColumn<aylo::test::column::Plain>();
    EXPECT_EQ(column.getSize(), 0U);
    EXPECT_EQ(column.getCapacity(), 0U);
    EXPECT_EQ(column.getElementSize(), sizeof(aylo::test::column::Plain));
}

TEST(ByteColumn, EmplaceBackZeroFillsPlainData) {
    aylo::ecs::storage::ByteColumn column = makeColumn<aylo::test::column::Plain>();
    const aylo::test::column::Plain* value =
        aylo::ecs::storage::viewAs<aylo::test::column::Plain>(column.emplaceBack());
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->a, 0);
    EXPECT_EQ(value->b, 0.0F);
    EXPECT_EQ(column.getSize(), 1U);
}

TEST(ByteColumn, EmplaceBackAppliesDefaultMemberInitializers) {
    aylo::ecs::storage::ByteColumn column = makeColumn<aylo::test::column::Scaled>();
    EXPECT_EQ(aylo::ecs::storage::viewAs<aylo::test::column::Scaled>(column.emplaceBack())->scale, 1.0F);
}

TEST(ByteColumn, KeepsValuesWhenGrowing) {
    aylo::ecs::storage::ByteColumn column = makeColumn<aylo::test::column::Plain>();
    for (std::int32_t i = 0; i < 100; ++i) {
        const aylo::test::column::Plain value{.a = i, .b = static_cast<float>(i) / 2.0F};
        column.emplaceBackCopy(bytesOf(value));
    }
    ASSERT_EQ(column.getSize(), 100U);
    EXPECT_GE(column.getCapacity(), 100U);
    for (std::int32_t i = 0; i < 100; ++i) {
        EXPECT_EQ(valueAt<aylo::test::column::Plain>(column, static_cast<std::size_t>(i)).a, i);
    }
}

TEST(ByteColumn, CopiesAndMovesValuesIn) {
    aylo::ecs::storage::ByteColumn column = makeColumn<aylo::test::column::Tracked>();
    aylo::test::column::Tracked source{.items = {1, 2, 3}};
    column.emplaceBackCopy(bytesOf(std::as_const(source)));
    EXPECT_EQ(source.items.size(), 3U);  // copied, untouched
    column.emplaceBackMove(bytesOf(source));
    EXPECT_EQ(valueAt<aylo::test::column::Tracked>(column, 0).items, (std::vector<std::int32_t>{1, 2, 3}));
    EXPECT_EQ(valueAt<aylo::test::column::Tracked>(column, 1).items, (std::vector<std::int32_t>{1, 2, 3}));
}

TEST(ByteColumn, ReadsValuesThroughAConstColumn) {
    aylo::ecs::storage::ByteColumn column = makeColumn<aylo::test::column::Plain>();
    const aylo::test::column::Plain value{.a = 42, .b = 0.5F};
    column.emplaceBackCopy(bytesOf(value));
    const aylo::ecs::storage::ByteColumn& readOnly = column;
    const std::span<const std::byte> bytes = readOnly.at(0);
    EXPECT_EQ(bytes.size(), sizeof(aylo::test::column::Plain));
    EXPECT_EQ(aylo::ecs::storage::viewAs<aylo::test::column::Plain>(bytes)->a, 42);
}

TEST(ByteColumn, SwapRemoveMovesTheLastValueIntoTheHole) {
    aylo::ecs::storage::ByteColumn column = makeColumn<aylo::test::column::Plain>();
    for (std::int32_t i = 0; i < 3; ++i) {
        const aylo::test::column::Plain value{.a = i, .b = 0.0F};
        column.emplaceBackCopy(bytesOf(value));
    }
    column.swapRemove(0);  // 2 moves into position 0
    ASSERT_EQ(column.getSize(), 2U);
    EXPECT_EQ(valueAt<aylo::test::column::Plain>(column, 0).a, 2);
    EXPECT_EQ(valueAt<aylo::test::column::Plain>(column, 1).a, 1);
    column.swapRemove(1);  // the last one: nothing moves
    ASSERT_EQ(column.getSize(), 1U);
    EXPECT_EQ(valueAt<aylo::test::column::Plain>(column, 0).a, 2);
}

TEST(ByteColumn, NonTrivialValuesSurviveGrowthAndRemoval) {
    liveCounters = 0;
    {
        aylo::ecs::storage::ByteColumn column = makeColumn<aylo::test::column::Tracked>();
        for (std::int32_t i = 0; i < 50; ++i) {
            aylo::test::column::Tracked* value =
                aylo::ecs::storage::viewAs<aylo::test::column::Tracked>(column.emplaceBack());
            value->items = {i, i + 1};
        }
        EXPECT_EQ(liveCounters, 50);
        column.swapRemove(10);  // 49 moves into position 10
        EXPECT_EQ(valueAt<aylo::test::column::Tracked>(column, 10).items, (std::vector<std::int32_t>{49, 50}));
        EXPECT_EQ(liveCounters, 49);
        column.clear();
        EXPECT_EQ(liveCounters, 0);
        column.emplaceBack();
        EXPECT_EQ(liveCounters, 1);
    }
    EXPECT_EQ(liveCounters, 0);  // the destructor destroyed the last value
}

TEST(ByteColumn, KeepsValuesAlignedAfterGrowth) {
    aylo::ecs::storage::ByteColumn column = makeColumn<aylo::test::column::Aligned16>();
    for (int i = 0; i < 200; ++i) {
        column.emplaceBack();
    }
    for (std::size_t position = 0; position < column.getSize(); ++position) {
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(column.at(position).data()) % 16, 0U) << "position " << position;
    }
}

TEST(ByteColumn, TagsOnlyCount) {
    CountingResource resource;
    {
        aylo::ecs::storage::ByteColumn column{aylo::ecs::ComponentTraits<aylo::test::column::Dead>::makeInfo().type,
                                              &resource};
        EXPECT_TRUE(column.emplaceBack().empty());
        column.emplaceBackCopy({});
        EXPECT_EQ(column.getSize(), 2U);
        EXPECT_TRUE(column.at(0).empty());
        column.swapRemove(0);
        EXPECT_EQ(column.getSize(), 1U);
    }
    EXPECT_EQ(resource.allocations, 0);
}

TEST(ByteColumn, GivesMemoryBackToItsResource) {
    CountingResource resource;
    {
        aylo::ecs::storage::ByteColumn column{aylo::ecs::ComponentTraits<aylo::test::column::Plain>::makeInfo().type,
                                              &resource};
        for (int i = 0; i < 100; ++i) {
            column.emplaceBack();
        }
        column.reserve(1000);
    }
    EXPECT_GT(resource.allocations, 1);
    EXPECT_EQ(resource.deallocations, resource.allocations);
    EXPECT_FALSE(resource.mismatch);
}

TEST(ByteColumn, MovingTransfersTheValues) {
    liveCounters = 0;
    {
        aylo::ecs::storage::ByteColumn column = makeColumn<aylo::test::column::Tracked>();
        aylo::ecs::storage::viewAs<aylo::test::column::Tracked>(column.emplaceBack())->items = {7};
        aylo::ecs::storage::ByteColumn moved{std::move(column)};
        EXPECT_EQ(moved.getSize(), 1U);
        EXPECT_EQ(valueAt<aylo::test::column::Tracked>(moved, 0).items, (std::vector<std::int32_t>{7}));
        EXPECT_EQ(column.getSize(), 0U);  // NOLINT(bugprone-use-after-move): moved-from is specified empty
        column.emplaceBack();             // the moved-from column is still usable
        aylo::ecs::storage::ByteColumn assigned = makeColumn<aylo::test::column::Tracked>();
        assigned.emplaceBack();
        assigned = std::move(moved);  // destroys assigned's own value first
        EXPECT_EQ(assigned.getSize(), 1U);
        EXPECT_EQ(liveCounters, 2);  // assigned's value + the one in the moved-from column
    }
    EXPECT_EQ(liveCounters, 0);
}

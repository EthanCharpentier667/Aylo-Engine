/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** BytecodeTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include <utility>

#include "aylo/luau/Bytecode.hpp"
#include "aylo/luau/CHelper.hpp"

namespace {
// NOLINTNEXTLINE(*-avoid-c-arrays)
aylo::luau::CPtr<char[]> makeBuffer(const std::size_t size) {
    // NOLINTNEXTLINE(*-avoid-c-arrays,*-no-malloc,*-owning-memory)
    auto* raw = static_cast<char*>(std::malloc(size));
    std::memset(raw, 'x', size);
    // NOLINTNEXTLINE(*-avoid-c-arrays)
    return aylo::luau::CPtr<char[]>{raw};
}
}  // namespace

TEST(Bytecode, ExposesSizeAndData) {
    const aylo::luau::Bytecode bytecode{makeBuffer(8), 8};

    EXPECT_EQ(bytecode.size(), 8U);
    ASSERT_NE(bytecode.data().get(), nullptr);
    EXPECT_EQ(bytecode.data()[0], 'x');
}

TEST(Bytecode, MoveConstructionTransfersBuffer) {
    aylo::luau::Bytecode source{makeBuffer(4), 4};
    const char* raw = source.data().get();

    const aylo::luau::Bytecode moved{std::move(source)};

    EXPECT_EQ(moved.data().get(), raw);
    EXPECT_EQ(moved.size(), 4U);
}

TEST(Bytecode, MoveAssignmentTransfersBuffer) {
    aylo::luau::Bytecode source{makeBuffer(6), 6};
    aylo::luau::Bytecode target{makeBuffer(2), 2};
    const char* raw = source.data().get();

    target = std::move(source);

    EXPECT_EQ(target.data().get(), raw);
    EXPECT_EQ(target.size(), 6U);
}

TEST(Bytecode, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<aylo::luau::Bytecode>);
    static_assert(!std::is_copy_assignable_v<aylo::luau::Bytecode>);
    static_assert(std::is_nothrow_move_constructible_v<aylo::luau::Bytecode>);
    static_assert(std::is_nothrow_move_assignable_v<aylo::luau::Bytecode>);
    SUCCEED();
}

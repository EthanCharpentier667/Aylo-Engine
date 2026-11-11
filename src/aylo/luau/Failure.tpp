/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Failure
*/

#include <tl/expected.hpp>
#include <utility>

namespace aylo::luau {
template <typename T>
Failure::operator Result<T>() && noexcept {
    return tl::unexpected{std::move(_error)};
}
}  // namespace aylo::luau

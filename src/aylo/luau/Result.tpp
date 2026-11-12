/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Result
*/

#pragma once

#include <string>

#include "Exceptions.hpp"

namespace aylo::luau {
template <typename DestT, typename SrcT>
// NOLINTNEXTLINE(*-use-internal-linkage)
Result<DestT> forwardError(Result<SrcT> from) {
    return tl::unexpected{std::move(from).error()};
}

template <typename T>
// NOLINTNEXTLINE(*-use-internal-linkage)
T expect(Result<T> result) {
    if (result) {
        if constexpr (std::is_void_v<T>) {
            return;
        } else {
            return std::move(*result);
        }
    }
    throw exceptions::RuntimeError{std::string{result.error().message()}};
}
}  // namespace aylo::luau

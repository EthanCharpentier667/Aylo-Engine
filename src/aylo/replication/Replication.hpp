/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Replication
*/

#pragma once

#include <string_view>

#include "aylo/replication/Export.hpp"

namespace aylo::replication {

/// @brief Name of the library, also the first exported symbol of aylo-replication (a shared library needs one
/// to be linkable on Windows).
[[nodiscard]] AYLO_REPLICATION_API std::string_view libraryName() noexcept;

}  // namespace aylo::replication

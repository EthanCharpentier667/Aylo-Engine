/*
** EPITECH PROJECT, 2026
** aylo
** File description:
** Version
*/

#pragma once

#include <string_view>

#include "Export.hpp"

namespace aylo::ecs {
/// @brief Version of the aylo-ecs library.
/// @return The version as `MAJOR.MINOR.PATCH`, e.g. "0.1.0".
[[nodiscard]] AYLO_ECS_API std::string_view getVersion() noexcept;
}  // namespace aylo::ecs
